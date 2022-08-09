// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <cmath>
#include <d3dcompiler.h>
#include <directxmath.h>
using namespace DirectX;

#include "DirectX.h"
#include "Resources/ImGui/backends/imgui_impl_dx11.h"
#include "Interface.h"
#include "Engine.h"
#include "Profiler.h"

#ifdef ISADEBUG
#pragma comment (lib, "dxguid.lib")
#endif

HRESULT init_params()
{
	RECT winRect;
	GetClientRect(window_handle, &winRect);
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
	g_DeviceContext->RSSetViewports(1, &viewport);

	g_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_DeviceContext->IASetInputLayout(g_InputLayout);

	g_DeviceContext->VSSetShader(g_VertexShader, nullptr, 0);
	g_DeviceContext->PSSetShader(g_PixelShader, nullptr, 0);

	UINT strideI[2] = { sizeof(XMFLOAT3), sizeof(instance_data) };
	UINT offsetI[2] = { 0,0 };
	ID3D11Buffer* BTV[2] = { g_VertexBuffer, g_InstanceBuffer };

	g_DeviceContext->IASetVertexBuffers(0, 2, BTV, strideI, offsetI);

	D3D11_RASTERIZER_DESC2 custom_raster;
	ZeroMemory(&custom_raster, sizeof(custom_raster));
	custom_raster.FillMode = D3D11_FILL_SOLID;
	custom_raster.CullMode = D3D11_CULL_NONE;
	//custom_raster.ScissorEnable = TRUE;
	//custom_raster.AntialiasedLineEnable = TRUE;

	auto hr = g_D3DDevice->CreateRasterizerState2(&custom_raster, &g_RasterDesc);
	assert(SUCCEEDED(hr));

	g_DeviceContext->RSSetState(g_RasterDesc);
	return hr;
}

