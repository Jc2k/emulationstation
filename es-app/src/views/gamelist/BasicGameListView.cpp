#include "views/gamelist/BasicGameListView.h"
#include "views/ViewController.h"
#include "Renderer.h"
#include "Window.h"
#include "ThemeData.h"
#include "SystemData.h"
#include "Settings.h"
#include "Locale.h"
#include <boost/assign.hpp>

BasicGameListView::BasicGameListView(Window* window, FileData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	addChild(&mList);

	populateList(root->getChildren());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);
}

void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	ISimpleGameListView::onFileChanged(file, change);

	if(change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

}

static const std::map<std::string, const char*> favorites_icons_map = boost::assign::map_list_of
		("snes", "\uF25e ")
		("c64", "\uF24c ")
		("nes", "\uF25c ")
		("n64", "\uF260 ")
		("gba", "\uF266 ")
		("gbc", "\uF265 ")
		("gb", "\uF264 ")
		("fds", "\uF25d ")
		("virtualboy", "\uF25f ")
		("gw", "\uF278 ")
		("dreamcast", "\uF26e ")
		("megadrive", "\uF26b ")
		("segacd", "\uF26d ")
		("sega32x", "\uF26c ")
		("mastersystem", "\uF26a ")
		("gamegear", "\uF26f ")
		("sg1000", "\uF269 ")
		("psp", "\uF274 ")
		("psx", "\uF275 ")
		("pcengine", "\uF271 ")
		("pcenginecd", "\uF273 ")
		("supergrafx", "\uF272 ")
		("scummvm", "\uF27a ")
		("dos", "\uF24a ")
		("fba", "\uF252 ")
		("fba_libretro", "\uF253 ")
		("mame", "\uF255 ")
		("neogeo", "\uF257 ")
		("colecovision", "\uF23f ")
		("atari2600", "\uF23c ")
		("atari7800", "\uF23e ")
		("lynx", "\uF270 ")
		("ngp", "\uF258 ")
		("ngpc", "\uF259 ")
		("wswan", "\uF25a ")
		("wswanc", "\uF25b ")
		("prboom", "\uF277 ")
		("vectrex", "\uF240 ")
		("lutro", "\uF27d ")
		("cavestory", "\uF276 ")
		("atarist", "\uF248 ")
		("amstradcpc", "\uF246 ")
		("msx", "\uF24d ")
		("msx1", "\uF24e ")
		("msx2", "\uF24f ")
		("odyssey2", "\uF241 ")
		("zx81", "\uF250 ")
		("zxspectrum", "\uF251 ")
		("moonlight", "\uF27e ")
		("apple2", "\uF247 ")
		("gamecube", "\uF262 ")
		("wii", "\uF263 ")
		("imageviewer", "\uF27b ");

