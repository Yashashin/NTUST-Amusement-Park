#version 440 core
	in vec2 tex_cord;
	in vec3 v_normal;
	in vec3 v_pos;
	in vec4 bw;
	out vec4 FragColor;
	uniform sampler2D diff_texture;
	vec3 lightPos = vec3(0.2, 1.0, -3.0);
	
    
	struct DirLight{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

	uniform vec3 viewPos;
	uniform DirLight dirLight;

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir);

	void main()
	{
	vec3 norm = normalize(v_normal);
    vec3 viewDir = normalize(viewPos - v_pos);
    vec3 color = vec3(texture(diff_texture,tex_cord));
    vec3 dirlight=CalcDirLight(dirLight, norm, viewDir);
 
    FragColor = vec4(color,0.5)+vec4(dirlight,0.5);
	}

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir)
{
    vec3 lightDir=normalize(-light.direction);
    float diff=max(dot(normal,lightDir),0.0);
    vec3 reflectDir=reflect(-lightDir,normal);
    float spec=pow(max(dot(viewDir,reflectDir),0.0),10);
    vec3 ambient = light.ambient*vec3(0.5,0.5,0.5);
    vec3 diffuse=light.diffuse*diff*vec3(0.5,0.5,0.5);
    vec3 specular=light.specular*spec*vec3(1,1,1);
    return (ambient+diffuse+specular);
}
