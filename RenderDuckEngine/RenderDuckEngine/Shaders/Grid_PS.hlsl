
// Include common HLSL code.
#include "Common.hlsl"

//cbuffer GridConstants : register(b2)
//{
    
//}

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH : POSITION1;
    float3 PosW : POSITION2;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

float PristineGrid(float2 uv, float2 lineWidth)
{
    lineWidth = saturate(lineWidth);
    float4 uvDDXY = float4(ddx(uv), ddy(uv));
    float2 uvDeriv = float2(length(uvDDXY.xz), length(uvDDXY.yw));
    bool2 invertLine = lineWidth > 0.5;
    float2 targetWidth = lerp(lineWidth, 1.0 - lineWidth, invertLine);
    float2 drawWidth = clamp(targetWidth, uvDeriv, 0.5);
    float2 lineAA = max(uvDeriv, 0.000001) * 1.5;
    float2 gridUV = abs(frac(uv) * 2.0 - 1.0);
    gridUV = lerp(gridUV, 1.0 - gridUV, invertLine);
    float2 grid2 = smoothstep(drawWidth + lineAA, drawWidth - lineAA, gridUV);
    grid2 *= saturate(targetWidth / drawWidth);
    grid2 = lerp(grid2, targetWidth, saturate(uvDeriv * 2.0 - 1.0));
    grid2 = lerp(grid2, 1.0 - grid2, invertLine);
    return lerp(grid2.x, 1.0, grid2.y);
}

float4 PSMain(VertexOut pin) : SV_Target
{
    float LineWidth = 0.01f;
    float4 BaseColor = float4(0.318627, 0.318627, 0.318627, 1.0);
    float4 LineColor = 1.xxxx;
    
    float grid = PristineGrid(pin.TexC * 1000.0f, float2(LineWidth, LineWidth));
    
    float gridFadeMin = 10.0;
    float gridFadeMax = 50.0;
    float gridFade = saturate(smoothstep(gridFadeMax, gridFadeMin, length(gEyePosW - pin.PosW)));
    
    float gridColour = lerp(BaseColor, LineColor, grid * LineColor.a * gridFade);
    return gridColour;
}


