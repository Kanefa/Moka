#include "objectGroups.h"

#include <cassert>
#include <vector>
#include <stdexcept>


ObjectGroups::ObjectGroups(const std::string &filename)
{
	read(filename);
}

const PreventionGroup & ObjectGroups::getPreventionGroup() const
{
	return mPreventionGroup;
}

void ObjectGroups::read(const std::string &filename)
{
	tinyxml2::XMLDocument config;
	loadFile(filename, config);
	readPreventionGroup(config);
}

void ObjectGroups::loadFile(const std::string &filename, tinyxml2::XMLDocument &config)
{
	if (config.LoadFile(filename.c_str()) != tinyxml2::XML_NO_ERROR)
	{
		throw std::runtime_error("TinyXML2 - Failed to load " + filename);
	}
}

void ObjectGroups::readPreventionGroup(tinyxml2::XMLDocument &config)
{
	tinyxml2::XMLElement *map = config.FirstChildElement("map");
	assert(("ALW - Logic Error: There is no map to read! This must be a incorrectly formatted TMX file.", map));

	tinyxml2::XMLElement *objectGroup = map->FirstChildElement("objectgroup");
	assert(("ALW - Logic Error: There must be at least one object group!", objectGroup));

	std::string objectGroupName = "Prevention";
	while (objectGroupName != objectGroup->Attribute("name"))
	{
		objectGroup = objectGroup->NextSiblingElement("objectgroup");
		assert(("ALW - Logic Error: There must be an object group named Prevention!", objectGroup));
	}
	
	assert(("ALW - Logic Error : There is no name attribute!", objectGroup->Attribute("name")));
	mPreventionGroup.setName(static_cast<std::string>(objectGroup->Attribute("name")));
	assert(("ALW - Logic Error : There is no width attribute!", objectGroup->Attribute("width")));
	mPreventionGroup.setWidth(objectGroup->IntAttribute("width"));
	assert(("ALW - Logic Error : There is no height attribute!", objectGroup->Attribute("height")));
	mPreventionGroup.setHeight(objectGroup->IntAttribute("height"));

	std::vector<PreventionObject> preventionObjects;
	tinyxml2::XMLElement *object = objectGroup->FirstChildElement("object");
	assert(("ALW - Logic Error: The Prevention object group is empty!", objectGroup));
	for (; object != nullptr; object = object->NextSiblingElement("object"))
	{
		assert(("ALW - Logic Error : There is no name attribute!", object->Attribute("name")));
		std::string name = object->Attribute("name");
		assert(("ALW - Logic Error : There is no type attribute!", object->Attribute("type")));
		std::string type = object->Attribute("type");
		assert(("ALW - Logic Error : There is no x attribute!", object->IntAttribute("x")));
		int x = object->IntAttribute("x");
		assert(("ALW - Logic Error : There is no x attribute!", object->IntAttribute("y")));
		int y = object->IntAttribute("y");
		assert(("ALW - Logic Error : There is no x attribute!", object->IntAttribute("width")));
		int width = object->IntAttribute("width");
		assert(("ALW - Logic Error : There is no x attribute!", object->IntAttribute("height")));
		int height = object->IntAttribute("height");
		preventionObjects.push_back(PreventionObject(name, type, x, y, width, height));
	}

	mPreventionGroup.setPreventionObjects(preventionObjects);
}
