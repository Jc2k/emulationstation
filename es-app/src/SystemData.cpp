#include "SystemData.h"
#include "Gamelist.h"
#include <boost/filesystem.hpp>
#include <fstream>
#include <utility>
#include <stdlib.h>
#include <SDL_joystick.h>
#include <lua.hpp>

#include "Renderer.h"
#include "AudioManager.h"
#include "VolumeControl.h"
#include "Log.h"
#include "InputManager.h"
#include <iostream>
#include "Settings.h"
#include "FileSorts.h"
#include "Util.h"
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/make_shared.hpp>
#include "Lobby.h"
#include "EmulatorData.h"
#include "FavoriteData.h"


std::vector<SystemData*> SystemData::sSystemVector;

namespace fs = boost::filesystem;

SystemData::SystemData(std::string name, std::string fullName, std::string themeFolder) {
  mName = name;
	mFullName = fullName;
  mThemeFolder = themeFolder;
  mStartPath = "";
	mLaunchScript = "";

	mRootFolder = new FileData(FOLDER, mStartPath, this);
	mRootFolder->metadata.set("name", mFullName);

	mIsFavorite = false;
	mPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);

	loadTheme();
}

SystemData::SystemData(std::string name, std::string fullName, std::string startPath,
                           std::vector<std::string> extensions, std::string launchScript,
                           std::vector<PlatformIds::PlatformId> platformIds, std::string themeFolder,
                           std::map<std::string, std::vector<std::string>*>* emulators)
{
	mName = name;
	mFullName = fullName;
	mStartPath = getExpandedPath(startPath);
	mEmulators = emulators;

	// make it absolute if needed
	{
		const std::string defaultRomsPath = getExpandedPath(Settings::getInstance()->getString("DefaultRomsPath"));

		if (!defaultRomsPath.empty())
		{
			mStartPath = fs::absolute(mStartPath, defaultRomsPath).generic_string();
		}
	}

	mSearchExtensions = extensions;
	mLaunchScript = launchScript;
	mPlatformIds = platformIds;
	mThemeFolder = themeFolder;

	mRootFolder = new FileData(FOLDER, mStartPath, this);
	mRootFolder->metadata.set("name", mFullName);

	if(!Settings::getInstance()->getBool("ParseGamelistOnly"))
		populateFolder(mRootFolder);

	if(!Settings::getInstance()->getBool("IgnoreGamelist"))
		parseGamelist(this);

	mRootFolder->sort(FileSorts::SortTypes.at(0));
	mIsFavorite = false;
	loadTheme();
}

SystemData::SystemData(std::string name, std::string fullName, std::string launchScript,
					   std::string themeFolder, std::vector<SystemData*>* systems)
{
	mName = name;
	mFullName = fullName;
	mStartPath = "";

	mLaunchScript = launchScript;
	mThemeFolder = themeFolder;

	mRootFolder = new FileData(FOLDER, mStartPath, this);
	mRootFolder->metadata.set("name", mFullName);

	for(auto system = systems->begin(); system != systems->end(); system ++){
		std::vector<FileData*> favorites = (*system)->getFavorites();
		for(auto favorite = favorites.begin(); favorite != favorites.end(); favorite++){
			mRootFolder->addAlreadyExisitingChild((*favorite));
		}
	}

	if(mRootFolder->getChildren().size())
		mRootFolder->sort(FileSorts::SortTypes.at(0));
	mIsFavorite = true;
	mPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);

	loadTheme();
}

SystemData::~SystemData()
{
	updateGamelist(this);
	delete mRootFolder;
}


std::string strreplace(std::string str, const std::string& replace, const std::string& with)
{
	size_t pos;
	while((pos = str.find(replace)) != std::string::npos)
		str = str.replace(pos, replace.length(), with.c_str(), with.length());

	return str;
}

// plaform-specific escape path function
// on windows: just puts the path in quotes
// everything else: assume bash and escape special characters with backslashes
std::string escapePath(const boost::filesystem::path& path)
{
#ifdef WIN32
	// windows escapes stuff by just putting everything in quotes
	return '"' + fs::path(path).make_preferred().string() + '"';
#else
	// a quick and dirty way to insert a backslash before most characters that would mess up a bash path
	std::string pathStr = path.string();

	const char* invalidChars = " '\"\\!$^&*(){}[]?;<>";
	for(unsigned int i = 0; i < pathStr.length(); i++)
	{
		char c;
		unsigned int charNum = 0;
		do {
			c = invalidChars[charNum];
			if(pathStr[i] == c)
			{
				pathStr.insert(i, "\\");
				i++;
				break;
			}
			charNum++;
		} while(c != '\0');
	}

	return pathStr;
#endif
}

