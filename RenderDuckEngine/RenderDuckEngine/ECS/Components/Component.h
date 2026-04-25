#pragma once
#include "../../EngineCore.h"
#include "../EntityForwardDeclarations.h"

class Component
{
public:
	Component() {}
	~Component() {}

	virtual void OnAdded() {}
	virtual void OnDestroy() {};
};

class ComponentPool
{
public:
    ComponentPool(u32 componentSizeBytes)
    {
        m_ComponentSizeBytes = componentSizeBytes;
        m_ComponentData = new char[m_ComponentSizeBytes * c_MaxEntities];
    }

    ~ComponentPool()
    {
        delete[] m_ComponentData;
    }

    inline void* GetComponent(u32 index)
    {
        return m_ComponentData + index * m_ComponentSizeBytes;
    }

private:
    char* m_ComponentData{ nullptr };
    u32 m_ComponentSizeBytes { 0 };
};
typedef std::shared_ptr<ComponentPool> ComponentPoolRef;

/// <summary>
/// TRANSFORM COMPONENT
/// </summary>
class TransformComponent : public Component
{
public:
    TransformComponent()
    {
        m_Position = DirectX::XMVectorZero();
        m_Scale = DirectX::XMVectorZero();
        m_Rotation = DirectX::XMVectorZero();
    }
    ~TransformComponent() {}

    void SetPosition(const vec3 & pos)
    {
        m_Position = pos;
    }

    void SetRotation(const quat & rot)
    {
        m_Rotation = rot;
    }

    void SetScale(const vec3 & scl)
    {
        m_Scale = scl;
    }

    mtx44 GetTransformMatrix() const {
        using namespace DirectX;

        // Compose the transform
        return DirectX::XMMatrixAffineTransformation(m_Scale, XMVectorZero(), m_Rotation, m_Position);
    }

private:
    vec3 m_Position;
    quat m_Rotation;
    vec3 m_Scale;
};



/// <summary>
/// MESH COMPONENT
/// </summary>
class MeshComponent : public Component
{
public:
    vec3 m_Data;
};






