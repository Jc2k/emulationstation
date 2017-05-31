#pragma once

#include "SystemData.h"


class LobbyData : public SystemData
{
public:
  LobbyData(std::vector<SystemData*>* systems);

  bool hasAnyThumbnails() const override;
  bool allowGameOptions() const override;
  bool allowFavoriting() const override;

private:
  std::vector<SystemData*>* msystems;
  void refreshRootFolder();
  void onLobbyChange();
};
