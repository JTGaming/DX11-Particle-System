// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <numeric>
#include "Engine.h"
#include "DirectX.h"
#include "Interface.h"
#include "Configuration.h"
#include "XboxController.h"
#include "Profiler.h"

void particle_computations(generator* gen)
{
	gen->DoParticleWork();
}

std::vector<std::size_t> sort_permutation(
	const std::vector<generator>& vec)
{
	std::vector<std::size_t> p(vec.size());
	std::iota(p.begin(), p.end(), 0);
	std::sort(p.begin(), p.end(),
		[&](std::size_t i, std::size_t j) { return vec[i] < vec[j]; });
	return p;
}

#define BYTE_ALIGNMENT 0x1F // For 32-byte alignment
void sse_memcpy_skip(void* pvDest, const void* pvSrc, size_t nBytes) {
	assert(nBytes % (BYTE_ALIGNMENT + 1) == 0);
	assert((intptr_t(pvDest) & BYTE_ALIGNMENT) == 0);
	assert((intptr_t(pvSrc) & BYTE_ALIGNMENT) == 0);

	const __m256i* pSrc = (const __m256i*)(pvSrc);
	__m256i* pDest = (__m256i*)(pvDest);
	uint32_t nVects = nBytes / (BYTE_ALIGNMENT + 1);

	for (; nVects > 0; nVects--, pSrc += 2, pDest++)
		_mm256_store_si256(pDest, _mm256_load_si256(pSrc));
}

void sse_memcpy_full(void* pvDest, const void* pvSrc, size_t nBytes) {
	assert(nBytes % (BYTE_ALIGNMENT + 1) == 0);
	assert((intptr_t(pvDest) & BYTE_ALIGNMENT) == 0);
	assert((intptr_t(pvSrc) & BYTE_ALIGNMENT) == 0);

	const __m256i* pSrc = (const __m256i*)(pvSrc);
	__m256i* pDest = (__m256i*)(pvDest);
	uint32_t nVects = nBytes / (BYTE_ALIGNMENT + 1);

	for (; nVects > 0; nVects--, pSrc++, pDest++)
		_mm256_store_si256(pDest, _mm256_load_si256(pSrc));
}

void DoBulletTime(float real_delta_time)
{
	real_delta_time *= 0.001f;
	static bool set = false;
	static float old_sim = 0.f;
	static float current_sim = 0.f;
	auto time1 = std::chrono::high_resolution_clock::now();

	if (g_Options.Camera.BulletTime)
	{
		if (!set)
		{
			old_sim = g_Options.Menu.SimulationSpeed;
			current_sim = 0.1f;
			set = true;
		}
		
#define SMOOTH_SCALING_INCR_POS 50.f
#define SMOOTH_SCALING_INCR_ROT 12.5f
#define SMOOTH_SCALING_DECR 5.f
		const float scalar = SMOOTH_SCALING_INCR_POS * Length3D(g_Options.Camera.PosDelta) + SMOOTH_SCALING_INCR_ROT * Length3D(g_Options.Camera.RotDelta);
		current_sim += real_delta_time * scalar;
		current_sim -= real_delta_time * SMOOTH_SCALING_DECR;

		current_sim = g_Options.Menu.SimulationSpeed = std::clamp(current_sim, 0.1f, 2.5f);
	}
	else if (set)
	{
		g_Options.Menu.SimulationSpeed = old_sim;
		set = false;
	}

	g_Options.Camera.PosDelta = g_Options.Camera.RotDelta = XMFLOAT3(0.f, 0.f, 0.f);
}

