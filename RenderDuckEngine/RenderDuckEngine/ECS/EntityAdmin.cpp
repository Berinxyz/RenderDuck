#include "EntityAdmin.h"

EntityAdmin::EntityAdmin()
{
}

EntityHandle EntityAdmin::CreateEntity()
{
	EntityHandle handle = AllocateEntiyHandle();

	if (auto it = m_Entities.find(handle); it == m_Entities.end())
	{
		EntityRef entity = std::make_shared<Entity>(handle, this);
	}
	else
	{
		ASSERTFAILMSG("AllocateEntityHandle returned existing entity id");
	}
	return EntityHandle();
}

void EntityAdmin::DestroyEntity(EntityHandle& handle)
{
}

EntityHandle EntityAdmin::AllocateEntiyHandle()
{
	if (!m_FreeHandles.empty())
	{
		EntityHandle handle = m_FreeHandles.front();
		m_FreeHandles.pop();
		return handle;
	}

	return m_Entities.size();
}