void setglobal(lua_State *state, const char *key, std::string value) {
  lua_pushstring(state, value.c_str());
  lua_setglobal(state, key);
}

int SystemData::runLaunchGameScript(FileData *game) {
  lua_State *state = luaL_newstate();

  luaL_openlibs(state);

  setglobal(state, "rom", escapePath(game->getPath()));
  setglobal(state, "controllers_config", InputManager::getInstance()->configureEmulators()) ;
  setglobal(state, "basename", game->getPath().stem().string());
  setglobal(state, "rom_raw", fs::path(game->getPath()).make_preferred().string());
  setglobal(state, "system", game->metadata.get("system"));
  setglobal(state, "emulator", game->metadata.get("emulator"));
  setglobal(state, "core", game->metadata.get("core"));
  setglobal(state, "ratio", game->metadata.get("ratio"));
  setglobal(state, "peer", game->metadata.get("peer"));

  lua_pushboolean(state, true);
  lua_setglobal(state, "multiplayer_enabled");

  lua_pushboolean(state, !game->metadata.get("peer").empty());
  lua_setglobal(state, "join_existing");

  auto error = luaL_loadstring(state, game->getSystem()->mLaunchScript.c_str());
  if(error)
  {
      std::cout << lua_tostring(state, -1) << std::endl;
      lua_pop(state, 1);
      // return; //Perhaps throw or something to signal an error?
  }

  error = lua_pcall(state, 0, 0, 0);
  if (error) {
      std::cout << lua_tostring(state, -1) << std::endl;
      lua_pop(state, 1);
  }

  lua_close(state);

  return 0;
}

void SystemData::launchGame(Window* window, FileData* game)
{
	LOG(LogInfo) << "Attempting to launch game...";

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();

	window->deinit();

  if (game->metadata.get("peer").empty())
    LobbyThread::getInstance()->startBroadcast(game->metadata.get("hash"));

	std::cout << "==============================================\n";
  auto exitCode = runLaunchGameScript(game);
	std::cout << "==============================================\n";

	if(exitCode != 0){
		LOG(LogWarning) << "...launch terminated with nonzero exit code " << exitCode << "!";
	}

  if (game->metadata.get("peer").empty())
    LobbyThread::getInstance()->stopBroadcast();

	window->init();
	VolumeControl::getInstance()->init();
	AudioManager::getInstance()->resumeMusic();
	window->normalizeNextUpdate();

	//update number of times the game has been launched
	int timesPlayed = game->metadata.getInt("playcount") + 1;
	game->metadata.set("playcount", std::to_string(static_cast<long long>(timesPlayed)));

	//update last played time
	boost::posix_time::ptime time = boost::posix_time::second_clock::universal_time();
	game->metadata.setTime("lastplayed", time);
}

void SystemData::populateFolder(FileData* folder)
{
	FileData::populateRecursiveFolder(folder, mSearchExtensions, this);
}

std::vector<std::string> readList(const std::string& str, const char* delims = " \t\r\n,")
{
	std::vector<std::string> ret;

	size_t prevOff = str.find_first_not_of(delims, 0);
	size_t off = str.find_first_of(delims, prevOff);
	while(off != std::string::npos || prevOff != std::string::npos)
	{
		ret.push_back(str.substr(prevOff, off - prevOff));

		prevOff = str.find_first_not_of(delims, off);
		off = str.find_first_of(delims, prevOff);
	}

	return ret;
}

