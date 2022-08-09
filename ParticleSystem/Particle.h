#pragma once
#include <vector>
#include <array>
#include <deque>
#include <string>
#include <directxmath.h>
using namespace DirectX;

#include "functions.h"
#include "Variables.h"
#include "DirectX.h"

void PrefillSimulation();
void ResetSimulation();

class particle_data
{
public:
	enum VELOCITY_TYPE : int32_t
	{
		CUBE = 0,
		SPHERE,
		MOVEMENT,
		OUTLINE,
		FORWARDS,
		CUSTOM
	};

	particle_data(const std::string& n = "Default", bool r = false, float cs = 5.f, int32_t vt = VELOCITY_TYPE::CUSTOM, float ad = 0.235f, const XMFLOAT3& vel = XMFLOAT3(0, 0, -20), float s = 0.2f, const float* st = MenuColor::Orange.GetColor(), const float* en = MenuColor::Salad.GetColor(), float m = 0.33f, float lt = 2.f) :
		name(n), rainbow(r), cycle_speed(cs), vel_type(vt), velocity(vel), gravscale(m), airdrag(ad), lifetime(lt) {
		for (int32_t i = 0; i < 3; ++i)
		{
			start_color[i] = st[i];
			end_color[i] = en[i];	
		}
		start_color[3] = s;
		end_color[3] = s;
	}

	void GenerateRainbowColor();

	XMFLOAT3 velocity;
	float start_color[4], end_color[4];
	float cycle_speed;
	float gravscale;
	float airdrag;
	float lifetime;

private:
	float time_rainbow_updated = 0.f;

public:
	int32_t vel_type;
	std::string name;

	bool rainbow;

	bool XUID = false;
};

class particle
{
public:
	particle(const instance_data& d = instance_data(), const XMFLOAT3 & o = XMFLOAT3(), const XMFLOAT3 & v = XMFLOAT3(), float l = 0.f) :
		inst(d), origin(o), velocity(v), lifetime(l), start_life(g_Options.Menu.CurrentTime - 0.001f) {	}
	
	instance_data inst;

	XMFLOAT3 velocity;
	float lifetime;
	XMFLOAT3 origin;
	float start_life;
};

class generator
{
public:
	enum LOCATION_MODE : int32_t
	{
		STATIC = 0,
		ORBIT
	};

	enum POSITION_TYPE : int32_t
	{
		CUBE = 0,
		SPHERE,
		SPHERE_OUTLINE,
		CIRCLE,
		STAR,
		CUSTOM
	};

	generator(const std::string& n = "Default", bool sc = false, float bc = 0.5f, float st = 0.25f, bool eom = false, float p_t = 0.7f, float ps = 0.4f, bool pe = false, bool s_a = false, float m_s = 0, bool f_o = false, int32_t o_f = 0, int32_t pt = POSITION_TYPE::CUSTOM, float o_s = 1.f, int32_t loc = LOCATION_MODE::STATIC, const XMFLOAT3& o_loc = XMFLOAT3(0, 0, 0), float l = 0.f, bool gao = false, int32_t gl = 1, XMFLOAT3 pos = XMF3AddRet(g_Options.Camera.Position, XMF3MultRet(AngleVectorsReturn(g_Options.Camera.Rotation), 16)), bool en = true, const XMFLOAT3& s = XMFLOAT3(5, 5, 5), int32_t p_l = 10000, int32_t p = 0) :
		name(n), should_collide(sc), bounciness(bc), stickiness(st), emit_on_mouse(eom), perlin_enabled(pe), perlin_threshold(p_t), perlin_scalar(ps),  min_spread(m_s), spatial_aware(s_a), follow_object(f_o), obj_follow(o_f), pos_type(pt), orbit_speed(o_s), location(loc), offset_location(o_loc), lifetime(l), start_life(g_Options.Menu.CurrentTime), gen_at_once(gao), gen_limit(gl), origin(pos), enabled(en), particle_limit(p_l), particle_index(p), spread(s) {	}

	void DoParticleWork();


	inline bool operator < (const generator& gen) const
	{
		return (Length3DSqr(XMF3SubRet(position, g_Options.Camera.Position)) < Length3DSqr(XMF3SubRet(gen.position, g_Options.Camera.Position)));
	}

private:
	void ClearOldParticles();
	void UpdateParticles();
	void GenerateParticles();

public:
	std::vector<particle> gen_particles{};

	XMFLOAT3 spread;
	XMFLOAT3 position{};
	XMFLOAT3 origin;
	XMFLOAT3 offset_location;

private:
	XMFLOAT3 old_position{};
	XMFLOAT3 origin_offset{};
	XMFLOAT3 movement_velocity = XMFLOAT3(0, 0, 0);

public:
	uint32_t location;
	uint32_t obj_follow;
	uint32_t pos_type;
	uint32_t gen_limit;

	float min_spread;
	float lifetime;
	float start_life;
	float orbit_speed;
	float bounciness;
	float stickiness;

	uint32_t particle_limit;
	uint32_t particle_index;

	bool follow_object;
	bool spatial_aware;
	bool gen_at_once;
	bool enabled;
	bool emit_on_mouse;
	bool perlin_enabled;
	bool should_collide;

	float perlin_threshold;
	float perlin_scalar;
	XMFLOAT3 perlin_animated{};
	std::string name;

private:
	float last_upd_time = 0.f;
	float last_gen_time = 0.f;
	uint32_t gen_times = 0;

	static constexpr XMFLOAT3 down_vector = XMFLOAT3(0, 0, 1.f);
	static constexpr float gravity = 9.81f;
	static constexpr uint32_t GENERATOR_LIMIT = 50;

public:
	bool XUID = false;
};

static const char* VelocityTypes[] = {
	"Cube",
	"Sphere",
	"Inertia",
	"Outline",
	"Forwards",
	"Custom"
};

static const char* LocationTypes[] = {
	"Static",
	"Orbital"
};

static const char* SpreadTypes[] = {
	"Cube",
	"Sphere",
	"Sphere (Outline)",
	"Circle",
	"Star",
	"Custom"
};

extern std::vector<particle_data> particles;
extern std::vector<generator> generators;
