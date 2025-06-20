#pragma once
#include "Settings.h"

PROPERTY_CONFIG_BEGIN(RenderSettings)
	PROPERTY(ImVec4, MainViewportClearColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.f))
PROPERTY_CONFIG_END

PROPERTY_CONFIG_BEGIN(TestSettings)
	PROPERTY(s8, TestS8, -8)
	PROPERTY(s16, TestS16, -16000)
	PROPERTY(s32, TestS32, -32000)
	PROPERTY(s64, TestS64, -64000000000LL)
	PROPERTY(u8, TestU8, 8)
	PROPERTY(u16, TestU16, 16000)
	PROPERTY(u32, TestU32, 32000u)
	PROPERTY(u64, TestU64, 64000000000ULL)
	PROPERTY(f32, TestF32, 3.14159f)
	PROPERTY(f64, TestF64, 2.718281828459045)
	PROPERTY(float2, TestFloat2, float2(1.0f, 2.0f))
	PROPERTY(float3, TestFloat3, float3(1.0f, 2.0f, 3.0f))
	PROPERTY(float4, TestFloat4, float4(1.0f, 2.0f, 3.0f, 4.0f))
	PROPERTY(bool, TestBool, true)
	PROPERTY(ImVec4, TestColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f))
// unsupported types/
//PROPERTY(std::string, TestString, std::string("Hello, settings!"))
//SETTING(DirectX::XMVECTOR,  TestVector, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f))
PROPERTY_CONFIG_END

class IRenderSettings
{
public:
	virtual void SetRenderToMainRTV(bool renderToMainRTV) = 0;
	virtual RenderSettings& GetRenderSettings() = 0;
};

typedef std::shared_ptr<IRenderSettings> RendererInterfaceRef;
