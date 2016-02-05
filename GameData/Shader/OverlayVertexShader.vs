#version 330

in vec3 position;

uniform mat4 MVP;

uniform float width;
uniform float height;
uniform float buttom_left_x;
uniform float buttom_left_y;

out vec2 vertexUV;

void main() {
	vec4 pos4 = vec4(position, 1.0);

	gl_Position = MVP * vec4(position, 1.0);
	vertexUV = vec2((position.x - buttom_left_x)/width, (position.y - buttom_left_y)/height);
}
