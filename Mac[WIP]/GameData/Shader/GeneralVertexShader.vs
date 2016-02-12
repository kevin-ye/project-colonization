#version 330

// Model-Space coordinates
in vec3 position;
in vec3 normal;
in vec2 vertexUV;

struct LightSource {
    vec3 position;
    vec3 rgbIntensity;
};
uniform LightSource light;

uniform mat4 ModelView;
uniform mat4 Perspective;
uniform mat4 DepthMVP;


// Remember, this is transpose(inverse(ModelView)).  Normals should be
// transformed using this matrix instead of the ModelView matrix.
uniform mat3 NormalMatrix;

out VsOutFsIn {
	vec3 surfaceNormal;
	vec3 LightSourcePos;
	vec3 position_ES; // Eye-space position
	vec3 normal_ES;   // Eye-space normal
	vec3 modelPos;
	LightSource light;
} vs_out;

out vec2 uv;
out vec4 shadowUV;

void main() {
	vec4 pos4 = vec4(position, 1.0);
	vs_out.surfaceNormal = normal;

	shadowUV = (DepthMVP * pos4);

	vs_out.light = light;
	vs_out.modelPos = position;
	vs_out.LightSourcePos = vs_out.light.position;
	//vs_out.light.position = (ModelView * vec4(light.position, 1.0)).xyz;
	vs_out.position_ES = (ModelView * pos4).xyz;
	vs_out.normal_ES = normalize(NormalMatrix * normal);

	gl_Position = Perspective * ModelView * vec4(position, 1.0);

	uv = vertexUV;
}
