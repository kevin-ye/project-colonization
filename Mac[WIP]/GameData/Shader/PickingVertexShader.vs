#version 330

// Model-Space coordinates
in vec3 position;

uniform mat4 ModelView;
uniform mat4 Perspective;

void main() {
	gl_Position = Perspective * ModelView * vec4(position, 1.0);
}
