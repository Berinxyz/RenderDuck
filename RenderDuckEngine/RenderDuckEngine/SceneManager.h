#pragma once
#include "EngineCore.h"

class Scene
{

};

class SceneManager
{
	SceneManager();

	std::unique_ptr<Scene> m_ActiveScene;
};