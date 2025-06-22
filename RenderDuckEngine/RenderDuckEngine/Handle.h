#pragma once
#include "types.h"

template<typename T>
class Handle
{
public:

    static constexpr T s_Invalid = static_cast<T>(-1);
    Handle() : m_Value(s_Invalid) {}
    Handle(T v) : m_Value(v) {}

    Handle(const Handle&) = default;
    Handle(Handle&&) = default;
    Handle& operator=(const Handle&) = default;
    Handle& operator=(Handle&&) = default;

    bool operator==(const Handle& other) const { return m_Value == other.m_Value; }
    bool operator!=(const Handle& other) const { return m_Value != other.m_Value; }
    bool operator==(T v) const { return m_Value == v; }
    bool operator!=(T v) const { return m_Value != v; }

    bool operator<(T v) const { return m_Value < v; }
    bool operator>(T v) const { return m_Value > v; }

    operator T() const { return m_Value; }
    void Invalidate() { m_Value = s_Invalid; }
    bool IsValid() const { return m_Value != s_Invalid; }

    T GetValue() const { return m_Value; }
private:
    T m_Value;
};

namespace std {
    template <typename T>
    struct hash<Handle<T>> {
        size_t operator()(const Handle<T>& handle) const noexcept 
        {
            return (size_t)handle.GetValue();
        }
    };
}

typedef Handle<u64> Handle64;
typedef Handle<u32> Handle32; 
typedef Handle<u16> Handle16;