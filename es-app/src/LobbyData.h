#pragma once

#include "SystemData.h"
#include "Lobby.h"


class LobbyData : public SystemData
{
public:
  LobbyData(std::vector<SystemData*>* systems);

  bool hasAnyThumbnails() const override;
  bool allowGameOptions() const override;
  bool allowFavoriting() const override;

private:
  void addPlayer(Session *session);
  void removePlayer(Session *session);

  std::vector<SystemData*>* msystems;

  void refreshRootFolder();
  void onLobbyChange();
};