void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
	mList.clear();

	const FileData* root = getRoot();
	const SystemData* systemData = root->getSystem();
	mHeaderText.setText(systemData ? systemData->getFullName() : root->getCleanName());

	bool favoritesOnly = false;
	bool showHidden = Settings::getInstance()->getBool("ShowHidden");

	if (Settings::getInstance()->getBool("FavoritesOnly") && !systemData->isFavorite())
	{
		for (auto it = files.begin(); it != files.end(); it++)
		{
			if ((*it)->getType() == GAME)
			{
				if ((*it)->metadata.get("favorite").compare("true") == 0)
				{
					favoritesOnly = true;
					break;
				}
			}
		}
	}

	// The TextListComponent would be able to insert at a specific position,
	// but the cost of this operation could be seriously huge.
	// This naive implemention of doing a first pass in the list is used instead.
	if(!Settings::getInstance()->getBool("FavoritesOnly") || systemData->isFavorite()){
		for(auto it = files.begin(); it != files.end(); it++)
		{
			if ((*it)->getType() != FOLDER && (*it)->metadata.get("favorite").compare("true") == 0) {
				if ((*it)->metadata.get("hidden").compare("true") != 0) {
					if((favorites_icons_map.find((*it)->getSystem()->getName())) != favorites_icons_map.end()) {
						mList.add((favorites_icons_map.find((*it)->getSystem()->getName())->second) + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}else {
						mList.add("\uF006 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER)); // FIXME Folder as favorite ?
					}
				}else {
					if((favorites_icons_map.find((*it)->getSystem()->getName())) != favorites_icons_map.end()) {
						mList.add((favorites_icons_map.find((*it)->getSystem()->getName())->second) + std::string("\uF070 ") + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}else {
						mList.add("\uF006 \uF070 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}
			}
		}
	}

	// Do not show double names in favorite system.
	if(!systemData->isFavorite())
	{
		for (auto it = files.begin(); it != files.end(); it++) {
			if (favoritesOnly) {
				if ((*it)->getType() == GAME) {
					if ((*it)->metadata.get("favorite").compare("true") == 0) {
						if (!showHidden) {
							if ((*it)->metadata.get("hidden").compare("true") != 0) {
								mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}
						}
						else {
							if ((*it)->metadata.get("hidden").compare("true") == 0) {
								mList.add("\uF070 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}else {
								mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}
						}
					}
				}
			}
			else {
				if (!showHidden) {
					if ((*it)->metadata.get("hidden").compare("true") != 0) {
						if ((*it)->getType() != FOLDER && (*it)->metadata.get("favorite").compare("true") == 0) {
							if((favorites_icons_map.find((*it)->getSystem()->getName())) != favorites_icons_map.end()) {
								mList.add((favorites_icons_map.find((*it)->getSystem()->getName())->second) + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}else {
								mList.add("\uF006 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}
						}else {
							mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
						}
					}
				}
				else {
					if ((*it)->getType() != FOLDER && (*it)->metadata.get("favorite").compare("true") == 0) {
						if ((*it)->metadata.get("hidden").compare("true") != 0) {
							if((favorites_icons_map.find((*it)->getSystem()->getName())) != favorites_icons_map.end()) {
								mList.add((favorites_icons_map.find((*it)->getSystem()->getName())->second) + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}else {
								mList.add("\uF006 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}
						}else {
							if((favorites_icons_map.find((*it)->getSystem()->getName())) != favorites_icons_map.end()) {
								mList.add((favorites_icons_map.find((*it)->getSystem()->getName())->second) + std::string("\uF070 ") + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}else {
								mList.add("\uF006 \uF070 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
							}
						}
					}else if ((*it)->metadata.get("hidden").compare("true") == 0) {
						mList.add("\uF070 " + (*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}else {
						mList.add((*it)->getName(), *it, ((*it)->getType() == FOLDER));
					}
				}
			}
		}
	}
	if(files.size() == 0){
		while(!mCursorStack.empty()){
			mCursorStack.pop();
		}
	}
}

FileData* BasicGameListView::getCursor()
{
	if (!isEmpty())
		return mList.getSelected();
	return NULL;
}

void BasicGameListView::setCursorIndex(int cursor){
	mList.setCursorIndex(cursor);
}

int BasicGameListView::getCursorIndex(){
	return mList.getCursorIndex();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	if(!mList.setCursor(cursor))
	{
		populateList(mRoot->getChildren());
		mList.setCursor(cursor);

		// update our cursor stack in case our cursor just got set to some folder we weren't in before
		if(mCursorStack.empty() || mCursorStack.top() != cursor->getParent())
		{
			std::stack<FileData*> tmp;
			FileData* ptr = cursor->getParent();
			while(ptr && ptr != mRoot)
			{
				tmp.push(ptr);
				ptr = ptr->getParent();
			}

			// flip the stack and put it in mCursorStack
			mCursorStack = std::stack<FileData*>();
			while(!tmp.empty())
			{
				mCursorStack.push(tmp.top());
				tmp.pop();
			}
		}
	}
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(getRoot()->getSystem(), game);
}

std::vector<HelpPrompt> BasicGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect") && !Settings::getInstance()->getBool("HideSystemView"))
	  prompts.push_back(HelpPrompt("left/right", _("SYSTEM")));
	prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
	prompts.push_back(HelpPrompt("a", _("LAUNCH")));
	if(!Settings::getInstance()->getBool("HideSystemView"))
	  prompts.push_back(HelpPrompt("b", _("BACK")));

	if (getRoot()->getSystem()->allowFavoriting())
	  prompts.push_back(HelpPrompt("y", _("Favorite")));

	if (getRoot()->getSystem()->allowGameOptions())
	  prompts.push_back(HelpPrompt("select", _("OPTIONS")));

	return prompts;
}
