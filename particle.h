#pragma once
/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef PARTICLE
#define PARTICLE
#include <vector>
#include <utility>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <ctime>

#include "src/RenderUtilities/Shader.h"

const float GRAVITY = -9.8;

class Particle {
public:
	glm::vec3 position;// particle in world
	glm::vec3 velocity;//particle's speed direction //length of the vector show speed
	float gravity_effect;
	float lifetime;
	float rotate;
	float scale;
	float elapsedTime = 0.0f;
	glm::vec3 col;
	int type = 0;

	typedef glm::vec3 vec3;
	Particle(vec3 pos,vec3 vel,float gravity,float lifeLen,float r,float s) {
		this->position = pos;
		this->velocity = vel;
		this->gravity_effect = gravity;
		this->lifetime = lifeLen;
		this->rotate = r;
		this->scale = s;
		this->createVAO(std::vector<GLfloat>());
		this->col = vec3(0.2, 0.2, 0.2);
	}
	Particle(vec3 pos, vec3 vel, float gravity, float lifeLen, float r, float s,std::vector<GLfloat>& vector) {
		this->position = pos;
		this->velocity = vel;
		this->gravity_effect = gravity;
		this->lifetime = lifeLen;
		this->rotate = r;
		this->scale = s;
		this->createVAO(vector);
	}


	bool update() //to indicate the particle should stay alive or not
	{
		//if live longer enough the particle should be removed
		t1 = !t2?GetTickCount64()-1:t2;
		t2 = GetTickCount64();
		float frameTime = (t2 - t1) * 0.01;
		if (frameTime >= 100)
		{
			frameTime /= frameTime;
			frameTime *= 0.1;
		}
		else if (frameTime == 0)
			frameTime = 0.1;
		this->velocity.y += GRAVITY * this->gravity_effect * frameTime;
		vec3 change = this->velocity;
		change = change * frameTime;
		this->position = change + this->position;
		this->elapsedTime += frameTime;
		return this->elapsedTime < this->lifetime;
	}

	void createVAO(std::vector<GLfloat> vertices) 
	{
		glGenVertexArrays(1, &this->vao);
		glGenBuffers(1, this->vbo);

		glBindVertexArray(this->vao);


		// Position attribute
		if (vertices.size() >= 1) {
			glBindBuffer(GL_ARRAY_BUFFER, this->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);
			this->vertices_size = vertices.size();
		}
		else {
			float default_vertexs[] = 
			{//eight point no clue
				-0.5f,0.5f,-0.5f,
				-0.5f,0.5f,0.5f,
				0.5f,-0.5f
			};


			GLfloat  vertices_p4[] = {
				-0.5f ,0.0f , -0.5f,
				-0.5f ,0.0f , 0.5f ,
				0.5f ,0.0f ,0.5f ,
				0.5f ,0.0f ,-0.5f };

			GLuint element[] = {
				0, 1, 2,
				0, 2, 3, };
			glBindBuffer(GL_ARRAY_BUFFER, this->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(default_vertexs), default_vertexs, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (GLvoid*)0);
			glEnableVertexAttribArray(0);
			this->vertices_size = sizeof(default_vertexs) / sizeof(float);


			//Element attribute
			/*glBindBuffer(GL_ARRAY_BUFFER, this->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_p4), vertices_p4, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);
			this->vertices_size = sizeof(vertices_p4) / sizeof(GLfloat);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(element), element, GL_STATIC_DRAW);*/
		}

		// Unbind VAO
		glBindVertexArray(0);
	}
	


	void bindVAOAndDrawElement() {
		//bind VAO
		glBindVertexArray(this->vao);
		//glDrawElements(GL_TRIANGLES, this->vertices_size, GL_UNSIGNED_INT, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, this->vertices_size);
	}


	void prepare() {
		//¶Ã¶}¤@³q
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC0_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
		glDisable(GL_DEPTH);
	}
	

	void draw(Shader& shader)
	{
		//glm::mat4 view_matrix,model_matrix;
		//glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
		this->prepare();
		//this->updateModelViewMatrix(this->position, this->rotate, this->scale, view_matrix,model_matrix
		//we need to bind to shader first
		this->bindVAOAndDrawElement();
		this->exit();

	}

	void exit() {
		glEnable(GL_DEPTH);
		glDisable(GL_BLEND);
		// Unbind VAO
		glBindVertexArray(0);
		//unbind shader(switch to fixed pipeline)
		glUseProgram(0);
	}
	
	
	void setType(int v) {
		this->type = v;
	}
	void setColor(vec3& v) {
		this->col = v;
	}

private:
	float t1 = 0.0f;
	float t2 = 0.0f;

	GLuint vao;
	GLuint vbo[3];
	GLuint vertices_size;
	GLuint ebo;
};




#endif