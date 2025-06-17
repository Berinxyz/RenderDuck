#include "Settings.h"

#include "XMLParser.h"

void SettingConfig::LoadSettings()
{
}
void SettingConfig::SaveSettings()
{
}

void SettingsManager::RegisterSettingConfig(SettingConfig* settingConfig)
{
	m_SettingConfigs.push_back(settingConfig);
}

void SettingsManager::WriteSettingsXML()
{
	for (auto& it : m_SettingConfigs)
	{
		SettingConfig* config = it;
		std::string fileName = XML_FILE_PATH + config->GetFileName();
		CreateFile(fileName.c_str());
	}
}

void SettingsManager::ReadSettingsXML()
{
}
