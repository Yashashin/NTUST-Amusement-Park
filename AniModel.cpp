#include "AniModel.h"

inline glm::mat4 assimpToGlmMatrix(aiMatrix4x4 mat) {
	glm::mat4 m;
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			m[x][y] = mat[y][x];
		}
	}
	return m;
}

inline glm::vec3 assimpToGlmVec3(aiVector3D vec) {
	return glm::vec3(vec.x, vec.y, vec.z);
}

inline glm::quat assimpToGlmQuat(aiQuaternion quat) {
	glm::quat q;
	q.x = quat.x;
	q.y = quat.y;
	q.z = quat.z;
	q.w = quat.w;
	


	return q;
}



void AniModel::loadModel(string modelPath,string texturePath)
{
	Assimp::Importer import;
	// The aiProcess_FlipUVs flips the texture coordinates on the y-axis where necessary during processing
	  ///
	   /// aiProcess_GenNormals: creates normal vectors for each vertex if the model doesn't contain normal vectors.
	   /// aiProcess_SplitLargeMeshes: splits large meshes into smaller sub - meshes which is useful if your rendering has a maximum number of vertices allowedand can only process smaller meshes.
	   /// aiProcess_OptimizeMeshes : does the reverse by trying to join several meshes into one larger mesh, reducing drawing calls for optimization.
	   ///

	const aiScene* scene = import.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}

	aiMesh* mesh = scene->mMeshes[0];

	
	//as the name suggests just inverse the global transform
	globalInverseTransform = assimpToGlmMatrix(scene->mRootNode->mTransformation);
	globalInverseTransform = glm::inverse(globalInverseTransform);

	this->processNode(scene, mesh, this->vertices, this->indices, this->skeleton, this->boneCount);
	this->loadAnimation(scene, this->animation);
	vao = createVertexArray(vertices, indices);
	diffuseTexture.id = createTexture(texturePath);
	diffuseTexture.path = texturePath;
	cout << mesh->mMaterialIndex << endl;
	glm::mat4 identity = glm::mat4(1.0);
	currentPose.resize(boneCount, identity);
}

void AniModel::processNode(const aiScene* scene, aiMesh* mesh, std::vector<Ani::Vertex>& verticesOutput, std::vector<uint>& indicesOutput, Bone& skeletonOutput, uint& nBoneCount)
{
	verticesOutput = {};
	indicesOutput = {};
	//load position, normal, uv
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		//process position 
		Ani::Vertex vertex;
		glm::vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.position = vector;
		//process normal
		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.normal = vector;
		//process uv
		glm::vec2 vec;
		vec.x = mesh->mTextureCoords[0][i].x;
		vec.y = mesh->mTextureCoords[0][i].y;
		vertex.uv = vec;

		vertex.boneIds = glm::ivec4(0);
		vertex.boneWeights = glm::vec4(0.0f);

		verticesOutput.push_back(vertex);
	}

	//load boneData to vertices
	std::unordered_map<std::string, std::pair<int, glm::mat4>> boneInfo = {};
	std::vector<uint> boneCounts;
	boneCounts.resize(verticesOutput.size(), 0);
	nBoneCount = mesh->mNumBones;

	//loop through each bone
	for (uint i = 0; i < nBoneCount; i++) {
		aiBone* bone = mesh->mBones[i];
		glm::mat4 m = assimpToGlmMatrix(bone->mOffsetMatrix);
		boneInfo[bone->mName.C_Str()] = { i, m };

		//loop through each vertex that have that bone
		for (int j = 0; j < bone->mNumWeights; j++) {
			uint id = bone->mWeights[j].mVertexId;
			float weight = bone->mWeights[j].mWeight;
			boneCounts[id]++;
			switch (boneCounts[id]) {
			case 1:
				verticesOutput[id].boneIds.x = i;
				verticesOutput[id].boneWeights.x = weight;
				break;
			case 2:
				verticesOutput[id].boneIds.y = i;
				verticesOutput[id].boneWeights.y = weight;
				break;
			case 3:
				verticesOutput[id].boneIds.z = i;
				verticesOutput[id].boneWeights.z = weight;
				break;
			case 4:
				verticesOutput[id].boneIds.w = i;
				verticesOutput[id].boneWeights.w = weight;
				break;
			default:
				//std::cout << "err: unable to allocate bone to vertex" << std::endl;
				break;

			}
		}
	}



	//normalize weights to make all weights sum 1
	for (int i = 0; i < verticesOutput.size(); i++) {
		glm::vec4& boneWeights = verticesOutput[i].boneWeights;
		float totalWeight = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
		if (totalWeight > 0.0f) {
			verticesOutput[i].boneWeights = glm::vec4(
				boneWeights.x / totalWeight,
				boneWeights.y / totalWeight,
				boneWeights.z / totalWeight,
				boneWeights.w / totalWeight
			);
		}
	}


	//load indices
	for (int i = 0; i < mesh->mNumFaces; i++) {
		aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indicesOutput.push_back(face.mIndices[j]);
	}

	// create bone hirerchy
	readSkeleton(skeletonOutput, scene->mRootNode, boneInfo);
}

