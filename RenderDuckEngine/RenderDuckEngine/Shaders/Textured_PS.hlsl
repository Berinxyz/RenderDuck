
// Include common HLSL code.
#include "Common.hlsl"

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH   : POSITION1;
    float3 PosW    : POSITION2;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexC    : TEXCOORD;
};

float4 PSMain(VertexOut pin) : SV_Target
{
    float3 colour = ((pin.NormalW.xyz + 1.0f) / 2.0f);
    colour.xy =  pin.TexC;
    return float4(pin.TexC.xy,0.0f, 1.0f);
}


