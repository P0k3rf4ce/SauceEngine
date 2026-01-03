#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main()
{
	float lightDist = length(FragPos.xyz - lightPos);

	// map to [0,1] range
	lightDist /= farPlane;

	gl_FragDepth = lightDist;
}
