#pragma once

#include "../Entity.h"


class Component
{
public:
	Component() {}
	~Component() {}
		
	virtual void OnAdded() {};
	virtual void OnRemoved() {};

	virtual void OnEnabled() {};
	virtual void OnDisabled() {};

};

#define COMPONENT_CLASS(name) class name;\
typedef std::shared_ptr<name> name##Ref;\
typedef std::weak_ptr<name> name##WeakRef;\
class name


