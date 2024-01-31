#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
}v_out;

uniform sampler2D heightMap;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    float amplitude=1;
    vec3 pos=aPos; 
    pos.y=pos.y+(texture(heightMap,aTexCoords).r)*amplitude;
    v_out.position = vec3(model * vec4(pos, 1.0f));
    v_out.normal = mat3(transpose(inverse(model))) * aNormal;
    v_out.texture_coordinate = aTexCoords;  
    gl_Position = projection * view * model * vec4(pos, 1.0);
}