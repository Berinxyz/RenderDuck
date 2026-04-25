#pragma once
#include "../EngineCore.h"
#include "EntityForwardDeclarations.h"
#include "Components/Component.h"

#include <queue>
#include <unordered_map>

struct Entity
{
	Entity(EntityHandle handle) : m_Handle(handle) {};
	ComponentMask m_ComponentMask;
	EntityHandle m_Handle;
};

extern u32 s_ComponentCounter;
template<class T>
inline int GetComponentMaskID()
{
	static int s_ComponentId = s_ComponentCounter++;
	return s_ComponentId;
}

class EntityAdmin
{
public:

	EntityAdmin();

	const EntityHandle CreateEntity();
	void DestroyEntity(const EntityHandle& handle);

	const Entity& GetEntity(const EntityIndex& index) { return m_EntityPool[index]; }
	const EntityIndex GetEntityIndex(const EntityHandle& handle) { return handle >> 32; }
	const EntityVersion GetEntityVersion(const EntityHandle& handle) { return (EntityVersion)handle; }
	const u32 GetEntityCount() { return m_EntityPool.size(); }

	bool IsEntityValid(const EntityIndex& index) { return index != EntityIndex(-1); }
	bool IsEntityValid(const EntityHandle& handle) { return IsEntityValid(GetEntityIndex(handle)); }

	template<typename T>
	T* AddComponent(const EntityHandle& handle);
	template<typename T>
	T* GetComponent(const EntityHandle& handle);
	template<typename T>
	void DestroyComponent(const EntityHandle& handle);
	template<typename T>
	bool HasComponent(const EntityHandle& handle);

private:

	Entity& GetEntity(const EntityHandle& handle) { return m_EntityPool[GetEntityIndex(handle)]; }
	const EntityHandle PackEntityHandle(const EntityIndex& index, const EntityVersion version) { return ((EntityHandle)index << 32) | ((EntityHandle)version); }

	u32 m_NextEntityHandle = 0;
	std::vector<Entity> m_EntityPool;
	std::queue<EntityIndex> m_FreeHandles;
	std::vector<ComponentPoolRef> m_ComponentPools;
};


template<typename T>
inline T* EntityAdmin::AddComponent(const EntityHandle& handle)
{
	u32 componentID = GetComponentMaskID<T>();

	// make the component pool larger if needed
	if (m_ComponentPools.size() <= componentID)
	{
		m_ComponentPools.resize(componentID + 1, nullptr);
	}

	// if this is a new component type, make a new pool
	if (m_ComponentPools[componentID] == nullptr)
	{
		m_ComponentPools[componentID] = std::make_shared<ComponentPool>(sizeof(T));
	}

	GetEntity(handle).m_ComponentMask.set(componentID);
	T* component = new (GetComponent<T>(handle)) T();

return component;
}

template<typename T>
inline T* EntityAdmin::GetComponent(const EntityHandle& handle)
{
	u32 componentID = GetComponentMaskID<T>();
	if (HasComponent<T>(handle))
	{
		return static_cast<T*>(m_ComponentPools[componentID]->GetComponent(GetEntityIndex(handle)));
	}

	return nullptr;
}

template<typename T>
inline void EntityAdmin::DestroyComponent(const EntityHandle& handle)
{
	if (GetEntity(handle).m_Handle != handle)
	{
		return;
	}

	u32 componentID = GetComponentMaskID<T>();
	GetEntity(handle).m_ComponentMask.reset(componentID);
}

template<typename T>
inline bool EntityAdmin::HasComponent(const EntityHandle& handle)
{
	u32 componentID = GetComponentMaskID<T>();
	return GetEntity(handle).m_ComponentMask.test(componentID);
}



template<typename... ComponentTypes>
class EntityView
{
public:
	EntityView(EntityAdminRef entityAdmin) : m_EntityAdmin(entityAdmin)
	{
		if (sizeof...(ComponentTypes) == 0)
		{
			m_All = true;
		}
		else
		{
			u32 componentIds[] = { 0, GetComponentMaskID<ComponentTypes>() ... };
			for (u32 i = 1; i < (sizeof...(ComponentTypes) + 1); ++i)
			{
				m_ComponentMask.set(componentIds[i]);
			}
		}
	}

	class Iterator
	{
	public:
		Iterator(EntityAdminRef entityAdmin, EntityIndex index, ComponentMask mask, bool all)
			: m_EntityAdmin(entityAdmin), m_EntityIndex(index), m_ComponentMask(mask), m_All(all) {
		}

		EntityHandle operator*() const
		{
			return m_EntityAdmin->GetEntity(m_EntityIndex).m_Handle;
		}
		bool operator==(const Iterator& other) const
		{
			return m_EntityIndex == other.m_EntityIndex || m_EntityIndex == m_EntityAdmin->GetEntityCount();
		}
		bool operator !=(const Iterator& other)
		{
			return m_EntityIndex != other.m_EntityIndex && m_EntityIndex != m_EntityAdmin->GetEntityCount();
		}
		Iterator& operator++()
		{
			do
			{
				++m_EntityIndex;
			} while (m_EntityIndex < m_EntityAdmin->GetEntityCount() && !ValidIndex());
			return *this;
		}
		Iterator& operator--()
		{
			do
			{
				--m_EntityIndex;
			} while (m_EntityIndex < m_EntityAdmin->GetEntityCount() && !ValidIndex());
			return *this;
		}
	private:
		bool ValidIndex() { return m_EntityAdmin->IsEntityValid(m_EntityIndex) && (m_All || m_ComponentMask == m_EntityAdmin->GetEntity(m_EntityIndex).m_ComponentMask); }

		EntityIndex m_EntityIndex;
		EntityAdminRef m_EntityAdmin;
		ComponentMask m_ComponentMask;
		bool m_All = false;
	};

	const Iterator begin() const
	{
		u32 firstIndex = 0;
		while (firstIndex < m_EntityAdmin->GetEntityCount() && (m_ComponentMask != m_EntityAdmin->GetEntity(firstIndex).m_ComponentMask || !m_EntityAdmin->IsEntityValid(firstIndex)))
		{
			++firstIndex;
		}

		return Iterator(m_EntityAdmin, firstIndex, m_ComponentMask, m_All);
	}
	const Iterator end() const 
	{
		return Iterator(m_EntityAdmin, EntityIndex(m_EntityAdmin->GetEntityCount()), m_ComponentMask, m_All);
	}

private:

	EntityAdminRef m_EntityAdmin;
	ComponentMask m_ComponentMask;
	bool m_All = false;
};

