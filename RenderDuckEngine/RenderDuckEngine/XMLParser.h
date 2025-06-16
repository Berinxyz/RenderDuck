#pragma once

#include "Settings.h"

#include "include/rapidxml/rapidxml.hpp"
#include "include/rapidxml/rapidxml_utils.hpp"
#include "include/rapidxml/rapidxml_print.hpp"
#include "include/rapidxml/rapidxml_iterators.hpp"

inline bool FileExists(const char* fileName) 
{
	struct stat buffer;
	return (stat(fileName, &buffer) == 0);
}

void SaveSettings()
{
	/*if (!FileExists(SETTINGS_FILE_NAME))
	{
		std::ofstream{ SETTINGS_FILE_NAME };
	}*/
}

template<typename T>
void SaveSetting(SettingMap<T>& setting)
{
	using namespace rapidxml;

	file<> xmlFile("somefile.xml");
	xml_document<> doc;
	xml_node<>* node = doc.allocate_node(node_element, "a", "Google");
	doc.append_node(node);
	xml_attribute<>* attr = doc.allocate_attribute("href", "google.com");
	node->append_attribute(attr);
}

