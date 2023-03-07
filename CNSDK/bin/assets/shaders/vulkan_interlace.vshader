// Compatibility definitions to make HLSL/GLSL work with minimal changes
#if USE_HLSL == 1

#define uniform /**/

#define InTex input.Tex
#define OutTexCoord output.TexCoord
#define OutScreenTexCoord output.ScreenTexCoord 
#define OutPos output.Pos 

#else

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float4x4 mat4
#define fmod mod
#define lerp mix
#define mul(x,y) ((x)*(y))

#define InTex aTexCoord
#define OutTexCoord TexCoord
#define OutScreenTexCoord ScreenTexCoord 
#define OutPos gl_Position

#endif

// Shader input/output signatures
#if USE_HLSL == 1

struct VSInput
{
    float3 Pos : POSITION;
    float3 Col : COLOR;
    float2 Tex : TEXCOORD;
};

struct PSInput
{
    float4               Pos            : SV_POSITION;
    noperspective float2 TexCoord       : TEXCOORD;
    noperspective float2 ScreenTexCoord : TEXCOORD2;
};

#else

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec2 TexCoord;
layout(location = 1) out vec2 ScreenTexCoord;

#endif

// Global variables (uniforms for GLSL and constant buffer for HLSL)
#if USE_HLSL == 1
cbuffer LeiaInterlaceShaderConstantBufferData : register(b0)
{
#else
layout(binding = 0) uniform UniformBufferObject
{
#endif
    float  hardwareViewsX;
    float  hardwareViewsY;
    float  viewResX;
    float  viewResY;

    float  viewportX;
    float  viewportY;
    float  viewportWidth;
    float  viewportHeight;

    float  minView;
    float  maxView;
    float  n;
    float  d_over_n;

    float  p_over_du;
    float  p_over_dv;
    float  colorSlant;
    float  colorInversion;

    float  faceX;
    float  faceY;
    float  faceZ;
    float  pixelPitch;

    float  du;
    float  dv;
    float  s;
    float  cos_theta;

    float  sin_theta;
    float  peelOffset;
    float  No;
    float  displayResX;

    float  displayResY;
    float  blendViews;
    int    interlaceMode;
    int    debugMode;

    int    perPixelCorrection;
    int    viewTextureType;
    float  reconvergenceAmount;
    float  softwareViews;

    float  absReconvergenceAmount;
    float  padding0;
    float  padding1;
    float  padding2;

    float4x4 textureTransform;

    float4x4 rectTransform;
#if USE_HLSL == 1
};
#else
} ubo;
#endif

#if USE_HLSL == 1
PSInput main(VSInput input)
#else
void main()
#endif
{
#if USE_HLSL == 1
    PSInput output = (PSInput)0;
#endif

    // Scale and offset texture coordinates.
    OutTexCoord = mul(ubo.textureTransform, float4(InTex, 0.0, 1.0)).xy;

    // Scale of offset position of rectangle being rendered.
    OutScreenTexCoord = mul(ubo.rectTransform, float4(InTex, 0.0, 1.0)).xy;

    // Scale and offset position.
    OutPos = float4((OutScreenTexCoord * 2.0) - 1.0, 0.0, 1.0);

#if UV_TOP_LEFT == 1
    OutTexCoord.y = 1.0 - OutTexCoord.y;
#endif

#if USE_HLSL == 1
    return output;
#endif
}