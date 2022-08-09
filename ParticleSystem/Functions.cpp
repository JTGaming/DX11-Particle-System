// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <algorithm>
#include <cmath>
#include <vector>

#include "Functions.h"
#include "Color.h"
#include "Particle.h"

#define znew (zeta=36969*(zeta&65535)+(zeta>>16))
#define wnew (wox=18000*(wox&65535)+(wox>>16))
#define MWC ((znew<<16)+wnew )
#define SHR3 (jsr^=(jsr<<17), jsr^=(jsr>>13), jsr^=(jsr<<5))
#define CONG (jcong=69069*jcong+1234567)
#define KISS ((MWC^CONG)+SHR3)
#define UNI (static_cast<float>((KISS)*2.328306e-10))
/* Global static variables: */
static unsigned long zeta = 362436069, wox = 521288629, jsr = 123456789, jcong = 380116160;

__forceinline bool compare_float(float A, float B) {
	// Make sure maxUlps is non-negative and small enough that the    
	// default NAN won't compare as equal to anything.    
	int32_t aInt = *(int32_t*)&A;
	// Make aInt lexicographically ordered as a twos-complement int32_t    
	if (aInt < 0)
		aInt = INT_MAX - aInt;
	// Make bInt lexicographically ordered as a twos-complement int32_t    
	int32_t bInt = *(int32_t*)&B;
	if (bInt < 0)
		bInt = INT_MAX - bInt;
	int32_t intDiff = std::abs(aInt - bInt);
	if (intDiff <= 4) //max ulps
		return true;
	return false;
}

__forceinline uint32_t __fastcall GenerateRandomUInt(uint32_t min, uint32_t max)
{
	assert(min != max);
	assert(min < max);
	return min + static_cast<uint32_t>((max - min) * UNI + 0.5f);
}

__forceinline float GenerateRandomFloatDirect(float min, float max)
{
	return min + (max - min) * UNI;
}

__forceinline float GenerateRandomFloat(float offset)
{
	return GenerateRandomFloatDirect(-offset, offset);
}

__forceinline float GenerateRandomFloat(float min, float max)
{
	assert(!compare_float(min, max));
	assert(min < max);
	assert(!compare_float(max - min, 0.f));

	return GenerateRandomFloatDirect(min, max);
}

void AngleVectors(const XMFLOAT3& angles, XMFLOAT3& forward, XMFLOAT3& right, XMFLOAT3& up)
{
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.x), &sp, &cp);
	SinCos(DEG2RAD(angles.z), &sr, &cr);

	forward.x = (cp * cy);
	forward.y = (cp * sy);
	forward.z = (-sp);
	right.x = (-1 * sr * sp * cy + -1 * cr * -sy);
	right.y = (-1 * sr * sp * sy + -1 * cr * cy);
	right.z = (-1 * sr * cp);
	up.x = (cr * sp * cy + -sr * -sy);
	up.y = (cr * sp * sy + -sr * cy);
	up.z = (cr * cp);
}

