#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "Shader_Loader.h"
#include "Render_Utils.h"

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


struct Particle {
	glm::vec3 position;
	glm::vec3 velocity;
	float lifetime;
	float lifespan;
};

const int MAX_PARTICLES = 3;
Particle particles[MAX_PARTICLES];



class Boid {
public:
	glm::vec3 position;
	glm::vec3 velocity;  // (szybkość + kierunek)
	glm::vec3 targetPosition;
	int groupId;
	bool isShark;
	bool isLeader;
	int followGroup;
	int attackGroup;

	static const int PARTICLES_COUNT = 3;
	std::vector<Particle> particles;


	float maxSpeed = 0.05f;
	float maxForce = 0.2f;

	float NEIGHBOUR_DISTANCE = 2.f;
	float AVOID_RADIUS = 0.5f;

	float ALIGMENT_WEIGHT = 1.0f;
	float COHESION_WEIGHT = 0.001f;
	float SEPARATION_WEIGHT = 2.0f;
	float FLEE_WEIGHT = 3.0f;
	float FOLLOW_WEIGHT = 3.0f;

	float minX = -5.f, maxX = 7.0f;
	float minY = 1.f, maxY = 6.0f;
	float minZ = -1.6f, maxZ = 1.6f;


	Boid(glm::vec3 startPosition, bool isShark, int groupId, bool isLeader) : position(startPosition), isShark(isShark) , groupId(groupId), isLeader(isLeader){
		if (isShark == true)
		{
			attackGroup = randomInt(0, 1);
		}
		if (isLeader == true)
		{
			followGroup = randomInt(0,1);
		}
		velocity = glm::vec3(0.01f);
	}


