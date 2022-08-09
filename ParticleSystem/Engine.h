#pragma once
#include <vector>
#include <chrono>
#include <unordered_map>

#include "Particle.h"

extern XMFLOAT3 bbmin, bbmax;

void CreateViewAndPerspective(bool window_focused, float delta);
void particle_computations(generator* gen);
void DoParticleStuff();
void DoTiming(float ms_delta);
[[nodiscard]] std::array<XMFLOAT3X4, MAX_ROTATIONS>* get_rotations();
