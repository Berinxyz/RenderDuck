#include "XMLSerialiser.h"

#include <cstdio>
#include <sstream>

#define Serialize(type) else if (typeName == #type)\
    {\
        SerialiseValue(valueStr, static_cast<type*>(valuePtr));\
    }

bool XMLSerialiser::SerialiseValueByName(const std::string& typeName, std::string& valueStr, void* valuePtr)
{
    if (typeName == "bool")
    {
        SerialiseValue(valueStr, static_cast<bool*>(valuePtr));
    }
    else if (typeName == "string")
    {
        SerialiseValue(valueStr, static_cast<std::string*>(valuePtr));
    }
    else if (typeName == "f32")
    {
        SerialiseValue(valueStr, static_cast<f32*>(valuePtr));
    }
    else if (typeName == "f64")
    {
        SerialiseValue(valueStr, static_cast<f64*>(valuePtr));
    }
    else if (typeName == "s8")
    {
        SerialiseValue(valueStr, static_cast<s8*>(valuePtr));
    }
    else if (typeName == "s16")
    {
        SerialiseValue(valueStr, static_cast<s16*>(valuePtr));
    }
    else if (typeName == "s32")
    {
        SerialiseValue(valueStr, static_cast<s32*>(valuePtr));
    }
    else if (typeName == "s64")
    {
        SerialiseValue(valueStr, static_cast<s64*>(valuePtr));
    }
    else if (typeName == "u8")
    {
        SerialiseValue(valueStr, static_cast<u8*>(valuePtr));
    }
    else if (typeName == "u16")
    {
        SerialiseValue(valueStr, static_cast<u16*>(valuePtr));
    }
    else if (typeName == "u32")
    {
        SerialiseValue(valueStr, static_cast<u32*>(valuePtr));
    }
    else if (typeName == "u64")
    {
        SerialiseValue(valueStr, static_cast<u64*>(valuePtr));
    }
    else if (typeName == "XMVECTOR")
    {
        SerialiseValue(valueStr, static_cast<DirectX::XMVECTOR*>(valuePtr));
    }
    else if (typeName == "ImVec4")
    {
        SerialiseValue(valueStr, static_cast<ImVec4*>(valuePtr));
    }
    else
    {
        return false;
    }

    return true;
}


s8 XMLSerialiser::SerialiseValue(std::string& valueStr, s8* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        s8 val = static_cast<s8>(std::stoi(valueStr));
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

s16 XMLSerialiser::SerialiseValue(std::string& valueStr, s16* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        s16 val = static_cast<s16>(std::stoi(valueStr));
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

s32 XMLSerialiser::SerialiseValue(std::string& valueStr, s32* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        s32 val = std::stoi(valueStr);
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

s64 XMLSerialiser::SerialiseValue(std::string& valueStr, s64* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        s64 val = std::stoll(valueStr);
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

u8 XMLSerialiser::SerialiseValue(std::string& valueStr, u8* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        u8 val = static_cast<u8>(std::stoul(valueStr));
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

u16 XMLSerialiser::SerialiseValue(std::string& valueStr, u16* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        u16 val = static_cast<u16>(std::stoul(valueStr));
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

u32 XMLSerialiser::SerialiseValue(std::string& valueStr, u32* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        u32 val = std::stoul(valueStr);
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

u64 XMLSerialiser::SerialiseValue(std::string& valueStr, u64* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        u64 val = std::stoull(valueStr);
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0;
}

f32 XMLSerialiser::SerialiseValue(std::string& valueStr, f32* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        f32 val = std::stof(valueStr);
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0.0f;
}

f64 XMLSerialiser::SerialiseValue(std::string& valueStr, f64* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = std::to_string(*value);
    }
    else
    {
        f64 val = std::stod(valueStr);
        if (value)
        {
            *value = val;
        }
        return val;
    }
    return 0.0;
}

bool XMLSerialiser::SerialiseValue(std::string& valueStr, bool* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = *value ? "true" : "false";
    }
    else
    {
        bool val = valueStr == "true" ? true : false;
        if (value)
        {
            *value = val;
        }

        return val;
    }
}

std::string XMLSerialiser::SerialiseValue(std::string& valueStr, std::string* value)
{
    if (m_Writing)
    {
        assert(value);
        valueStr = *value;
    }
    else
    {
        if (value)
        {
            *value = valueStr;
        }

        return valueStr;
    }
}

DirectX::XMVECTOR XMLSerialiser::SerialiseValue(std::string& valueStr, DirectX::XMVECTOR* value)
{
    if (m_Writing)
    {
        assert(value);
        std::ostringstream oss;
        oss << value->m128_f32[0] << " " << value->m128_f32[1] << " " << value->m128_f32[2] << " " << value->m128_f32[3];
        valueStr = oss.str();
    }
    else
    {
        float x, y, z, w;
        sscanf_s(valueStr.c_str(), "%f,%f,%f,%f", &x, &y, &z, &w);

        if (value)
        {
            *value = DirectX::XMVectorSet(x, y, z, w);
        }

        return DirectX::XMVectorSet(x, y, z, w);
    }
}

ImVec4 XMLSerialiser::SerialiseValue(std::string& valueStr, ImVec4* value)
{
    if (m_Writing)
    {
        assert(value);
        std::ostringstream oss;
        oss << value->x << " " << value->y << " " << value->z << " " << value->w;
        valueStr = oss.str();
    }
    else
    {
        float x, y, z, w;
        sscanf_s(valueStr.c_str(), "%f,%f,%f,%f", &x, &y, &z, &w);

        if (value)
        {
            *value = ImVec4({ x, y, z, w });
        }

        return ImVec4({ x, y, z, w });
    }
}
