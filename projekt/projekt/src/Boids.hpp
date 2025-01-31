#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "Shader_Loader.h"
#include "Render_Utils.h"
//#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

#include <cstdlib>
#include <ctime> 
#include <vector>

int randomInt(int min, int max) {
	return min + rand() % (max - min + 1);
}


class Boid {
public:
	glm::vec3 position;
	glm::vec3 velocity;  // (szybkość + kierunek)
	int groupId;

	float maxSpeed = 0.04f;
	float maxForce = 0.2f;

	float NEIGHBOUR_DISTANCE = 2.f;
	float AVOID_RADIUS = 0.5f;

	float ALIGMENT_WEIGHT = 1.0f;
	float COHESION_WEIGHT = 0.001f;
	float SEPARATION_WEIGHT = 2.0f;

	float minX = -4.6f, maxX = 4.6f;
	float minY = 0.5f, maxY = 5.0f;
	float minZ = -1.6f, maxZ = 1.6f;


	Boid(glm::vec3 startPosition) : position(startPosition) {
		groupId = randomInt(0, 3);
		velocity = glm::vec3(0.01f);
	}

	glm::vec3 getGroupColor() {
		switch (groupId) {
		case 0: return glm::vec3(1.0f, 0.0f, 0.0f);  // grupa 1 --> czerwony
		case 1: return glm::vec3(0.0f, 1.0f, 0.0f);  // grupa 2 --> zielony
		case 2: return glm::vec3(0.0f, 0.0f, 1.0f);  // grupa 3 --> niebieski
		case 3: return glm::vec3(1.0f, 1.0f, 0.0f);  // grupa 4 --> żółty
		default: return glm::vec3(1.0f, 0.0f, 0.0f);
		}
	}



	void applyBoundaryForce() {
		glm::vec3 boundaryForce(0.0f);

		if (position.x > maxX) {
			boundaryForce.x = -maxForce;
		}
		else if (position.x < minX) {
			boundaryForce.x = maxForce;
		}

		if (position.y > maxY) {
			boundaryForce.y = -maxForce;
		}
		else if (position.y < minY) {
			boundaryForce.y = maxForce;
		}

		if (position.z > maxZ) {
			boundaryForce.z = -maxForce;
		}
		else if (position.z < minZ) {
			boundaryForce.z = maxForce;
		}
		velocity += boundaryForce;
	}

	void setSeparationWeight(float weight)
	{
		SEPARATION_WEIGHT = weight;
	}

	void setAlignmentWeight(float weight)
	{
		ALIGMENT_WEIGHT = weight;
	}

	void setCohesionWeight(float weight)
	{
		COHESION_WEIGHT = weight;
	}

	glm::vec3 separate(std::vector<Boid*>& boids) {
		glm::vec3 avoid_vector(0.0f);
		float distance;

		for (Boid* other_boid : boids) {
			if (other_boid->groupId == groupId) {
				distance = glm::distance(position, other_boid->position);
				if (distance < AVOID_RADIUS) {
					if (distance != 0) {
						avoid_vector += glm::normalize(position - other_boid->position) / distance;
					}
				}
			}
		}
		return avoid_vector;
	}

	glm::vec3 align(std::vector<Boid*>& boids) {

		glm::vec3 avg_boid_vector(0.0f);
		int count = 0;
		float distance;

		for (Boid* other_boid : boids) {
			if (other_boid->groupId == groupId) {
				distance = glm::distance(position, other_boid->position);
				if (distance <= NEIGHBOUR_DISTANCE) {
					avg_boid_vector += other_boid->velocity;
					count++;
				}
			}
		}
		if (count > 0) {
			avg_boid_vector = avg_boid_vector / count;
			avg_boid_vector = glm::normalize(avg_boid_vector) * maxSpeed;
		}
		return avg_boid_vector - velocity;
	}



	glm::vec3 cohesion(std::vector<Boid*>& boids) {
		glm::vec3  center_of_group(0.0f);
		int count = 0;
		float distance;
		for (Boid* other_boid : boids) {
			if (other_boid->groupId == groupId) {
				distance = glm::distance(position, other_boid->position);
				if (distance <= NEIGHBOUR_DISTANCE) {
					center_of_group += other_boid->position;
					count++;
				}
			}
		}

		if (count > 0) {
			center_of_group = center_of_group / count;
			return glm::normalize(center_of_group - position);
		}
		return glm::vec3(0.0f);
	}


	void update(std::vector<Boid*>& boids) {

		glm::vec3 _separation = separate(boids) * SEPARATION_WEIGHT;
		glm::vec3 _aligment = align(boids) * ALIGMENT_WEIGHT;
		glm::vec3 _cohesion = cohesion(boids) * COHESION_WEIGHT;

		glm::vec3 steering = _separation + _aligment + _cohesion;
		velocity += steering;

		if (glm::length(velocity) > maxSpeed) {
			velocity = glm::normalize(velocity) * maxSpeed;
		}
		applyBoundaryForce();
		position += velocity;
	}



	glm::mat4 getMatrix() {
		glm::vec3 direction = glm::normalize(velocity);
		glm::vec3 referenceVector = glm::vec3(0, 0, 1);

		glm::vec3 rotationAxis = glm::cross(referenceVector, direction);
		float angle = acos(glm::dot(referenceVector, direction));

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);

		glm::mat4 yRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 1, 0));

		glm::mat4 modelMatrix = glm::translate(position) * rotationMatrix * yRotationMatrix * glm::scale(glm::vec3(0.7f));

		return modelMatrix;
	}

};