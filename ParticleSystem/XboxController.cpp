// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <Windows.h>
#include "XboxController.h"
#include "Interface.h"
#include "Variables.h"

#pragma comment (lib, "Xinput.lib")

XboxController Controller{};

void XboxController::SetState(const XINPUT_STATE& st)
{
	if (st.dwPacketNumber == state.dwPacketNumber)
		return;
	prev_state = state;
	state = st;

	for (auto button : keyMap)
	{
		// If button is pushed
		if ((state.Gamepad.wButtons & button.first) != 0)
			// Send keyboard event
			SendMessage(window_handle, WM_KEYDOWN, button.second,
				((prev_state.Gamepad.wButtons & button.first) == 0 ? 0 << 30 : 1 << 30));

		// Checking for button release events, will cause the state
		// packet number to be incremented
		// if the button was pressed but is no longer pressed
		if ((state.Gamepad.wButtons & button.first) == 0
			&& (prev_state.Gamepad.wButtons & button.first) != 0)
			// Send keyboard event
			SendMessage(window_handle, WM_KEYUP, button.second, 0);
	}
}

float XboxController::GetTriggerInput(Stick idx)
{
	float retval{};
	if (!Connected)
		return retval;

	BYTE trigger;
	uint32_t deadzone;

	switch (idx)
	{
	case Stick::LEFT:
	{
		trigger = state.Gamepad.bLeftTrigger;
		deadzone = INPUT_DEADZONE_T;
	}
	break;
	case Stick::RIGHT:
	{
		trigger = state.Gamepad.bRightTrigger;
		deadzone = INPUT_DEADZONE_T;
	}
	break;
	default:
		assert(0);
		return retval;
	}

	if (trigger > deadzone)
	{

		float OldRange = (255.f - deadzone);
		retval = (trigger - deadzone) / OldRange;
		retval = std::powf(retval, 3);
	}
	else
		retval = 0.f;

	return retval;
}

XboxController::Vecf2D XboxController::GetStickInput(Stick idx)
{
	Vecf2D retval{};
	if (!Connected)
		return retval;

	SHORT stick_x, stick_y;
	uint32_t deadzone;
	switch (idx)
	{
	case Stick::LEFT:
	{
		stick_x = state.Gamepad.sThumbLX;
		stick_y = state.Gamepad.sThumbLY;
		deadzone = INPUT_DEADZONE_L;
	}
	break;
	case Stick::RIGHT:
	{
		stick_x = state.Gamepad.sThumbRX;
		stick_y = state.Gamepad.sThumbRY;
		deadzone = INPUT_DEADZONE_R;
	}
	break;
	default:
		assert(0);
		return retval;
	}

	//determine how far the controller is pushed
	float magnitude = std::sqrtf(stick_x * (float)stick_x + stick_y * (float)stick_y);

	//check if the controller is outside a circular dead zone
	if (magnitude > deadzone)
	{
		float OldMin = -32768;
		float OldRange = (32767 - OldMin);
		retval.x = (((stick_x - OldMin) * 2) / OldRange) - 1;
		retval.y = (((stick_y - OldMin) * 2) / OldRange) - 1;

		retval.x = std::powf(retval.x, 3);
		retval.y = std::powf(retval.y, 3);
	}
	return retval;
}

void UpdateControllerInputs()
{
    DWORD err;
    if (!Controller.IsConnected() && !Controller.ShouldCheckConnection())
        return;

    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));

    // Simply get the state of the controller from XInput.
    err = XInputGetState(Controller.GetIndex(), &state);

    if (err == ERROR_SUCCESS)
    {
        // Controller is connected
        Controller.ControllerConnected();
        Controller.SetState(state);
    }
    else
    {
        // Controller is not connected
        Controller.ControllerDisconnected();
    }

    run_toggle(ImGui::GetIO().KeysDown[VK_INSERT], interface_open);
    if (!interface_open)
        run_toggle(ImGui::GetIO().KeysDown[VK_SPACE], g_Options.Menu.PauseToggle);
}