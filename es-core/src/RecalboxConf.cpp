#include "RecalboxConf.h"
#include <iostream>
#include <fstream>
#include "Log.h"
#include <boost/algorithm/string/predicate.hpp>

RecalboxConf *RecalboxConf::sInstance = NULL;


RecalboxConf::RecalboxConf() {
    loadRecalboxConf();
}

RecalboxConf *RecalboxConf::getInstance() {
    if (sInstance == NULL)
        sInstance = new RecalboxConf();

    return sInstance;
}

bool RecalboxConf::loadRecalboxConf() {
    return true;
}


bool RecalboxConf::saveRecalboxConf() {
    return true;
}

std::string RecalboxConf::get(const std::string &name) {
    if (confMap.count(name)) {
        return confMap[name];
    }
    return "";
}
std::string RecalboxConf::get(const std::string &name, const std::string &defaut) {
    if (confMap.count(name)) {
        return confMap[name];
    }
    return defaut;
}

void RecalboxConf::set(const std::string &name, const std::string &value) {
    confMap[name] = value;
}
