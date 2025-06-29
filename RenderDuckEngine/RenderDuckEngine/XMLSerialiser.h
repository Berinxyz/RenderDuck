#pragma once
#include "types.h"
#include <string>

#include "imgui.h"

class ValueSerialiser
{
public:

    enum Enum
    {
        FromString,
        ToString
    };

    ValueSerialiser() : m_Writing(false) {};
    ValueSerialiser(Enum mode) : m_Writing(mode == ToString) {};

    s8                  SerialiseValue(std::string& valueStr, s8* value);
    s16                 SerialiseValue(std::string& valueStr, s16* value);
    s32                 SerialiseValue(std::string& valueStr, s32* value);
    s64                 SerialiseValue(std::string& valueStr, s64* value);

    u8                  SerialiseValue(std::string& valueStr, u8* value);
    u16                 SerialiseValue(std::string& valueStr, u16* value);
    u32                 SerialiseValue(std::string& valueStr, u32* value);
    u64                 SerialiseValue(std::string& valueStr, u64* value);

    f32                 SerialiseValue(std::string& valueStr, f32* value);
    f64                 SerialiseValue(std::string& valueStr, f64* value);

    float2              SerialiseValue(std::string& valueStr, float2* value);
    float3              SerialiseValue(std::string& valueStr, float3* value);
    float4              SerialiseValue(std::string& valueStr, float4* value);

    bool                SerialiseValue(std::string& valueStr, bool* value);
    std::string         SerialiseValue(std::string& valueStr, std::string* value);
    vec4                SerialiseValue(std::string& valueStr, vec4* value);
    ImVec4              SerialiseValue(std::string& valueStr, ImVec4* value);

    bool SerialiseValueByName(const std::string& typeName, std::string& valueStr, void* valuePtr);

private:
    bool m_Writing;
};
