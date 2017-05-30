#include "EmulatorData.h"

EmulatorData::EmulatorData(std::string name, std::string fullName, std::string startPath,
                           std::vector<std::string> extensions, std::string command,
                           std::vector<PlatformIds::PlatformId> platformIds, std::string themeFolder,
                           std::map<std::string, std::vector<std::string>*>* emulators) :

  SystemData(name, fullName, startPath, extensions, command, platformIds, themeFolder, emulators)
{


}

bool EmulatorData::allowGameOptions() const {
  return true;
}

bool EmulatorData::allowFavoriting() const {
  return true;
}
