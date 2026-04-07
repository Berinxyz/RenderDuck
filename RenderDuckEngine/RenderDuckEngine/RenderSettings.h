#pragma once
#include "Settings.h"

PROPERTY_CONFIG_BEGIN(RenderSettings)
	PROPERTY(ImVec4, MainViewportClearColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.f))
	PROPERTY(bool, RenderSkyBox, true)
	PROPERTY(float, HelloWorld, 22.0f)
PROPERTY_CONFIG_END

typedef std::shared_ptr<RenderSettings> RendererInterfaceRef;
