#include "EntityAdmin.h"

EntityAdmin::EntityAdmin()
{
	m_EntityPool.reserve(c_MaxEntities);
}

const EntityHandle EntityAdmin::CreateEntity()
{
	// get any deleted handles and see if we can use them
	if(!m_FreeHandles.empty())
	{
		// pop the next handle
		EntityIndex newIndex = m_FreeHandles.front();
		m_FreeHandles.pop();
		EntityVersion version = GetEntityVersion(m_EntityPool[newIndex].m_Handle);
		EntityHandle newHandle = PackEntityHandle(newIndex, version);
		m_EntityPool[newIndex].m_Handle = newHandle;
		return m_EntityPool[newIndex].m_Handle;
	}

	m_EntityPool.push_back(Entity(PackEntityHandle(m_EntityPool.size(), 0)));
	return m_EntityPool.back().m_Handle;
}

void EntityAdmin::DestroyEntity(const EntityHandle& handle)
{
	EntityIndex index = GetEntityIndex(handle);
	// invalidate the entity handle
	m_EntityPool[index].m_Handle = PackEntityHandle(EntityIndex(-1), GetEntityVersion(handle) + 1);
	m_EntityPool[index].m_ComponentMask.reset();
	m_FreeHandles.push(index);
}

void EntityAdmin::ClearScene()
{
	u32 entityChecked = 0;
	for (EntityHandle entity : EntityView<>(std::make_shared<EntityAdmin>(*this)))
	{
		entityChecked++;
		DestroyEntity(entity);
	}

	m_EntityPool.clear();
}
