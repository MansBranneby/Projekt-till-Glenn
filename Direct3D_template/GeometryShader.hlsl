struct GS_IN
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD;
};

struct GS_OUT
{
	float4 Pos : SV_POSITION;
	float4 WorldPos : World_POSITION;
	float4 WorldNor : World_NORMAL;
	float2 Tex : TEXCOORD;
};

cbuffer GS_CONSTANT_BUFFER : register(b0)
{
	matrix world, worldViewProj;
};

[maxvertexcount(6)]
void GS_main( triangle GS_IN input[3], inout TriangleStream< GS_OUT > output)
{
	GS_OUT element;
	float4 normal = float4(normalize(cross(input[1].Pos - input[0].Pos, input[2].Pos - input[0].Pos)), 0);
	for (uint i = 0; i < 3; i++)
	{
		element.Pos = mul(input[i].Pos, worldViewProj);
		element.WorldPos = mul(input[i].Pos, world);
		element.WorldNor = mul(normal, world);
		element.Tex = input[i].Tex;
		output.Append(element);
	}
	output.RestartStrip();
	
	for (uint i = 0; i < 3; i++)
	{
		element.Pos = mul(input[i].Pos + normal*0.5, worldViewProj);
		element.WorldPos = mul(input[i].Pos + normal * 0.5, world);
		element.WorldNor = mul(normal, world);
		element.Tex = input[i].Tex;
		output.Append(element);
	}
	output.RestartStrip();
}