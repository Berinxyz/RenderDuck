#pragma once
#include "EngineCore.h"

#include "XMLSerialiser.h"

#include <cstring>
#include <string>
//
// <------------------------------------------------ SETTING MANAGER ---------------------------------------------->
//

class PropertyConfig;

class PropertyManager;
typedef std::shared_ptr<PropertyManager> SettingManagerRef;
class PropertyManager
{
public:
	// Singleton
	PropertyManager() = default;
	~PropertyManager() = default;
	PropertyManager(PropertyManager const&) = delete;
	void operator=(PropertyManager const&) = delete;

	static PropertyManager& Instance() 
	{
		static PropertyManager Instance;
		return Instance;
	};

	// Called by settings configs to register them in m_SettingConfigs
	void RegisterSettingConfig(PropertyConfig* propertyConfig);

	// Called by UIManager
	void WriteXMLs();
	void ReadXMLs();

private:

	std::vector<PropertyConfig*> m_PropertyConfigs;
};

//
// <------------------------------------------------ SETTING CONFIG ---------------------------------------------->
//

struct IProperty;
class PropertyConfig
{
public:

	PropertyConfig(std::string name) : m_FileName(name)
	{
		PropertyManager::Instance().RegisterSettingConfig(this);
		// Loads the recorded setting count from the xml
		LoadPropertyCount();
	}

	void RegisterProperty(IProperty* property) 
	{ 
		assert(property != nullptr);
		m_Properties.push_back(property);

		// Once all the properties for this class have registered to the config, we can load their values from the xml
		if (m_PropertyCount > 0 && m_Properties.size() == m_PropertyCount)
		{
			LoadProperties();
		}
	}

	const std::string& GetFileName() 
	{ 
		return m_FileName; 
	}

	void LoadPropertyCount();
	void LoadProperties();
	void SaveProperties();

private:

	u32 m_PropertyCount;
	std::string m_FileName;
	std::vector<IProperty*> m_Properties;
};

//
// <------------------------------------------------ SETTING MAP ---------------------------------------------->
//

// template typeless interface used by the config to store list of setting maps
struct IProperty 
{
	virtual ~IProperty() = default;
	virtual std::string GetTypeName() const = 0;
	virtual std::string GetName() const = 0;
	virtual std::string GetValueAsString() const = 0;
	virtual void SetValueFromString(std::string& str) = 0;
};

class UIManager;
template<typename T>
class PropertyMap : IProperty
{
public:
	friend UIManager;

	PropertyMap(PropertyConfig* parentConfig, std::string typeName, std::string name, T value)
		: m_TypeName(typeName), m_Value(value), m_Name(name)
	{	
		// register to the config when the setting map is created
		parentConfig->RegisterProperty(this);
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
		ValueSerialiser serialiser(ValueSerialiser::ToString);

		std::string valStr;
		if (serialiser.SerialiseValueByName(m_TypeName, valStr, (void*)&m_Value))
		{
			return valStr;
		}

		return "Unsupported Type";
	}

	virtual void SetValueFromString(std::string& valueStr)
	{
		ValueSerialiser serialiser(ValueSerialiser::FromString);
		serialiser.SerialiseValueByName(m_TypeName, valueStr, (void*)&m_Value);
	}


private:

	PropertyMap() = delete;

	T m_Value;
	std::string m_Name;
	std::string m_TypeName;
};

//
// <------------------------------------------------ SETUP MACRO ---------------------------------------------->
//

#define PROPERTY_STR(s) #s

#define PROPERTY_CONFIG_BEGIN(a) \
	class a : PropertyConfig {  \
		public: \
			a() : PropertyConfig(PROPERTY_STR(a)) {}	

#define PROPERTY_CONFIG_END };

#define PROPERTY(a, b, c)	PropertyMap<a> m_##b = PropertyMap<a>(this, PROPERTY_STR(a), PROPERTY_STR(b), c);


PROPERTY_CONFIG_BEGIN(TestSettings)
	PROPERTY(s8, TestS8, -8)
	PROPERTY(s16, TestS16, -16000)
	PROPERTY(s32, TestS32, -32000)
	PROPERTY(s64, TestS64, -64000000000LL)
	PROPERTY(u8, TestU8, 8)
	PROPERTY(u16, TestU16, 16000)
	PROPERTY(u32, TestU32, 32000u)
	PROPERTY(u64, TestU64, 64000000000ULL)
	PROPERTY(f32, TestF32, 3.14159f)
	PROPERTY(f64, TestF64, 2.718281828459045)
	PROPERTY(float2, TestFloat2, float2(1.0f, 2.0f))
	PROPERTY(float3, TestFloat3, float3(1.0f, 2.0f, 3.0f))
	PROPERTY(float4, TestFloat4, float4(1.0f, 2.0f, 3.0f, 4.0f))
	PROPERTY(bool, TestBool, true)
	PROPERTY(ImVec4, TestColour, ImVec4(30.f / 255.f, 30.f / 255.f, 30.f / 255.f, 1.0f))
	// unsupported types/
	//PROPERTY(std::string, TestString, std::string("Hello, settings!"))
	//SETTING(DirectX::XMVECTOR,  TestVector, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f))
PROPERTY_CONFIG_END