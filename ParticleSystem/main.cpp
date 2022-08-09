// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "resource.h"
#include "Interface.h"
#include "Engine.h"
#include "DirectX.h"
#include "Configuration.h"
#include "Profiler.h"

#include "Resources/ImGui/backends/imgui_impl_dx11.h"
#include "Resources/ImGui/backends/imgui_impl_win32.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
WNDCLASSEX wc{};

void main_loop();
bool init_setup(HINSTANCE hInstance);
bool load_info();
int32_t cleanup_exit(bool imgui = false, bool exit = false);
bool create_window(HINSTANCE hInstance);

int32_t APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ PSTR, _In_ int32_t)
{
	srand((uint32_t)time(NULL));

	if (!init_setup(hInstance))
		return 1;

	main_loop();

	cleanup_exit(true, true);

	return 0;
}

void main_loop()
{
	MSG msg{};
	while (g_Options.Menu.SimRunning)
	{
		g_Profiler.calc_avg();
		SCOPE_EXEC_TIME("main_loop");

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				g_Options.Menu.SimRunning = false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (IsIconic(window_handle))
		{
			Sleep(10);
			continue;
		}

		PreRenderSetup(g_Options.Menu.Background.GetColor());

		const bool window_focused = GetForegroundWindow() == window_handle;

		ImGuiRender(window_focused);

		DoParticleStuff();

		DoRender();
	}
}

bool init_setup(HINSTANCE hInstance)
{
	if (!load_info())
		return false;

	if (!create_window(hInstance))
		return false;

	// Initialize Direct3D
	if (!CreateDeviceD3D(window_handle))
		return cleanup_exit();

	// Setup Dear ImGui context
	ImGui::CreateContext();

	if (FAILED(init_scene()))
		return cleanup_exit(true);

	// Setup Dear ImGui style
	SetupImGui();

	// Show the window
	ShowWindow(window_handle, SW_SHOWDEFAULT);
	UpdateWindow(window_handle);

	PrefillSimulation();
	return true;
}

bool load_info()
{
	char path[MAX_PATH];
	if (!SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) return false;
	pathToConfigs = std::string(path) + "\\particle system\\";
	CreateDirectoryA(pathToConfigs.c_str(), NULL);
	ConfigSys.LoadConfigs();

	//Get Application Name
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);
	std::wstring name = szFileName;
	const auto offset = name.find_last_of('\\') + 1;
	name = name.substr(offset, name.length() - offset - 4);
	wcstombs_s(NULL, szFileName_narrow, name.c_str(), wcslen(szFileName) + 1);
	wcscpy_s(szFileName_wide, name.c_str());

	return true;
}

int32_t cleanup_exit(bool imgui, bool exit)
{
	if (imgui)
	{
		if (exit)
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
		}
		ImGui::DestroyContext();
	}
	if (g_DeviceContext)
	{
		g_DeviceContext->ClearState();
		g_DeviceContext->Flush();
	}

	CleanupDeviceD3D();
	DestroyWindow(window_handle);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

#ifdef ISADEBUG
	if (g_DXGIDebug)
	{
		g_DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		g_DXGIDebug->Release();
		g_DXGIDebug = NULL;
	}
#endif

	return 1;
}

bool create_window(HINSTANCE hInstance)
{
	ZeroMemory(&wc, sizeof(wc));
	// Create application window
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIconSm = wc.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szFileName_wide;

	ATOM class_atom = ::RegisterClassEx(&wc);
	if (!class_atom)
		return false;

	g_Options.Menu.AppSize = Variables::Vector2D(WINDOW_WIDTH, WINDOW_HEIGHT);

	RECT screen_rect;
	GetWindowRect(GetDesktopWindow(), &screen_rect);
	window_x = (screen_rect.right - g_Options.Menu.AppSize.x) / 2;
	window_y = (screen_rect.bottom - g_Options.Menu.AppSize.y) / 2;
	window_handle = ::CreateWindowEx(0, MAKEINTATOM(class_atom), szFileName_wide, WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX, window_x, window_y, g_Options.Menu.AppSize.x, g_Options.Menu.AppSize.y, NULL, NULL, hInstance, NULL);
	if (!window_handle)
		return false;

	SetCursorPos(screen_rect.right / 2, screen_rect.bottom / 2);
	return true;
}
