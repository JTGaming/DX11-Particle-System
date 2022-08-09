#pragma once
#include <algorithm>

#define BG_COL_X 40 / 255.f
#define BG_COL_Y 40 / 255.f
#define BG_COL_Z 45 / 255.f

struct MenuColor
{
	void init(float r = 0.75f, float g = 0.2f, float b = 1.f)
	{
		color[0] = r;
		color[1] = g;
		color[2] = b;
	}

	MenuColor(const MenuColor& copy_a)
	{
		*this = copy_a;
	}

	MenuColor(float r = 0.75f, float g = 0.2f, float b = 1.f)
	{
		init(r, g, b);
	};
	
	MenuColor(int32_t r, int32_t g, int32_t b)
	{
		init(r / 255.f, g / 255.f, b / 255.f);
	};

	MenuColor(const float* rgb)
	{
		init(rgb[0], rgb[1], rgb[2]);
	};

	void operator=(const MenuColor& src)
	{
		color[0] = src.color[0];
		color[1] = src.color[1];
		color[2] = src.color[2];
	}

	bool operator==(const MenuColor& src) const
	{
		return std::equal(std::begin(color), std::end(color), std::begin(src.color), std::end(src.color));
	}

	float& operator[](int32_t i)
	{
		return ((float*)this)[i];
	}

	float operator[](int32_t i) const
	{
		return ((float*)this)[i];
	}

	float* GetColor();
	MenuColor Scale(float scalar);

	//user configurable
	float color[3];
	float a = 1.f;

	static MenuColor Orange;
	static MenuColor Salad;
	static MenuColor Style;
};
