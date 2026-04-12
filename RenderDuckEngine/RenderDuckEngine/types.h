#pragma once

#include <DirectXMath.h>

struct	D3D12_GPU_DESCRIPTOR_HANDLE;
struct	D3D12_CPU_DESCRIPTOR_HANDLE;
typedef D3D12_GPU_DESCRIPTOR_HANDLE GPUTextureHandle;
typedef D3D12_CPU_DESCRIPTOR_HANDLE CPUTextureHandle;

typedef __int8			s8;
typedef __int16			s16;
typedef int				s32;
typedef __int64			s64;

typedef unsigned __int8	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef size_t			u64;

typedef float			f32;
typedef double			f64;

typedef DirectX::XMMATRIX mtx44;
typedef DirectX::XMVECTOR quat;
typedef DirectX::XMVECTOR vec4;
typedef DirectX::XMVECTOR vec3;
typedef DirectX::XMVECTOR vec2;

typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT2 float2;

static vec4 Vec4Make(float x, float y, float z, float w) { return { x, y, z, w }; }
static vec4 Vec4Make(const float4& f) { return XMLoadFloat4(&f);  }
static vec3 Vec3Make(float x, float y, float z) { return { x, y, z, 0.0f }; }
static vec4 Vec3Make(const float3& f) { return XMLoadFloat3(&f); }
static vec2 Vec2Make(float x, float y) { return { x, y, 0.0f, 0.0f }; }
static vec4 Vec2Make(const float2& f) { return XMLoadFloat2(&f); }

static float4 VecToFloat4(const vec4 v) { return float4(v.m128_f32[0], v.m128_f32[1], v.m128_f32[2], v.m128_f32[3]); }
static float3 VecToFloat3(const vec3 v) { return float3(v.m128_f32[0], v.m128_f32[1], v.m128_f32[2]); }
static float2 VecToFloat2(const vec2 v) { return float2(v.m128_f32[0], v.m128_f32[1]); }

