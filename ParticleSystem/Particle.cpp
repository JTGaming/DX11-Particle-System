// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <cmath>
#include <array>

#include "Particle.h"
#include "Engine.h"
#include "Resources/perlin_noise.h"
#include "Resources/ImGUI/imgui.h"
#include "Profiler.h"

void particle_data::GenerateRainbowColor()
{
	if (rainbow)
	{
		if (!compare_float(time_rainbow_updated, g_Options.Menu.CurrentTime))
		{
			time_rainbow_updated = g_Options.Menu.CurrentTime;

			GetRainbowColor(start_color, std::fmodf(g_Options.Menu.CurrentTime + cycle_speed * 0.66f, cycle_speed), cycle_speed);
			GetRainbowColor(end_color, std::fmodf(g_Options.Menu.CurrentTime, cycle_speed), cycle_speed);
		}
	}
}

void generator::DoParticleWork()
{
	gen_particles.reserve(particle_limit);
	ClearOldParticles();
	GenerateParticles();
	UpdateParticles();
}

void generator::ClearOldParticles()
{
	SCOPE_EXEC_TIME("Clear Particles");

	if (gen_particles.size() > particle_limit)
	{
		gen_particles.resize(particle_limit);
		gen_particles.shrink_to_fit();
	}
	gen_particles.erase(std::partition(begin(gen_particles), end(gen_particles), [](const particle& item) { return item.start_life + item.lifetime > g_Options.Menu.CurrentTime; }), end(gen_particles));
}

void generator::UpdateParticles()
{
	SCOPE_EXEC_TIME("Update Particles");
	const uint32_t part_size = gen_particles.size();
	if (particles.size() <= particle_index || !part_size)
	{
		gen_particles.clear();
		return;
	}
	const particle_data& part_info = particles[particle_index];
	const float delta_time = std::clamp(g_Options.Menu.CurrentTime - last_upd_time, 0.0000001f, 0.1f);
	last_upd_time = g_Options.Menu.CurrentTime;
	bool add_offs = false;
	const auto grav = 0.5f * gravity * part_info.gravscale * delta_time;
	XMFLOAT3 offs_pos;
	if (!spatial_aware && follow_object && obj_follow >= 1 && generators.size() > obj_follow - 1)
	{
		const generator& gen2 = generators[obj_follow - 1];
		if (gen2.spatial_aware)
		{
			offs_pos = generators[obj_follow - 1].origin_offset;
			add_offs = true;
		}
	}
	
	const float stick_norm = 1.f - stickiness * 0.5f;
	const float bounce_norm = 0.5f - bounciness * 0.5f;
	const float bounce_stick = bounce_norm * stick_norm;
	const bool do_collisions = g_Options.Menu.Collision & should_collide;

	for (auto& part : gen_particles)
	{
		XMFLOAT3 offset{};

		const float time_lived = (g_Options.Menu.CurrentTime - part.start_life);
		const float life_perc = time_lived / part.lifetime;

		//blend color
		MuxColors(part_info.start_color, part_info.end_color, part.inst.color_scale, life_perc);

		//#define START_BLEND 0.1f
		//#define END_BLEND 0.9f
		//if (life_perc < START_BLEND)
		//	part.inst.scale = part_info.start_scale * (life_perc / START_BLEND);
		//else if (life_perc > END_BLEND)
		//	part.inst.scale = part_info.end_scale * (1.f - life_perc) / (1.f - END_BLEND);
		//else
		part.inst.color_scale.w *= std::clamp(20.f * life_perc, 0.f, 1.f) * std::clamp(20.f * (1.f - life_perc), 0.f, 1.f);
		XMF3Mult(part.velocity, (1.f - part_info.airdrag * delta_time));
		part.velocity.z += grav;

		//update pos
		XMF3Add(offset, XMF3MultRet(part.velocity, delta_time));

		if (spatial_aware)
		{
			//if (life_perc < .8f)
			//	XMF3Add(part.inst.pos, XMF3MultRet(part.origin, (1.f - life_perc * 1.25f)));
			XMF3Add(offset, part.origin);
			part.origin = XMF3MultRet(origin_offset, (float)!gen_at_once);
		}
		else if (add_offs)
			XMF3Add(offset, offs_pos);

		if (do_collisions)
		{
			auto diff = offset;
			auto len = Length3D(diff);
			if (!compare_float(len, 0.f))
			{
				XMF3Div(diff, len);

				auto inv_diff = XMF3InvRet(diff);
				XMFLOAT3 normal{};
				float dist = ray_box_intersection(part.inst.pos, inv_diff, bbmin, bbmax, normal);
				if (dist <= len)
				{
					NormalizeVector(normal);
					offset = XMF3MultRet(diff, -dist);

					auto dot = XMF3DotRet(part.velocity, normal);
					auto wall_perpen = XMF3MultRet(normal, dot);
					auto wall_parall = XMF3SubRet(part.velocity, wall_perpen);
					auto vel = XMF3SubRet(wall_parall, wall_perpen);
					auto diff_vel = XMF3SubRet(part.velocity, vel);

					auto scale = Length3DSqr(diff_vel) / Length3DSqr(part.velocity);
					part.velocity = XMF3MultRet(vel, stick_norm - scale * bounce_stick);
				}
			}
		}
		XMF3Add(part.inst.pos, offset);
	}
}

