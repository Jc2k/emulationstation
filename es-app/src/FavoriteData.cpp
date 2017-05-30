#include "FavoriteData.h"

FavoriteData::FavoriteData(std::string fullName, std::string command, std::string themeFolder, std::vector<SystemData*>* systems) :
    SystemData(std::string("favorites"), std::string(fullName), std::string(command), std::string(themeFolder), systems) {

}
