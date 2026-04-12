#pragma once
#include "Settings.h"

PROPERTY_CONFIG_BEGIN(RenderSettings)
	PROPERTY(ImVec4, MainViewportClearColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.f))
	PROPERTY(bool, EnableSkyBox, true)
PROPERTY_CONFIG_END