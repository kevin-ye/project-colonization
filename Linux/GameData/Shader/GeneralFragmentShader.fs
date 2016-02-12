#version 330

struct LightSource {
    vec3 position;
    vec3 rgbIntensity;
};

in VsOutFsIn {
    vec3 surfaceNormal;
    vec3 LightSourcePos;
    vec3 position_ES; // Eye-space position
    vec3 normal_ES;   // Eye-space normal
    vec3 modelPos;
    LightSource light;
} fs_in;

in vec2 uv;
in vec4 shadowUV;

out vec4 fragColour;

struct Material {
    vec3 ks;
    float shininess;
};
uniform Material material;

// Ambient light intensity for each RGB component.
uniform vec3 ambientIntensity;
uniform vec3 fogColor;

uniform sampler2D MeshTextureSampler;
uniform sampler2D ShadowMapTextureSampler;
uniform bool enableShadow;

float ShadowCalculation(vec4 suv)
{
    vec3 shadowCoord = suv.xyz / suv.w;
    shadowCoord = shadowCoord * 0.5 + 0.5; 
    float closestDepth = texture(ShadowMapTextureSampler, shadowCoord.xy).r;
    vec3 lightDir = fs_in.LightSourcePos - vec3(0, 0, 0);
    float bias = max(0.003 * (1.0 - dot(fs_in.surfaceNormal, lightDir)), 0.0003);  
    float currentDepth = shadowCoord.z; 
    float shadow = 0.0;

    // PCF
    vec2 texelSize = 1.0 / textureSize(ShadowMapTextureSampler, 0);
    for(int x = -2; x <= 2; ++x)
    {
        for(int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(ShadowMapTextureSampler, shadowCoord.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 16.0;

    // "sun light simulation bias"
    float biasRadius = distance(fs_in.LightSourcePos, vec3(0, 0, 0));
    float distanceToOrigin = distance(fs_in.modelPos, vec3(0, 0, 0));
    if (distanceToOrigin >= biasRadius)
    {
        shadow = 0.0;
    }

    if (!enableShadow)
    {
        shadow = 0.0;
    }

    return (1 - shadow);
}

vec3 phongModel() {
	LightSource light = fs_in.light;

    // Direction from fragment to light source.
    vec3 l = normalize(light.position - fs_in.position_ES);

    // Direction from fragment to viewer (origin - fragPosition).
    vec3 v = normalize(-fs_in.position_ES.xyz);

    float n_dot_l = max(dot(fs_in.normal_ES, l), 0.0);

	vec3 diffuse;
    diffuse = texture(MeshTextureSampler, uv).rgb * n_dot_l;

    vec3 specular = vec3(0.0);

    if (n_dot_l > 0.0) {
		// Halfway vector.
		vec3 h = normalize(v + l);
        float n_dot_h = max(dot(fs_in.surfaceNormal, h), 0.0);

        specular = material.ks * pow(n_dot_h, material.shininess);
    }

    return ambientIntensity + texture(MeshTextureSampler, uv).rgb * 0.05 + ShadowCalculation(shadowUV) * light.rgbIntensity * (diffuse + specular);
}

vec3 distanceFog(vec3 lightcolor)
{
    float distance = length(fs_in.modelPos);
    //float FogDensity = 0.003;
    // float fogFactor = 1.0 /exp(distance * FogDensity);
    // fogFactor = clamp( fogFactor, 0.0, 1.0 );
    float fogFactor = (300 - distance)/(300 - 140);
    fogFactor = clamp( fogFactor, 0.0, 1.0 );
 
    return mix(fogColor, lightcolor, fogFactor);
}

void main() {
    fragColour = vec4(distanceFog(phongModel()), 1.0);
}
