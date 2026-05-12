static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

static float3 colors[3] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
);

struct VertexOutput {
    float3 color;
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput vs_main(uint vid : SV_VertexID) {
    VertexOutput output;
    output.position = float4(positions[vid], 0.0, 1.0);
    output.color = colors[vid];
    return output;
}

[shader("fragment")]
float4 fs_main(VertexOutput vertex) : SV_Target
{
    float3 color = vertex.color;
    return float4(color, 1.0);
}
