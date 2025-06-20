#include "Settings.h"

#include <string>

#include "XMLParser.h"
#include "XMLSerialiser.h"

#define PROP_STR "property"
#define TYPE_STR "type"
#define NAME_STR "name"
#define VALUE_STR "value"

void PropertyConfig::LoadPropertyCount()
{
    using namespace rapidxml;
    XMLParser parser(m_FileName);

    XMLNode* root = parser.GetRootNode();

    XMLNodeList nodes;
    parser.GetAllNodes(root, PROP_STR, nodes);
    
    m_PropertyCount = nodes.size();
}

void PropertyConfig::LoadProperties()
{
    XMLParser parser(m_FileName);

    XMLNode* root = parser.GetRootNode();

    std::unordered_map<std::string, IProperty*> settingLookup;
    for (auto* setting : m_Properties)
    {
        settingLookup[setting->GetName()] = setting;
    }
     
    XMLNodeList nodes;
    parser.GetAllNodes(root, PROP_STR, nodes);

    if (nodes.size() > 0)
    {
        // Iterate through all <Property> nodes
        for (XMLNode* node : nodes)
        {
            std::string typeStr = parser.GetAttribute(node, TYPE_STR);
            std::string nameStr = parser.GetAttribute(node, NAME_STR);
            std::string valueStr = parser.GetAttribute(node, VALUE_STR);

            auto it = settingLookup.find(nameStr);
            if (it != settingLookup.end() && valueStr != "")
            {
                IProperty* setting = it->second;
                setting->SetValueFromString(valueStr);
            }
        }
    }
}

void PropertyConfig::SaveProperties()
{
    XMLParser parser(m_FileName);

    parser.ClearDoc();

    XMLNode* root = parser.AddRootNode(m_FileName);

    for (IProperty* setting : m_Properties)
    {
        std::string type = setting->GetTypeName();
        std::string name = setting->GetName();
        std::string value = setting->GetValueAsString();

        XMLNode* settingNode = parser.AddNode(root, PROP_STR);
        parser.SetAttribute(settingNode, TYPE_STR, type);
        parser.SetAttribute(settingNode, NAME_STR, name);
        parser.SetAttribute(settingNode, VALUE_STR, value);
    }

    parser.SaveFile();
}

void PropertyManager::RegisterSettingConfig(PropertyConfig* settingConfig)
{
	m_PropertyConfigs.push_back(settingConfig);
}

void PropertyManager::WriteXMLs()
{
	for (auto& it : m_PropertyConfigs)
	{
		it->SaveProperties();
	}
}

void PropertyManager::ReadXMLs()
{
}
