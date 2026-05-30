struct Transformations {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
}

struct VertexInput {
    float3 position;
    float3 color;
    float2 tex;
}

struct VertexOutput {
    float4 position : SV_Position;
    float3 color;
    float2 tex;
};

ConstantBuffer<Transformations> xforms;

[shader("vertex")]
VertexOutput vs_main(VertexInput in) {
    VertexOutput out;
    out.position = mul(xforms.projection, mul(xforms.view, mul(xforms.model, float4(in.position, 1.0))));
    out.color = in.color;
    out.tex = in.tex;
    return out;
}

Sampler2D texture;

[shader("fragment")]
float4 fs_main(VertexOutput in) : SV_Target {
    return float4(in.color * texture.Sample(in.tex).rgb, 1.0);
}