bool AniModel::readSkeleton(Bone& boneOutput, aiNode* node, std::unordered_map<std::string, std::pair<int, glm::mat4>>& boneInfoTable)
{
	if (boneInfoTable.find(node->mName.C_Str()) != boneInfoTable.end()) { // if node is actually a bone
		boneOutput.name = node->mName.C_Str();
		boneOutput.id = boneInfoTable[boneOutput.name].first;
		boneOutput.offset = boneInfoTable[boneOutput.name].second;

		for (int i = 0; i < node->mNumChildren; i++) {
			Bone child;
			readSkeleton(child, node->mChildren[i], boneInfoTable);
			boneOutput.children.push_back(child);
		}
		return true;
	}
	else { // find bones in children
		for (int i = 0; i < node->mNumChildren; i++) {
			if ((this->readSkeleton(boneOutput, node->mChildren[i], boneInfoTable))) {
				return true;
			}

		}
	}
	return false;
}

void AniModel::loadAnimation(const aiScene* scene, Animation& animation)
{
	//loading  first Animation
	aiAnimation* anim = scene->mAnimations[0];

	if (anim->mTicksPerSecond != 0.0f)
		animation.ticksPerSecond = anim->mTicksPerSecond;
	else
		animation.ticksPerSecond = 1;


	animation.duration = anim->mDuration * anim->mTicksPerSecond;
	animation.boneTransforms = {};

	//load positions rotations and scales for each bone
	// each channel represents each bone
	for (int i = 0; i < anim->mNumChannels; i++) {
		aiNodeAnim* channel = anim->mChannels[i];
		BoneTransformTrack track;
		for (int j = 0; j < channel->mNumPositionKeys; j++) {
			track.positionTimestamps.push_back(channel->mPositionKeys[j].mTime);
			track.positions.push_back(assimpToGlmVec3(channel->mPositionKeys[j].mValue));
		}
		for (int j = 0; j < channel->mNumRotationKeys; j++) {
			track.rotationTimestamps.push_back(channel->mRotationKeys[j].mTime);
			track.rotations.push_back(assimpToGlmQuat(channel->mRotationKeys[j].mValue));

		}
		for (int j = 0; j < channel->mNumScalingKeys; j++) {
			track.scaleTimestamps.push_back(channel->mScalingKeys[j].mTime);
			track.scales.push_back(assimpToGlmVec3(channel->mScalingKeys[j].mValue));

		}
		animation.boneTransforms[channel->mNodeName.C_Str()] = track;
	}
}

uint AniModel::createVertexArray(std::vector<Ani::Vertex>& vertices, std::vector<uint> indices)
{
	uint
		vao = 0,
		vbo = 0,
		ebo = 0;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Ani::Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Ani::Vertex), (GLvoid*)offsetof(Ani::Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Ani::Vertex), (GLvoid*)offsetof(Ani::Vertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Ani::Vertex), (GLvoid*)offsetof(Ani::Vertex, uv));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Ani::Vertex), (GLvoid*)offsetof(Ani::Vertex, boneIds));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Ani::Vertex), (GLvoid*)offsetof(Ani::Vertex, boneWeights));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint), &indices[0], GL_STATIC_DRAW);
	glBindVertexArray(0);
	return vao;
}

