#pragma once
#include <directxmath.h>
using namespace DirectX;

#include "Color.h"

struct Variables
{
	Variables()
	{
		Menu = Menu_v();
		Camera = Camera_v();
	}

	struct Vector2D
	{
		int32_t x, y;
	};

	struct Menu_v //-V730
	{
		uint32_t ConfigFile = 0;
		uint32_t SelectedMenu = 0;
		uint32_t SelectedParticle = 0;
		uint32_t SelectedGenerator = 0;
		float CurrentTime = 0.f;
		float SimulationSpeed = 1.f;
		uint32_t Throughput = 0;
		std::string AvgThroughput = "";
		Vector2D WindowSize{};
		Vector2D AppSize{};

		char GeneratorName[128]{};
		char ParticleName[128]{};
		char ConfigName[128]{};

		MenuColor Background = MenuColor::Style;

		uint32_t ParticleCount = 0;
		uint32_t RandomPerlinSeed = 0;
		int32_t TestInt1 = 0;
		int32_t TestInt2 = 0;
		float TestFlt = 0.f;
		bool DeleteBox = false;
		bool LoadBox = false;
		bool SaveBox = false;
		bool NewConfigBox = false;
		bool QuitBox = false;
		bool DebugMenu = false;
		bool PauseToggle = false;
		bool SimRunning = true;
		bool Collision = false;

		bool BorderlessWindowed = false;
		bool ChangedWindowSize = false;
	} Menu{};

	struct Camera_v
	{
		XMFLOAT3 Position{};
		XMFLOAT3 PosDelta{};
		XMFLOAT3 Rotation{};
		XMFLOAT3 RotDelta{};

		float MoveSpeed = 5;
		float MouseScale = 0.2f;
		float ControllerScale = 0.5f;
		float FOV = 90.f;
		bool NoGenBelowHorizon = false;
		bool BulletTime = false;
		bool IsMoving = false;
	} Camera{};
};

extern Variables g_Options;