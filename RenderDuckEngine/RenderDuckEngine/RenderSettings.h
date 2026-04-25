#pragma once
#include "Settings.h"

PROPERTY_CONFIG_BEGIN(RenderSettings)
	PROPERTY(ImVec4, MainViewportClearColour, ImVec4(0.318627f, 0.318627f, 0.318627f, 1.f))
	PROPERTY(bool, EnableSkyBox, true)
PROPERTY_CONFIG_END