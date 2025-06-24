#pragma once
#include <memory>
#include "../../Handle.h"
#include "Components/Component.h"

typedef Handle64 EntityHandle;

class EntityAdmin;
class Entity
{
public:
	Entity(EntityHandle handle, EntityAdmin* admin) : m_Handle(handle), m_EntityAdmin(admin) {}
	const EntityHandle GetHandle() { return m_Handle; };

	template<typename T>
	std::shared_ptr<T> GetComponent()
	{
		T* component = static_cast<Component*>();
		return m_EntityAdmin->GetComponent(m_Handle);
	}

private:

	EntityHandle m_Handle;
	EntityAdmin* m_EntityAdmin;
};

typedef std::shared_ptr<Entity> EntityRef; 
typedef std::weak_ptr<Entity> EntityWeakRef;