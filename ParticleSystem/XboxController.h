#pragma once
#include <array>
#include <map>
#include <assert.h>
#include <cmath>
#include <Xinput.h>
#include <chrono>
#include "Interface.h"

void UpdateControllerInputs();

static std::map<WORD, int32_t> keyMap =
{
	{(WORD)XINPUT_GAMEPAD_B, VK_ESCAPE},
	{(WORD)XINPUT_GAMEPAD_RIGHT_SHOULDER, VK_LBUTTON},
	{(WORD)XINPUT_GAMEPAD_START, VK_INSERT},
	{(WORD)XINPUT_GAMEPAD_BACK, VK_SPACE},
};

class XboxController
{
public:
	struct Vecf2D
	{
		float x = 0.f, y = 0.f;
	};

	enum class Stick : uint32_t
	{
		LEFT = 0,
		RIGHT,
		NUM_STICKS
	};

	XboxController(uint32_t idx = 0) : index(idx) {
		ZeroMemory(&state, sizeof(XINPUT_STATE));
	};
	
	void SetState(const XINPUT_STATE& st);
	float GetTriggerInput(Stick idx);
	Vecf2D GetStickInput(Stick idx);

	void ControllerConnected()
	{
		if (!Connected)
		{

		}
		Connected = true;
	}

	void ControllerDisconnected()
	{
		if (Connected)
		{

		}
		Connected = false;
	}

	bool IsButtonHeld(WORD btn)
	{
		return Connected && (state.Gamepad.wButtons & btn);
	}

	bool IsButtonPressed(WORD btn)
	{
		return Connected && (change_wButtons & btn) && IsButtonHeld(btn);
	}

	bool IsConnected()
	{
		return Connected;
	}

	bool ShouldCheckConnection()
	{
		std::chrono::steady_clock::time_point time = std::chrono::high_resolution_clock::now();

		auto ms_delta = ((std::chrono::duration<float, std::milli>)(time - old_time_gen)).count();

		return ms_delta > TIMEOUT_CHECK;
	}

	uint32_t GetIndex()
	{
		return index;
	}

private:
	uint32_t index;
	XINPUT_STATE state;
	XINPUT_STATE prev_state{};
	WORD change_wButtons{};
	bool Connected = false;
	std::chrono::steady_clock::time_point old_time_gen{};

	static constexpr uint32_t INPUT_DEADZONE_L = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
	static constexpr uint32_t INPUT_DEADZONE_R = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
	static constexpr uint32_t INPUT_DEADZONE_T = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	static constexpr uint32_t TIMEOUT_CHECK = 1000 * 5; //5 seconds
};

extern XboxController Controller;
