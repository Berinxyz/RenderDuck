#pragma once
#include "Settings.h"

SETTING_CONFIG_BEGIN(RenderSettings)
SETTING(ImVec4, MainViewportClearColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f))

SETTING_CONFIG_END

SETTING_CONFIG_BEGIN(TestSettings)
SETTING(ImVec4, TestColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f))

// Signed integers
SETTING(s8, TestS8, -8)
SETTING(s16, TestS16, -16000)
SETTING(s32, TestS32, -32000)
SETTING(s64, TestS64, -64000000000LL)

// Unsigned integers
SETTING(u8, TestU8, 8)
SETTING(u16, TestU16, 16000)
SETTING(u32, TestU32, 32000u)
SETTING(u64, TestU64, 64000000000ULL)

// Floating point
SETTING(f32, TestF32, 3.14159f)
SETTING(f64, TestF64, 2.718281828459045)

// Other useful types
SETTING(bool, TestBool, true)
SETTING(std::string, TestString, std::string("Hello, settings!"))
//SETTING(DirectX::XMVECTOR,  TestVector, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f))
SETTING_CONFIG_END

class IRenderSettings
{
public:
	virtual void SetRenderToMainRTV(bool renderToMainRTV) = 0;
	virtual RenderSettings& GetRenderSettings() = 0;
};

typedef std::shared_ptr<IRenderSettings> RendererInterfaceRef;
