#pragma once
#include <string>
#include <ShlObj.h>

#include "Resources/ImGui/imgui.h"

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

extern bool interface_open;
extern ImVec2 InterfaceSize, InterfacePos, MenuButtonSize, PopupSize, PopupPos, DebugPos, DebugSize;
extern int32_t window_x, window_y;
extern char szFileName_narrow[MAX_PATH];
extern wchar_t szFileName_wide[MAX_PATH];
extern HWND window_handle;

enum class DialogReturn
{
	RETURN_NONE = 0,
	RETURN_OK,
	RETURN_NO
};

DialogReturn InputBox(const std::string& String, bool new_box, char* input_var, int32_t str_len);
DialogReturn AcceptBox(const std::string& String);
void RenderPopupBoxes();
void ImGuiRender(bool window_focused);
void SetupImGui();
void run_toggle(bool key_down, bool& val_to_switch);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
