#include "SceneManager.h"

SceneManager::SceneManager()
{
	m_ActiveScene = std::make_unique<Scene>();
}
