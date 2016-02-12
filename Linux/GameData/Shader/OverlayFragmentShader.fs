#version 330

in vec2 vertexUV;

out vec4 fragColour;

uniform sampler2D TextureSampler;

void main() {
    vec3 texturecolor = texture(TextureSampler, vertexUV).rgb;
    if ((texturecolor.x == 0) && (texturecolor.y == 0) && (texturecolor.z == 0))
    {
        fragColour = vec4(texturecolor, 0.0);
    } else {
        fragColour = vec4(texturecolor, 0.7);
    }
    
}
