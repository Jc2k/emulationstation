#include "EmulatorData.h"

EmulatorData::EmulatorData(std::string name, std::string fullName, std::string startPath,
                           std::vector<std::string> extensions, std::string command,
                           std::string hostCommand, std::string joinCommand,
                           std::vector<PlatformIds::PlatformId> platformIds, std::string themeFolder,
                           std::map<std::string, std::vector<std::string>*>* emulators) :

  SystemData(name, fullName, startPath, extensions, command, hostCommand, joinCommand, platformIds, themeFolder, emulators)
{


}

bool EmulatorData::hasAnyThumbnails() const {
	auto files = getRootFolder()->getFilesRecursive(GAME | FOLDER);
	for(auto it = files.begin(); it != files.end(); it++) {
		if(!(*it)->getThumbnailPath().empty()) {
      return true;
		}
	}
  return false;
}

bool EmulatorData::allowGameOptions() const {
  return true;
}

bool EmulatorData::allowFavoriting() const {
  return true;
}
