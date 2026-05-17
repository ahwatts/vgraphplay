struct VertexInput {
    float2 position;
    float3 color;
}

struct VertexOutput {
    float3 color;
    float4 position : SV_Position;
};

[shader("vertex")]
VertexOutput vs_main(VertexInput in) {
    VertexOutput out;
    out.position = float4(in.position, 0.0, 1.0);
    out.color = in.color;
    return out;
}

[shader("fragment")]
float4 fs_main(VertexOutput in) : SV_Target {
    return float4(in.color, 1.0);
}
