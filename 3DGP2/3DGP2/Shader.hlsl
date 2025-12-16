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

struct HS_OUTPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};

struct HS_CONSTANT
{
    float fTessEdges[3] : SV_TessFactor;
    float fTessInsides : SV_InsideTessFactor;
};

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstant")]
HS_OUTPUT HSMain(InputPatch<VS_OUTPUT, 3> input, uint i : SV_OutputControlPointID, int patchID : SV_PrimitiveID)
{
    HS_OUTPUT output;
    
    output.pos = input[i].position;
    output.color = input[i].color;
    output.tex = input[i].tex;

    return output;
}

HS_CONSTANT HSConstant(InputPatch<VS_OUTPUT, 3> input)
{
    HS_CONSTANT output;

    output.fTessEdges[0] = 4;
    output.fTessEdges[1] = 4;
    output.fTessEdges[2] = 4;
    
    output.fTessInsides = 4;

    return output;
}

struct DS_OUTPUT
{
    float4 posision : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};

[domain("tri")]
DS_OUTPUT DSMain(HS_CONSTANT input, float3 uvw : SV_DomainLocation, OutputPatch<HS_OUTPUT, 3> patch)
{
    DS_OUTPUT output;
    
    float3 pos = uvw.x * patch[0].pos + uvw.y * patch[1].pos + uvw.z * patch[2].pos;
    output.posision = float4(pos, 1.0f);
    output.color = uvw.x * patch[0].color + uvw.y * patch[1].color + uvw.z * patch[2].color;
    output.tex = uvw.x * patch[0].tex + uvw.y * patch[1].tex + uvw.z * patch[2].tex;
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    return tex.Sample(sample, input.tex);
}