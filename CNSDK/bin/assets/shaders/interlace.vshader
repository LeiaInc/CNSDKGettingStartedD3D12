// Compatibility definitions to make HLSL/GLSL work with minimal changes
#if USE_HLSL == 1

#define uniform /**/
#define highp /**/
#define gv(x) x

#define InTex input.Tex
#define OutTexCoord output.TexCoord
#define OutScreenTexCoord output.ScreenTexCoord 
#define OutPos output.Pos 

#else

#if USE_GLSL_VULKAN
#define gv(x) ubo.x
#define LAYOUTLOC(x) layout(location = x)
#else
#define gv(x) x
#define LAYOUTLOC(x) /**/
#endif

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

LAYOUTLOC(0) in vec3 aPos;
LAYOUTLOC(1) in vec3 aColor;
LAYOUTLOC(2) in vec2 aTexCoord;

LAYOUTLOC(0) out vec2 TexCoord;
LAYOUTLOC(1) out vec2 ScreenTexCoord;

#endif

// Global variables (uniforms for GLSL and constant buffer for HLSL)
#if USE_HLSL == 1
cbuffer LeiaInterlaceShaderConstantBufferData : register(b0)
{
#elif USE_GLSL_VULKAN == 1
layout(binding = 0) uniform UniformBufferObject
{
#endif

    //Must have 16 byte padding
    uniform float  hardwareViewsX;
    uniform float  hardwareViewsY;
    uniform float  viewResX;
    uniform float  viewResY;

    uniform highp float  viewportX;
    uniform highp float  viewportY;
    uniform highp float  viewportWidth;
    uniform highp float  viewportHeight;

    uniform float  minView;
    uniform float  maxView;
    uniform highp float  n;
    uniform highp float  d_over_n;

    uniform highp float  p_over_du;
    uniform highp float  p_over_dv;
    uniform float  colorSlant;
    uniform float  colorInversion;

    uniform highp float  faceX;
    uniform highp float  faceY;
    uniform highp float  faceZ;
    uniform highp float  pixelPitch;

    uniform highp float  du;
    uniform highp float  dv;
    uniform highp float  s;
    uniform highp float  cos_theta;

    uniform highp float  sin_theta;
    uniform highp float  peelOffset;
    uniform highp float  No;
    uniform float  displayResX;

    uniform float  displayResY;
    uniform float  blendViews;
    uniform int    interlaceMode;
    uniform int    debugMode;

    uniform int    perPixelCorrection;
    uniform int    viewTextureType;
    uniform float  reconvergenceAmount;
    uniform float  softwareViews;

    uniform highp float  absReconvergenceAmount;
    uniform highp float  alpha;
    uniform float  padding1;
    uniform float  padding2;

    uniform highp float4x4 textureTransform;

    uniform highp float4x4 rectTransform;

    uniform highp float customTextureScaleX;
    uniform highp float customTextureScaleY;
    uniform highp float reconvergenceZoomX;
    uniform highp float reconvergenceZoomY;

#if  SP_ACT == 1
    uniform float ACT_gamma;
    uniform float ACT_invGamma;
    uniform float ACT_singletap_coef;
    uniform float ACT_norm;
#endif

#if USE_HLSL == 1
};
#elif USE_GLSL_VULKAN == 1
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
    OutTexCoord = mul(gv(textureTransform), float4(InTex, 0.0, 1.0)).xy;

    // Scale of offset position of rectangle being rendered.
    OutScreenTexCoord = mul(gv(rectTransform), float4(InTex, 0.0, 1.0)).xy;

    // Scale and offset position.
    OutPos = float4((OutScreenTexCoord * 2.0) - 1.0, 0.0, 1.0);

#if UV_TOP_LEFT == 1
    OutTexCoord.y = 1.0 - OutTexCoord.y;
#endif

#if USE_HLSL == 1
    return output;
#endif
}