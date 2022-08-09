// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Color.h"

MenuColor MenuColor::Orange(0.8f, 0.2f, 0.1f);
MenuColor MenuColor::Salad(0.6f, 0.95f, 0.f);
MenuColor MenuColor::Style(BG_COL_X, BG_COL_Y, BG_COL_Z);

float* MenuColor::GetColor()
{
	return color;
}

MenuColor MenuColor::Scale(float scalar)
{
	float cols[3];
	cols[0] = std::clamp(color[0] * scalar, 0.f, 1.f);
	cols[1] = std::clamp(color[1] * scalar, 0.f, 1.f);
	cols[2] = std::clamp(color[2] * scalar, 0.f, 1.f);
	return MenuColor(cols);
}
