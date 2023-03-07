layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;
layout (location = 2) in vec2 aTexCoord;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 worldViewProj;
} ubo;
    
layout (location = 0) out vec2 TexCoord;
layout (location = 1) out vec4 Color;
layout (location = 2) flat out int TexIdx;

void main()
{
    gl_Position =  ubo.worldViewProj * vec4((aPos), 1.0f);
    TexIdx = int(aCol.r);
    TexCoord = aTexCoord;
    Color = vec4(aCol, 1.0f);
}