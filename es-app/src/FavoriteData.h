#pragma once

#include "SystemData.h"


class FavoriteData : public SystemData
{
public:
  FavoriteData(std::string fullName, std::string command, std::string themeFolder, std::vector<SystemData*>* systems);
};