HRESULT create_input_layout()
{
	// Create Vertex Shader
	ID3DBlob* vsBlob;

	{
		ID3DBlob* shaderCompileErrorsBlob;
		HRESULT hResult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "vs_main", "vs_5_0", 0, 0, &vsBlob, &shaderCompileErrorsBlob);
		if (FAILED(hResult))
		{
			const char* errorString = NULL;
			if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
				errorString = "Could not compile shader; file not found";
			else if (shaderCompileErrorsBlob) {
				errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
				shaderCompileErrorsBlob->Release();
			}
			MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
			return 1;
		}

		hResult = g_D3DDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_VertexShader);
		assert(SUCCEEDED(hResult));
	}

	// Create Pixel Shader
	{
		ID3DBlob* psBlob;
		ID3DBlob* shaderCompileErrorsBlob;
		HRESULT hResult = D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "ps_main", "ps_5_0", 0, 0, &psBlob, &shaderCompileErrorsBlob);
		if (FAILED(hResult))
		{
			const char* errorString = NULL;
			if (hResult == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
				errorString = "Could not compile shader; file not found";
			else if (shaderCompileErrorsBlob) {
				errorString = (const char*)shaderCompileErrorsBlob->GetBufferPointer();
				shaderCompileErrorsBlob->Release();
			}
			MessageBoxA(0, errorString, "Shader Compiler Error", MB_ICONERROR | MB_OK);
			return 1;
		}

		hResult = g_D3DDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_PixelShader);
		assert(SUCCEEDED(hResult));
		psBlob->Release();
	}

	// Create Input Layout
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		// Data from the vertex buffer
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},

		{ "INST_POS_ROT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_POS_ROT", 1, DXGI_FORMAT_R32_UINT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_COL_SCALE", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "INST_COL_SCALE", 1, DXGI_FORMAT_R32_FLOAT, 1, 28, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};
	HRESULT hResult = g_D3DDevice->CreateInputLayout(inputElementDesc, ARRAYSIZE(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_InputLayout);
	assert(SUCCEEDED(hResult));
	vsBlob->Release();
	return hResult;
}

HRESULT load_z_buffer()
{
	DXGI_SWAP_CHAIN_DESC1 sd;
	g_SwapChain->GetDesc1(&sd);

	g_Options.Menu.WindowSize = Variables::Vector2D(sd.Width, sd.Height);
	D3D11_TEXTURE2D_DESC1 descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = sd.Width;
	descDepth.Height = sd.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	descDepth.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;
	auto hr = g_D3DDevice->CreateTexture2D1(&descDepth, NULL, &g_DepthStencil);
	if (FAILED(hr))
		return hr;

	D3D11_DEPTH_STENCIL_DESC dsDesc;

	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_GREATER;
	// Stencil test parameters
	dsDesc.StencilEnable = false;

	// Create depth stencil state
	ID3D11DepthStencilState* pDSState;
	g_D3DDevice->CreateDepthStencilState(&dsDesc, &pDSState);

	g_DeviceContext->OMSetDepthStencilState(pDSState, 1);
	pDSState->Release();
	pDSState = NULL;

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	descDSV.Flags = 0;

	// Create the depth stencil view
	hr = g_D3DDevice->CreateDepthStencilView(g_DepthStencil, // Depth stencil texture
		&descDSV, // Depth stencil desc
		&g_DepthView);  // [out] Depth stencil view

	return S_OK;
}

HRESULT load_constant_buffers()
{
	{
		// Fill in a buffer description.
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = static_cast<int32_t>(std::ceil(sizeof(cb_per_frame) / 16.f)) * 16;
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Create the buffer.
		auto hr = g_D3DDevice->CreateBuffer(&cbDesc, NULL,
			&g_PerFrameCBuffer);

		if (FAILED(hr))
			return hr;
	}

	{
		// Fill in a buffer description.
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = static_cast<int32_t>(std::ceil(sizeof(cb_rotations) / 16.f)) * 16;
		cbDesc.Usage = D3D11_USAGE_IMMUTABLE;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data;
		ZeroMemory(&data, sizeof(data));
		std::array<XMFLOAT3X4, MAX_ROTATIONS>* rotations = get_rotations();
		data.pSysMem = &(*rotations)[0];

		// Create the buffer.
		auto hr = g_D3DDevice->CreateBuffer(&cbDesc, &data,
			&g_RotationsCBuffer);

		if (FAILED(hr))
		{
			delete rotations;
			return hr;
		}
			delete rotations;
	}

	ID3D11Buffer* buffers[] = { g_PerFrameCBuffer, g_RotationsCBuffer };
	g_DeviceContext->VSSetConstantBuffers(0, ARRAYSIZE(buffers), buffers);

	D3D11_BLEND_DESC1 omDesc;
	ZeroMemory(&omDesc, sizeof(omDesc));
	omDesc.RenderTarget[0].BlendEnable = true;
	omDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	omDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	omDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	omDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	omDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	omDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;


	if (FAILED(g_D3DDevice->CreateBlendState1(&omDesc, &g_BlendState)))
		return E_FAIL;
	g_DeviceContext->OMSetBlendState(g_BlendState, 0, 0xffffffff);

	return S_OK;
}

HRESULT load_index_buffer()
{
	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = sizeof(IndexBuffer);
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Define the resource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = IndexBuffer;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the buffer with the device.
	HRESULT hr = g_D3DDevice->CreateBuffer(&bufferDesc, &InitData, &g_IndexBuffer);
	if (FAILED(hr))
		return hr;

	g_DeviceContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	return S_OK;
}

HRESULT load_vertex_buffer()
{
	// Supply the actual vertex data.

	// Fill in a buffer description.
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = sizeof(VertexBuffer);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = VertexBuffer;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// Create the vertex buffer.
	HRESULT hr = g_D3DDevice->CreateBuffer(&bufferDesc, &InitData, &g_VertexBuffer);

	if (FAILED(hr))
		return hr;

	// Fill in a buffer description.
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(instance_data) * MAX_RENDER_INSTANCES;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	// Create the vertex buffer.
	return g_D3DDevice->CreateBuffer(&bufferDesc, NULL, &g_InstanceBuffer);
}

HRESULT init_scene()
{
	if (load_vertex_buffer() != S_OK)
		return E_FAIL;
	if (load_index_buffer() != S_OK)
		return E_FAIL;
	if (create_input_layout() != S_OK)
		return E_FAIL;
	if (load_constant_buffers() != S_OK)
		return E_FAIL;
	if (load_z_buffer() != S_OK)
		return E_FAIL;
	if (init_params() != S_OK)
		return E_FAIL;

	return S_OK;
}

bool CreateDeviceD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC1 sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Width = 0;
	sd.Height = 0;
	sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.Stereo = false;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc;
	ZeroMemory(&fsSwapChainDesc, sizeof(fsSwapChainDesc));
	fsSwapChainDesc.Windowed = TRUE;
	fsSwapChainDesc.Scaling = DXGI_MODE_SCALING_CENTERED;

	D3D_FEATURE_LEVEL  FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_1;
	UINT               numLevelsRequested = 1;
	D3D_FEATURE_LEVEL  FeatureLevelsSupported;

	UINT D3D_flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef ISADEBUG
	D3D_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ID3D11Device* temp_device = nullptr;
	ID3D11DeviceContext* temp_context = nullptr;
	if (FAILED(D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		D3D_flags,
		&FeatureLevelsRequested,
		numLevelsRequested,
		D3D11_SDK_VERSION,
		&temp_device,
		&FeatureLevelsSupported,
		&temp_context
	)))
		return false;

	if (FAILED(temp_device->QueryInterface(__uuidof(ID3D11Device5), (void**)&g_D3DDevice)))
		return false;
	temp_device->Release();
	temp_device = nullptr;

	if (FAILED(temp_context->QueryInterface(__uuidof(ID3D11DeviceContext4), (void**)&g_DeviceContext)))
		return false;
	temp_context->Release();
	temp_context = nullptr;

	if (FAILED(g_D3DDevice->QueryInterface(__uuidof(IDXGIDevice4), (void**)&g_DXGIDevice)))
		return false;
	if (FAILED(g_DXGIDevice->GetAdapter((IDXGIAdapter**)(&g_DXGIAdapter))))
		return false;
	if (FAILED(g_DXGIAdapter->GetParent(__uuidof(IDXGIFactory7), (void**)&g_IDXGIFactory)))
		return false;

	if (FAILED(g_IDXGIFactory->CreateSwapChainForHwnd(
		g_D3DDevice,
		hWnd,
		&sd,
		&fsSwapChainDesc,
		NULL,
		&g_SwapChain1
	)))
		return false;

	// Next upgrade the IDXGISwapChain to a IDXGISwapChain3 interface and store it in a private member variable named m_swapChain.
	// This will allow us to use the newer functionality such as getting the current back buffer index.
	if (FAILED(g_SwapChain1->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&g_SwapChain)))
		return false;

	if (FAILED(g_IDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)))
		return false;

