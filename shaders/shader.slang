static float2 positions[3] = float2[](
	float2(0.0f, -0.5f),
	float2(0.5f, 0.5f),
	float2(-0.5f, 0.5f)
);

static float3 colours[3] = float3[](
	float3(1.0f, 0.0f, 0.0f),
	float3(0.0f, 1.0f, 0.0f),
	float3(0.0f, 0.0f, 1.0f)
);


struct VertexOutput
{
    float3 colour;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexOutput vertMain(uint vid: SV_VertexID)
{
    VertexOutput output;
    output.sv_position = float4(positions[vid], 0.0,1.0);
    output.colour = colours[vid];
	return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
	float3 colour = inVert.colour;
	return float4(colour, 1.0f);
}