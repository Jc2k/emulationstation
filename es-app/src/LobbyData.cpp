#include "FileSorts.h"
#include "Log.h"
#include "LobbyData.h"
#include "Lobby.h"
#include "views/ViewController.h"


LobbyData::LobbyData(std::vector<SystemData*>* systems) : SystemData("lobby", std::string("Lobby"), std::string("lobby")) {
  msystems = systems;

  LobbyThread::getInstance()->subscribeStartedPlaying([this](Session *session) { this->addPlayer(session); });
  LobbyThread::getInstance()->subscribeStoppedPlaying([this](Session *session) { this->removePlayer(session); });
}

void LobbyData::refreshRootFolder() {
}

void LobbyData::addPlayer(Session *session) {
  for(auto system = msystems->begin(); system != msystems->end(); system ++) {
    std::vector<FileData*> games = (*system)->getRootFolder()->getFilesRecursive(GAME);
    for(auto game = games.begin(); game != games.end(); game++) {
      if ((*game)->metadata.get("hash").compare(session->gameHash) == 0) {
        // FIXME: A clone won't get metadata updates from its parent!
        auto clone = (*game)->clone();
        clone->metadata.set("peer", session->peer);
        mRootFolder->addChild(clone);
      }
    }
  }
  mRootFolder->sort(FileSorts::SortTypes.at(0));

  ViewController::get()->reloadGameListView(this);
}

void LobbyData::removePlayer(Session *session) {
  std::cerr << "/* removePlayer */" << '\n';
  std::vector<FileData*> games = mRootFolder->getFilesRecursive(GAME);
  for(auto game = games.begin(); game != games.end(); game++) {
    std::cerr << (*game)->metadata.get("peer") << std::endl;
    if ((*game)->metadata.get("peer").compare(session->peer) == 0) {
      mRootFolder->removeAlreadyExisitingChild((*game));
    }
  }
  mRootFolder->sort(FileSorts::SortTypes.at(0));

  ViewController::get()->reloadGameListView(this);
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
