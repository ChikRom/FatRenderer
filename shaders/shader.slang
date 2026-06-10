struct VertexInput
{
    float3 inPosition;
    float2 inTexCoord;
    float3 inColor;
}
struct VertexOutput
{
    float4 sv_position : SV_Position;
    float2 texCoord;
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
    output.sv_position = mul(ubo.projection,mul(ubo.view,mul(ubo.model,float4(input.inPosition,1.0))));
    output.texCoord = input.inTexCoord;
    output.color = input.inColor;
	return output;
}


Sampler2D texture;
[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    // float3 color = inVert.color;
    // return float4(inVert.texCoord,0.0f, 1.0f);
    return texture.Sample(inVert.texCoord);
}