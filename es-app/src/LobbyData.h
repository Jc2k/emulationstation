#pragma once

#include "SystemData.h"


class LobbyData : public SystemData
{
public:
  LobbyData();

  bool hasAnyThumbnails() const override;
  bool allowGameOptions() const override;
  bool allowFavoriting() const override;
};
