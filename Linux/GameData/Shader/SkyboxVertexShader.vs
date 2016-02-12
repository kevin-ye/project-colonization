#version 330

// Model-Space coordinates
in vec3 position;
// in vec3 normal;

uniform mat4 MVP;
uniform float colorratio;
out vec3 TexCoord;
out vec3 addedcolor;

void main()
{
	vec4 pos4 = vec4(position, 1.0);

	gl_Position = (MVP * pos4);

	TexCoord = position;

	addedcolor = vec3(1.0, 1.0, 1.0) * colorratio;
}
