

cbuffer CBuffer0
{
	float red_fraction;  //4bytes
	float3 packing; // 3x4 bytes = 12 bytes

}




struct Vout
{
	float4 position : SV_POSITION;
	float4 color : COLOR;

};

Vout VShader(float4 position : POSITION, float4 color : COLOR)
{
	Vout output;

	color.r *= red_fraction;
	output.position = position;
	output.color = color;

	return output;
}

float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
	return color;
}

