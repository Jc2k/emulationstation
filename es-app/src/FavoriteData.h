#pragma once

#include "SystemData.h"


class FavoriteData : public SystemData
{
public:
  FavoriteData(std::string fullName, std::string command, std::string themeFolder, std::vector<SystemData*>* systems);

  bool hasAnyThumbnails() const override;
  bool allowGameOptions() const override;
  bool allowFavoriting() const override;
};
