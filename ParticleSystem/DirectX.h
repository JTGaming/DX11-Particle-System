#pragma once
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <directxmath.h>
using namespace DirectX;
#ifdef ISADEBUG
#include <dxgidebug.h>
#endif

#define MAX_RENDER_INSTANCES 2000001
#define MAX_ROTATIONS (4096/3)

struct instance_data
{
	XMFLOAT3 pos; uint32_t rot = 0;
	XMFLOAT4 color_scale{};
};

struct cb_per_frame
{
	XMFLOAT4X4 mViewProj;
};

struct cb_rotations
{
	XMFLOAT3X4 mRotation[MAX_ROTATIONS];
};

extern ID3D11Buffer* g_VertexBuffer;
extern ID3D11Buffer* g_IndexBuffer;
extern ID3D11Buffer* g_PerFrameCBuffer;
extern ID3D11Buffer* g_RotationsCBuffer;
extern ID3D11Buffer* g_InstanceBuffer;
extern ID3D11Texture2D1* g_DepthStencil;
extern ID3D11Device5* g_D3DDevice;
extern IDXGISwapChain4* g_SwapChain;
extern ID3D11DeviceContext4* g_DeviceContext;
extern ID3D11RenderTargetView1* g_RenderTargetView;
extern ID3D11InputLayout* g_InputLayout;
extern ID3D11VertexShader* g_VertexShader;
extern ID3D11PixelShader* g_PixelShader;
extern ID3D11RasterizerState2* g_RasterDesc;
extern ID3D11DepthStencilView* g_DepthView;
extern ID3D11BlendState1* g_BlendState;
extern IDXGIDevice4* g_DXGIDevice;
extern IDXGIAdapter4* g_DXGIAdapter;
extern IDXGIFactory7* g_IDXGIFactory;
extern IDXGISwapChain1* g_SwapChain1;
extern ID3D11InfoQueue* g_InfoQueue;
extern ID3D11Texture2D1* g_FrameBuffer;
#ifdef ISADEBUG
extern IDXGIDebug1* g_DXGIDebug;
#endif

HRESULT init_params();
HRESULT create_input_layout();
HRESULT load_z_buffer();
HRESULT load_constant_buffers();
HRESULT load_vertex_buffer();
HRESULT load_index_buffer();
HRESULT init_scene();
bool CreateDeviceD3D(HWND hWnd);
void ResetD3D();
void CleanupDeviceD3D();
void PreRenderSetup(const float*);
void DoRender();

static XMFLOAT3 VertexBuffer[] =
{
	//0 particle
	{0.5f,  0.5f, 0.5f},
	{0.33f, 0.5f, -0.5f},
	{0.33f, -0.5f,  0.5f},
	{0.5f, -0.5f,-0.5f},

	//4 cube
	{-1.f, 1.f, 1.f},     //4  Front-top-left
	{1.f, 1.f, 1.f},      //5  Front-top-right
	{-1.f, -1.f, 1.f},    //6  Front-bottom-left
	{1.f, -1.f, 1.f},     //7  Front-bottom-right
	{1.f, -1.f, -1.f},    //8  Back-bottom-right
	{1.f, 1.f, -1.f},     //9  Back-top-right
	{-1.f, 1.f, -1.f},    //10 Back-top-left
	{-1.f, -1.f, -1.f},   //11 Back-bottom-left
};

static uint32_t IndexBuffer[] = {
	//particle
	0, 1, 2, 3,
	//cube
	4, 5, 6, 7, 8, 5, 9, 4, 10, 6, 11, 8, 10, 9
};
