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

    void Increment() { ++m_ComponentCount; }
    void Decrement() { --m_ComponentCount; }
    u32 Count() { return m_ComponentCount; }

private:
    char* m_ComponentData{ nullptr };
    u32 m_ComponentSizeBytes { 0 };
    u32 m_ComponentCount { 0 };
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
        m_Scale = Vec4Make(1, 1, 1, 1);
        m_Rotation = DirectX::XMVectorZero();
    }
    ~TransformComponent() {}

    mtx44 GetTransformMatrix() const {
        // Compose the transform
        return DirectX::XMMatrixAffineTransformation(m_Scale, DirectX::XMVectorZero(), m_Rotation, m_Position);
    }

    vec3 m_Position;
    quat m_Rotation;
    vec3 m_Scale;

};

/// <summary>
/// MESH COMPONENT
/// </summary>
class RenderModel;
class ModelComponent : public Component
{
public:

    ModelComponent() : m_RenderModel(nullptr), m_RenderModelIndex(-1) {};

    RenderModel* m_RenderModel;
    u32 m_RenderModelIndex;
};






