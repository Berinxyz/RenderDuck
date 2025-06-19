#pragma once
#include "EngineCore.h"

#include "XMLSerialiser.h"

#include <cstring>
#include <string>
//
// <------------------------------------------------ SETTING MANAGER ---------------------------------------------->
//

class SettingConfig;

class SettingsManager;
typedef std::shared_ptr<SettingsManager> SettingManagerRef;
class SettingsManager
{
public:
	// Singleton
	SettingsManager() = default;
	~SettingsManager() = default;
	SettingsManager(SettingsManager const&) = delete;
	void operator=(SettingsManager const&) = delete;

	static SettingsManager& Instance() 
	{
		static SettingsManager Instance;
		return Instance;
	};

	// Called by settings configs to register them in m_SettingConfigs
	void RegisterSettingConfig(SettingConfig* settingConfig);

	// Called by UIManager
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

	SettingConfig(std::string name) : m_FileName(name)
	{
		SettingsManager::Instance().RegisterSettingConfig(this);
		// Loads the recorded setting count from the xml
		//LoadSettingCount();
	}

	void RegisterSetting(ISetting* setting) 
	{ 
		assert(setting != nullptr);
		m_Settings.push_back(setting);

		// Once all the setting maps have registered to the config, we can load their values from the xml
		if (m_Settings.size() == m_SettingCount)
		{
			LoadSettings();
		}
	}

	const std::string& GetFileName() 
	{ 
		return m_FileName; 
	}

	void LoadSettingCount();
	void LoadSettings();
	void SaveSettings();

private:

	u32 m_SettingCount;
	std::string m_FileName;
	std::vector<ISetting*> m_Settings;
};

//
// <------------------------------------------------ SETTING MAP ---------------------------------------------->
//

// template typeless interface used by the config to store list of setting maps
struct ISetting 
{
	virtual ~ISetting() = default;
	virtual std::string GetTypeName() const = 0;
	virtual std::string GetName() const = 0;
	virtual std::string GetValueAsString() const = 0;
	virtual void SetValueFromString(const std::string& str) = 0;
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
		// register to the config when the setting map is created
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

	const std::string GetLabelessName() 
	{ 
		return "###" + m_Name; 
	}

	const T& GetValue() const 
	{ 
		return m_Value; 
	}

	// saving and loading from xml
	virtual std::string GetValueAsString() const override
	{
		XMLSerialiser serialiser;
		serialiser.Write();

		std::string valStr;
		if (serialiser.SerialiseValueByName(m_TypeName, valStr, (void*)&m_Value))
		{
			return valStr;
		}

		return "Unsupported Type";
	}

	virtual void SetValueFromString(const std::string& valueStr)
	{
		XMLSerialiser serialiser;
		serialiser.Read();
		std::string str = valueStr;
		serialiser.SerialiseValueByName(m_TypeName, str, (void*)&m_Value);
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

