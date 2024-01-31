#version 330 core
out vec4 FragColor;

struct DirLight{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in V_OUT
{
vec3 position;
vec3 normal;
vec2 texture_coordinate;
}f_in;

uniform sampler2D u_texture;
uniform vec3 viewPos;
uniform DirLight dirLight;

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir);

void main()
{    
    vec3 norm = normalize(f_in.normal);
    vec3 viewDir = normalize(viewPos - f_in.position);
    vec3 color = vec3(texture(u_texture,f_in.texture_coordinate));
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