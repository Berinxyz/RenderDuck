#ifndef ENGINE_UTILS
#define ENGINE_UTILS

#include "imgui.h"
#include <DirectXColors.h>

static ImVec4 DirectXColorToImVec4(const DirectX::XMVECTORF32& in)
{
    ImVec4 out;
    DirectX::XMFLOAT4 col;
    DirectX::XMStoreFloat4(&col, in);
    out.x = col.x;
    out.y = col.y;
    out.z = col.z;
    out.w = col.w;
    return out;
}

static const DirectX::XMVECTORF32 DirectXColorFromImVec4(const ImVec4& in)
{
    DirectX::XMFLOAT4 col(in.x, in.y, in.z, in.w);
    DirectX::XMVECTORF32 out;
    out.v = DirectX::XMLoadFloat4(&col);
    return out;
}

#endif