//
// Created by zr on 22-11-9.
//

#ifndef __ORM_RULES_HPP__
#define __ORM_RULES_HPP__
#include <mutex>
#include "nlohmann/json.hpp"
#include <fstream>
using json = nlohmann::json;

class Config
{
public:
    static Config* getInstance();
    auto operator[](const std::string& key);
private:
    Config();
    static Config* instance;
    static std::mutex mu;
    json config;
};

Config* Config::instance = nullptr;
std::mutex Config::mu;

Config* Config::getInstance()
{
    if (!instance)
    {
        std::lock_guard<std::mutex> lg(mu);
        if (!instance)
            instance = new Config();
    }
    return instance;
}

Config::Config()
{
    char buf[50];
    memset(buf, 0, sizeof(buf));
    realpath(__FILE__, buf);
    std::string path(buf);
    size_t pos = path.find_last_of("/");
    path = path.substr(0, pos + 1);
    std::ifstream conf(path + "rules.json");
    config = json::parse(conf);
}

auto Config::operator[](const std::string &key)
{
    return config[key];
}

#endif