#ifdef ISADEBUG
	g_D3DDevice->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&g_InfoQueue));
	g_InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
	g_InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
	//g_InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);

	typedef HRESULT(__stdcall* fPtr)(const IID&, void**);
	HMODULE hDll = LoadLibrary(L"dxgidebug.dll");
	if (hDll)
	{
		fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hDll, "DXGIGetDebugInterface");
		DXGIGetDebugInterface(__uuidof(IDXGIDebug), reinterpret_cast<void**>(&g_DXGIDebug));
	}
#endif

	// Create Framebuffer Render Target
	HRESULT hResult = g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&g_FrameBuffer);
	if (FAILED(hResult))
		return false;

	hResult = g_D3DDevice->CreateRenderTargetView1(g_FrameBuffer, 0, &g_RenderTargetView);
	assert(SUCCEEDED(hResult));

	return true;
}

void ResetD3D()
{
	if (g_InfoQueue) { g_InfoQueue->Release(); g_InfoQueue = NULL; }
	if (g_FrameBuffer) { g_FrameBuffer->Release(); g_FrameBuffer = NULL; }
	if (g_SwapChain1) { g_SwapChain1->Release(); g_SwapChain1 = NULL; }
	if (g_IDXGIFactory) { g_IDXGIFactory->Release(); g_IDXGIFactory = NULL; }
	if (g_DXGIAdapter) { g_DXGIAdapter->Release(); g_DXGIAdapter = NULL; }
	if (g_DXGIDevice) { g_DXGIDevice->Release(); g_DXGIDevice = NULL; }
	if (g_DepthStencil) { g_DepthStencil->Release(); g_DepthStencil = NULL; }
	if (g_SwapChain) { g_SwapChain->Release(); g_SwapChain = NULL; }
	if (g_DeviceContext) { g_DeviceContext->Release(); g_DeviceContext = NULL; }
	if (g_RenderTargetView) { g_RenderTargetView->Release(); g_RenderTargetView = NULL; }
	if (g_InputLayout) { g_InputLayout->Release(); g_InputLayout = NULL; }
	if (g_VertexShader) { g_VertexShader->Release(); g_VertexShader = NULL; }
	if (g_PixelShader) { g_PixelShader->Release(); g_PixelShader = NULL; }
	if (g_VertexBuffer) { g_VertexBuffer->Release(); g_VertexBuffer = NULL; }
	if (g_IndexBuffer) { g_IndexBuffer->Release(); g_IndexBuffer = NULL; }
	if (g_PerFrameCBuffer) { g_PerFrameCBuffer->Release(); g_PerFrameCBuffer = NULL; }
	if (g_RotationsCBuffer) { g_RotationsCBuffer->Release(); g_RotationsCBuffer = NULL; }
	if (g_InstanceBuffer) { g_InstanceBuffer->Release(); g_InstanceBuffer = NULL; }
	if (g_RasterDesc) { g_RasterDesc->Release(); g_RasterDesc = NULL; }
	if (g_DepthView) { g_DepthView->Release(); g_DepthView = NULL; }
	if (g_BlendState) { g_BlendState->Release(); g_BlendState = NULL; }
}