void DoParticleStuff()
{
	static uint32_t instances = 0;
	static std::chrono::steady_clock::time_point old_time_gen{};
	std::chrono::steady_clock::time_point time = std::chrono::high_resolution_clock::now();
	ImGuiIO& io = ImGui::GetIO();

#define SIM_FRAMERATE 165
#define SIM_FRAMETIME (1000.f / SIM_FRAMERATE)
	auto ms_delta = ((std::chrono::duration<float, std::milli>)(time - old_time_gen)).count();
	UpdateControllerInputs();
	CreateViewAndPerspective(GetForegroundWindow() == window_handle, io.DeltaTime);

	if (ms_delta >= SIM_FRAMETIME)
	{
		DoTiming(ms_delta);
		if (!g_Options.Menu.PauseToggle && !compare_float(g_Options.Menu.SimulationSpeed, 0.f))
		{
			DoBulletTime(ms_delta);
			g_Options.Menu.ParticleCount = 0;
			for (const auto& gen : generators)
				g_Options.Menu.ParticleCount += gen.gen_particles.size();
			for (auto& part : particles)
				part.GenerateRainbowColor();

			bbmin = XMF3AddRet(XMF3MultRet(VertexBuffer[11], cube_instance.color_scale.w), cube_instance.pos);
			bbmax = XMF3AddRet(XMF3MultRet(VertexBuffer[5], cube_instance.color_scale.w), cube_instance.pos);

			for (auto& gen : generators)
				gen.DoParticleWork();
				//pool.push_task(particle_computations, &gen);
			//pool.wait_for_tasks();
			old_time_gen = time;
			SCOPE_EXEC_TIME("Data Storage");

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			HRESULT hr = g_DeviceContext->Map(g_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if (FAILED(hr))
				return;

			if (g_Options.Menu.Collision)
				sse_memcpy_full(mappedResource.pData, &cube_instance, sizeof(instance_data));
			auto sort_list = sort_permutation(generators);
			instances = g_Options.Menu.Collision;
			for (auto val : sort_list)
			{
				const auto& gen = generators.at(val).gen_particles;
				uint32_t part_size = gen.size();
				sse_memcpy_skip((void*)(intptr_t(mappedResource.pData) + instances * sizeof(instance_data)), gen.data(), part_size * sizeof(instance_data));
				instances += part_size;
			}
			instances -= g_Options.Menu.Collision;
			g_DeviceContext->Unmap(g_InstanceBuffer, 0);

			g_Options.Menu.Throughput += instances * sizeof(instance_data);
		}
	}

	assert(instances <= MAX_RENDER_INSTANCES);
	if (g_Options.Menu.Collision)
		g_DeviceContext->DrawIndexed(14, 4, 0);
	if (instances > 0)
		g_DeviceContext->DrawInstanced(4, instances, 0, g_Options.Menu.Collision);
}

void CreateViewAndPerspective(bool window_focused, float delta_time)
{
	// Use DirectXMath to create view and perspective matrices.
	static XMVECTOR up = XMVectorSet(0.0f, 0.0f, -1.0f, 0.f);
	static XMFLOAT3 forward, right, down, saved_position, saved_rotation, saved_rotation2;
	static XMMATRIX viewMatrix, projectionMatrix;
	static float old_FOV = 0.f;
	static bool should_init = true;
	g_Options.Camera.IsMoving = false;

	if (!XMF3Equals(g_Options.Camera.Rotation, saved_rotation))
	{
		saved_rotation = g_Options.Camera.Rotation;
		should_init = true;
	}
	bool updated_view = false, updated_proj = false;
	bool in_sim = window_focused && !interface_open && !g_Options.Menu.QuitBox;
	ImGuiIO& io = ImGui::GetIO();

	for (auto& gen : generators)
		if (gen.emit_on_mouse)
			gen.enabled = in_sim && (io.MouseDown[0] || Controller.IsButtonHeld(XINPUT_GAMEPAD_RIGHT_SHOULDER));

	if (should_init || in_sim)
	{
		//ROTATION
		//keyboard
		{
			bool delta_moved = in_sim && (io.MouseDelta.x || io.MouseDelta.y);
			if (should_init || delta_moved)
			{
				if (delta_moved)
				{
					RECT rect;
					GetWindowRect(window_handle, &rect);
					int32_t width = rect.right - rect.left;
					int32_t height = rect.bottom - rect.top;

					float scalarx = WINDOW_WIDTH / (float)width;
					float scalary = WINDOW_HEIGHT / (float)height;

					g_Options.Camera.Rotation.y -= g_Options.Camera.MouseScale * io.MouseDelta.x * scalarx;
					g_Options.Camera.Rotation.x -= g_Options.Camera.MouseScale * io.MouseDelta.y * scalary;
					sanitize_angles(g_Options.Camera.Rotation);

					XMF3Add(g_Options.Camera.RotDelta, XMF3SubRet(g_Options.Camera.Rotation, saved_rotation2));
					sanitize_angles(g_Options.Camera.RotDelta);
					saved_rotation2 = g_Options.Camera.Rotation;
					g_Options.Camera.IsMoving = true;
				}
				AngleVectors(g_Options.Camera.Rotation, forward, right, down);

				updated_view = true;
				should_init = false;
			}
		}
		//controller
		{
			auto right_thumb = Controller.GetStickInput(XboxController::Stick::RIGHT);

			if (in_sim && (!compare_float(right_thumb.x, 0.f) || !compare_float(right_thumb.y, 0.f)))
			{
				RECT rect;
				GetWindowRect(window_handle, &rect);
				int32_t width = rect.right - rect.left;
				int32_t height = rect.bottom - rect.top;

				float scalarx = WINDOW_WIDTH / (float)width;
				float scalary = WINDOW_HEIGHT / (float)height;

				g_Options.Camera.Rotation.y -= g_Options.Camera.ControllerScale * right_thumb.x * scalarx * delta_time * 250.f;
				g_Options.Camera.Rotation.x += g_Options.Camera.ControllerScale * right_thumb.y * scalary * delta_time * 250.f;
				sanitize_angles(g_Options.Camera.Rotation);

				XMF3Add(g_Options.Camera.RotDelta, XMF3SubRet(g_Options.Camera.Rotation, saved_rotation2));
				sanitize_angles(g_Options.Camera.RotDelta);
				saved_rotation2 = g_Options.Camera.Rotation;
				g_Options.Camera.IsMoving = true;

				AngleVectors(g_Options.Camera.Rotation, forward, right, down);
				updated_view = true;
			}
		}

		//MOVEMENT
		{
			XMFLOAT3 movement = XMFLOAT3();
			//keyboard
			{
				if (io.KeysDown[0x57])//W
					XMF3Add(movement, forward);
				if (io.KeysDown[0x53])//S
					XMF3Sub(movement, forward);
				if (io.KeysDown[0x41])//A
					XMF3Sub(movement, right);
				if (io.KeysDown[0x44])//D
					XMF3Add(movement, right);
				if (io.KeysDown[81])//Q
					XMF3Add(movement, down);
				if (io.KeysDown[69])//E
					XMF3Sub(movement, down);
				NormalizeVector(movement);
			}
			//controller
			{
				auto left_thumb = Controller.GetStickInput(XboxController::Stick::LEFT);
				XMF3Add(movement, XMF3MultRet(forward, left_thumb.y));
				XMF3Add(movement, XMF3MultRet(right, left_thumb.x));

				auto left_trig = Controller.GetTriggerInput(XboxController::Stick::LEFT);
				XMF3Add(movement, XMF3MultRet(down, left_trig));
				auto right_trig = Controller.GetTriggerInput(XboxController::Stick::RIGHT);
				XMF3Sub(movement, XMF3MultRet(down, right_trig));
			}

			if (!compare_float(Length3DSqr(movement), 0.f))
			{
				float scalar = 10.f;
				if (io.KeysDown[VK_SHIFT])
					scalar *= 0.5f;
				if (io.KeysDown[VK_CONTROL])
					scalar *= 0.25f;
				const float STEP = scalar * g_Options.Camera.MoveSpeed * io.DeltaTime;
				XMF3Add(g_Options.Camera.Position, XMF3MultRet(movement, STEP));
			}
		}
	}

	if (!XMF3Equals(g_Options.Camera.Position, saved_position))
	{
		g_Options.Camera.IsMoving = true;
		XMF3Add(g_Options.Camera.PosDelta, XMF3SubRet(g_Options.Camera.Position, saved_position));
		saved_position = g_Options.Camera.Position;
		updated_view = true;
	}

	if (updated_view)
	{
		const XMFLOAT3 lookat_vector = XMF3AddRet(g_Options.Camera.Position, XMF3MultRet(forward, 512.f));
		XMVECTOR eye = XMVectorSet(g_Options.Camera.Position.x, g_Options.Camera.Position.y, g_Options.Camera.Position.z, 0.f);
		XMVECTOR at = XMVectorSet(lookat_vector.x, lookat_vector.y, lookat_vector.z, 0.f);
		viewMatrix = XMMatrixLookAtLH(eye, at, up);
	}

	if (!compare_float(g_Options.Camera.FOV, old_FOV))
	{
		constexpr float aspectRatio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
		constexpr float nearZ = 4096.f;
		constexpr float farZ = 1.f;
		float fovRadians = (g_Options.Camera.FOV / 360.0f) * XM_2PI;
		projectionMatrix = XMMatrixPerspectiveFovLH(fovRadians, aspectRatio, nearZ, farZ);
		old_FOV = g_Options.Camera.FOV;
		updated_proj = true;
	}

	if (updated_view || updated_proj)
	{
		alignas(32) cb_per_frame data;
		XMStoreFloat4x4(
			&data.mViewProj,
			XMMatrixTranspose(viewMatrix * projectionMatrix));

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = g_DeviceContext->Map(g_PerFrameCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(hr))
			return;

		sse_memcpy_full(mappedResource.pData, &data, sizeof(cb_per_frame));
		g_Options.Menu.Throughput += sizeof(cb_per_frame);
		g_DeviceContext->Unmap(g_PerFrameCBuffer, 0);
	}
}

std::array<XMFLOAT3X4, MAX_ROTATIONS>* get_rotations()
{
	std::array<XMFLOAT3X4, MAX_ROTATIONS>* ret = new std::array<XMFLOAT3X4, MAX_ROTATIONS>;
	constexpr float max_angle = 44.5f;
	constexpr int32_t max_angle_dual = static_cast<int32_t>(max_angle * 2);
	constexpr int32_t max_angle_offset = max_angle_dual * 2;
	constexpr int32_t change_offset_every_x = 20; //std::cbrt(rotations.size());
	constexpr int32_t individual_angle_offset = max_angle_offset / change_offset_every_x + 1;

	for (uint32_t i = 0; i < MAX_ROTATIONS; ++i)
	{
		int32_t x, y, z;
		x = (i % change_offset_every_x) * individual_angle_offset - max_angle_dual;
		y = ((i / change_offset_every_x) % change_offset_every_x) * individual_angle_offset - max_angle_dual;
		z = ((i / (change_offset_every_x * change_offset_every_x)) % change_offset_every_x) * individual_angle_offset - max_angle_dual;
		XMStoreFloat3x4(
			&(*ret)[i],
			XMMatrixTranspose(XMMatrixRotationRollPitchYaw(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z))));
	}
	return ret;
}

void DoTiming(float ms_delta)
{
	const float old_current = g_Options.Menu.CurrentTime;

#define MIN_SIM_RATE 10
#define MIN_FRAME_TIME 1.f / MIN_SIM_RATE
	float delta_time = std::clamp(ms_delta * 0.001f * g_Options.Menu.SimulationSpeed, 0.f, MIN_FRAME_TIME);
	g_Options.Menu.CurrentTime += delta_time;

	if (g_Options.Menu.PauseToggle)
		g_Options.Menu.CurrentTime = old_current;
	if (g_Options.Menu.CurrentTime > 163840.f)
		g_Options.Menu.CurrentTime = 0.f;
}

XMFLOAT3 bbmin, bbmax;
