#pragma once

#include "Mesh.h"
#include <opencv2\opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include "RenderUtilities/Shader.h"
#include "RenderUtilities//Texture.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
using namespace std;

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model
{
public:
    Model(string path)
    {
        loadModel(path);
    }
    void Draw(Shader* shader);


    void addTexture(char* path);
    void addTexture(string path);
    vector<Mesh> meshes;
private:
    // model data
   
    string directory;

    void loadModel(string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        string typeName);


    vector<Texture> textures_loaded;
};