SystemData * createSystem(pugi::xml_node * systemsNode, int index ){
	std::string name, fullname, path, themeFolder;
	PlatformIds::PlatformId platformId = PlatformIds::PLATFORM_UNKNOWN;

	int myIndex = 0;
	pugi::xml_node * system;
	for(pugi::xml_node systemIn = systemsNode->child("system"); systemIn; systemIn = systemIn.next_sibling("system"))
	{
		if(myIndex >= index) {
			system = &(systemIn);
			break;
		}
		myIndex++;
	}
	name = system->child("name").text().get();
	fullname = system->child("fullname").text().get();
	path = system->child("path").text().get();

	// convert extensions list from a string into a vector of strings
	std::vector<std::string> extensions = readList(system->child("extension").text().get());

	std::string launchScript = system->child("launchScript").text().get();

	// platform id list
	const char* platformList = system->child("platform").text().get();
	std::vector<std::string> platformStrs = readList(platformList);
	std::vector<PlatformIds::PlatformId> platformIds;
	for(auto it = platformStrs.begin(); it != platformStrs.end(); it++)
	{
		const char* str = it->c_str();
		PlatformIds::PlatformId platformId = PlatformIds::getPlatformId(str);

		if(platformId == PlatformIds::PLATFORM_IGNORE)
		{
			// when platform is ignore, do not allow other platforms
			platformIds.clear();
			platformIds.push_back(platformId);
			break;
		}

		// if there appears to be an actual platform ID supplied but it didn't match the list, warn
		if(str != NULL && str[0] != '\0' && platformId == PlatformIds::PLATFORM_UNKNOWN)
		LOG(LogWarning) << "  Unknown platform for system \"" << name << "\" (platform \"" << str << "\" from list \"" << platformList << "\")";
		else if(platformId != PlatformIds::PLATFORM_UNKNOWN)
			platformIds.push_back(platformId);
	}

	// theme folder
	themeFolder = system->child("theme").text().as_string(name.c_str());

	//validate
	if(name.empty() || path.empty() || extensions.empty() || launchScript.empty())
	{
		LOG(LogError) << "System \"" << name << "\" is missing name, path, extension, or launchScript!";
		return NULL;
	}

	//convert path to generic directory seperators
	boost::filesystem::path genericPath(path);
	path = genericPath.generic_string();

	// emulators and cores
	std::map<std::string, std::vector<std::string>*> * systemEmulators = new std::map<std::string, std::vector<std::string>*>();
	pugi::xml_node emulatorsNode = system->child("emulators");
	for(pugi::xml_node emuNode = emulatorsNode.child("emulator"); emuNode; emuNode = emuNode.next_sibling("emulator")) {
		std::string emulatorName = emuNode.attribute("name").as_string();
		(*systemEmulators)[emulatorName] = new std::vector<std::string>();
		pugi::xml_node coresNode = emuNode.child("cores");
		for (pugi::xml_node coreNode = coresNode.child("core"); coreNode; coreNode = coreNode.next_sibling("core")) {
			std::string corename = coreNode.text().as_string();
			(*systemEmulators)[emulatorName]->push_back(corename);
		}
	}


	SystemData* newSys = new EmulatorData(name,
										fullname,
										path, extensions,
										launchScript,
                    platformIds,
										themeFolder,
										systemEmulators);
	if(newSys->getRootFolder()->getChildren().size() == 0)
	{
		LOG(LogWarning) << "System \"" << name << "\" has no games! Ignoring it.";
		delete newSys;
		return NULL;
	}else{
		LOG(LogWarning) << "Adding \"" << name << "\" in system list.";
		return newSys;
	}
}

