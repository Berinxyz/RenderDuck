#pragma once
#include "../../EngineCore.h"
#include "Component.h"


COMPONENT_CLASS(TransformComponent) : public Component
{
public:
    TransformComponent() 
    {    
        m_Position = DirectX::XMVectorZero();
        m_Scale = DirectX::XMVectorZero();
        m_Rotation = DirectX::XMVectorZero();
    }
    ~TransformComponent() {}

    void SetPosition(const vec3& pos) 
    {
        m_Position = pos;
    }

    void SetRotation(const quat& rot) 
    {
        m_Rotation = rot;
    }

    void SetScale(const vec3& scl) 
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