void CleanupDeviceD3D()
{
	ResetD3D();
	if (g_D3DDevice) { g_D3DDevice->Release(); g_D3DDevice = NULL; }
}

void HandleWindowChange()
{
	if (!g_Options.Menu.ChangedWindowSize)
		return;

	g_DeviceContext->OMSetRenderTargets(0, 0, 0);

	// Release all outstanding references to the swap chain's buffers.
	if (g_FrameBuffer) { g_FrameBuffer->Release(); g_FrameBuffer = NULL; }
	if (g_RenderTargetView) { g_RenderTargetView->Release(); g_RenderTargetView = NULL; }
	if (g_DepthView) { g_DepthView->Release(); g_DepthView = NULL; }
	if (g_DepthStencil) { g_DepthStencil->Release(); g_DepthStencil = NULL; }
	if (g_RasterDesc) { g_RasterDesc->Release(); g_RasterDesc = NULL; }

	g_DeviceContext->Flush();

	// Preserve the existing buffer count and format.
	// Automatically choose the width and height to match the client rect for HWNDs.
	if (FAILED(g_SwapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)))
		return;

	// Perform error handling here!
	if (FAILED(g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&g_FrameBuffer)))
		return;
	if (FAILED(g_D3DDevice->CreateRenderTargetView1(g_FrameBuffer, 0, &g_RenderTargetView)))
		return;

	g_DeviceContext->OMSetRenderTargets(1,          // One rendertarget view
		(ID3D11RenderTargetView* const*)(&g_RenderTargetView),      // Render target view, created earlier
		NULL);     // Depth stencil view for the render target

	load_z_buffer();

	// Set up the viewport.
	RECT winRect;
	GetClientRect(window_handle, &winRect);
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)(winRect.right - winRect.left), (FLOAT)(winRect.bottom - winRect.top), 0.0f, 1.0f };
	g_DeviceContext->RSSetViewports(1, &viewport);

	SetupImGui();
	g_Options.Menu.ChangedWindowSize = false;
}

void PreRenderSetup(const float* col)
{
	HandleWindowChange();
	g_DeviceContext->OMSetRenderTargets(1,          // One rendertarget view
		(ID3D11RenderTargetView*const*)(&g_RenderTargetView),      // Render target view, created earlier
		g_DepthView);     // Depth stencil view for the render target

	g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, col);
	g_DeviceContext->ClearDepthStencilView(g_DepthView, D3D11_CLEAR_DEPTH, 0.f, 0);
}

void DoRender()
{
	SCOPE_EXEC_TIME("Render");
	if (interface_open || g_Options.Menu.DebugMenu || g_Options.Menu.QuitBox)
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	g_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
}

ID3D11Buffer* g_VertexBuffer = nullptr;
ID3D11Buffer* g_IndexBuffer = nullptr;
ID3D11Buffer* g_PerFrameCBuffer = nullptr;
ID3D11Buffer* g_RotationsCBuffer = nullptr;
ID3D11Buffer* g_InstanceBuffer = nullptr;
ID3D11Texture2D1* g_DepthStencil = nullptr;
ID3D11Device5* g_D3DDevice = nullptr;
IDXGISwapChain4* g_SwapChain = nullptr;
ID3D11DeviceContext4* g_DeviceContext = nullptr;
ID3D11RenderTargetView1* g_RenderTargetView = nullptr;
ID3D11InputLayout* g_InputLayout = nullptr;
ID3D11VertexShader* g_VertexShader = nullptr;
ID3D11PixelShader* g_PixelShader = nullptr;
ID3D11RasterizerState2* g_RasterDesc = nullptr;
ID3D11DepthStencilView* g_DepthView = nullptr;
ID3D11BlendState1* g_BlendState = nullptr;
IDXGIDevice4* g_DXGIDevice = nullptr;
IDXGIAdapter4* g_DXGIAdapter = nullptr;
IDXGIFactory7* g_IDXGIFactory = nullptr;
IDXGISwapChain1* g_SwapChain1 = nullptr;
ID3D11InfoQueue* g_InfoQueue = nullptr;
ID3D11Texture2D1* g_FrameBuffer = nullptr;
#ifdef ISADEBUG
IDXGIDebug1* g_DXGIDebug = nullptr;
#endif
