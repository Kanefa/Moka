#include "interactiveGroup.h"


InteractiveGroup::InteractiveGroup()
{
}

const std::vector<InteractiveObject> & InteractiveGroup::getInteractiveObjects() const
{
	return mInteractiveObjects;
}

void InteractiveGroup::setName(const std::string &name)
{
	mName = name;
}

void InteractiveGroup::setWidth(int width)
{
	mWidth = width;
}

void InteractiveGroup::setHeight(int height)
{
	mHeight = height;
}

void InteractiveGroup::setInteractiveObjects(const std::vector<InteractiveObject> &interactiveObjects)
{
	mInteractiveObjects = interactiveObjects;
}
