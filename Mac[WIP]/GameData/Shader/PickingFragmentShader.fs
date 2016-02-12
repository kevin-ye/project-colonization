#version 330

out vec4 fragColour;

uniform vec3 colorID;

void main() {
	fragColour = vec4(colorID, 1.0);
}
