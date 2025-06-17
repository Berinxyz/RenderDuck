#pragma once
#include <filesystem>
#include "Settings.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstring> // For std::strcpy

#include "include/rapidxml/rapidxml.hpp"
#include "include/rapidxml/rapidxml_utils.hpp"
#include "include/rapidxml/rapidxml_print.hpp"
#include "include/rapidxml/rapidxml_iterators.hpp"

inline bool FileExists(const char* fileName) 
{
	struct stat buffer;
	return (stat(fileName, &buffer) == 0);
}

void CreateFile(const char* fileName)
{
	if (!FileExists(fileName))
	{
		std::ofstream{fileName};
	}
}


void ParseConfigTest()
{

}


template<typename T>
void SaveSetting()
{
	using namespace rapidxml;

	file<> xmlFile("somefile.xml");
	xml_document<> doc;
	xml_node<>* node = doc.allocate_node(node_element, "a", "Google");
	doc.append_node(node);
	xml_attribute<>* attr = doc.allocate_attribute("href", "google.com");
	node->append_attribute(attr);
}

