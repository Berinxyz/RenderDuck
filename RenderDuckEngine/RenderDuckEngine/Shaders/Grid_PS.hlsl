
struct PSIn
{
	float4 m_WorldPosition    : SV_POSITION;
	float3 m_Normal     : TANGENT;
	float2 m_UV    : TEXCOORD;
};

float4 PSMain(PSIn pin) : SV_Target
{
    float3 colour = ((pin.m_Normal.xyz + 1.0f) / 2.0f);
    return float4(pin.m_UV.xy, 0.0f, 1.0f);
}
