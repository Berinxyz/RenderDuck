#pragma once
#include "EngineCore.h"

struct Light
{
	vec3 m_Position;
	vec4 m_Orientation;
	vec3 m_Colour;
};

class LightManager
{
public:

	LightManager();
	~LightManager();

	void CreateLight();
	void DeleteLight();

	void Update();

private:

	void SubmitLights();

};

