#pragma once
#include <directxmath.h>
using namespace DirectX;

#define M_PI       3.14159265358979323846   // pi
#define RADPI (float)(180.f / (float)M_PI)
#define DEG2RAD( x ) ( ( float )( x ) / RADPI )
#define RAD2DEG( x ) ( ( float )( x ) * RADPI )

extern bool compare_float(float A, float B);
extern uint32_t __fastcall GenerateRandomUInt(uint32_t min, uint32_t max);
extern float GenerateRandomFloat(float offset);
extern float GenerateRandomFloat(float min, float max);
void AngleVectors(const XMFLOAT3& angles, XMFLOAT3& forward, XMFLOAT3& right, XMFLOAT3& up);
void AngleVectors(const XMFLOAT3& angles, XMFLOAT3& forward);
XMFLOAT3 AngleVectorsReturn(const XMFLOAT3& angles);
void VectorAngles(const XMFLOAT3& forward, XMFLOAT3& angles);
extern void MuxColors(const float* start, const float* end, XMFLOAT4& output, float perc_lived);
extern void NormalizeVector(XMFLOAT3&);
extern XMFLOAT3 NormalizeVectorReturn(XMFLOAT3);
extern float Length2D(const XMFLOAT3& vec);
extern float Length3D(const XMFLOAT3& vec);
extern float Length3DSqr(const XMFLOAT3& vec);
extern void sanitize_angles(XMFLOAT3& angles);
extern constexpr float Q_rsqrt(float);
extern void SinCos(float radians, float* __restrict sine, float* __restrict cosine);

extern void XMF3Mult(XMFLOAT3&, float);
extern void XMF3Mult(XMFLOAT3&, const XMFLOAT3&);
[[nodiscard]] extern XMFLOAT3 XMF3MultRet(const XMFLOAT3&, float);
[[nodiscard]] extern XMFLOAT3 XMF3MultRet(const XMFLOAT3& vec, const XMFLOAT3& s);
[[nodiscard]] extern XMFLOAT3 XMF3AddRet(const XMFLOAT3& dst, const XMFLOAT3& add);
extern void XMF3Sub(XMFLOAT3& dst, const XMFLOAT3& add);
[[nodiscard]] extern XMFLOAT3 XMF3SubRet(const XMFLOAT3& dst, const XMFLOAT3& add);
[[nodiscard]] extern XMFLOAT3 XMF3InvRet(const XMFLOAT3& dst);
[[nodiscard]] extern float XMF3DotRet(const XMFLOAT3& src1, const XMFLOAT3& src2);
extern void XMF3Add(XMFLOAT3& dst, const XMFLOAT3& add);
extern void XMF3Div(XMFLOAT3& dst, float s);
[[nodiscard]] extern XMFLOAT3 XMF3DivRet(const XMFLOAT3& dst, float s);
extern bool XMF3Equals(const XMFLOAT3&, const XMFLOAT3&);
void GetRainbowColor(float(&output)[4], float curtime, float maxTime);
extern float ray_box_intersection(const XMFLOAT3& origin, const XMFLOAT3& inv_dir, const XMFLOAT3& min, const XMFLOAT3& max, XMFLOAT3& normal);
