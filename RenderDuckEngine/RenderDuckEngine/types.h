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

typedef DirectX::XMMATRIX mtx44;
typedef DirectX::XMVECTOR vec4;
typedef DirectX::XMVECTOR vec3;
typedef DirectX::XMVECTOR vec2;