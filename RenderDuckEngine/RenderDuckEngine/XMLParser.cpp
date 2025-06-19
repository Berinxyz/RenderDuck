#include "XMLParser.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <cstring> // For std::strcpy
#include <sstream>

XMLParser::XMLParser(std::string& fileName)
{
    m_FilePath = GetFullPath(fileName.c_str());

    if (!FileExists(m_FilePath.c_str()))
    {
        CreateFilePath();
    }

    LoadFile(&fileName);
}

XMLParser::XMLParser()
{}

bool XMLParser::FileExists(const char* fileName)
{
    struct stat buffer;
    return (stat(fileName, &buffer) == 0);
}

void XMLParser::CreateFilePath()
{
    namespace fs = std::filesystem;
    fs::path filePath = m_FilePath;
    if (!fs::exists(filePath)) {
        fs::path parentDir = filePath.parent_path();

        // Create directories if they don't exist
        if (!fs::exists(parentDir)) 
        {
            if (!fs::create_directories(parentDir)) 
            {
                std::cerr << "Failed to create directories: " << parentDir << '\n';
                return;
            }
        }

        // Create the file
        std::ofstream file(m_FilePath);
        if (!file) 
        {
            std::cerr << "Failed to create file: " << filePath << '\n';
        }
        else 
        {
            std::cout << "Created file: " << filePath << '\n';
        }
    }
}

XMLDoc* XMLParser::GetDoc()
{
    return &m_Doc;
}

bool XMLParser::LoadFile(const std::string* filePath)
{
    if (!filePath)
    {
        filePath = &m_FilePath;
    }

    std::fstream file(m_FilePath);
    if (!file.is_open()) 
    {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_Content = buffer.str();
    file.close();

    m_Doc.parse<0>(&m_Content[0]);

    return true;
}

bool XMLParser::SaveFile(const std::string* filePath)
{
    if (!filePath)
    {
        filePath = &m_FilePath;
    }

    std::ofstream file(m_FilePath, std::ofstream::out | std::ofstream::trunc);
    if (!file.is_open())
    {
        return false;
    }

    file << m_Doc;
    file.close();
    return true;
}

XMLNode* XMLParser::GetRootNode()
{
    return m_Doc.first_node();
}

XMLNode* XMLParser::GetNode(XMLNode* parent, const std::string& name)
{
    return parent ? parent->first_node(name.c_str()) : nullptr;
}

std::vector<XMLNode*> XMLParser::GetAllNodes(XMLNode* parent, const std::string& name)
{
    std::vector<XMLNode*> nodes;

    for (XMLNode* node = parent->first_node(name.c_str()); node; node = node->next_sibling())
    {
        nodes.push_back(node);
    }

    return nodes;
}

std::string XMLParser::GetAttribute(XMLNode* node, const std::string& attrName)
{
    if (auto attr = node->first_attribute(attrName.c_str())) 
    {
        return attr->value();
    }
    return "";
}

void XMLParser::SetAttribute(XMLNode* node, const std::string& name, const std::string& value) 
{
    char* attrName = m_Doc.allocate_string(name.c_str());
    char* attrVal = m_Doc.allocate_string(value.c_str());
    auto* attr = m_Doc.allocate_attribute(attrName, attrVal);
    node->append_attribute(attr);
}

XMLNode* XMLParser::AddNode(XMLNode* parent, const std::string& name, const std::string& value) 
{
    char* nodeName = m_Doc.allocate_string(name.c_str());
    char* nodeVal = m_Doc.allocate_string(value.c_str());

    XMLNode* node = m_Doc.allocate_node(rapidxml::node_element, nodeName, nodeVal);
    parent->append_node(node);
    return node;
}

XMLNode* XMLParser::AddRootNode(const std::string& name, const std::string& value)
{
    char* nodeName = m_Doc.allocate_string(name.c_str());
    char* nodeVal = m_Doc.allocate_string(value.c_str());

    XMLNode* node = m_Doc.allocate_node(rapidxml::node_element, nodeName, nodeVal);
    m_Doc.append_node(node);
    return node;
}





