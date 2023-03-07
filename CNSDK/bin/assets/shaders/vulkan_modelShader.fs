layout (location = 0) out vec4 FragColor;
  
layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec4 Color;
layout (location = 2) flat in int TexIdx;

layout(binding = 1) uniform sampler2D ourTexture[5];

void main()
{
    //vec4 outColor = texture(ourTexture[TexIdx], TexCoord); // Output texture
    vec4 outColor = Color;                                   // Output color

    FragColor = outColor;
}