uint AniModel::createTexture(string path)
{
	int width, height, nrChannels;

	Ani::Texture texture;
	cv::Mat img;
	//cout << "The loading path is " << str.C_Str() << " " << directory << endl;
	//cv::imread(path, cv::IMREAD_COLOR).convertTo(img, CV_32FC3, 1 / 255.0f);	//unsigned char to float
	img = cv::imread(path, cv::IMREAD_COLOR);
	//cv::cvtColor(img, img, CV_BGR2RGB);
	
	if (img.data) {
		glGenTextures(1, &texture.id);

		glBindTexture(GL_TEXTURE_2D, texture.id);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (img.type() == CV_8UC3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
		else if (img.type() == CV_8UC4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.cols, img.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, img.data);
		glBindTexture(GL_TEXTURE_2D, 0);

		img.release();




		texture.path = path;
		textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
	}
	else {
		cout << "this "+path+" has problem"<< endl;
		cout << "Error!!!!! img loading bad" << endl;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture.id;
}

std::pair<uint, float> AniModel::getTimeFraction(std::vector<float>& times, float& dt)
{
	uint segment = 0;
	while (dt > times[segment])
		segment++;
	float start = times[segment - 1];
	float end = times[segment];
	float frac = (dt - start) / (end - start);
	return { segment, frac };
}

void AniModel::getPose(Animation& animation, Bone& skeletion, float dt, std::vector<glm::mat4>& output, glm::mat4& parentTransform, glm::mat4& globalInverseTransform)
{
	BoneTransformTrack& btt = animation.boneTransforms[skeletion.name];
	dt = fmod(dt, animation.duration);
	std::pair<uint, float> fp;
	//calculate interpolated position
	if (btt.positionTimestamps.size() >= 1) {
		fp = getTimeFraction(btt.positionTimestamps, dt);

		glm::vec3 position1 = btt.positions[fp.first - 1];
		glm::vec3 position2 = btt.positions[fp.first];

		glm::vec3 position = glm::mix(position1, position2, fp.second);

		//calculate interpolated rotation
		fp = getTimeFraction(btt.rotationTimestamps, dt);
		glm::quat rotation1 = btt.rotations[fp.first - 1];
		glm::quat rotation2 = btt.rotations[fp.first];

		glm::quat rotation = glm::slerp(rotation1, rotation2, fp.second);

		//calculate interpolated scale
		fp = getTimeFraction(btt.scaleTimestamps, dt);
		glm::vec3 scale1 = btt.scales[fp.first - 1];
		glm::vec3 scale2 = btt.scales[fp.first];

		glm::vec3 scale = glm::mix(scale1, scale2, fp.second);

		glm::mat4 positionMat = glm::mat4(1.0),
			scaleMat = glm::mat4(1.0);


		// calculate localTransform
		positionMat = glm::translate(positionMat, position);
		glm::mat4 rotationMat = glm::toMat4(rotation);
		scaleMat = glm::scale(scaleMat, scale);
		glm::mat4 localTransform = positionMat * rotationMat * scaleMat;
		glm::mat4 globalTransform = parentTransform * localTransform;

		output[skeletion.id] = globalInverseTransform * globalTransform * skeletion.offset;
		//update values for children bones
		for (Bone& child : skeletion.children) {
			getPose(animation, child, dt, output, globalTransform, globalInverseTransform);
		}
	}
	
	//std::cout << dt << " => " << position.x << ":" << position.y << ":" << position.z << ":" << std::endl;
}


void AniModel::Draw(Shader& shader,float elapsedTime) {
	this->getPose(this->animation, this->skeleton, elapsedTime, this->currentPose,(this->identity),(this->globalInverseTransform));

	//assume one texture, one mesh

	//go bind your shader yourself by lo wen yi

	if (currentPose.size() == 0) {
		currentPose.push_back(glm::mat4());
	}
	glBindVertexArray(vao);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseTexture.id);
	//glUniform1i(, 0);
	glUniform1i(glGetUniformLocation(shader.Program, "diff_texture"), 0);
	glUniformMatrix4fv(glGetUniformLocation(shader.Program,"bone_transforms"), boneCount, GL_FALSE, &(currentPose[0])[0][0]);

	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);


}