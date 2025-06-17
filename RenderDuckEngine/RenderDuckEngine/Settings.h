#pragma once
#include "EngineCore.h"

#include <cstring>
#include <string>

#define XML_FILE_PATH "xmls/"

typedef u64 ConfigNameHash;

//
// <------------------------------------------------ SETTING MANAGER ---------------------------------------------->
//

class SettingConfig;

class SettingsManager;
typedef std::shared_ptr<SettingsManager> SettingManagerRef;
class SettingsManager
{
public:
	SettingsManager() = default;
	~SettingsManager() = default;
	SettingsManager(SettingsManager const&) = delete;
	void operator=(SettingsManager const&) = delete;

	static SettingsManager& Instance() 
	{
		static SettingsManager Instance;
		return Instance;
	};

	void RegisterSettingConfig(SettingConfig* settingConfig);

	void WriteSettingsXML();
	void ReadSettingsXML();

private:

	std::vector<SettingConfig*> m_SettingConfigs;
};

//
// <------------------------------------------------ SETTING CONFIG ---------------------------------------------->
//

struct ISetting;
class SettingConfig
{
public:

	SettingConfig(std::string name) : m_FileName(name + ".xml")
	{
		SettingsManager::Instance().RegisterSettingConfig(this);
	}

	void RegisterSetting(ISetting* setting) 
	{ 
		m_Settings.push_back(setting); 
	}


	const std::string& GetFileName() { return m_FileName; }
private:

	virtual void LoadSettings();
	virtual void SaveSettings();

	std::string m_FileName;
	std::vector<ISetting*> m_Settings;
};

//
// <------------------------------------------------ SETTING MAP ---------------------------------------------->
//

struct ISetting 
{
	virtual ~ISetting() = default;
	virtual std::string GetTypeName() const = 0;
	virtual std::string GetName() const = 0;
	virtual std::string GetValueAsString() const = 0;
};

class UIManager;
template<typename T>
class SettingMap : ISetting
{
public:
	friend UIManager;

	SettingMap(SettingConfig* parentConfig, std::string typeName, std::string name, T value)
		: m_TypeName(typeName), m_Value(value), m_Name(name)
	{	
		parentConfig->RegisterSetting(this);
	}

	virtual std::string GetTypeName() const override 
	{ 
		return m_TypeName; 
	}

	virtual std::string GetName() const override 
	{ 
		return m_Name; 
	}

	virtual std::string GetValueAsString() const override
	{
		if constexpr (std::is_same_v<T, std::string>)
		{
			return m_value;
		}
		else
		{
			//return std::to_string(m_Value);
		}
		return " ";
	}

	const std::string GetLabelessName() 
	{ 
		return "###" + m_Name; 
	}

	const T& GetValue() const 
	{ 
		return m_Value; 
	}

private:

	SettingMap() = delete;

	T m_Value;
	std::string m_Name;
	std::string m_TypeName;
};

//
// <------------------------------------------------ SETUP MACRO ---------------------------------------------->
//

#define SETTING_STR(s) #s

#define SETTING_CONFIG_BEGIN(a) \
	class a : SettingConfig {  \
		public: \
			a() : SettingConfig(SETTING_STR(a)) {}	

#define SETTING_CONFIG_END };

#define SETTING(a, b, c) SettingMap<a> m_##b = SettingMap<a>(this, SETTING_STR(a), SETTING_STR(b), c);


/*class Settings
{
	void Initialise()
	{
		m_DockSpace = SettingMap<bool>("DockSpace", false);
		
	}

private:
	SettingMap<bool> m_DockSpace;
};*/