Texture2D txDiffuse : register(t0);
SamplerState sampAni;

struct GS_OUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos : World_POSITION;
	float4 WorldNor : World_NORMAL;
	float2 Tex : TEXCOORD;
};

cbuffer FS_CONSTANT_BUFFER : register(b0)
{
	float3 lightPos;
	float3 lightCol;
};

float4 PS_main(GS_OUT input) : SV_Target
{
	float3 textureCol = txDiffuse.Sample(sampAni, input.Tex).xyz;
	float3 ambientCol = { 0.2, 0.2, 0.2 };
	float3 fragmentCol = textureCol * ambientCol;
	float diffuseFactor = max(dot(normalize(lightPos - input.WorldPos.xyz), normalize(input.WorldNor.xyz)), 0);
	fragmentCol += textureCol * diffuseFactor * lightCol;
	return float4(fragmentCol, 1.0f);
};