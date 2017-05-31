#include "LobbyData.h"


LobbyData::LobbyData() : SystemData("lobby", std::string("Lobby"), std::string("lobby")) {

}

bool LobbyData::hasAnyThumbnails() const {
  return false;
}

bool LobbyData::allowGameOptions() const {
  return false;
}

bool LobbyData::allowFavoriting() const {
  return false;
}
