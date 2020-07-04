#pragma once

#include "Omnia.pch"
#include "Core.h"
#include "Omnia/Log.h"


// ToDo: Add yaml-cpp to the project.
#include <yaml-cpp/yaml.h>

namespace Omnia {

class Config {
	std::string ConfigFile;
	YAML::Node ConfigData;

	std::string AppCaption;
	std::string AppVersion;

	size_t WindowHeight;
	size_t WindowWidth;

public:
	Config(const string &object = "./Data/config.yml");
	~Config() = default;

	template <typename T = string>
	T GetSetting(const string &key, const string &value) const{
		if (ConfigData["Settings"][key][value].IsDefined()) {
			try {
				return ConfigData["Settings"][key][value].as<T>();
			} catch (std::exception ex) {
				AppLogError("[Config]: Could not deduce the type of '", key, ":", value, "'!");
				return T {};
			}
		} else {
			AppLogWarning("[Config]: The requested setting '", key, ":", value, "' doesn't exist!");
		}
		return T {};
	}
};

}