void generator::GenerateParticles()
{
	SCOPE_EXEC_TIME("Generate Particles");

	if (particles.size() <= particle_index || !enabled)
	{
		gen_times = 0;
		return;
	}

	if (!compare_float(lifetime, 0.f) && (g_Options.Menu.CurrentTime - (start_life + lifetime) >= 0))
	{
		gen_times = 0;
		enabled = false;
		return;
	}

	const particle_data& part_info = particles[particle_index];

	const float random_lifetime = std::min(0.75f, std::max(part_info.lifetime * 0.2f, 0.05f));
	const float generate_every_x_s = std::max((part_info.lifetime - random_lifetime * 0.75f) / particle_limit, 0.0000001f);

	const float clamped_delta = std::clamp(g_Options.Menu.CurrentTime - last_gen_time, 0.0000001f, 0.1f);
	uint32_t num_to_gen = gen_at_once ? particle_limit : (uint32_t)std::floor(clamped_delta / generate_every_x_s + 0.4f);
	const uint32_t limit = particle_limit - gen_particles.size();
	if (num_to_gen > limit || num_to_gen < limit && num_to_gen * 2 > limit)
		num_to_gen = limit;

	if (num_to_gen <= 0 || gen_at_once && gen_particles.size() > 0) //-V648
		return;
	XMFLOAT3 forward, right, down;
	AngleVectors(g_Options.Camera.Rotation, forward, right, down);
	const XMFLOAT3 fwd_pos = XMF3AddRet(g_Options.Camera.Position, XMF3MultRet(forward, 32.f));

	auto offset = offset_location;
	if (follow_object)
	{
		if (!obj_follow)
		{
			origin = g_Options.Camera.Position;

			offset = XMF3MultRet(forward, offset_location.x);
			XMF3Add(offset, XMF3MultRet(right, offset_location.y));
			XMF3Add(offset, XMF3MultRet(down, offset_location.z));
		}
		else if (generators.size() > obj_follow - 1)
		{
			const generator& gen2 = generators[obj_follow - 1];
			origin = gen2.position;
		}
	}

	XMFLOAT3 angled_dir = XMF3SubRet(fwd_pos, origin);
	NormalizeVector(angled_dir);

	const bool do_minspread = !compare_float(min_spread, 0.f);
	const bool do_movement = (part_info.vel_type == particle_data::VELOCITY_TYPE::MOVEMENT || part_info.vel_type == particle_data::VELOCITY_TYPE::FORWARDS) && !compare_float(part_info.velocity.x, 0.f);
	const float delta_time = clamped_delta / num_to_gen;
	float counter = 1.f;
	const siv::BasicPerlinNoise<float> perlin{ g_Options.Menu.RandomPerlinSeed };
	const XMFLOAT3 perlin_offset = XMF3MultRet(perlin_animated, g_Options.Menu.CurrentTime * 2.f);
	XMFLOAT3 pos_perlin{};
	int32_t max_retries = num_to_gen / 2;
	for (int32_t i = num_to_gen; i > 0; i--, counter++)
	{
		const float current_time = last_gen_time + delta_time * counter;

		switch (location)
		{
		case(LOCATION_MODE::STATIC):
		{
			position = XMF3AddRet(origin, offset);
		}
		break;
		case(LOCATION_MODE::ORBIT):
		{
			float sin, cos;
			SinCos(current_time * orbit_speed, &sin, &cos);
			XMFLOAT3 orbit = XMFLOAT3(offset_location.x * sin, offset_location.y * cos, offset_location.z * sin);
			position = XMF3AddRet(origin, orbit);
		}
		break;
		}
		origin_offset = XMF3SubRet(position, old_position);

		if (do_movement)
		{
			XMFLOAT3 wish_direction = XMF3Equals(old_position, position) ? down_vector : origin_offset;
			NormalizeVector(wish_direction);

			wish_direction = XMF3AddRet(XMF3MultRet(wish_direction, delta_time), XMF3MultRet(XMF3DivRet(movement_velocity, part_info.velocity.x), (1.f - delta_time)));
			movement_velocity = XMF3MultRet(wish_direction, part_info.velocity.x);
		}

		XMFLOAT3 velocity, cust_spread{};

		switch (pos_type)
		{
		case(POSITION_TYPE::CUBE):
		{
			cust_spread.x = GenerateRandomFloat(spread.x);
			cust_spread.y = GenerateRandomFloat(spread.x);
			cust_spread.z = GenerateRandomFloat(spread.x);
		}
		break;
		case(POSITION_TYPE::SPHERE):
		{
			XMFLOAT3 vec(GenerateRandomFloat(1.f), GenerateRandomFloat(1.f), GenerateRandomFloat(1.f));
			NormalizeVector(vec);

			float c = std::cbrtf(GenerateRandomFloat(1.f)) * spread.x;
			cust_spread = XMF3MultRet(vec, c);
		}
		break;
		case(POSITION_TYPE::SPHERE_OUTLINE):
		{
			XMFLOAT3 vec(GenerateRandomFloat(1.f), GenerateRandomFloat(1.f), GenerateRandomFloat(1.f));
			NormalizeVector(vec);

			cust_spread = XMF3MultRet(vec, spread.x);
		}
		break;
		case(POSITION_TYPE::CIRCLE):
		{
			float x1 = GenerateRandomFloat(1.f);
			float x2 = GenerateRandomFloat(1.f);

			float mag = Q_rsqrt(x1 * x1 + x2 * x2);
			x1 *= mag; x2 *= mag;

			cust_spread.x = x1 * spread.x;
			cust_spread.y = x2 * spread.x;
			cust_spread.z = GenerateRandomFloat(spread.y);
		}
		break;
		case(POSITION_TYPE::STAR):
		{
			float x1 = GenerateRandomFloat(1.f);
			float x2 = GenerateRandomFloat(1.f);

			float mag = Q_rsqrt(x1 * x1 + x2 * x2);
			x1 /= mag; x2 /= mag;

			cust_spread.x = x1 * spread.x;
			cust_spread.y = x2 * spread.x;
			cust_spread.z = GenerateRandomFloat(spread.y);
		}
		break;
		case(POSITION_TYPE::CUSTOM):
		{
			cust_spread.x = GenerateRandomFloat(spread.x);
			cust_spread.y = GenerateRandomFloat(spread.y);
			cust_spread.z = GenerateRandomFloat(spread.z);
		}
		break;
		}

		switch (part_info.vel_type)
		{
		case(particle_data::VELOCITY_TYPE::CUBE): //cube
		{
			uint32_t s = GenerateRandomUInt(0, 5); // returns 0 to 5, uniformly distributed
			uint32_t c = s % 3; // get the axis perpendicular to the side you just picked
			float* vec = (float*)&velocity;
			vec[c] = part_info.velocity.x * (float)(GenerateRandomUInt(0, 1) * 2u - 1u);
			vec[(c + 1) % 3] = GenerateRandomFloat(0, part_info.velocity.x) * (float)(GenerateRandomUInt(0, 1) * 2u - 1u);
			vec[(c + 2) % 3] = GenerateRandomFloat(0, part_info.velocity.x) * (float)(GenerateRandomUInt(0, 1) * 2u - 1u);
		}
		break;
		case(particle_data::VELOCITY_TYPE::SPHERE): //sphere
		{
			XMFLOAT3 vec(GenerateRandomFloat(1.f), GenerateRandomFloat(1.f), GenerateRandomFloat(1.f));
			NormalizeVector(vec);

			velocity = XMF3MultRet(vec, part_info.velocity.x);
		}
		break;
		case(particle_data::VELOCITY_TYPE::MOVEMENT): //movement
		{
			velocity = movement_velocity;
		}
		break;
		case(particle_data::VELOCITY_TYPE::OUTLINE): //outline
		{
			XMFLOAT3 vec = cust_spread;
			NormalizeVector(vec);

			velocity = XMF3MultRet(vec, part_info.velocity.x);
			XMF3Add(velocity, movement_velocity);
		}
		break;
		case(particle_data::VELOCITY_TYPE::FORWARDS): //forwards
		{
			velocity = XMF3MultRet(angled_dir, part_info.velocity.x);
			XMFLOAT3 vec(GenerateRandomFloat(1.f), GenerateRandomFloat(1.f), GenerateRandomFloat(1.f));
			NormalizeVector(vec);
			XMF3Add(velocity, XMF3MultRet(vec, XMF3MultRet(cust_spread, 0.05f)));
		}
		break;
		case(particle_data::VELOCITY_TYPE::CUSTOM): //custom
		{
			velocity = part_info.velocity;
		}
		break;
		}

		if (do_minspread)
		{
			XMFLOAT3 dir = NormalizeVectorReturn(cust_spread);
			XMF3Add(cust_spread, XMF3MultRet(dir, min_spread));
		}

		XMFLOAT3 pos = XMF3AddRet(position, cust_spread);

		if (g_Options.Camera.NoGenBelowHorizon && pos.z > 0.f ||
			perlin_enabled && (pos_perlin = XMF3MultRet(XMF3AddRet(pos, perlin_offset), perlin_scalar * 0.1f), perlin.octave3D_01(pos_perlin.x, pos_perlin.y, pos_perlin.z, 3, 0.4f) < perlin_threshold))
		{
			if (max_retries > 0)
			{
				max_retries--;
				counter--;
				i++;
			}
			
			continue;
		}
		{
			instance_data data;
			data.pos = std::move(pos);
			data.rot = GenerateRandomUInt(1, MAX_ROTATIONS);

			gen_particles.emplace_back(std::move(data),
				std::move(XMF3MultRet(origin_offset, -1.f)),
				std::move(velocity),
				part_info.lifetime + GenerateRandomFloat(-random_lifetime, 0.f));
		}
	}
	old_position = position;
	last_gen_time = g_Options.Menu.CurrentTime;

	if (gen_at_once)
	{
		gen_times++;
		if (gen_limit && gen_times >= gen_limit)
			enabled = false;
	}
	else
		gen_times = 0;
}

void PrefillSimulation()
{
	generators.reserve(50);
	particles.reserve(50);

	Sleep(10);
	particles.emplace_back();
	generators.emplace_back();
	g_Options.Menu.RandomPerlinSeed = GenerateRandomUInt(1000u, 1000000u);
}

void ResetSimulation()
{
	particles.clear();
	generators.clear();
	g_Options = Variables();

	PrefillSimulation();
}

std::vector<particle_data> particles{};
std::vector<generator> generators{};
