#include "Settings.h"


void SettingConfig::LoadSettings()
{
}
void SettingConfig::SaveSettings()
{
}

	
SettingManagerRef SettingsManager::Instance()
{
	return std::make_shared<SettingsManager>(m_Instance);
}

void SettingsManager::RegisterSettingConfig(std::string name, SettingConfig& settingConfig)
{
	FileNameHash fileNameHash = std::hash<std::string>{}(name);
	m_SettingConfigs[fileNameHash] = std::make_shared<SettingConfig>(settingConfig);
}

void SettingsManager::OutputSettingsXML()
{
	for (auto it : m_SettingConfigs)
	{
		it.second->SaveSettings();
	}
}
