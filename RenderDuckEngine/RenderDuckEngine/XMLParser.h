#pragma once
#include "types.h"

#include "include/rapidxml/rapidxml.hpp"
#include "include/rapidxml/rapidxml_utils.hpp"
#include "include/rapidxml/rapidxml_print.hpp"
#include "include/rapidxml/rapidxml_iterators.hpp"

#define XML_FILE_PATH "xml/"
#define XML_FILE_EXT ".xml"

typedef rapidxml::xml_document<> XMLDoc;
typedef rapidxml::xml_node<> XMLNode;
typedef rapidxml::xml_attribute<> XMLAttr;

class XMLParser
{
public:
    // loads file on construction
    XMLParser(std::string& fileName);
    XMLParser();

    std::string GetFullPath(std::string filename) { return XML_FILE_PATH + filename + XML_FILE_EXT; };

    XMLDoc* GetDoc();

    XMLNode* GetRootNode();
    XMLNode* GetNode(XMLNode* parent, const std::string& name);
    std::vector<XMLNode*> GetAllNodes(XMLNode* parent, const std::string& name);
    std::string GetAttribute(XMLNode* node, const std::string& attrName);
    void XMLParser::SetAttribute(XMLNode* node, const std::string& name, const std::string& value);
    XMLNode* XMLParser::AddNode(XMLNode* parent, const std::string& name, const std::string& value = "");
    XMLNode* XMLParser::AddRootNode(const std::string& name, const std::string& value = "");

    // defaults to saving and loading current m_FilePath
    bool LoadFile(const std::string* filename = nullptr);
    bool SaveFile(const std::string* filename = nullptr);

    void ClearDoc() { m_Doc.clear(); }

private:

    void CreateFilePath();
    bool FileExists(const char* fileName);

    bool m_Writing;
    std::string m_FilePath;

    XMLDoc m_Doc;
    std::string m_Content;

};





