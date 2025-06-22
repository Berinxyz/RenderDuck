#pragma once
#include "../EngineCore.h"
#include "../Handle.h"
#include "Entity.h"

#include <queue>
#include <unordered_map>

#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"

/// Macro for declaring the required map and getter function for each component type
/// Declares a private map of (EntityHandle, ComponentRef), and a public getter function GetComponent<ComponentType>(Entityhandle) 
#define COMPONENT_MAP_NAME(name) m_##name##s
#define COMPONENT_REF(name) name##WeakRef
#define REGISTER_COMPONENT(ComponentName) private: \
	std::unordered_map<EntityHandle, COMPONENT_REF(ComponentName)> COMPONENT_MAP_NAME(ComponentName);\
public: \
template<class ComponentName>\
COMPONENT_REF(ComponentName) GetComponent(const EntityHandle& handle)\
{\
	ASSERTMSG(handle.IsValid(), "Get" #ComponentName " called on invalid entity handle");\
	if (auto& it = COMPONENT_MAP_NAME(ComponentName).find(handle); it != COMPONENT_MAP_NAME(ComponentName).end()) { return it->second; }\
	else { ASSERTFAILMSG("Entity does not have requested " #ComponentName); }\
};


class EntityAdmin
{
public:

	EntityAdmin();

	EntityHandle CreateEntity();
	void DestroyEntity(EntityHandle& handle);

	REGISTER_COMPONENT(TransformComponent)
	REGISTER_COMPONENT(MeshComponent)

private:

	EntityHandle AllocateEntiyHandle();

	std::unordered_map<EntityHandle, Entity> m_Entities;
	std::queue<EntityHandle> m_FreeHandles;
};
