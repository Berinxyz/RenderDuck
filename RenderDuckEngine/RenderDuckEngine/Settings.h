#pragma once
#include "EngineCore.h"

#include <cstring>
#include <string>
#include <set>

#define FILE_PATH "../../xmls/"
typedef u64 FileNameHash;

class SettingConfig;
typedef std::shared_ptr<SettingConfig> SettingConfigRef;

class SettingsManager;
typedef std::shared_ptr<SettingsManager> SettingManagerRef;
class SettingsManager
{
public:
	SettingsManager() {};

	static SettingManagerRef Instance();
	void RegisterSettingConfig(std::string name, SettingConfig& settingConfig);

	void OutputSettingsXML();

private:
	static SettingsManager m_Instance;

	std::unordered_map<FileNameHash, SettingConfigRef> m_SettingConfigs;
		
};

class SettingConfig
{
public:
	friend SettingsManager;

	SettingConfig(std::string name) : m_FileName(name)
	{
		//SettingsManager::Instance()->RegisterSettingConfig(name, *this);
	}

private:

	virtual void LoadSettings();
	virtual void SaveSettings();

	std::string m_FileName;
};

class UIManager;
template<typename T>
class SettingMap
{
public:
	friend UIManager;

	SettingMap(std::string name, T value) : m_Value(value), m_Name(name) {}
	SettingMap(SettingMap<T>& other) : m_Value(other.m_Value), m_Name(other.m_Name){}

	const char* GetName() { return m_Name.c_str(); }
	const std::string GetLabelessName() { return "###" + m_Name; }
	const T& GetValue() const { return m_Value; }

private:

	T m_Value;
	std::string m_Name;
};

#define SETTING_STR(s) #s
#define SETTING_CONFIG_BEGIN(a) class a : SettingConfig {  \
	public: \
		a() : SettingConfig(SETTING_STR(a)) {}	
#define SETTING_CONFIG_END };
#define SETTING(a, b, c) SettingMap<a> m_##b = SettingMap<a>(SETTING_STR(b), c);


/*class Settings
{
	void Initialise()
	{
		m_DockSpace = SettingMap<bool>("DockSpace", false);
		
	}

private:
	SettingMap<bool> m_DockSpace;
};*/