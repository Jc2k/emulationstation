#include "LobbyData.h"
#include "views/ViewController.h"


LobbyData::LobbyData(std::vector<SystemData*>* systems) : SystemData("lobby", std::string("Lobby"), std::string("lobby")) {
  msystems = systems;
  refreshRootFolder();
}

void LobbyData::refreshRootFolder() {
  mRootFolder->clear();
  for(auto system = msystems->begin(); system != msystems->end(); system ++){
		std::vector<FileData*> games = (*system)->getRootFolder()->getFilesRecursive(GAME);
		for(auto game = games.begin(); game != games.end(); game++){
			mRootFolder->addAlreadyExisitingChild((*game));
		}
	}
}

void LobbyData::onLobbyChange() {
  refreshRootFolder();

  auto viewController = ViewController::get();
  auto view = viewController->getGameListView(this);
  viewController->reloadGameListView(view.get(), false);
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
