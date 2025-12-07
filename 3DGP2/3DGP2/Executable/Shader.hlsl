struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};

Texture2D tex : register(t0);
SamplerState sample : register(s0);

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.position = float4(input.position, 1.0f); 
    output.color = input.color;
    output.tex = input.tex;
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return tex.Sample(sample, input.tex);
}