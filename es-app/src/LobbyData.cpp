#include "FileSorts.h"
#include "Log.h"
#include "LobbyData.h"
#include "views/ViewController.h"


LobbyData::LobbyData(std::vector<SystemData*>* systems) : SystemData("lobby", std::string("Lobby"), std::string("lobby")) {
  msystems = systems;
  refreshRootFolder();
}

void LobbyData::refreshRootFolder() {
  mRootFolder->clear();
  addPlayer("d41d8cd98f00b204e9800998ecf8427e");
}

void LobbyData::addPlayer(std::string gameHash) {
  for(auto system = msystems->begin(); system != msystems->end(); system ++) {
    std::vector<FileData*> games = (*system)->getRootFolder()->getFilesRecursive(GAME);
    for(auto game = games.begin(); game != games.end(); game++) {
      if ((*game)->metadata.get("hash").compare(gameHash) == 0) {
        // FIXME: Really we want to clone the FileData so that the Lobby can store remote player ip etc on it
        // If we don't ephemeral multiplayer related data will end up in gamelist.xml
        mRootFolder->addAlreadyExisitingChild((*game));
      }
    }
  }
  mRootFolder->sort(FileSorts::SortTypes.at(0));
}

void LobbyData::removePlayer(std::string gameHash) {
  std::vector<FileData*> games = mRootFolder->getFilesRecursive(GAME);
  for(auto game = games.begin(); game != games.end(); game++) {
    if ((*game)->metadata.get("hash").compare(gameHash) == 0) {
      mRootFolder->removeAlreadyExisitingChild((*game));
    }
  }
  mRootFolder->sort(FileSorts::SortTypes.at(0));
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
