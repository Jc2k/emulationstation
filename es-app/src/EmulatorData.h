#pragma once

#include "SystemData.h"


class EmulatorData : public SystemData
{
public:
  EmulatorData(std::string name, std::string fullName, std::string startPath,
                             std::vector<std::string> extensions, std::string command,
                             std::vector<PlatformIds::PlatformId> platformIds, std::string themeFolder,
                             std::map<std::string, std::vector<std::string>*>* emulators);

  bool allowGameOptions() const override;
  bool allowFavoriting() const override;
};
