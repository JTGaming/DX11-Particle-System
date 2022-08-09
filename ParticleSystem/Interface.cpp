// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Windows.h>
#include <ShellAPI.h>
#include <map>

#include "Interface.h"
#include "DirectX.h"
#include "Configuration.h"

#include "Resources/ImGui/backends/imgui_impl_win32.h"
#include "Resources/ImGui/backends/imgui_impl_dx11.h"
#include "Engine.h"
#include "XboxController.h"
#include "Profiler.h"

void ImGuiRender(bool window_focused)
{
	SCOPE_EXEC_TIME("GUI");
	ImGuiIO& io = ImGui::GetIO();

	// Start the Dear ImGui frame
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//debug menu
	if (g_Options.Menu.DebugMenu)
	{
		ImGui::SetNextWindowPos(DebugPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(DebugSize);

		ImGui::Begin("##debug_menu", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		{
			char head[48];
			sprintf_s(head, "ROT Yaw: %.2f | Pitch: %.2f", g_Options.Camera.Rotation.y, g_Options.Camera.Rotation.x);
			ImGui::CenteredText(head);
			sprintf_s(head, "POS X: %.2f | Y: %.2f | Z: %.2f", g_Options.Camera.Position.x, g_Options.Camera.Position.y, g_Options.Camera.Position.z);
			ImGui::CenteredText(head);
			ImGui::Separator();

			sprintf_s(head, "Particles: %d", (int32_t)g_Options.Menu.ParticleCount);
			ImGui::CenteredText(head);
			ImGui::Separator();

			if (!g_Profiler.avg_timeframes.empty())
			{
				sprintf_s(head, "FPS: %.0f    |    Frametime: %.3f ms", 1000.f / g_Profiler.avg_timeframes[0].second, g_Profiler.avg_timeframes[0].second);
				ImGui::CenteredText(head);
				ImGui::CenteredText(g_Options.Menu.AvgThroughput.c_str());

				for (uint32_t i = 1; i < g_Profiler.avg_timeframes.size(); ++i)
				{
					const auto& profile = g_Profiler.avg_timeframes[i];
					sprintf_s(head, "%s: %.3f ms", profile.first.c_str(), profile.second);
					ImGui::CenteredText(head);
				}
			}
		}

		ImGui::End();
	}

	RenderPopupBoxes();

	//setup interface
	if (interface_open)
	{
		ImGui::SetNextWindowSize(InterfaceSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(InterfacePos, ImGuiCond_Always);

		ImGui::Begin(szFileName_narrow, NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::SelectableButton("Generators", true, (int32_t*)&g_Options.Menu.SelectedMenu, 0, MenuButtonSize);
		ImGui::SameLine();
		ImGui::SelectableButton("Particles", true, (int32_t*)&g_Options.Menu.SelectedMenu, 1, MenuButtonSize);
		ImGui::SameLine();
		ImGui::SelectableButton("Camera", true, (int32_t*)&g_Options.Menu.SelectedMenu, 2, MenuButtonSize);
		ImGui::SameLine();
		ImGui::SelectableButton("Debug", true, (int32_t*)&g_Options.Menu.SelectedMenu, 3, MenuButtonSize);

		switch (g_Options.Menu.SelectedMenu)
		{
		case(0): //Generators
		{
			ImGui::InputText("Name", g_Options.Menu.GeneratorName, IM_ARRAYSIZE(g_Options.Menu.GeneratorName));
			if (ImGui::Selectable("Create New") & (g_Options.Menu.GeneratorName[0] != '\0'))
			{
				if (generators.size() > g_Options.Menu.SelectedGenerator)
				{
					generator gen = generators.at(g_Options.Menu.SelectedGenerator);
					gen.name = g_Options.Menu.GeneratorName;
					gen.origin = XMF3AddRet(g_Options.Camera.Position, XMF3MultRet(AngleVectorsReturn(g_Options.Camera.Rotation), 16));
					generators.emplace_back(std::move(gen));
				}
				else
					generators.emplace_back(g_Options.Menu.GeneratorName);
				
				g_Options.Menu.SelectedGenerator = generators.size() - 1;
				g_Options.Menu.GeneratorName[0] = '\0';
			}
			ImGui::Separator();

			ImGui::Combo("Generator", (int32_t*)&g_Options.Menu.SelectedGenerator,
				[](void* data, int32_t idx, char const** out_text) {
					*out_text = ((const std::vector<generator>*)data)->at(idx).name.c_str(); return true;
				}, (void*)&generators, (int32_t)generators.size());

			if (ImGui::Selectable("Delete") & (generators.size() > g_Options.Menu.SelectedGenerator))
			{
				generators.erase(generators.begin() + g_Options.Menu.SelectedGenerator);
				g_Options.Menu.SelectedGenerator = std::clamp(g_Options.Menu.SelectedGenerator, 0u, generators.size() - 1);
			}
			ImGui::Separator();

			if (generators.size() > g_Options.Menu.SelectedGenerator)
			{
				generator& gen = generators.at(g_Options.Menu.SelectedGenerator);

				if (ImGui::Selectable("Enabled", &gen.enabled))
					gen.start_life = g_Options.Menu.CurrentTime;
				if (ImGui::SliderFloat("Lifetime", &gen.lifetime, 0.f, 5, "%.1f"))
					gen.start_life = g_Options.Menu.CurrentTime;
				ImGui::Combo("Particle", (int32_t*)&gen.particle_index,
					[](void* data, int32_t idx, char const** out_text) {
						*out_text = ((const std::vector<particle_data>*)data)->at(idx).name.c_str(); return true;
					}, (void*)&particles, (int32_t)particles.size());
				constexpr uint32_t gen_limit = 500000;
				if (ImGui::Selectable("Update Position"))
					gen.origin = g_Options.Camera.Position;

				ImGui::SliderInt("Particle Limit", (int32_t*)&gen.particle_limit, 100, gen_limit);
				if (gen.particle_limit > gen_limit)
					gen.particle_limit = gen_limit;

				ImGui::Selectable("Emit On Click", &gen.emit_on_mouse);
				ImGui::Selectable("Generate at Once", &gen.gen_at_once);
				if (gen.gen_at_once)
					ImGui::SliderInt("Disable After X", (int32_t*)&gen.gen_limit, 0, 25, "%.f");

				ImGui::Selectable("Follow Object", &gen.follow_object);
				if (gen.follow_object)
				{
					std::vector<std::string> followables{};
					followables.emplace_back("Camera");
					for (const auto& gen2 : generators)
						followables.emplace_back(gen2.name);
					ImGui::Combo("Object", (int32_t*)&gen.obj_follow,
						[](void* data, int32_t idx, char const** out_text) {
							*out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true;
						}, (void*)&followables, (int32_t)followables.size());
				}
				ImGui::Separator();

				ImGui::Combo("Location", (int32_t*)&gen.location, LocationTypes, ARRAYSIZE(LocationTypes));
				ImGui::Selectable("Spatially Aware", &gen.spatial_aware);

				switch (gen.location)
				{
				case(generator::STATIC):
				{
					if (gen.follow_object && !gen.obj_follow)
					{
						ImGui::SliderFloat("Offset (B - F)", &gen.offset_location.x, -20, 20, "%.f");
						ImGui::SliderFloat("Offset (L - R)", &gen.offset_location.y, -20, 20, "%.f");
						ImGui::SliderFloat("Offset (U - D)", &gen.offset_location.z, -20, 20, "%.f");
					}
					else
					{
						ImGui::SliderFloat("Offset (X)", &gen.offset_location.x, -20, 20, "%.f");
						ImGui::SliderFloat("Offset (Y)", &gen.offset_location.y, -20, 20, "%.f");
						ImGui::SliderFloat("Offset (Z)", &gen.offset_location.z, -20, 20, "%.f");
					}
				}
				break;
				case(generator::ORBIT):
				{
					ImGui::SliderFloat("Orbit Speed", &gen.orbit_speed, 0.1f, 5, "%.1f");
					ImGui::SliderFloat("Scale (X)", &gen.offset_location.x, -20, 20, "%.f");
					ImGui::SliderFloat("Scale (Y)", &gen.offset_location.y, -20, 20, "%.f");
					ImGui::SliderFloat("Scale (Z)", &gen.offset_location.z, -20, 20, "%.f");
				}
				break;
				}
				ImGui::Separator();

				ImGui::Combo("Spread Type", (int32_t*)&gen.pos_type, SpreadTypes, ARRAYSIZE(SpreadTypes));
				ImGui::SliderFloat("Spread Offset", &gen.min_spread, 0, 100, "%.f");

				switch (gen.pos_type)
				{
				case(generator::POSITION_TYPE::CIRCLE):
				case(generator::POSITION_TYPE::STAR):
				{
					ImGui::SliderFloat("Thickness", &gen.spread.y, 0, 20, "%.f");
				} //-V796
				case(generator::POSITION_TYPE::CUBE):
				case(generator::POSITION_TYPE::SPHERE):
				case(generator::POSITION_TYPE::SPHERE_OUTLINE):
				{
					ImGui::SliderFloat("Scale", &gen.spread.x, 0, 20, "%.f");
				}
				break;
				case(generator::POSITION_TYPE::CUSTOM):
				{
					ImGui::SliderFloat("Start Spread (X)", &gen.spread.x, 0, 20, "%.f");
					ImGui::SliderFloat("Start Spread (Y)", &gen.spread.y, 0, 20, "%.f");
					ImGui::SliderFloat("Start Spread (Z)", &gen.spread.z, 0, 20, "%.f");
				}
				break;
				}
				ImGui::Separator();
				ImGui::Selectable("Perlin Noise", &gen.perlin_enabled);
				if (gen.perlin_enabled)
				{
					ImGui::SliderFloat("Threshold", &gen.perlin_threshold, 0.1f, 0.9f, "%.2f");
					ImGui::SliderFloat("Scale##perlin", &gen.perlin_scalar, 0.1f, 1.f, "%.2f");
					ImGui::SliderFloat("Animation (X)", &gen.perlin_animated.x, 0.f, 10.f, "%.1f");
					ImGui::SliderFloat("Animation (Y)", &gen.perlin_animated.y, 0.f, 10.f, "%.1f");
					ImGui::SliderFloat("Animation (Z)", &gen.perlin_animated.z, 0.f, 10.f, "%.1f");
				}

				if (g_Options.Menu.Collision)
				{
					ImGui::Separator();
					ImGui::Selectable("Check Collisions", &gen.should_collide);

					ImGui::SliderFloat("Bounciness", &gen.bounciness, 0.f, 1.f, "%.1f");
					ImGui::SliderFloat("Stickiness", &gen.stickiness, 0.f, 1.f, "%.1f");
				}
			}
		}
		break;
		case(1): //Particles
		{
			ImGui::InputText("Name", g_Options.Menu.ParticleName, IM_ARRAYSIZE(g_Options.Menu.ParticleName));
			if (ImGui::Selectable("Create New") & (g_Options.Menu.ParticleName[0] != '\0'))
			{
				if (particles.size() > g_Options.Menu.SelectedParticle)
				{
					particle_data part = particles.at(g_Options.Menu.SelectedParticle);
					part.name = g_Options.Menu.ParticleName;
					particles.emplace_back(std::move(part));
				}
				else
					particles.emplace_back(g_Options.Menu.ParticleName);

				g_Options.Menu.SelectedParticle = particles.size() - 1;
				g_Options.Menu.ParticleName[0] = '\0';
			}
			ImGui::Separator();

			ImGui::Combo("Particle", (int32_t*)&g_Options.Menu.SelectedParticle,
				[](void* data, int32_t idx, char const** out_text) {
					*out_text = ((const std::vector<particle_data>*)data)->at(idx).name.c_str(); return true;
				}, (void*)&particles, (int32_t)particles.size());

			if (ImGui::Selectable("Delete") & (particles.size() > g_Options.Menu.SelectedParticle))
			{
				particles.erase(particles.begin() + g_Options.Menu.SelectedParticle);
				g_Options.Menu.SelectedParticle = std::clamp(g_Options.Menu.SelectedParticle, 0u, particles.size() - 1);
			}
			ImGui::Separator();

			if (particles.size() > g_Options.Menu.SelectedParticle)
			{
				particle_data& part = particles.at(g_Options.Menu.SelectedParticle);

				ImGui::SliderFloat("Start Scale", &part.start_color[3], 0.1f, 10.f, "%.1f");
				ImGui::SliderFloat("End Scale", &part.end_color[3], 0.1f, 10.f, "%.1f");
				ImGui::Separator();

				ImGui::Combo("Velocity Type", &part.vel_type, VelocityTypes, ARRAYSIZE(VelocityTypes));
				switch (part.vel_type)
				{
				case(particle_data::VELOCITY_TYPE::CUBE): //cube
				case(particle_data::VELOCITY_TYPE::SPHERE): //sphere
				case(particle_data::VELOCITY_TYPE::MOVEMENT): //movement
				case(particle_data::VELOCITY_TYPE::OUTLINE): //movement
				case(particle_data::VELOCITY_TYPE::FORWARDS): //forwards
				{
					ImGui::SliderFloat("Magnitude", &part.velocity.x, 1, 20, "%.f");
				}
				break;
				case(particle_data::VELOCITY_TYPE::CUSTOM): //custom
				{
					ImGui::SliderFloat("Velocity (X)", &part.velocity.x, -20, 20, "%.f");
					ImGui::SliderFloat("Velocity (Y)", &part.velocity.y, -20, 20, "%.f");
					ImGui::SliderFloat("Velocity (Z)", &part.velocity.z, -20, 20, "%.f");
				}
				break;
				}
				ImGui::Separator();

				ImGui::SliderFloat("Air Drag", &part.airdrag, 0.1f, 1.f, "%.3f");
				ImGui::SliderFloat("Gravity Scale", &part.gravscale, 0.1f, 10, "%.2f");
				ImGui::SliderFloat("Lifetime", &part.lifetime, 0.1f, 5, "%.1f");
				ImGui::Separator();

				ImGui::Selectable("Rainbow Color", &part.rainbow);
				if (part.rainbow)
					ImGui::SliderFloat("Cycle Lifetime", &part.cycle_speed, 1, 50, "%.1f");
				else
				{
					ImGui::ColorEdit3("Start Color", part.start_color, ImGuiColorEditFlags_NoInputs);// , ImGui::ImGuiColorEditFlags_NoSliders);
					ImGui::ColorEdit3("End Color", part.end_color, ImGuiColorEditFlags_NoInputs);//, ImGui::ImGuiColorEditFlags_NoSliders);
				}
			}
		}
		break;
		case(2): //Camera
		{
			ImGui::SliderFloat("Move Speed", &g_Options.Camera.MoveSpeed, 1, 10, "%.f");
			ImGui::SliderFloat("Mouse Scaling", &g_Options.Camera.MouseScale, 0.01f, 1, "%.2f");
			ImGui::SliderFloat("Controller Scaling", &g_Options.Camera.ControllerScale, 0.01f, 1, "%.2f");
			ImGui::SliderFloat("Field of View", &g_Options.Camera.FOV, 60.f, 110.f, "%.f");
			ImGui::Selectable("Bullet Time", &g_Options.Camera.BulletTime);
		}
		break;
		case(3): //Debug
		{
			if (ImGui::Combo("Config", (int32_t*)&g_Options.Menu.ConfigFile,
				[](void* data, int32_t idx, char const** out_text) {
					*out_text = ((const std::vector<std::string>*)data)->at(idx).c_str(); return true;
				}, (void*)&configFiles, (int32_t)configFiles.size()))
				g_Options.Menu.LoadBox = g_Options.Menu.SaveBox = g_Options.Menu.DeleteBox = g_Options.Menu.NewConfigBox = false;

			if (ImGui::Selectable("Load", false))
				if (!g_Options.Menu.LoadBox)
				{
					g_Options.Menu.LoadBox = true;
					g_Options.Menu.SaveBox = g_Options.Menu.DeleteBox = g_Options.Menu.NewConfigBox = false;
				}
			if (ImGui::Selectable("Save", false))
				if (!g_Options.Menu.SaveBox && configFiles.size() > g_Options.Menu.ConfigFile && configFiles.at(g_Options.Menu.ConfigFile).length() > 0)
				{
					g_Options.Menu.SaveBox = true;
					g_Options.Menu.LoadBox = g_Options.Menu.DeleteBox = g_Options.Menu.NewConfigBox = false;
				}
			if (ImGui::Selectable("Delete", false))
				if (!g_Options.Menu.DeleteBox && configFiles.size() > g_Options.Menu.ConfigFile && configFiles.at(g_Options.Menu.ConfigFile).length() > 0)
				{
					g_Options.Menu.DeleteBox = true;
					g_Options.Menu.LoadBox = g_Options.Menu.SaveBox = g_Options.Menu.NewConfigBox = false;
				}
			if (ImGui::Selectable("Create New", false))
				if (!g_Options.Menu.NewConfigBox)
				{
					g_Options.Menu.NewConfigBox = true;
					g_Options.Menu.LoadBox = g_Options.Menu.SaveBox = g_Options.Menu.DeleteBox = false;
				}
			ImGui::Separator();
			
			if (ImGui::Selectable("Reset"))
				ResetSimulation();
			if (ImGui::Selectable("Open Config Folder", false))
				ShellExecuteA(NULL, "open", pathToConfigs.c_str(), NULL, NULL, SW_NORMAL);
			ImGui::Separator();

			ImGui::SliderFloat("Simulation Speed", &g_Options.Menu.SimulationSpeed, 0.1f, 5, "%.1f");
			ImGui::MyColorEdit3("Background", &g_Options.Menu.Background, ImGuiColorEditFlags_NoInputs);//, ImGui::ImGuiColorEditFlags_NoSliders);
			ImGui::Selectable("No Gen Below Horizon", &g_Options.Camera.NoGenBelowHorizon);
			ImGui::Selectable("Draw Debug Menu", &g_Options.Menu.DebugMenu);
			ImGui::Separator();

			ImGui::Selectable("Do Collisions", &g_Options.Menu.Collision);

			if (g_Options.Menu.Collision)
			{
				ImGui::SliderFloat("Offset (X)", &cube_instance.pos.x, -20, 20, "%.f");
				ImGui::SliderFloat("Offset (Y)", &cube_instance.pos.y, -20, 20, "%.f");
				ImGui::SliderFloat("Offset (Z)", &cube_instance.pos.z, -20, 20, "%.f");
				ImGui::SliderFloat("Scale", &cube_instance.color_scale.w, 16, 256, "%.f");
				ImGui::ColorEdit3("Collider", (float*)(&cube_instance.color_scale), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);//, ImGui::ImGuiColorEditFlags_NoSliders);
			}
		}
		}

		ImGui::End();
	}

	static POINT oldMousePos, mousePos;
	RECT rect;
	//resetting cursor to stay in center
	GetWindowRect(window_handle, &rect);

	GetCursorPos(&mousePos);
	bool in_sim = window_focused && !interface_open && !g_Options.Menu.QuitBox;
	if (in_sim)
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_None);
		static POINT offset{};
		const LONG offset_hor = (LONG)((rect.right - rect.left) * 0.5f);
		const LONG offset_ver = (LONG)((rect.bottom - rect.top) * 0.5f);

		if (offset.x || offset.y)
		{
			io.MouseDelta.x += offset.x;
			io.MouseDelta.y += offset.y;

			offset = POINT();
		}

		if (mousePos.x > rect.right - offset_hor * 0.25f)
		{
			mousePos.x = oldMousePos.x - offset_hor;
			offset.x += offset_hor;
		}
		if (mousePos.x < rect.left + offset_hor * 0.25f)
		{
			mousePos.x = oldMousePos.x + offset_hor;
			offset.x -= offset_hor;
		}
		if (mousePos.y > rect.bottom - offset_ver * 0.25f)
		{
			mousePos.y = oldMousePos.y - offset_ver;
			offset.y += offset_ver;
		}
		if (mousePos.y < rect.top + offset_ver * 0.25f)
		{
			mousePos.y = oldMousePos.y + offset_ver;
			offset.y -= offset_ver;
		}

		if (offset.x || offset.y)
			SetCursorPos(mousePos.x, mousePos.y);
	}
	oldMousePos = mousePos;

	ImGui::Render();
}

DialogReturn InputBox(const std::string& String, bool new_box, char* input_var, int32_t str_len)
{
	if (!input_var)
		return DialogReturn::RETURN_NO;

	//std::string old_menu_item(g_Options.Menu.subMenuName);
	//memset(&g_Options.Menu.subMenuName[0], 0, sizeof(g_Options.Menu.subMenuName));
	//strncpy_s(g_Options.Menu.subMenuName, _countof(g_Options.Menu.subMenuName), TabNames[NUM_OF_TABS + NUM_OF_ACCEPT_BOX_TYPES + 1], _TRUNCATE);

	DialogReturn ret = DialogReturn::RETURN_NONE;
	ImGui::SetNextWindowPos(PopupPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(PopupSize);
	if (new_box)
		ImGui::SetNextWindowFocus();
	std::string name = "configs##" + String;
	if (ImGui::Begin(name.c_str(), NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::BeginChild("first child input", ImVec2(-1, -1), true);
		{
			ImGui::Text("");
			ImGui::CenteredText(String.c_str());
			ImGui::Text("");
			ImGui::PushItemWidth(-1);
			if (new_box)
				ImGui::SetKeyboardFocusHere();
			ImGui::InputText("##input_var", input_var, str_len);
			ImGui::Text("");

			ImGui::Columns(2, "GOOD/CANCEL", false);
			ImGui::Spacing(); //-V760
			ImGui::SameLine();
			ImGui::Spacing();
			ImGui::SameLine();

			ImGui::SetCursorPosYScale(0.7f);
			if (ImGui::Button("Create", ImVec2((PopupSize.x * 0.95f - 2 * 35) * 0.5f, 0)))
				ret = DialogReturn::RETURN_OK;
			ImGui::NextColumn();
			ImGui::Spacing();
			ImGui::SameLine();

			ImGui::SetCursorPosYScale(0.7f);
			if (ImGui::Button("Cancel", ImVec2((PopupSize.x * 0.95f - 2 * 35) * 0.5f, 0)))
				ret = DialogReturn::RETURN_NO;
		}ImGui::EndChild();
	}ImGui::End();

	//memset(&g_Options.Menu.subMenuName[0], 0, sizeof(g_Options.Menu.subMenuName));
	//strncpy_s(g_Options.Menu.subMenuName, _countof(g_Options.Menu.subMenuName), old_menu_item.c_str(), _TRUNCATE);

	return ret;
}

DialogReturn AcceptBox(const std::string& String)
{
	//std::string old_menu_item(g_Options.Menu.subMenuName);
	//memset(&g_Options.Menu.subMenuName[0], 0, sizeof(g_Options.Menu.subMenuName));
	//strncpy_s(g_Options.Menu.subMenuName, _countof(g_Options.Menu.subMenuName), TabNames[NUM_OF_TABS + accept_box_type], _TRUNCATE);

	DialogReturn ret = DialogReturn::RETURN_NONE;
	ImGui::SetNextWindowPos(PopupPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(PopupSize);

	char buffer[32];
	_snprintf_s(buffer, _TRUNCATE, "configs##%d", (int32_t)String.length());

	if (ImGui::Begin(buffer, NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::BeginChild("first child accept", ImVec2(-1, -1), true);
		{
			ImGui::Text("");
			ImGui::CenteredText(String.c_str());
			ImGui::Text("");

			ImGui::Columns(2, "YES/NO", false);
			ImGui::Spacing(); //-V760
			ImGui::SameLine();
			ImGui::Spacing();
			ImGui::SameLine();

			ImGui::SetCursorPosYScale(0.7f);
			if (ImGui::Button("Yes", ImVec2((PopupSize.x * 0.95f - 2 * 35) * 0.5f, 0)))
				ret = DialogReturn::RETURN_OK;
			ImGui::NextColumn();
			ImGui::Spacing();
			ImGui::SameLine();

			ImGui::SetCursorPosYScale(0.7f);
			if (ImGui::Button("No", ImVec2((PopupSize.x * 0.95f - 2 * 35) * 0.5f, 0)))
				ret = DialogReturn::RETURN_NO;
		}ImGui::EndChild();
	}ImGui::End();

	//memset(&g_Options.Menu.subMenuName[0], 0, sizeof(g_Options.Menu.subMenuName));
	//strncpy_s(g_Options.Menu.subMenuName, _countof(g_Options.Menu.subMenuName), old_menu_item.c_str(), _TRUNCATE);

	return ret;
}

void RenderPopupBoxes()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));

	static bool new_input_box = true;
	if (g_Options.Menu.NewConfigBox) {
		auto var = &g_Options.Menu.ConfigName;
		auto ret = InputBox("Input Config Name:", new_input_box, *var, 32);
		switch (ret)
		{
		case(DialogReturn::RETURN_OK):
			ConfigSys.Save(true);
			[[fallthrough]];
		case(DialogReturn::RETURN_NO):
			memset(&var[0], 0, sizeof(*var));
			g_Options.Menu.NewConfigBox = false;
		}

		new_input_box = false;
	}
	else
	{
		auto var = &g_Options.Menu.ConfigName;
		memset(&var[0], 0, sizeof(*var));
		new_input_box = true;
	}

	if (g_Options.Menu.QuitBox)
	{
		std::string quitString = "Do you want to quit?";
		auto ret = AcceptBox(quitString);
		switch (ret)
		{
		case(DialogReturn::RETURN_OK):
			g_Options.Menu.SimRunning = false;
			[[fallthrough]];
		case(DialogReturn::RETURN_NO):
			g_Options.Menu.QuitBox = false;
		}
	}

	if (g_Options.Menu.DeleteBox)
	{
		std::string deleteString = "Delete '" + configFiles.at(g_Options.Menu.ConfigFile) + "'?";
		auto ret = AcceptBox(deleteString);
		switch (ret)
		{
		case(DialogReturn::RETURN_OK):
			ConfigSys.Delete();
			[[fallthrough]];
		case(DialogReturn::RETURN_NO):
			g_Options.Menu.DeleteBox = false;
		}
	}

	if (g_Options.Menu.LoadBox)
	{
		std::string loadString = "Load " + configFiles.at(g_Options.Menu.ConfigFile) + "'?";
		auto ret = AcceptBox(loadString);
		switch (ret)
		{
		case(DialogReturn::RETURN_OK):
			if (configFiles.size() > g_Options.Menu.ConfigFile && configFiles.at(g_Options.Menu.ConfigFile).length() > 0)
				ConfigSys.Load();
			[[fallthrough]];
		case(DialogReturn::RETURN_NO):
			g_Options.Menu.LoadBox = false;
		}
	}

	if (g_Options.Menu.SaveBox)
	{
		std::string saveString = "Overwrite '" + configFiles.at(g_Options.Menu.ConfigFile) + "'?";
		auto ret = AcceptBox(saveString);
		switch (ret)
		{
		case(DialogReturn::RETURN_OK):
			ConfigSys.Save();
			[[fallthrough]];
		case(DialogReturn::RETURN_NO):
			g_Options.Menu.SaveBox = false;
		}
	}

	ImGui::PopStyleVar();
}

void SetupImGui()
{
	ImColor im_col_main = ImColor(175, 25, 235);
	ImVec4 col_invisible = ImColor(0, 0, 0, 0);
	ImVec4 col_text = ImColor(235, 225, 250);
	ImVec4 col_text_disabled = col_text.Scaled(0.75f);
	ImVec4 col_theme = ImColor(130, 73, 230); //regular
	ImVec4 col_theme_light = col_theme.Scaled(1.1f); //clicked and hovered
	ImVec4 col_theme_hover = col_theme.Scaled(0.9f); //hovered not clicked
	ImVec4 col_theme_active = col_theme.Scaled(0.8f); //clicked not hovered
	ImVec4 col_theme_inactive = col_theme.Scaled(0.6f); //not clicked not hovered
	ImVec4 col_main = im_col_main;
	ImVec4 background = ImColor(40, 40, 45);
	ImVec4 background_hover = background.Scaled(1.1f);
	ImVec4 background_active = background.Scaled(0.9f);
	ImVec4 black = ImColor(0, 0, 0);

	{
		RECT rect;
		GetWindowRect(window_handle, &rect);
		int32_t width = rect.right - rect.left;
		int32_t height = rect.bottom - rect.top;
		g_Options.Menu.AppSize = Variables::Vector2D(width, height);
	}

#define BUTTONS 4
#define POPUP_SCALE 4.f
	InterfaceSize = ImVec2(WINDOW_WIDTH / 3.5f, (float)g_Options.Menu.WindowSize.y);
	InterfacePos = ImVec2(g_Options.Menu.WindowSize.x - InterfaceSize.x + 1, 0);
	MenuButtonSize = ImVec2(InterfaceSize.x / BUTTONS - 10, 26 * 1.5f);
	PopupSize = ImVec2(WINDOW_WIDTH / POPUP_SCALE, WINDOW_HEIGHT / POPUP_SCALE);
	PopupPos = ImVec2((g_Options.Menu.WindowSize.x - PopupSize.x) / 2, (g_Options.Menu.WindowSize.y - PopupSize.y) / 2);
	DebugSize = ImVec2(WINDOW_WIDTH / POPUP_SCALE / 1.25f, WINDOW_HEIGHT / POPUP_SCALE * 1.75f);
	DebugPos = ImVec2(0, g_Options.Menu.WindowSize.y - DebugSize.y + 1);

	auto& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_Text] = col_text;
	style.Colors[ImGuiCol_TextDisabled] = col_text_disabled;
	style.Colors[ImGuiCol_WindowBg] = background;
	style.Colors[ImGuiCol_ChildBg] = background;
	style.Colors[ImGuiCol_Border] = im_col_main.ShiftHSV(0.02f, -0.3f, -0.625f);
	style.Colors[ImGuiCol_BorderShadow] = col_invisible;
	//style.Colors[ImGuiCol_ComboBg] = background.Scaled(0.97f);
	style.Colors[ImGuiCol_FrameBg] = background.Scaled(0.97f);
	style.Colors[ImGuiCol_FrameBgHovered] = background_hover.Scaled(0.97f);
	style.Colors[ImGuiCol_FrameBgActive] = background_active.Scaled(0.97f);
	style.Colors[ImGuiCol_TitleBg] = col_theme;
	style.Colors[ImGuiCol_TitleBgCollapsed] = col_theme;
	style.Colors[ImGuiCol_TitleBgActive] = col_theme;
	style.Colors[ImGuiCol_MenuBarBg] = col_theme;
	style.Colors[ImGuiCol_ScrollbarBg] = background;
	style.Colors[ImGuiCol_ScrollbarGrab] = col_theme_hover;
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = col_theme_light;
	style.Colors[ImGuiCol_ScrollbarGrabActive] = col_theme_active;
	style.Colors[ImGuiCol_CheckMark] = col_theme;
	style.Colors[ImGuiCol_SliderGrabActive] = col_theme;
	style.Colors[ImGuiCol_Button] = col_theme;
	style.Colors[ImGuiCol_ButtonInactive] = col_theme_inactive;
	style.Colors[ImGuiCol_ButtonHovered] = col_theme_hover;
	style.Colors[ImGuiCol_ButtonActive] = col_theme_active;
	style.Colors[ImGuiCol_ButtonHoveredActive] = col_theme_light;
	style.Colors[ImGuiCol_Header] = col_theme;
	style.Colors[ImGuiCol_HeaderInactive] = background_hover;
	style.Colors[ImGuiCol_HeaderHovered] = col_theme_hover;
	style.Colors[ImGuiCol_HeaderActive] = col_theme_active;
	style.Colors[ImGuiCol_HeaderHoveredActive] = col_theme_light;
	style.Colors[ImGuiCol_CloseButton] = col_text.AlphaScaled(0.25f);
	style.Colors[ImGuiCol_CloseButtonHovered] = col_text;
	style.Colors[ImGuiCol_CloseButtonActive] = col_text.AlphaScaled(0.75f);
	style.Colors[ImGuiCol_TextSelectedBg] = col_theme.AlphaScaled(0.8f);
	style.Colors[ImGuiCol_PopupBg] = background;

	style.Alpha = 1.f;
	style.WindowPadding = ImVec2(8, 8);
	style.WindowMinSize = ImVec2(32, 32);
	style.WindowRounding = 0.5f;
	style.WindowTitleAlign = ImVec2(0.f, 0.5f);
	style.ChildRounding = 5.f;
	style.FramePadding = ImVec2(4, 4);
	style.FrameRounding = 5.f;
	style.ItemSpacing = ImVec2(8, 8);
	style.ItemInnerSpacing = ImVec2(4, 4);
	style.TouchExtraPadding = ImVec2(0, 0);
	style.IndentSpacing = 22.0f;
	style.ColumnsMinSpacing = 0.f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 4.f;
	style.GrabMinSize = 0.1f;
	style.GrabRounding = 5.f;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.DisplayWindowPadding = ImVec2(22, 22);
	style.DisplaySafeAreaPadding = ImVec2(4, 4);
	style.AntiAliasedLines = true;
	style.AntiAliasedFill = true;
	style.CurveTessellationTol = 1.f;
	style.FrameBorderSize = 1.f;

	static bool first_time = true;
	if (first_time)
	{
		// Load Fonts
		ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Tahoma.ttf", 16.f * 1.5f);

		// Setup Platform/Renderer bindings
		ImGui_ImplWin32_Init(window_handle);
		ImGui_ImplDX11_Init(g_D3DDevice, g_DeviceContext);
		ImGui_ImplDX11_NewFrame();
		first_time = false;
	}
}

void run_toggle(bool pressed_key, bool& val_to_switch)
{
	static std::map<bool*, bool> enabled_toggle{};
	static std::map<bool*, bool> check_toggle{};

	auto& en_tg = enabled_toggle[&val_to_switch];
	auto& ch_tg = check_toggle[&val_to_switch];

	if (pressed_key)
	{
		if (!ch_tg)
			en_tg = !en_tg;
		ch_tg = true;
	}
	else
		ch_tg = false;
	if (en_tg)
	{
		val_to_switch = !val_to_switch;
		en_tg = false;
	}
}

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SYSKEYDOWN:
		// Implements the classic ALT+ENTER fullscreen toggle
		if (wParam == VK_RETURN && (lParam & 0x60000000) == 0x20000000)
		{
			g_Options.Menu.BorderlessWindowed = !g_Options.Menu.BorderlessWindowed;

			RECT screen_rect;
			GetWindowRect(GetDesktopWindow(), &screen_rect);

			if (g_Options.Menu.BorderlessWindowed)
			{
				SetWindowLongPtr(window_handle, GWL_STYLE, WS_VISIBLE);
				SetWindowPos(window_handle, HWND_TOP, 0, 0, screen_rect.right, screen_rect.bottom, SWP_FRAMECHANGED);
			}
			else
			{
				window_x = (screen_rect.right - WINDOW_WIDTH) / 2;
				window_y = (screen_rect.bottom - WINDOW_HEIGHT) / 2;

				SetWindowLongPtr(window_handle, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
				SetWindowPos(window_handle, NULL, window_x, window_y, WINDOW_WIDTH, WINDOW_HEIGHT, SWP_FRAMECHANGED);
			}
			g_Options.Menu.ChangedWindowSize = true;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case(VK_INSERT):
		{
			//interface_open = !interface_open;
			g_Options.Menu.DeleteBox = g_Options.Menu.LoadBox = g_Options.Menu.SaveBox = g_Options.Menu.NewConfigBox = false;
		}
		break;
		case(VK_ESCAPE):
		{
			if (g_Options.Menu.DeleteBox || g_Options.Menu.LoadBox || g_Options.Menu.SaveBox || g_Options.Menu.NewConfigBox)
				g_Options.Menu.DeleteBox = g_Options.Menu.LoadBox = g_Options.Menu.SaveBox = g_Options.Menu.NewConfigBox = false;
			else if (interface_open)
				interface_open = false;
			else if (!g_Options.Menu.QuitBox)
				g_Options.Menu.QuitBox = true;
			else
				::PostQuitMessage(0);
		}
		}
		break;
	case WM_SIZE:
		if (g_D3DDevice != NULL)
		{
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_CHAR:
		if (wParam == VK_ESCAPE)
			return 0;
		else
			break; //-V796
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


bool interface_open = false;
ImVec2 InterfaceSize, InterfacePos, MenuButtonSize, PopupSize, PopupPos, DebugPos, DebugSize;
int32_t window_x, window_y;
char szFileName_narrow[MAX_PATH];
wchar_t szFileName_wide[MAX_PATH];
HWND window_handle;
Profiler g_Profiler;