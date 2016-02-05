#version 330

in vec3 TexCoord;
in vec3 addedcolor;

out vec4 fragColor;

uniform samplerCube skyboxTexture;

void main()
{
    fragColor = texture(skyboxTexture, TexCoord) - vec4(addedcolor, 0);
} 