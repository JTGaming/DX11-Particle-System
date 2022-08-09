#define MAX_ROTATIONS (4096 / 3)
#define IDENTITY_MATRIX matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)

cbuffer cb_per_frame : register(b0)
{
    matrix mViewProj;
};

cbuffer cb_rotations : register(b1)
{
    float4x3 mRotation[MAX_ROTATIONS];
};

struct VertexInputType
{
    float3 position : POSITION;
    float3 pos : INST_POS_ROT0;
    uint rot : INST_POS_ROT1;
    
    float3 color : INST_COL_SCALE0;
    float scale : INST_COL_SCALE1;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

matrix m_scale(matrix m, float x)
{
    m[0][0] *= x;
    m[1][0] *= x;
    m[2][0] *= x;
    m[0][1] *= x;
    m[1][1] *= x;
    m[2][1] *= x;
    m[0][2] *= x;
    m[1][2] *= x;
    m[2][2] *= x;
    m[0][3] *= x;
    m[1][3] *= x;
    m[2][3] *= x;

    return m;
}

matrix m_translate(matrix m, float3 v)
{
    float x = v.x, y = v.y, z = v.z;
    m[0][3] = x;
    m[1][3] = y;
    m[2][3] = z;
    return m;
}

matrix m_rotate(float4x3 rotation)
{
    matrix m = IDENTITY_MATRIX;
    m[0].xyz = rotation[0];
    m[1].xyz = rotation[1];
    m[2].xyz = rotation[2];
    return m;
}

PixelInputType vs_main(VertexInputType input)
{
    PixelInputType output;
    output.position = mul(mul(float4(input.position, 1.f),
                    transpose(
                        m_translate(
                            m_scale(input.rot ?
                                m_rotate(
                                mRotation[input.rot - 1]) : IDENTITY_MATRIX,
                            input.scale),
                        input.pos))),
                    mViewProj);
    
    output.color = float4(input.color, 1.f);
    return output;
}


float4 ps_main(PixelInputType input) : SV_TARGET
{
    return input.color;
}
