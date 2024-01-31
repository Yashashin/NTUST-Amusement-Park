#pragma once

#include <Fl/Fl_Gl_Window.h>

#include <AL/alut.h>

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glu.h>


#include "RenderUtilities/Shader.h"

#include <string>
#include <vector>
using namespace std;
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    string type;
    string path;  // we store the path of the texture to compare with other textures
};


class Mesh {
public:
    // mesh data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    Mesh();
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures);
    void Draw(Shader* shader);
    unsigned int VAO, VBO, EBO;
private:
    //  render data
   
    void setupMesh();
};