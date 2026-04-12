#pragma once

#include "Settings.h"

PROPERTY_CONFIG_BEGIN(CameraSettings)
	PROPERTY(float3, CameraPosition, float3(0, 0, -5))
	PROPERTY(float, CameraSpeed, 7.0f)
PROPERTY_CONFIG_END