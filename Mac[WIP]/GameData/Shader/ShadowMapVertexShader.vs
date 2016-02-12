#version 330

// Model-Space coordinates
in vec3 position;
// in vec3 normal;

uniform mat4 ModelView;
uniform mat4 Perspective;

void main()
{
	vec4 pos4 = vec4(position, 1.0);

	gl_Position = Perspective * ModelView * pos4;
}
