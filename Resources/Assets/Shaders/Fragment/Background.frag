out vec4 FragColor;

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec3 Tangent;
	vec2 TexCoords;
} fs_in;

void main()
{
	FragColor = texture(TEXTURE_DIFFUSE0, fs_in.TexCoords).rgba;
}