__forceinline float Length2D(const XMFLOAT3& vec)
{
	return std::sqrtf(vec.x * vec.x + vec.y * vec.y);
}
__forceinline float Length3D(const XMFLOAT3& vec)
{
	return std::sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

__forceinline constexpr float Q_rsqrt(float number)
{
	static_assert(std::numeric_limits<float>::is_iec559); // (enable only on IEEE 754)

	float const y = std::bit_cast<float>(
		0x5f3759df - (std::bit_cast<std::uint32_t>(number) >> 1));
	return y * (1.5f - (number * 0.5f * y * y));
}

__forceinline float Length3DSqr(const XMFLOAT3& vec)
{
	return (vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

void VectorAngles(const XMFLOAT3& forward, XMFLOAT3& angles)
{
	if (compare_float(forward.y, 0.f) && compare_float(forward.x, 0.f))
	{
		angles.x = (forward.z > 0.0f) ? 270.f : 90.f;
		angles.y = 0.f;
	}
	else
	{
		angles.x = RAD2DEG(atan2f(-forward.z, Length2D(forward)));
		angles.y = RAD2DEG(atan2f(forward.y, forward.x));
	}
	angles.z = 0.f;
}

void AngleVectors(const XMFLOAT3& angles, XMFLOAT3& forward)
{
	float sp, sy, cp, cy;

	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.x), &sp, &cp);

	forward.x = (cp * cy);
	forward.y = (cp * sy);
	forward.z = (-sp);
}

XMFLOAT3 AngleVectorsReturn(const XMFLOAT3& angles)
{
	XMFLOAT3 forward;
	float sp, sy, cp, cy;

	SinCos(DEG2RAD(angles.y), &sy, &cy);
	SinCos(DEG2RAD(angles.x), &sp, &cp);

	forward.x = (cp * cy);
	forward.y = (cp * sy);
	forward.z = (-sp);
	return forward;
}

__forceinline void normalize_angles(XMFLOAT3& angles)
{
	angles.x = remainderf(angles.x, 360.f);
	angles.y = remainderf(angles.y, 360.f);
}

__forceinline void clamp_angles(XMFLOAT3& angles)
{
	if (angles.x > 89.f) angles.x = 89.f;
	else if (angles.x < -89.f) angles.x = -89.f;

	if (angles.y > 180.f) angles.y = 180.f;
	else if (angles.y < -180.f) angles.y = -180.f;

	angles.z = 0;
}
__forceinline void sanitize_angles(XMFLOAT3& angles)
{
	XMFLOAT3 temp = angles;
	normalize_angles(temp);
	clamp_angles(temp);

	if (!isfinite(temp.x) ||
		!isfinite(temp.y) ||
		!isfinite(temp.z))
		return;

	angles = temp;
}

__forceinline void MuxColors(const float* start, const float* end, XMFLOAT4& output, float perc_lived)
{
	const float anti = 1.f - perc_lived;
	XMVECTOR l = XMLoadFloat4((const XMFLOAT4*)start) * anti;
	XMVECTOR s = XMLoadFloat4((const XMFLOAT4*)end) * perc_lived;
	XMStoreFloat4(&output, l + s);
}

__forceinline float VectorLengthInverse(const XMFLOAT3& v)
{
	return Q_rsqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

__forceinline void NormalizeVector(XMFLOAT3& vec)
{
	float l = VectorLengthInverse(vec);
	XMF3Mult(vec, l);
}

__forceinline XMFLOAT3 NormalizeVectorReturn(XMFLOAT3 vec)
{
	float l = VectorLengthInverse(vec);
	XMF3Mult(vec, l);
	return vec;
}

__forceinline XMFLOAT3 XMF3MultRet(const XMFLOAT3& vec, float s)
{
	return XMFLOAT3(vec.x * s, vec.y * s, vec.z * s);
}

__forceinline XMFLOAT3 XMF3MultRet(const XMFLOAT3& vec, const XMFLOAT3& s)
{
	return XMFLOAT3(vec.x * s.x, vec.y * s.y, vec.z * s.z);
}

__forceinline void XMF3Mult(XMFLOAT3& vec, float s)
{
	vec.x *= s;
	vec.y *= s;
	vec.z *= s;
}

__forceinline void XMF3Mult(XMFLOAT3& vec, const XMFLOAT3& s)
{
	vec.x *= s.x;
	vec.y *= s.y;
	vec.z *= s.z;
}

__forceinline XMFLOAT3 XMF3AddRet(const XMFLOAT3& dst, const XMFLOAT3& add)
{
	return XMFLOAT3(dst.x + add.x, dst.y + add.y, dst.z + add.z);
}

__forceinline void XMF3Sub(XMFLOAT3& dst, const XMFLOAT3& add)
{
	dst.x -= add.x;
	dst.y -= add.y;
	dst.z -= add.z;
}

__forceinline XMFLOAT3 XMF3SubRet(const XMFLOAT3& dst, const XMFLOAT3& add)
{
	return XMFLOAT3(dst.x - add.x, dst.y - add.y, dst.z - add.z);
}

__forceinline XMFLOAT3 XMF3InvRet(const XMFLOAT3& src)
{
	return XMFLOAT3(1.f / src.x, 1.f / src.y, 1.f / src.z);
}

__forceinline float XMF3DotRet(const XMFLOAT3& a, const XMFLOAT3& b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

__forceinline void XMF3Add(XMFLOAT3& dst, const XMFLOAT3& add)
{
	dst.x += add.x;
	dst.y += add.y;
	dst.z += add.z;
}

__forceinline void XMF3Div(XMFLOAT3& dst, float s)
{
	dst.x /= s;
	dst.y /= s;
	dst.z /= s;
}

__forceinline XMFLOAT3 XMF3DivRet(const XMFLOAT3& dst, float s)
{
	return XMFLOAT3(dst.x / s, dst.y / s, dst.z / s);
}

__forceinline bool XMF3Equals(const XMFLOAT3& x, const XMFLOAT3& y)
{
	return compare_float(x.x, y.x) && compare_float(x.y, y.y) && compare_float(x.z, y.z);
}

static const std::vector<MenuColor> rainbowColors = {
	MenuColor(247, 7, 39),
	MenuColor(237, 28, 5),
	MenuColor(255, 57, 13),
	MenuColor(255, 88, 10),
	MenuColor(255, 159, 15),
	MenuColor(242, 198, 24),
	MenuColor(224, 224, 34),
	MenuColor(182, 224, 27),
	MenuColor(134, 235, 19),
	MenuColor(81, 219, 39),
	MenuColor(39, 219, 66),
	MenuColor(34, 204, 136),
	MenuColor(18, 189, 189),
	MenuColor(30, 144, 201),
	MenuColor(66, 106, 237),
	MenuColor(81, 43, 252),
	MenuColor(134, 28, 255),
	MenuColor(224, 32, 245),
	MenuColor(242, 31, 158),
	MenuColor(245, 15, 88),
	//MenuColor(255, 20, 60),
	//MenuColor(255, 90, 20),
	//MenuColor(235, 255, 25),
	//MenuColor(20, 255, 60),
	//MenuColor(20, 100, 255),
	//MenuColor(90, 30, 255),
	//MenuColor(210, 60, 211)
};

void GetRainbowColor(float(&output)[4], float curtime, float maxTime)
{
	if (compare_float(maxTime, 0.f))
		return;
	int32_t size = rainbowColors.size();
	float step = maxTime / size;
	int32_t idx = static_cast<int32_t>(curtime / step);
	const MenuColor& colStart = rainbowColors[idx % size];
	const MenuColor& colEnd = rainbowColors[(idx + 1) % size];
	float pos = std::fmodf(curtime, step);

	output[0] = colStart[0] / (float)step * (step - pos) + colEnd[0] / (float)step * pos;
	output[1] = colStart[1] / (float)step * (step - pos) + colEnd[1] / (float)step * pos;
	output[2] = colStart[2] / (float)step * (step - pos) + colEnd[2] / (float)step * pos;
}

// Math routines done in optimized assembly math package routines
void __forceinline SinCos(float radians, float* __restrict sine, float* __restrict cosine)
{
	_asm
	{
		fld		DWORD PTR[radians]
		fsincos

		mov edx, DWORD PTR[cosine]
		mov eax, DWORD PTR[sine]

		fstp DWORD PTR[edx]
		fstp DWORD PTR[eax]
	}
}

/// Ray-AABB intersection test, by the slab method.  Highly optimized.
inline float ray_box_intersection(const XMFLOAT3& origin, const XMFLOAT3& inv_dir, const XMFLOAT3& min, const XMFLOAT3& max, XMFLOAT3& normal)
{
	float tx1 = (min.x - origin.x) * inv_dir.x;
	float tx2 = (max.x - origin.x) * inv_dir.x;

	float ty1 = (min.y - origin.y) * inv_dir.y;
	float ty2 = (max.y - origin.y) * inv_dir.y;

	float tz1 = (min.z - origin.z) * inv_dir.z;
	float tz2 = (max.z - origin.z) * inv_dir.z;

	float tmin = std::min(tx1, tx2);
	float tmax = std::max(tx1, tx2);

	tmin = std::max(tmin, std::min(ty1, ty2));
	tmax = std::min(tmax, std::max(ty1, ty2));

	tmin = std::max(tmin, std::min(tz1, tz2));
	tmax = std::min(tmax, std::max(tz1, tz2));

	if (tmax < 0.f)
		return 16384.f;

	if (tmin < 0.f)
	{
		normal.x = (float)(std::abs(origin.x - max.x) < 0.1f) - (std::abs(origin.x - min.x) < 0.1f);
		normal.y = (float)(std::abs(origin.y - max.y) < 0.1f) - (std::abs(origin.y - min.y) < 0.1f);
		normal.z = (float)(std::abs(origin.z - max.z) < 0.1f) - (std::abs(origin.z - min.z) < 0.1f);

		return tmax;
	}

	normal.x = (float)(origin.x > max.x) - (origin.x < min.x);
	normal.y = (float)(origin.y > max.y) - (origin.y < min.y);
	normal.z = (float)(origin.z > max.z) - (origin.z < min.z);
	return tmin;
}
