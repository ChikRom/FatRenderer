struct VertexInput
{
    float2 inPosition;
    float3 inColor;
}
struct VertexOutput
{
    float4 sv_position : SV_Position;
    float3 color;
};

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
}

ConstantBuffer<UniformBuffer> ubo;

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;
    output.sv_position = mul(ubo.projection,mul(ubo.view,mul(ubo.model,float4(input.inPosition,0.0,1.0))));
    output.color = input.inColor;
	return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    float3 color = inVert.color;
    return float4(color, 1.0f);
}