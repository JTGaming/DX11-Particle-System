// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <filesystem>
#include <fstream>
#include <map>
#include <windows.h>

#include "Configuration.h"
#include "Resources/nameof.h"

particle_data ConfigParticleData[50];
generator ConfigGenerator[50];
alignas(32) instance_data cube_instance;

static constexpr char const* VariableCategories[] = {
	"Menu",
	"Camera",
	"Particle",
	"Generator",
	"Collision"
};

void CConfig::GetAllConfigsInFolder(std::vector<std::string>& files, const std::string& path, const std::string& ext)
{
	if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
		for (auto& el : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
			if (std::filesystem::is_regular_file(el) && el.path().extension() == ext)
			{
				std::string tmp_f_name = el.path().filename().string();
				size_t pos = tmp_f_name.rfind(ext);
				std::string fName = (std::string::npos == pos) ? tmp_f_name : tmp_f_name.substr(0, pos);

				files.emplace_back(fName);
			}
}

void CConfig::LoadConfigs() {
	configFiles.clear();

	GetAllConfigsInFolder(configFiles, pathToConfigs, ".ps");
	std::sort(configFiles.begin(), configFiles.end());

	g_Options.Menu.ConfigFile = std::clamp(g_Options.Menu.ConfigFile, 0u, configFiles.size() - 1);
}

void ConfigLoad() {
	particles.clear();
	generators.clear();

	for (auto& part : ConfigParticleData)
		if (part.XUID)
			particles.emplace_back(part);

	for (auto& gen : ConfigGenerator)
		if (gen.XUID)
			generators.emplace_back(gen);
}

void ConfigSave() {
	for (uint32_t i = 0; i < 50; ++i)
		ConfigParticleData[i].XUID = ConfigGenerator[i].XUID = false;

	for (uint32_t i = 0; i < particles.size(); ++i)
	{
		ConfigParticleData[i] = particles[i];
		ConfigParticleData[i].XUID = true;
	}
	for (uint32_t i = 0; i < generators.size(); ++i)
	{
		const auto& gen = generators[i];
		ConfigGenerator[i] = gen;
		ConfigGenerator[i].XUID = true;
	}
}

void CConfig::Setup()
{
	ints.clear();
	bools.clear();
	floats.clear();
	chars.clear();
	strings.clear();
	cfg_particles.clear();
	cfg_generators.clear();

	SetupValue(g_Options.Camera.Position, XMFLOAT3(0, 0, 0), VariableCategories[1], ("Camera Position"));
	SetupValue(g_Options.Camera.FOV, 90.f, VariableCategories[1], ("Camera FOV"));
	SetupValue(g_Options.Camera.MoveSpeed, 5.f, VariableCategories[1], ("Movement Speed"));
	SetupValue(g_Options.Camera.MouseScale, 0.2f, VariableCategories[1], ("Mouse Scaling"));
	SetupValue(g_Options.Camera.ControllerScale, 0.5f, VariableCategories[1], ("Controller Scaling"));
	SetupValue(g_Options.Camera.NoGenBelowHorizon, false, VariableCategories[1], ("NoGenBelowHorizon"));
	SetupValue(g_Options.Camera.BulletTime, false, VariableCategories[1], ("BulletTime"));
	SetupValue(g_Options.Menu.Background, MenuColor::Style, VariableCategories[0], ("Background Color"));

	SetupValue(g_Options.Menu.Collision, false, VariableCategories[4], ("Collision"));
	SetupValue(cube_instance.color_scale.x, 0.4f, VariableCategories[4], ("coll r"));
	SetupValue(cube_instance.color_scale.y, 0.1f, VariableCategories[4], ("coll g"));
	SetupValue(cube_instance.color_scale.z, 0.2f, VariableCategories[4], ("coll b"));
	SetupValue(cube_instance.pos.x, 0.f, VariableCategories[4], ("coll x"));
	SetupValue(cube_instance.pos.y, 0.f, VariableCategories[4], ("coll y"));
	SetupValue(cube_instance.pos.z, 0.f, VariableCategories[4], ("coll z"));
	SetupValue(cube_instance.color_scale.w, 16.f, VariableCategories[4], ("coll scale"));

	for (int32_t i = 0; i < 50; i++)
		SetupValue(ConfigParticleData[i], VariableCategories[2], i);
	for (int32_t i = 0; i < 50; i++)
		SetupValue(ConfigGenerator[i], VariableCategories[3], i);
}

void CConfig::SetupValue(int32_t& value, int32_t def, const std::string& category, const std::string& name)
{
	value = def;
	ints.emplace_back(category, name, &value, &def);
}

void CConfig::SetupValue(float& value, float def, const std::string& category, const std::string& name)
{
	value = def;
	floats.emplace_back(category, name, &value, &def);
}

void CConfig::SetupValue(bool& value, bool def, const std::string& category, const std::string& name)
{
	value = def;
	bools.emplace_back(category, name, &value, &def);
}

void CConfig::SetupValue(char* value, const char* def, const std::string& category, const std::string& name)
{
	strncpy_s(value, 128, def, _TRUNCATE);
	chars.emplace_back(category, name, value, def);
}

void CConfig::SetupValue(std::string& value, const std::string& def, const std::string& category, const std::string& name)
{
	value = def;
	strings.emplace_back(category, name, &value, &def);
}

void CConfig::SetupValue(MenuColor& value, const MenuColor& def, const std::string& category, const std::string& name)
{
	value = def;
	floats.emplace_back(category, (name + "R"), &value.color[0], &def.color[0]);
	floats.emplace_back(category, (name + "G"), &value.color[1], &def.color[1]);
	floats.emplace_back(category, (name + "B"), &value.color[2], &def.color[2]);
}

void CConfig::SetupValue(XMFLOAT3& value, const XMFLOAT3& def, const std::string& category, const std::string& name)
{
	value = def;
	floats.emplace_back(category, (name + "X"), &value.x, &def.x);
	floats.emplace_back(category, (name + "Y"), &value.y, &def.y);
	floats.emplace_back(category, (name + "Z"), &value.z, &def.z);
}

void CConfig::SetupValue(particle_data& value, const std::string& category, int32_t idx)
{
	value = particle_data();
	cfg_particles.emplace_back(category, " Part" + std::to_string(idx), &value, nullptr);
}

void CConfig::SetupValue(generator& value, const std::string& category, int32_t idx)
{
	value = generator();
	cfg_generators.emplace_back(category, " Gen" + std::to_string(idx), &value, nullptr);
}

void CConfig::Delete()
{
	DeleteFileA((pathToConfigs + configFiles.at(g_Options.Menu.ConfigFile) + ".ps").c_str());
	LoadConfigs();
}

void CConfig::Save(bool filenameOverride)
{
	std::string config_name = g_Options.Menu.ConfigName;
	if (!config_name.empty() && filenameOverride && std::find(configFiles.begin(), configFiles.end(), config_name) == configFiles.end())
	{
	}
	else if (configFiles.size() > g_Options.Menu.ConfigFile && !filenameOverride)
		config_name = configFiles.at(g_Options.Menu.ConfigFile);
	else
		return;

	StartSave(pathToConfigs, pathToConfigs + config_name + ".ps", config_name);
}

void CConfig::Load()
{
	StartLoad(pathToConfigs, pathToConfigs + configFiles.at(g_Options.Menu.ConfigFile) + ".ps");
}

void CConfig::SaveToPath()
{
	const std::string folder = folder_to_save;
	const std::string file = file_to_save;
	std::map<std::string, std::map<std::string, std::string>> output_maps{};

	ConfigSave();

	auto insert_particle = [](std::map<std::string, std::string>& map_to_use, const ConfigValue<particle_data>& value) -> void
	{
		map_to_use.emplace(nameof(value.value->vel_type) + value.name, std::to_string(value.value->vel_type));
		map_to_use.emplace(("velocity.x") + value.name, std::to_string(value.value->velocity.x));
		map_to_use.emplace(("velocity.y") + value.name, std::to_string(value.value->velocity.y));
		map_to_use.emplace(("velocity.z") + value.name, std::to_string(value.value->velocity.z));
		map_to_use.emplace(("start_color[0]") + value.name, std::to_string(value.value->start_color[0]));
		map_to_use.emplace(("start_color[1]") + value.name, std::to_string(value.value->start_color[1]));
		map_to_use.emplace(("start_color[2]") + value.name, std::to_string(value.value->start_color[2]));
		map_to_use.emplace(("start_color[3]") + value.name, std::to_string(value.value->start_color[3]));
		map_to_use.emplace(("end_color[0]") + value.name, std::to_string(value.value->end_color[0]));
		map_to_use.emplace(("end_color[1]") + value.name, std::to_string(value.value->end_color[1]));
		map_to_use.emplace(("end_color[2]") + value.name, std::to_string(value.value->end_color[2]));
		map_to_use.emplace(("end_color[3]") + value.name, std::to_string(value.value->end_color[3]));
		map_to_use.emplace(nameof(value.value->rainbow) + value.name, std::to_string(value.value->rainbow));
		map_to_use.emplace(nameof(value.value->gravscale) + value.name, std::to_string(value.value->gravscale));
		map_to_use.emplace(nameof(value.value->airdrag) + value.name, std::to_string(value.value->airdrag));
		map_to_use.emplace(nameof(value.value->lifetime) + value.name, std::to_string(value.value->lifetime));
		map_to_use.emplace(nameof(value.value->name) + value.name, (value.value->name));
		map_to_use.emplace(nameof(value.value->cycle_speed) + value.name, std::to_string(value.value->cycle_speed));
	};
	
	auto insert_generator = [](std::map<std::string, std::string>& map_to_use, const ConfigValue<generator>& value) -> void
	{
		map_to_use.emplace(nameof(value.value->should_collide) + value.name, std::to_string(value.value->should_collide));
		map_to_use.emplace(nameof(value.value->bounciness) + value.name, std::to_string(value.value->bounciness));
		map_to_use.emplace(nameof(value.value->stickiness) + value.name, std::to_string(value.value->stickiness));
		map_to_use.emplace(nameof(value.value->particle_limit) + value.name, std::to_string(value.value->particle_limit));
		map_to_use.emplace(nameof(value.value->emit_on_mouse) + value.name, std::to_string(value.value->emit_on_mouse));
		map_to_use.emplace(nameof(value.value->min_spread) + value.name, std::to_string(value.value->min_spread));
		map_to_use.emplace(nameof(value.value->particle_index) + value.name, std::to_string(value.value->particle_index));
		map_to_use.emplace(nameof(value.value->lifetime) + value.name, std::to_string(value.value->lifetime));
		map_to_use.emplace(nameof(value.value->orbit_speed) + value.name, std::to_string(value.value->orbit_speed));
		map_to_use.emplace(nameof(value.value->spatial_aware) + value.name, std::to_string(value.value->spatial_aware));
		map_to_use.emplace(nameof(value.value->location) + value.name, std::to_string(value.value->location));
		map_to_use.emplace(nameof(value.value->obj_follow) + value.name, std::to_string(value.value->obj_follow));
		map_to_use.emplace(nameof(value.value->pos_type) + value.name, std::to_string(value.value->pos_type));
		map_to_use.emplace(nameof(value.value->follow_object) + value.name, std::to_string(value.value->follow_object));
		map_to_use.emplace(("spread.x") + value.name, std::to_string(value.value->spread.x));
		map_to_use.emplace(("spread.y") + value.name, std::to_string(value.value->spread.y));
		map_to_use.emplace(("spread.z") + value.name, std::to_string(value.value->spread.z));
		map_to_use.emplace(("position.x") + value.name, std::to_string(value.value->position.x));
		map_to_use.emplace(("position.y") + value.name, std::to_string(value.value->position.y));
		map_to_use.emplace(("position.z") + value.name, std::to_string(value.value->position.z));
		map_to_use.emplace(("origin.x") + value.name, std::to_string(value.value->origin.x));
		map_to_use.emplace(("origin.y") + value.name, std::to_string(value.value->origin.y));
		map_to_use.emplace(("origin.z") + value.name, std::to_string(value.value->origin.z));
		map_to_use.emplace(("offset_location.x") + value.name, std::to_string(value.value->offset_location.x));
		map_to_use.emplace(("offset_location.y") + value.name, std::to_string(value.value->offset_location.y));
		map_to_use.emplace(("offset_location.z") + value.name, std::to_string(value.value->offset_location.z));
		map_to_use.emplace(nameof(value.value->gen_at_once) + value.name, std::to_string(value.value->gen_at_once));
		map_to_use.emplace(nameof(value.value->gen_limit) + value.name, std::to_string(value.value->gen_limit));
		map_to_use.emplace(nameof(value.value->enabled) + value.name, std::to_string(value.value->enabled));
		map_to_use.emplace(nameof(value.value->name) + value.name, (value.value->name));
		map_to_use.emplace(nameof(value.value->perlin_enabled) + value.name, std::to_string(value.value->perlin_enabled));
		map_to_use.emplace(nameof(value.value->perlin_threshold) + value.name, std::to_string(value.value->perlin_threshold));
		map_to_use.emplace(nameof(value.value->perlin_scalar) + value.name, std::to_string(value.value->perlin_scalar));
		map_to_use.emplace(("perlin_animated.x") + value.name, std::to_string(value.value->offset_location.x));
		map_to_use.emplace(("perlin_animated.y") + value.name, std::to_string(value.value->offset_location.y));
		map_to_use.emplace(("perlin_animated.z") + value.name, std::to_string(value.value->offset_location.z));
	};

	for (const auto& value : cfg_particles)
		if (value.value->XUID)
		{
			auto found_map = output_maps.find(value.category);
			if (found_map != output_maps.end())
				insert_particle(found_map->second, value);
			else
			{
				std::map<std::string, std::string> new_map{};
				insert_particle(new_map, value);
				output_maps.emplace(value.category, new_map);
			}
		}
	for (const auto& value : cfg_generators)
		if (value.value->XUID)
		{
			auto found_map = output_maps.find(value.category);
			if (found_map != output_maps.end())
				insert_generator(found_map->second, value);
			else
			{
				std::map<std::string, std::string> new_map{};
				insert_generator(new_map, value);
				output_maps.emplace(value.category, new_map);
			}
		}

	for (const auto& value : ints)
	{
		auto found_map = output_maps.find(value.category);
		if (found_map != output_maps.end())
			found_map->second.emplace(value.name, std::to_string(*value.value));
		else
		{
			std::map<std::string, std::string> new_map{};
			new_map.emplace(value.name, std::to_string(*value.value));
			output_maps.emplace(value.category, new_map);
		}
	}

	for (const auto& value : floats)
	{
		auto found_map = output_maps.find(value.category);
		if (found_map != output_maps.end())
			found_map->second.emplace(value.name, std::to_string(*value.value));
		else
		{
			std::map<std::string, std::string> new_map{};
			new_map.emplace(value.name, std::to_string(*value.value));
			output_maps.emplace(value.category, new_map);
		}
	}

	for (const auto& value : bools)
	{
		auto found_map = output_maps.find(value.category);
		if (found_map != output_maps.end())
			found_map->second.emplace(value.name, std::to_string(*value.value));
		else
		{
			std::map<std::string, std::string> new_map{};
			new_map.emplace(value.name, std::to_string(*value.value));
			output_maps.emplace(value.category, new_map);
		}
	}

	for (const auto& value : chars)
	{
		auto found_map = output_maps.find(value.category);
		if (found_map != output_maps.end())
			found_map->second.emplace(value.name, value.value);
		else
		{
			std::map<std::string, std::string> new_map{};
			new_map.emplace(value.name, value.value);
			output_maps.emplace(value.category, new_map);
		}
	}

	for (const auto& value : strings)
	{
		auto found_map = output_maps.find(value.category);
		if (found_map != output_maps.end())
			found_map->second.emplace(value.name, *value.value);
		else
		{
			std::map<std::string, std::string> new_map{};
			new_map.emplace(value.name, *value.value);
			output_maps.emplace(value.category, new_map);
		}
	}

	std::ofstream output_file(file, std::ios::out | std::ios::trunc);
	if (output_file.is_open())
	{
		for (const auto& category : output_maps)
		{
			output_file << "[" << category.first << "]" << "\n";
			for (const auto& key : category.second)
				output_file << key.first << "=" << key.second << "\n";
		}
		output_file.close();
	}

	for (int32_t i = 0; i < 50; ++i)
	{
		ConfigParticleData[i] = particle_data();
		ConfigGenerator[i] = generator();
	}
	LoadConfigs();
}

void CConfig::StartLoad(const std::string& folder, const std::string& file)
{
	folder_to_load = folder;
	file_to_load = file;

	LoadFromPath();
}

void CConfig::StartSave(const std::string& folder, const std::string& file, const std::string& cfg_name)
{
	folder_to_save = folder;
	file_to_save = file;
	saveconfig_name = cfg_name;
	SaveToPath();
}

void CConfig::LoadFromPath()
{
	g_Options.Menu.PauseToggle = false;
	std::string folder = folder_to_load;
	std::string file = file_to_load;

	std::map<std::string, std::map<std::string, std::string>> input_maps{};

	std::string line;
	std::ifstream input_file(file);
	if (input_file.is_open())
	{
		std::string current_cat{};
		std::map<std::string, std::string> current_map{};

		while (getline(input_file, line))
		{
			size_t open_br = line.find('[');
			size_t close_br = line.find(']');
			if (!open_br && close_br != std::string::npos)
			{
				if (current_cat.length() && current_map.size())
				{
					input_maps.emplace(current_cat, current_map);
					current_map.clear();
				}
				current_cat = line.substr(1, line.length() - 2);
			}
			else
			{
				size_t sep = line.find('=');
				std::string key = line.substr(0, sep);
				std::string val = line.substr(sep + 1);
				current_map.emplace(key, val);
			}
		}
		if (current_cat.length() && current_map.size())
			input_maps.emplace(current_cat, current_map);

		input_file.close();
	}
	else
		return;

	auto load_particle = [](std::map<std::string, std::string>& map_to_use, const ConfigValue<particle_data>& value) -> void
	{
		auto key = map_to_use.find(nameof(value.value->vel_type) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->vel_type = atoi(key->second.c_str());

		key = map_to_use.find(("velocity.x") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->velocity.x = (float)atof(key->second.c_str());

		key = map_to_use.find(("velocity.y") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->velocity.y = (float)atof(key->second.c_str());

		key = map_to_use.find(("velocity.z") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->velocity.z = (float)atof(key->second.c_str());

		key = map_to_use.find(("start_color[0]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->start_color[0] = (float)atof(key->second.c_str());

		key = map_to_use.find(("start_color[1]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->start_color[1] = (float)atof(key->second.c_str());

		key = map_to_use.find(("start_color[2]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->start_color[2] = (float)atof(key->second.c_str());

		key = map_to_use.find(("start_color[3]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->start_color[3] = (float)atof(key->second.c_str());

		key = map_to_use.find(("end_color[0]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->end_color[0] = (float)atof(key->second.c_str());

		key = map_to_use.find(("end_color[1]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->end_color[1] = (float)atof(key->second.c_str());

		key = map_to_use.find(("end_color[2]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->end_color[2] = (float)atof(key->second.c_str());

		key = map_to_use.find(("end_color[3]") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->end_color[3] = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->rainbow) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->rainbow = (bool)atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->gravscale) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->gravscale = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->airdrag) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->airdrag = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->lifetime) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->lifetime = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->cycle_speed) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->cycle_speed = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->name) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->name = key->second;

		value.value->XUID = true;
	};

	auto load_generator = [](std::map<std::string, std::string>& map_to_use, const ConfigValue<generator>& value) -> void
	{
		auto key = map_to_use.find(nameof(value.value->bounciness) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->bounciness = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->stickiness) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->stickiness = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->should_collide) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->should_collide = (bool)atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->particle_limit) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->particle_limit = atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->emit_on_mouse) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->emit_on_mouse = (bool)atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->min_spread) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->min_spread = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->particle_index) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->particle_index = atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->lifetime) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->lifetime = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->orbit_speed) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->orbit_speed = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->spatial_aware) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->spatial_aware = (bool)atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->location) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->location = atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->obj_follow) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->obj_follow = atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->pos_type) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->pos_type = atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->follow_object) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->follow_object = (bool)atoi(key->second.c_str());

		key = map_to_use.find(("spread.x") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->spread.x = (float)atof(key->second.c_str());

		key = map_to_use.find(("spread.y") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->spread.y = (float)atof(key->second.c_str());

		key = map_to_use.find(("spread.z") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->spread.z = (float)atof(key->second.c_str());

		key = map_to_use.find(("position.x") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->position.x = (float)atof(key->second.c_str());

		key = map_to_use.find(("position.y") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->position.y = (float)atof(key->second.c_str());

		key = map_to_use.find(("position.z") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->position.z = (float)atof(key->second.c_str());

		key = map_to_use.find(("origin.x") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->origin.x = (float)atof(key->second.c_str());

		key = map_to_use.find(("origin.y") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->origin.y = (float)atof(key->second.c_str());

		key = map_to_use.find(("origin.z") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->origin.z = (float)atof(key->second.c_str());

		key = map_to_use.find(("offset_location.x") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->offset_location.x = (float)atof(key->second.c_str());

		key = map_to_use.find(("offset_location.y") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->offset_location.y = (float)atof(key->second.c_str());

		key = map_to_use.find(("offset_location.z") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->offset_location.z = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->gen_at_once) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->gen_at_once = (bool)atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->gen_limit) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->gen_limit = atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->enabled) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->enabled = (bool)atoi(key->second.c_str());

		key = map_to_use.find(nameof(value.value->name) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->name = (key->second.c_str());

		key = map_to_use.find(nameof(value.value->perlin_enabled) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->perlin_enabled = (bool)atoi(key->second.c_str());

		key = map_to_use.find(("perlin_animated.x") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->perlin_animated.x = (float)atof(key->second.c_str());

		key = map_to_use.find(("perlin_animated.y") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->perlin_animated.y = (float)atof(key->second.c_str());

		key = map_to_use.find(("perlin_animated.z") + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->perlin_animated.z = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->perlin_scalar) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->perlin_scalar = (float)atof(key->second.c_str());

		key = map_to_use.find(nameof(value.value->perlin_threshold) + value.name);
		if (key != map_to_use.end())
			if (key->second.length())
				value.value->perlin_threshold = (float)atof(key->second.c_str());

		value.value->XUID = true;
	};

	for (const auto& value : cfg_particles)
	{
		*value.value = *value.def;

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
			for (const auto& val : map->second)
				if (val.first.find(value.name) != std::string::npos)
				{
					load_particle(map->second, value);
					goto cont1;
				}
	cont1:;
	}
	
	for (const auto& value : cfg_generators)
	{
		*value.value = *value.def;

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
			for (const auto& val : map->second)
				if (val.first.find(value.name) != std::string::npos)
				{
					load_generator(map->second, value);
					goto cont2;
				}
	cont2:;
	}

	for (const auto& value : ints)
	{
		*value.value = *value.def;

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
		{
			auto key = map->second.find(value.name);
			if (key != map->second.end())
				if (key->second.length())
					*value.value = atoi(key->second.c_str());
		}
	}

	for (const auto& value : floats)
	{
		*value.value = *value.def;

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
		{
			auto key = map->second.find(value.name);
			if (key != map->second.end())
				if (key->second.length())
					*value.value = (float)atof(key->second.c_str());
		}
	}

	for (const auto& value : bools)
	{
		*value.value = *value.def;

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
		{
			auto key = map->second.find(value.name);
			if (key != map->second.end())
				if (key->second.length())
					*value.value = (bool)atoi(key->second.c_str());
		}
	}

	for (const auto& value : chars)
	{
		strncpy_s(value.value, 128, value.def, _TRUNCATE);

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
		{
			auto key = map->second.find(value.name);
			if (key != map->second.end())
				if (key->second.length())
					strncpy_s(value.value, std::min(128u, key->second.length() + 1), key->second.c_str(), _TRUNCATE);
		}
	}

	for (const auto& value : strings)
	{
		*value.value = *value.def;

		auto map = input_maps.find(value.category);
		if (map != input_maps.end())
		{
			auto key = map->second.find(value.name);
			if (key != map->second.end())
				if (key->second.length())
					*value.value = key->second;
		}
	}

	ConfigLoad();
}

Variables g_Options;
CConfig ConfigSys;
std::vector<std::string> configFiles;
std::string pathToConfigs;
