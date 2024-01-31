#pragma once

#pragma once
#include <Fl/Fl_Gl_Window.h>

#include <AL/alut.h>

#include <iostream>
#include <Fl/fl.h>
#include <windows.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glu.h>


#include <opencv2\opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include "src/RenderUtilities/Shader.h"

#include <string>
#include <vector>
#include <unordered_map>
using namespace std;


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
namespace Ani{
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec4 boneIds = glm::vec4(0);
        glm::vec4 boneWeights = glm::vec4(0.0f);
    };

    struct Texture {
        unsigned int id;
        string type;
        string path;  // we store the path of the texture to compare with other textures
    };
}


struct BoneTransformTrack {
    std::vector<float> positionTimestamps = {};
    std::vector<float> rotationTimestamps = {};
    std::vector<float> scaleTimestamps = {};

    std::vector<glm::vec3> positions = {};
    std::vector<glm::quat> rotations = {};
    std::vector<glm::vec3> scales = {};
};

struct Animation {
    float duration = 0.0f;
    float ticksPerSecond = 1.0f;
    std::unordered_map<std::string, BoneTransformTrack> boneTransforms;
};



struct Bone {
    int id = 0; // position of the bone in final upload array
    std::string name = "";
    glm::mat4 offset = glm::mat4(1.0f);
    std::vector<Bone> children = {};
};



class AniModel
{
public:
    AniModel(char* modelPath,char* texturePath)
    {
       
        loadModel(string(modelPath),string(texturePath));
    }
    void Draw(Shader& shader, float elapsedTime);


    void addTexture(char* path);
    void addTexture(string path);

private:

    void loadModel(string modelPath, string texturePath);
    void processNode(const aiScene* scene, aiMesh* mesh, std::vector<Ani::Vertex>& verticesOutput, std::vector<uint>& indicesOutput, Bone& skeletonOutput, uint& nBoneCount);
    bool readSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable);
    void loadAnimation(const aiScene* scene, Animation& animation);
    uint createVertexArray(std::vector<Ani::Vertex>& vertices, std::vector<uint> indices);
    uint createTexture(string path);
    std::pair<uint, float> getTimeFraction(std::vector<float>& times, float& dt);
    void getPose(Animation& animation, Bone& skeletion, float dt, std::vector<glm::mat4>& output, glm::mat4& parentTransform, glm::mat4& globalInverseTransform);





    std::vector<Ani::Vertex> vertices = {};
    std::vector<uint> indices = {};
    std::vector<glm::mat4> currentPose = {};
    uint boneCount = 0;
    Animation animation;
    uint vao = 0;
    Bone skeleton;
    Ani::Texture diffuseTexture;
    glm::mat4 identity;
    glm::mat4 globalInverseTransform;

    vector<Ani::Texture> textures_loaded;
};