//creates systems from information located in a config file
bool SystemData::loadConfig()
{
	deleteSystems();
	std::string path = getConfigPath(false);

	LOG(LogInfo) << "Loading system config file " << path << "...";

	if(!fs::exists(path))
	{
		LOG(LogError) << "es_systems.cfg file does not exist!";
		return false;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result res = doc.load_file(path.c_str());

	if(!res)
	{
		LOG(LogError) << "Could not parse es_systems.cfg file!";
		LOG(LogError) << res.description();
		return false;
	}

	//actually read the file
	pugi::xml_node systemList = doc.child("systemList");

	if(!systemList)
	{
		LOG(LogError) << "es_systems.cfg is missing the <systemList> tag!";
		return false;
	}

	// THE CREATION OF EACH SYSTEM
	boost::asio::io_service ioService;
	boost::thread_group threadpool;
	boost::asio::io_service::work work(ioService);
	std::vector<boost::thread *> threads;
	for (int i = 0; i < 4; i++) {
		threadpool.create_thread(
				boost::bind(&boost::asio::io_service::run, &ioService)
		);
	}

	int index = 0;
	std::vector<boost::unique_future<SystemData*>> pending_data;
	for(pugi::xml_node system = systemList.child("system"); system; system = system.next_sibling("system"))
	{
		LOG(LogInfo) << "creating thread for system " << system.child("name").text().get();
		typedef boost::packaged_task<SystemData*> task_t;
		boost::shared_ptr<task_t> task = boost::make_shared<task_t>(
				boost::bind(&createSystem, &systemList, index++));
		boost::unique_future<SystemData*> fut = task->get_future();
		pending_data.push_back(std::move(fut));
		ioService.post(boost::bind(&task_t::operator(), task));
	}
	boost::wait_for_all(pending_data.begin(), pending_data.end());

	for(auto pending = pending_data.begin(); pending != pending_data.end(); pending ++){
		SystemData * result = pending->get();
		if(result != NULL)
			sSystemVector.push_back(result);
	}
	ioService.stop();
	threadpool.join_all();

	return true;
}

bool deleteSystem(SystemData * system){
	delete system;
}

void SystemData::deleteSystems()
{
	if(sSystemVector.size()) {
		// THE DELETION OF EACH SYSTEM
		boost::asio::io_service ioService;
		boost::thread_group threadpool;
		boost::asio::io_service::work work(ioService);
		std::vector<boost::thread *> threads;
		for (int i = 0; i < 4; i++) {
			threadpool.create_thread(
					boost::bind(&boost::asio::io_service::run, &ioService)
			);
		}

		std::vector<boost::unique_future<bool>> pending_data;
		for (unsigned int i = 0; i < sSystemVector.size(); i++) {
			if(!(sSystemVector.at(i))->isFavorite()) {
				typedef boost::packaged_task<bool> task_t;
				boost::shared_ptr<task_t> task = boost::make_shared<task_t>(
						boost::bind(&deleteSystem, sSystemVector.at(i)));
				boost::unique_future<bool> fut = task->get_future();
				pending_data.push_back(std::move(fut));
				ioService.post(boost::bind(&task_t::operator(), task));
			}
		}

		boost::wait_for_all(pending_data.begin(), pending_data.end());

		ioService.stop();
		threadpool.join_all();
		sSystemVector.clear();
	}
}

std::string SystemData::getConfigPath(bool forWrite)
{
	fs::path path = getHomePath() + "/.emulationstation/es_systems.cfg";
	if(forWrite || fs::exists(path))
		return path.generic_string();

	return "/etc/emulationstation/es_systems.cfg";
}

std::string SystemData::getGamelistPath(bool forWrite) const {
	fs::path filePath;

	// If we have a gamelist in the rom directory, we use it
	filePath = mRootFolder->getPath() / "gamelist.xml";
	if (fs::exists(filePath))
		return filePath.generic_string();

	// else we try to create it
	if (forWrite) { // make sure the directory exists if we're going to write to it, or crashes will happen
		if (fs::exists(filePath.parent_path()) || fs::create_directories(filePath.parent_path())) {
			return filePath.generic_string();
		}
	}
	// Unable to get or create directory in roms, fallback on ~
	filePath = getHomePath() + "/.emulationstation/gamelists/" + mName + "/gamelist.xml";
	fs::create_directories(filePath.parent_path());
	return filePath.generic_string();
}

std::string SystemData::getThemePath() const
{
	// where we check for themes, in order:
	// 1. [SYSTEM_PATH]/theme.xml
	// 2. currently selected theme set

	// first, check game folder
	fs::path localThemePath = mRootFolder->getPath() / "theme.xml";
	if(fs::exists(localThemePath))
		return localThemePath.generic_string();

	// not in game folder, try theme sets
	return ThemeData::getThemeFromCurrentSet(mThemeFolder).generic_string();
}

bool SystemData::hasGamelist() const
{
	return (fs::exists(getGamelistPath(false)));
}

unsigned int SystemData::getGameCount() const
{
	return mRootFolder->getFilesRecursive(GAME).size();
}

unsigned int SystemData::getFavoritesCount() const
{
	return mRootFolder->getFavoritesRecursive(GAME).size();
}

unsigned int SystemData::getHiddenCount() const
{
	return mRootFolder->getHiddenRecursive(GAME).size();
}

void SystemData::loadTheme()
{
	mTheme = std::make_shared<ThemeData>();

	std::string path = getThemePath();

	if(!fs::exists(path)) // no theme available for this platform
		return;

	try
	{
		mTheme->loadFile(path);
		mHasFavorites = mTheme->getHasFavoritesInTheme();
	} catch(ThemeException& e)
	{
		LOG(LogError) << e.what();
		mTheme = std::make_shared<ThemeData>(); // reset to empty
	}
}

void SystemData::refreshRootFolder() {
  mRootFolder->clear();
  populateFolder(mRootFolder);
  mRootFolder->sort(FileSorts::SortTypes.at(0));
}

std::map<std::string, std::vector<std::string>*>* SystemData::getEmulators() {
	return mEmulators;
}

int SystemData::getSystemIndex(std::string name) {
    int index = 0;
	for(auto system = sSystemVector.begin(); system != sSystemVector.end(); system ++){
		if((*system)->mName == name){
			return index;
		}
        index++;
	}
	return -1;
}
