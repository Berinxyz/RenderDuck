
struct VSIn
{
	float3 m_Pos    : POSITION;
    float3 m_Normal : NORMAL;
	float2 m_UV     : TEXCOORD;
};

struct PSIn
{
	float4 m_WorldPosition    : SV_POSITION;
	float3 m_Normal           : TANGENT;
	float2 m_UV               : TEXCOORD;
};

// Constant data that varies per frame.
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

PSIn VSMain(VSIn vin)
{
    PSIn psin = (PSIn)0;
	psin.m_WorldPosition = mul(float4(vin.m_Pos, 1.0f), gWorld);
    psin.m_Normal = mul(vin.m_Normal, (float3x3)gWorld);	
    psin.m_UV = vin.m_UV;
    return psin;
}