#include "Settings.h"

#include <string>

#include "include/rapidxml/rapidxml.hpp"
#include "include/rapidxml/rapidxml_utils.hpp"
#include "include/rapidxml/rapidxml_print.hpp"
#include "include/rapidxml/rapidxml_iterators.hpp"

#include "XMLParser.h"

#define PROPERTY_STR "Property"
#define TYPE_STR "Type"
#define NAME_STR "Name"
#define VALUE_STR "Value"

void SettingConfig::LoadSettingCount()
{
    using namespace rapidxml;
    /*xml_document<> doc;
    std::string path = GetFullPath(m_FileName);
    if (!GetXMLDoc(path.c_str(), doc))
    {
        return;
    }

    rapidxml::xml_node<char>* root = doc.first_node(m_FileName.c_str());
    if (!root)
    {
        return;
    }

    std::string countStr = root->first_attribute("Count")->value();
    ValueFromString<u32>((u32)m_SettingCount, countStr.c_str());*/
}

void SettingConfig::LoadSettings()
{
    XMLParser parser(m_FileName);

    XMLNode* root = parser.GetRootNode();

    std::unordered_map<std::string, ISetting*> settingLookup;
    for (auto* setting : m_Settings)
    {
        settingLookup[setting->GetName()] = setting;
    }

    // Iterate through all <Setting> nodes
    for (XMLNode* node = parser.GetNode(root, PROPERTY_STR); node; node = node->next_sibling(PROPERTY_STR))
    {
        std::string typeStr = parser.GetAttribute(node, TYPE_STR);
        std::string nameStr = parser.GetAttribute(node, NAME_STR);
        std::string valueStr = parser.GetAttribute(node, VALUE_STR);

        auto it = settingLookup.find(nameStr);
        assert(it != settingLookup.end());

        ISetting* setting = it->second;
        setting->SetValueFromString(valueStr);
    }
}

void SettingConfig::SaveSettings()
{
    XMLParser parser(m_FileName);

    parser.ClearDoc();

    XMLNode* root = parser.AddRootNode(m_FileName);

    for (ISetting* setting : m_Settings)
    {
        std::string type = setting->GetTypeName();
        std::string name = setting->GetName();
        std::string value = setting->GetValueAsString();

        XMLNode* settingNode = parser.AddNode(root, PROPERTY_STR);
        parser.SetAttribute(settingNode, TYPE_STR, type);
        parser.SetAttribute(settingNode, NAME_STR, name);
        parser.SetAttribute(settingNode, VALUE_STR, value);
    }

    parser.SaveFile();
}

void SettingsManager::RegisterSettingConfig(SettingConfig* settingConfig)
{
	m_SettingConfigs.push_back(settingConfig);
}

void SettingsManager::WriteSettingsXML()
{
	for (auto& it : m_SettingConfigs)
	{
		it->SaveSettings();
	}
}

void SettingsManager::ReadSettingsXML()
{
}
