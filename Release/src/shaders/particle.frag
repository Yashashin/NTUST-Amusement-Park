#version 330 core

uniform vec3 givenColor;
out vec4 FragColor;



void main(){

    FragColor = vec4(givenColor+vec3(0.5,0.5,0.5),1.0f);
    
}