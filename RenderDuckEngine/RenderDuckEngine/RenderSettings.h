#pragma once
#include "Settings.h"

SETTING_CONFIG_BEGIN(RenderSettings)
	SETTING(ImVec4, MainViewportClearColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f))
SETTING_CONFIG_END

class IRenderSettings
{
public:
	virtual void SetRenderToMainRTV(bool renderToMainRTV) = 0;
	virtual RenderSettings& GetRenderSettings() = 0;
};

typedef std::shared_ptr<IRenderSettings> RendererInterfaceRef;