	glm::vec3 getGroupColor() {
		switch (groupId) {
		case 0: return glm::vec3(1.0f, 1.0f, 0.0f);  // grupa 2 --> żółty
		case 1: return glm::vec3(1.0f, 0.0f, 0.0f);  // grupa 1 --> czerwony
		case 2: return glm::vec3(0.5f, 0.5f, 0.5f);  // grupa 2 --> szary --> SHARK
		case 3: return glm::vec3(0.0f, 1.0f, 1.0f); //leader
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

	void setFleeWeight(float weight)
	{
		FLEE_WEIGHT = weight;
	}

	void setFollowWeight(float weight)
	{
		FOLLOW_WEIGHT = weight;
	}

	bool getIsShark()
	{
		return isShark;
	}

	bool getIsLeader() {
		return isLeader;
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
			glm::vec3 direction = center_of_group - position;
			if (glm::length(direction) > 0.0f) {
				return glm::normalize(direction);
			}
		}
		return glm::vec3(0.0f);
	}


	glm::vec3 sharkAttack(std::vector<Boid*>& boids) {
		for (Boid* other_boid : boids) {
			glm::vec3 direction = other_boid->position - position;
			if (glm::length(direction) > 0.0f) {
				glm::vec3 attackVector = glm::normalize(direction); 
				return attackVector;
			}
		}
		return glm::vec3(0.0f);
	}


	glm::vec3 fleeFromShark(std::vector<Boid*>& boids) {
		glm::vec3 escapeDirection(0.0f);
		float maxFleeBoost = 5.0f;

		for (Boid* other_boid : boids) {
			if (other_boid->isShark) {
				float distance = glm::distance(position, other_boid->position);
				if (distance <= NEIGHBOUR_DISTANCE) {
					escapeDirection += velocity * maxFleeBoost;
				}
			}
		}
		return escapeDirection;
	}

	glm::vec3 leaderBehaviour()
	{
		glm::vec3 direction = targetPosition - position;
		if (glm::length(direction) < 0.5f)
		{
			targetPosition = glm::vec3(
				/*randomFloat(minX, maxX),
				randomFloat(minY, maxY),
				randomFloat(minZ, maxZ)*/
			);
		}
		glm::vec3 desiredVelocity = glm::normalize(direction) * maxSpeed;
		glm::vec3 steering = desiredVelocity - velocity;
		steering = glm::clamp(steering, glm::vec3(-maxForce), glm::vec3(maxForce));
		return steering;
	}

	glm::vec3 followTheLeader(std::vector<Boid*>& boids)
	{
		glm::vec3 followDirection(0.0f);
		glm::vec3 leaderPosition(0.0f);
		glm::vec3 leaderVelocity(0.0f);
		bool foundLeader=false;

		for (Boid* other_boid : boids)
		{
			if (other_boid->isLeader)
			{
				leaderPosition = other_boid->position;
				leaderVelocity = other_boid->velocity;
				foundLeader = true;
				break;

			}
		}
			if (foundLeader)
			{
				followDirection = leaderPosition - position;
				float distance = glm::length(followDirection);

				if (distance <= NEIGHBOUR_DISTANCE) {
					followDirection = glm::normalize(followDirection) * FOLLOW_WEIGHT;
				}

			}
		
		return followDirection;
	}


	void update(std::vector<Boid*>& boids) {

		if (isLeader) {
			glm::vec3 leaderForce = leaderBehaviour();
			velocity += leaderForce;
		}

		else if (isShark) {
			glm::vec3 _shark_Attack = sharkAttack(boids);
			velocity = _shark_Attack * maxSpeed;

			if (glm::length(velocity) > maxSpeed) {
				velocity = glm::normalize(velocity) * maxSpeed;
			}
		}

		else {
			glm::vec3 _separation = separate(boids) * SEPARATION_WEIGHT;
			glm::vec3 _aligment = align(boids) * ALIGMENT_WEIGHT;
			glm::vec3 _cohesion = cohesion(boids) * COHESION_WEIGHT;
			glm::vec3 _flee = fleeFromShark(boids) * FLEE_WEIGHT;
			glm::vec3 _follow = followTheLeader(boids) * FOLLOW_WEIGHT;

			glm::vec3 steering = _separation + _aligment + _cohesion;

			velocity += steering;
			if (glm::length(velocity) > maxSpeed) {
				velocity = glm::normalize(velocity) * maxSpeed;
			}

			velocity = velocity + _flee;
			if (glm::length(velocity) > maxSpeed+0.1f) {
				velocity = glm::normalize(velocity) * (maxSpeed+0.1f);
			}
		}

		position += velocity;
		applyBoundaryForce();
		
	}



	glm::mat4 getMatrix() {
		glm::vec3 direction = glm::normalize(velocity);
		glm::vec3 referenceVector = glm::vec3(0, 0, 1);

		glm::vec3 rotationAxis = glm::cross(referenceVector, direction);
		float angle = acos(glm::dot(referenceVector, direction));

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, rotationAxis);

		glm::mat4 yRotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 1, 0));
		//std::cout << "Pozycja boida: " << position.x << ", " << position.y << ", " << position.z << std::endl;

		glm::mat4 modelMatrix = glm::translate(position) * rotationMatrix * yRotationMatrix * glm::scale(glm::vec3(0.7f));

		return modelMatrix;
	}



	// particles

	void initParticles() {
		for (int i = 0; i < PARTICLES_COUNT; ++i) {
			Particle p;
			p.position = position;
			p.velocity = glm::vec3(
				(rand() % 100 - 50) / 500.0f,
				(rand() % 100 - 50) / 500.0f,
				(rand() % 100 - 50) / 500.0f
			);
			p.lifetime = 0.0f;
			p.lifespan = (rand() % 100) / 100.0f * 2.0f + 1.0f;
			particles.push_back(p);
		}
	}


	void updateParticles(float deltaTime) {
		for (auto& p : particles) {
			if (p.lifetime < p.lifespan) {
				p.position += p.velocity * deltaTime;
				p.lifetime += deltaTime;
			}
			else {
				p.position = position;
				p.velocity = glm::vec3(
					(rand() % 100 - 50) / 500.0f,
					(rand() % 100 - 50) / 500.0f,
					(rand() % 100 - 50) / 500.0f
				);
				p.lifetime = 0.0f;
				p.lifespan = (rand() % 100) / 100.0f * 2.0f + 0.5f;
			}
		}
	}

};