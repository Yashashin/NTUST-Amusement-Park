#pragma once
#include "particle.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <ctime>

#include "src/RenderUtilities/Shader.h"

#include <cmath>
#include <vector>
using namespace std;

class ParticleSystem {
public:
	vector<Particle> particles;
	ParticleSystem() {
		this->particles = {};
	}
	void bindShaderProjectionMatrix(Shader& shader) {
		glm::mat4 project_matrix;
		glGetFloatv(GL_PROJECTION_MATRIX, &project_matrix[0][0]);
		glUniformMatrix4fv(
			glGetUniformLocation(shader.Program, "projection"), 1, GL_FALSE, &project_matrix[0][0]);
	}



	typedef glm::vec3 vec3;
	void updateModelViewMatrix(vec3 pos, float r, float s, glm::mat4& viewMatrix, glm::mat4& modelMatrix) {
		//set it like the billboard
		glm::mat4 model_matrix = glm::mat4();
		model_matrix = glm::translate(model_matrix, pos);

		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				model_matrix[i][j] = viewMatrix[j][i];
			}
		}
		model_matrix = glm::rotate(model_matrix, r, vec3(0, 0, 1));
		model_matrix = glm::scale(model_matrix, vec3(s, s, s));


		glm::mat4 tmp = viewMatrix * model_matrix;
		modelMatrix = model_matrix;
	}


	vector<Particle> explosion_1(Particle& currentPar)
	{
		vector<Particle> toAdd;

		vec3 randDir;
		for (int i = 0; i < 50; ++i) {
			/*randDir.x = 0.02f - float(rand() % 41) / 1000.0f;
			randDir.y = 0.02f - float(rand() % 41) / 1000.0f;
			randDir.z = 0.02f - float(rand() % 41) / 1000.0f;*/
			randDir.x = rand() % 21 - 10;
			randDir.y = rand() % 21 - 10;
			randDir.z = rand() % 21 - 10;

			toAdd.push_back(Particle(currentPar.position, currentPar.velocity + randDir, 0.6, 4, currentPar.rotate, currentPar.scale * 0.5));
			vec3 c;
			c.r = rand() % 257 / 256.0;
			c.g = rand() % 257 / 256.0;
			c.b = rand() % 257 / 256.0;
			toAdd[i].col = c;
		}
		return toAdd;
	}
	vector<Particle> explosion_2(Particle& currentPar)
	{
		vector<Particle> toAdd;
		float r1 = 2.0f;
		float offset = 2.0f;
		const float PI = 3.14159265358f;
		vec3 randDir;
		vec3 c;
		for (int layer = 1; layer <= 4; ++layer) {
			for (int i = 0; i < 50; ++i) {
				/*randDir.x = 0.02f - float(rand() % 41) / 1000.0f;
				randDir.y = 0.02f - float(rand() % 41) / 1000.0f;
				randDir.z = 0.02f - float(rand() % 41) / 1000.0f;*/
				randDir.x = (r1 + (layer - 1) * offset) * cos(i * 2 * PI / 50.0);
				randDir.y = (r1 + (layer - 1) * offset) * sin(i * 2 * PI / 50.0);
				randDir.z = 0.0;
				toAdd.push_back(Particle(currentPar.position, currentPar.velocity + randDir, 0.1, 8, currentPar.rotate, currentPar.scale * 0.5));

				c.r = rand() % 257 / 256.0;
				c.g = rand() % 257 / 256.0;
				c.b = rand() % 257 / 256.0;
				toAdd[i + (layer - 1) * 50].col = c;
			}
		}
		toAdd.push_back(Particle(currentPar.position, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale * 0.5));
		toAdd[toAdd.size() - 1].col = glm::vec3(1, 1, 1);
		return toAdd;
	}
	vector<Particle> explosionHELLO(Particle& currentPar) {
		vector<Particle> toAdd;
		float r1 = 2.0f;
		float offset = 2.0f;
		const float PI = 3.14159265358f;
		vec3 Dir(0, 0, 0);
		vec3 c(240.0 / 256.0, 250.0 / 256.0, -0.2);
		//Postion
		//H
		for (int i = 0; i < 10; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			Dir.y += 1 * currentPar.scale;
		}
		Dir.y = 4 * currentPar.scale;
		for (int i = 0; i < 5; ++i) {
			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			Dir.x += 1 * currentPar.scale;
		}
		Dir.y = 0;
		for (int i = 0; i < 10; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			Dir.y += 1 * currentPar.scale;
		}
		//Current string H 
		//set the position
		//E
		Dir.x += 2 * currentPar.scale;
		float currentX = Dir.x;
		Dir.y = 0;

		for (int i = 0; i < 10; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			toAdd[i].setColor(c);
			Dir.y += 1 * currentPar.scale;
		}
		//down position
		//e bottom
		Dir.y = 0;

		for (int j = 0; j < 3; ++j) {
			Dir.x = currentX;
			if (j != 2)
				Dir.y = j * 4 * currentPar.scale;
			else
				Dir.y = 9 * currentPar.scale;
			for (int i = 0; i < 5; ++i) {

				toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
				toAdd[i].setColor(c);
				Dir.x += 1 * currentPar.scale;
			}
		}
		//Current string HE
		//set position
		Dir.x = currentX + (4 + 2) * currentPar.scale;
		currentX = Dir.x;
		//-------Making L
		Dir.y = 0;

		for (int i = 0; i < 10; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			toAdd[i].setColor(c);
			Dir.y += 1 * currentPar.scale;
		}
		Dir.y = 0;
		for (int i = 0; i < 5; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			toAdd[i].setColor(c);
			Dir.x += 1 * currentPar.scale;
		}
		//-------------End 

		currentX = Dir.x + 1 * currentPar.scale;
		Dir.x = currentX;
		//-------Making L
		Dir.y = 0;

		for (int i = 0; i < 10; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			toAdd[i].setColor(c);
			Dir.y += 1 * currentPar.scale;
		}
		Dir.y = 0;
		for (int i = 0; i < 5; ++i) {

			toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
			toAdd[i].setColor(c);
			Dir.x += 1 * currentPar.scale;
		}
		//-------------End 


		//Current string HELL
		//-------Making O
		//setting offsetX
		currentX = Dir.x + 1 * currentPar.scale;
		Dir.x = currentX;
		Dir.y = 0;
		for (int j = 0; j < 2; ++j) {
			if (j == 1) {
				Dir.y = 9 * currentPar.scale;
			}
			for (int i = 1; i <= 3; ++i)
			{
				Dir.x += 1 * currentPar.scale;
				toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
				toAdd[i].setColor(c);
			}
			Dir.x = currentX;
		}

		Dir.x = currentX;

		for (int j = 0; j < 2; ++j) {
			Dir.y = 0;
			for (int i = 1; i <= 8; ++i) {
				Dir.y += 1 * currentPar.scale;
				toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
				toAdd[i].setColor(c);
			}
			Dir.x = currentX + 4 * currentPar.scale;
		}
		//HELLO
		//make !
		Dir.x += 4 * currentPar.scale;
		Dir.y = 0;

		for (int i = 0; i < 10; ++i) {
			if (i == 1 || i == 2) {
				Dir.y += currentPar.scale;
				continue;
			}

			else
			{
				toAdd.push_back(Particle(currentPar.position + Dir, currentPar.velocity, 0.1, 8, currentPar.rotate, currentPar.scale));
				toAdd[i].setColor(c);
				Dir.y += currentPar.scale;
			}
		}

		for (int i = 0; i < toAdd.size(); ++i) {
			toAdd[i].col = c;
		}
		//-----End
		return toAdd;
	}



	void explosionControl(Particle& currentPar) {
		vector<Particle> tmp;
		switch (currentPar.type) {
		case 1:
			tmp = this->explosion_1(currentPar);
			this->particles.insert(this->particles.end(), tmp.begin(), tmp.end());
			break;
		case 2:
			tmp = this->explosion_2(currentPar);
			this->particles.insert(this->particles.end(), tmp.begin(), tmp.end());
			break;
		case 3:
			tmp = this->explosionHELLO(currentPar);
			this->particles.insert(this->particles.end(), tmp.begin(), tmp.end());
		default://do nothing
			break;
		}
	}


	void update() {
		vector<Particle>::iterator it = this->particles.begin();

		for (int i = 0; i < this->particles.size(); ++i) {
			bool stillAlive = this->particles[i].update();
			if (!stillAlive) {
				if (this->particles[i].type >= 1) {
					this->explosionControl(this->particles[i]);
				}
				this->particles.erase(this->particles.begin() + (i), this->particles.begin() + (i + 1));
				--i;
			}
		}
	}

	void renderParticles(Shader& shader) {

		glm::mat4 viewMatrix;
		glGetFloatv(GL_MODELVIEW_MATRIX, &viewMatrix[0][0]);
		glm::mat4 model_matrix;
		for (auto p : this->particles)
		{
			shader.Use();
			this->bindShaderProjectionMatrix(shader);
			this->updateModelViewMatrix(p.position, p.rotate, p.scale, viewMatrix, model_matrix);
			glUniformMatrix4fv(
				glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, &model_matrix[0][0]);
			glUniformMatrix4fv(
				glGetUniformLocation(shader.Program, "view"), 1, GL_FALSE, &viewMatrix[0][0]);
			glUniform3fv(
				glGetUniformLocation(shader.Program, "givenColor"), 1, &p.col[0]);
			p.draw(shader);
		}
	}

	void addParticle(Particle p) {
		this->particles.push_back(p);
	}

private:


};