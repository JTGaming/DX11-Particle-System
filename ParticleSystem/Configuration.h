#pragma once
#include <vector>
#include <string>

#include "Particle.h"

template <typename T>
class ConfigValue
{
public:
	ConfigValue(const std::string& category_, const std::string& name_, T* value_, const T* const deflt) : category(category_), name(name_), value(value_)
	{
		if (!def)
		{
			def = new T;
			*def = deflt ? *deflt : *value_;
		}
	}

	ConfigValue(const std::string& category_, const std::string& name_, T* value_, const T& deflt) : category(category_), name(name_), value(value_)
	{
		if (!def)
		{
			def = new T;
			*def = deflt;
		}
	}

	std::string category, name;
	T* value, * def = nullptr;
};

class CConfig
{
protected:
	std::vector<ConfigValue<int32_t>> ints;
	std::vector<ConfigValue<bool>> bools;
	std::vector<ConfigValue<float>> floats;
	std::vector<ConfigValue<char>> chars;
	std::vector<ConfigValue<std::string>> strings;
	std::vector<ConfigValue<particle_data>> cfg_particles;
	std::vector<ConfigValue<generator>> cfg_generators;

private:

	void SetupValue(int32_t&, int32_t, const std::string&, const std::string&);
	void SetupValue(bool&, bool, const std::string&, const std::string&);
	void SetupValue(float&, float, const std::string&, const std::string&);
	void SetupValue(char*, const char*, const std::string&, const std::string&);
	void SetupValue(std::string&, const std::string&, const std::string&, const std::string&);
	void SetupValue(MenuColor&, const MenuColor&, const std::string&, const std::string&);
	void SetupValue(XMFLOAT3&, const XMFLOAT3&, const std::string&, const std::string&);
	void SetupValue(particle_data&, const std::string&, int32_t);
	void SetupValue(generator&, const std::string&, int32_t);

public:
	CConfig()
	{
		Setup();
	}

	void Setup();
	void LoadConfigs();

	void Save(bool FNO = false);
	void StartSave(const std::string&, const std::string&, const std::string & = "");
	void Load();
	void StartLoad(const std::string&, const std::string&);
	void Delete();

protected:
	void SaveToPath();
	void LoadFromPath();
	void GetAllConfigsInFolder(std::vector<std::string>& files, const std::string& path, const std::string& ext);

	std::string folder_to_load;
	std::string file_to_load;
	std::string folder_to_save;
	std::string file_to_save;
	std::string saveconfig_name;
};

extern CConfig ConfigSys;
extern std::vector<std::string> configFiles;
extern std::string pathToConfigs;
extern instance_data cube_instance;