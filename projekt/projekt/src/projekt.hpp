﻿#include "glew.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "Boids.hpp"

#include <cstdlib>
#include <ctime> 
#include <vector>

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;


//window:
int WIDTH = 500, HEIGHT = 500;
GLFWwindow* globalWindow;
bool fullScreen = false;
GLFWmonitor* monitor = nullptr;

//Terrain:

GLuint skyboxWater;

Terrain terrain;
unsigned int NUM_STRIPS;
unsigned int NUM_VERTS_PER_STRIP;

GLuint VAO, VBO;
float aspectRatio = 1.f;
float exposition = 1.f;

//models and textures:
namespace models {
	Core::RenderContext spaceshipContext;
	Core::RenderContext sphereContext;
	Core::RenderContext aquariumContext;
	Core::RenderContext goldfishContext;
	Core::RenderContext testContext;
	Core::RenderContext sharkContext;
	Core::RenderContext leaderContext;
	Core::RenderContext cubeContext;
}

namespace texture {
	GLuint ship;
	GLuint fish;
	GLuint leaderFish;
	GLuint brickwall;
	GLuint blueTest;
	GLuint shark;
	GLuint shipNormal;
	GLuint fishNormal;
	GLuint brickwall_normal;
	GLuint shark_normal;
}

Core::RenderContext shipContext;
Core::RenderContext sphereContext;

//shader programs:
GLuint programPhSh;
GLuint programTex;
GLuint programSkyBox;
GLuint programDisco;

Core::Shader_Loader shaderLoader;

//for camera:
glm::vec3 cameraPos = glm::vec3(0.479490f, 1.250000f, -2.124680f);
glm::vec3 cameraDir = glm::vec3(-0.354510f, 0.000000f, 0.935054f);
glm::vec3 spaceshipPos = glm::vec3(0.065808f, 1.250000f, -2.189549f);
glm::vec3 spaceshipDir = glm::vec3(-0.490263f, 0.000000f, 0.871578f);

//for mouse movement:
float yaw = -90.0f;
float pitch = 0.0f;
float lastMouseX = WIDTH/2.0;
float lastMouseY = HEIGHT/2.0;
bool firstMouse = true;
bool leftMousePressed = false;
bool rightMousePressed = false;


//lighting variables:
glm::vec3 sunPos = glm::vec3(-4.740971f, 2.149999f, 0.369280f);
glm::vec3 sunDir = glm::vec3(-0.93633f, 0.351106, 0.003226f);
glm::vec3 sunColor = glm::vec3(0.9f, 0.9f, 0.7f) * 0.5;

glm::vec3 pointlightPos = glm::vec3(0.f,5.f,-5.f);
glm::vec3 pointlightDir = glm::vec3(0., -0.3, 0.5);
glm::vec3 pointlightColor = glm::vec3(0.9, 0.6, 0.6);


// boids and obstacles
std::vector<Boid*> boids;
Obstacle obstacle;
glm::vec3 obstacleTransformation = glm::vec3(0.f, 7.f, 0.f);

float lastTime = -1.f;
float deltaTime = 0.f;

// global variables for imgui:
bool isMouseCaptured = false;
bool cursorDisabled = true;
bool iKeyPressedLastFrame = false;
bool useNormalMapping = true;
bool discoMode = false;
glm::vec3 obstacle_color = glm::vec3(0.0f, 0.0f, 1.0f);




void updateDeltaTime(float time) {
	if (lastTime < 0) {
		lastTime = time;
		return;
	}
	deltaTime = time - lastTime;
	if (deltaTime > 0.1) deltaTime = 0.1;
	lastTime = time;
}



glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);
	return cameraMatrix;
}


glm::mat4 createPerspectiveMatrix()
{
	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 30.;
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		1,0.,0.,0.,
		0.,aspectRatio,0.,0.,
		0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
		0.,0.,-1.,0.,
		});
	perspectiveMatrix = glm::transpose(perspectiveMatrix);
	return perspectiveMatrix;
}


glm::mat4 createLightViewProjection() {
	glm::mat4 view = glm::lookAt(pointlightDir, glm::vec3(0,0,0), glm::vec3(0, 1, 0));
	//jak ustawić w którą stronę patrzy światło... 
	//zeby zgadzalo sie z światłem kierunkowym

	glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.f, 50.f);
	return projection * view;
}

glm::vec3 rayPlaneIntersection(glm::vec3 rayOrigin, glm::vec3 rayDirection, float planeZ) {
	float t = (planeZ - rayOrigin.z) / rayDirection.z;
	if (t < 0.0f) return glm::vec3(0.0,0.0,0.0);

	return rayOrigin + t * rayDirection;
}

std::pair<glm::vec3, glm::vec3> screenToWorld(double mouseX, double mouseY, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, int screenWidth, int screenHeight)
{
	float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
	float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;

	glm::vec4 nearPlanePos = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
	glm::vec4 farPlanePos = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
	glm::mat4 inverseVP = glm::inverse(projectionMatrix * viewMatrix);

	glm::vec4 nearPlaneWorld = inverseVP * nearPlanePos;
	glm::vec4 farPlaneWorld = inverseVP * farPlanePos;
	nearPlaneWorld /= nearPlaneWorld.w;
	farPlaneWorld /= farPlaneWorld.w;

	glm::vec3 rayOrigin = glm::vec3(nearPlaneWorld);
	glm::vec3 rayDirection = glm::normalize(glm::vec3(farPlaneWorld - nearPlaneWorld));

	return { rayOrigin, rayDirection };
}


void applyTargetingForce(std::vector<Boid*>& boids, GLFWwindow* window, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, int screenWidth, int screenHeight) {

	if (!leftMousePressed) return; //callback changes this to true when you click on the screen

	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	//convert mouse click to world space:
	std::pair<glm::vec3, glm::vec3> result = screenToWorld(mouseX, mouseY, viewMatrix, projectionMatrix, screenWidth, screenHeight);
	glm::vec3 rayOrigin = result.first;
	glm::vec3 rayDirection = result.second;

	float targetDepth = 0.2f;
	glm::vec3 worldMousePos = rayPlaneIntersection(rayOrigin, rayDirection, targetDepth);
	if (worldMousePos == glm::vec3(0.0, 0.0, 0.0)) return;

	for (Boid* boid : boids)
	{
		glm::vec3 direction = worldMousePos - boid->position;
		float distance = glm::length(direction);

		if (distance > 0.01f)
		{
			direction = glm::normalize(direction);
			float forceStrength = 10.0f / (distance * 0.5f + 1.0f);
			boid->velocity += direction * forceStrength;
		}
	}

}

void applyRepulsionForce(std::vector<Boid*>& boids, GLFWwindow* window, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, int screenWidth, int screenHeight) {

	if (!rightMousePressed) return; //callback changes this to true when you click on the screen

	if (ImGui::GetIO().WantCaptureMouse)
	{
		return;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	//convert mouse click to world space:
	std::pair<glm::vec3, glm::vec3> result = screenToWorld(mouseX, mouseY, viewMatrix, projectionMatrix, screenWidth, screenHeight);
	glm::vec3 rayOrigin = result.first;
	glm::vec3 rayDirection = result.second;

	float targetDepth = 0.2f;
	glm::vec3 worldMousePos = rayPlaneIntersection(rayOrigin, rayDirection, targetDepth);
	if (worldMousePos == glm::vec3(0.0, 0.0, 0.0)) return;


	for (Boid* boid : boids)
	{
		glm::vec3 fleeDirection = boid->position - worldMousePos;
		float distance = glm::length(fleeDirection);

		if (distance > 0.01f)
		{
			fleeDirection = glm::normalize(fleeDirection);
			float forceStrength = 5.0f / (distance * 0.5f + 1.0f);
			boid->velocity += fleeDirection * forceStrength;

			boid->applyBoundaryForce(terrain); // new
		}
	}

}


void drawObjectPhong(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {
	glUseProgram(programPhSh);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programPhSh, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programPhSh, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programPhSh, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(programPhSh, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(programPhSh, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(programPhSh, "pointlightDir"), pointlightDir.x, pointlightDir.y, pointlightDir.z);
	glUniform3f(glGetUniformLocation(programPhSh, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);


	Core::DrawContext(context);
}



void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureId, GLuint normalId) {
	glUseProgram(programTex);
	
	Core::SetActiveTexture(textureId, "colorTexture", 0, 0);
	Core::SetActiveTexture(normalId, "normalSampler", 1, 1);
	
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programTex, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programTex, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programTex, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(programTex, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(programTex, "pointlightDir"), pointlightDir.x, pointlightDir.y, pointlightDir.z);
	glUniform3f(glGetUniformLocation(programTex, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	Core::DrawContext(context);
}

void drawObjectDisco(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {

	glUseProgram(programDisco);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programDisco, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programDisco, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programDisco, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(programDisco, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	glUniform3f(glGetUniformLocation(programDisco, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(programDisco, "pointlightDir"), pointlightDir.x, pointlightDir.y, pointlightDir.z);
	glUniform3f(glGetUniformLocation(programDisco, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	float time = glfwGetTime();
	glUniform1f(glGetUniformLocation(programDisco, "time"), time);
	Core::DrawContext(context);
}




void renderParticles(const std::vector<Boid*>& boids) {
	for (const auto& b : boids) {
		for (const auto& p : b->particles) {
			if (p.lifetime < p.lifespan) {
				glm::mat4 modelMatrix = glm::translate(p.position);
				modelMatrix = glm::scale(modelMatrix, glm::vec3(0.02f));
				drawObjectPhong(sphereContext, modelMatrix, glm::vec3(1.0f));
			}
		}
	}
}

void drawTerrain(glm::vec3 color, glm::mat4 modelMatrix) {
	glUseProgram(programPhSh);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programPhSh, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programPhSh, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programPhSh, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(programPhSh, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	//glUniform3f(glGetUniformLocation(programPhSh, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(programPhSh, "pointlightDir"), pointlightDir.x, pointlightDir.y, pointlightDir.z);
	glUniform3f(glGetUniformLocation(programPhSh, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	terrain.drawTerrain();
}

glm::vec3 randomVec3();

void drawSkyBox(){
	glDepthMask(GL_FALSE);
	glUseProgram(programSkyBox);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxWater);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * glm::mat4(glm::mat3(createCameraMatrix()));
	glm::mat4 transformation = viewProjectionMatrix;
	Core::SetActiveTexture(skyboxWater, "skybox",programSkyBox,0);
	glUniformMatrix4fv(glGetUniformLocation(programSkyBox, "transformation"), 1, GL_FALSE, (float*)&transformation);
	//glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
	Core::DrawContext(models::cubeContext);
	glDepthMask(GL_TRUE);
}

void renderScene(GLFWwindow* window) {

	glClearColor(0.4f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImGuiIO& io = ImGui::GetIO();
	//ImGUI new frame:
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	float time = glfwGetTime();
	updateDeltaTime(time);


	glUseProgram(programTex);
	glUniform1i(glGetUniformLocation(programTex, "useNormalMapping"), useNormalMapping);
	
	
	drawSkyBox();
	
	//drawObjectTexture(models::aquariumContext, glm::mat4() * glm::scale(glm::vec3(0.4)) * glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), 3, 4);
	drawObjectTexture(models::aquariumContext, glm::mat4() * 
		glm::scale(glm::vec3(0.3, 0.6,0.8)) * 
		glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * 
		glm::translate(glm::vec3(0.f,0.f,2.f)),
		texture::brickwall, texture::brickwall_normal);
	

	drawTerrain(glm::vec3(0.6, 0.8, 0.6), glm::mat4());
	


	//IMGUI WINDOWS:
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::Begin("PRESS \"I\" TO ENABLE/DISABLE CURSOR ");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::Text("Set obstacle color:");
	ImGui::ColorEdit3("change color", (float*)&obstacle_color);
	ImGui::Text("Enable or disable the rules:");


	// Seperation ON/OFF:
	static bool seperationEnabled = true;
	ImGui::Checkbox("Seperation", &seperationEnabled);
	float newSeparationWeight = seperationEnabled ? 2.0f : 0.0f;
	for (Boid* b : boids) {
		b->setSeparationWeight(newSeparationWeight);
	}


	//Alignment ON/OFF:
	static bool alignmentEnabled = true;
	ImGui::Checkbox("Alignment", &alignmentEnabled);
	float newAlignmentWeight = alignmentEnabled ? 1.0f : 0.0f;
	for (Boid* b : boids) {
		b->setAlignmentWeight(newAlignmentWeight);
	}

	//Cohesion ON/OFF:
	static bool cohesionEnabled = true;
	ImGui::Checkbox("Cohesion", &cohesionEnabled);
	float newCohesionWeight = cohesionEnabled ? 0.001f : 0.0f;
	for (Boid* b : boids) {
		b->setCohesionWeight(newCohesionWeight);
	}

	//Shark ON/OFF:
	static bool fleeEnabled = true;
	ImGui::Checkbox("Shark", &fleeEnabled);
	float newFleeWeight = fleeEnabled ? 3.0f : 0.0f;
	for (Boid* b : boids) {
		b->setFleeWeight(newFleeWeight);
	}

	//Normal mapping ON/OFF:
	ImGui::Checkbox("Normal mapping", &useNormalMapping);

	//Number of boids:
	static int boidCount = boids.size();
	ImGui::SliderInt("Number of boids", &boidCount, 1, 500);
	if (boidCount > boids.size())
	{
		while (boids.size() < boidCount)
		{
			boids.push_back(new Boid(randomVec3(), false, randomInt(0,1), false));
		}
	}
	if (boidCount < boids.size())
	{
		while (boids.size() > boidCount)
		{
			delete boids.back();
			boids.pop_back();
		}
	}

	// add shark if it gets lost while adding new boids:
	bool sharkExist = false;
	for (Boid* b : boids) {
		if (b->isShark)
		{
			sharkExist = true;
			break;
		}
	}

	if (!sharkExist && !boids.empty()) {
		boids[0]->isShark = true;
	}
	//

	//DISCO MODE:
	ImGui::Checkbox("DISCO MODE", &discoMode);

	ImGui::Text("PRESS ESC TO CLOSE PROGRAM");
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	


	//target force:
	glm::mat4 viewMatrix = createCameraMatrix();
	glm::mat4 projectionMatrix = createPerspectiveMatrix();
	applyTargetingForce(boids, window, viewMatrix, projectionMatrix, WIDTH, HEIGHT);

	//repulsion force:
	applyRepulsionForce(boids, window, viewMatrix, projectionMatrix, WIDTH, HEIGHT);


	// Draw boids & Particles
	for (Boid* b : boids) {
		b->update(boids, terrain, obstacle);
		b->updateParticles(deltaTime);

		if (b->isShark) {
			if (fleeEnabled) {
				if (discoMode)
				{
					drawObjectDisco(models::goldfishContext, b->getMatrix(), b->getGroupColor());
				}
				else {
					drawObjectTexture(models::goldfishContext, b->getMatrix(), texture::shark, texture::shark_normal);
				}
			}
		}
		else {
			if (discoMode)
			{
				drawObjectDisco(models::goldfishContext, b->getMatrix(), b->getGroupColor());
			}
			else {
				drawObjectTexture(models::goldfishContext, b->getMatrix(), texture::fish, texture::fishNormal);
			}
			
		}
	}
	renderParticles(boids);

	// obstacle
	if (discoMode)
	{
		drawObjectDisco(models::sphereContext, glm::mat4() * glm::translate(obstacleTransformation), obstacle_color);
	}
	else {
		drawObjectPhong(models::sphereContext, glm::mat4() * glm::translate(obstacleTransformation), obstacle_color);
	}
	


	glUseProgram(0);
	glfwSwapBuffers(window);
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
	WIDTH = width;
	HEIGHT = height;
}



void mouse_callback(GLFWwindow* window, double xpos, double ypos) 
{
	if (!cursorDisabled)
	{
		return;
	}
	//preventing sudden camera jump when you open the window:
	if (firstMouse)
	{
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouse = false;
	}

		float xOffset = xpos - lastMouseX;
		float yOffset = lastMouseY - ypos; //bc y-coordinates range from bottom to top
		lastMouseX = xpos;
		lastMouseY = ypos;

		const float sensitivity = 0.1f;
		xOffset *= sensitivity;
		yOffset *= sensitivity;

		yaw += xOffset;
		pitch += yOffset;

		//constrains so camera doesnt flip or some other weird stuff happens:
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		spaceshipDir = glm::normalize(direction);

}



void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS) {
			leftMousePressed = true; 
		}
		else if (action == GLFW_RELEASE) {
			leftMousePressed = false;
		}
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS) {
			rightMousePressed = true;
		}
		else if (action == GLFW_RELEASE) {
			rightMousePressed = false;
		}
	}

}




void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	context.initFromAssimpMesh(scene->mMeshes[0]);
}



float randomFloat(float min, float max) {
	return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min);
}


glm::vec3 randomVec3() {
	return glm::vec3(
		randomFloat(-3.f, 3.f),
		randomFloat(5.5f, 10.0f),
		randomFloat(-4.0f, 4.f)
	);
}

void initSkyBox() {
	glGenTextures(1, &skyboxWater);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxWater);

	int width, height, nrChannels;
	
	std::vector<std::string> faces{
	"img/underwater/uw_ft.jpg",//
	"img/underwater/uw_bk.jpg",//
	"img/underwater/uw_up.jpg",//
	"img/underwater/uw_dn.jpg",//
	"img/underwater/uw_rt.jpg",
	"img/underwater/uw_lf.jpg",
	};

	for(unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data;
		data = SOIL_load_image(faces[i].c_str(), &width, &height, &nrChannels, SOIL_LOAD_RGBA);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
		);
		SOIL_free_image_data(data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}


void init(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//input:
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//ImGUI initialization:
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");


	glEnable(GL_DEPTH_TEST);
	
	programPhSh = shaderLoader.CreateProgram("shaders/with_shadow_mapping.vert", "shaders/with_shadow_mapping.frag");
	programTex = shaderLoader.CreateProgram("shaders/with_textures.vert", "shaders/with_textures.frag");
	programSkyBox = shaderLoader.CreateProgram("shaders/cubemap.vert", "shaders/cubemap.frag");
	//programTex = shaderLoader.CreateProgram("shaders/normal_test.vert", "shaders/normal_test.frag");
	programDisco = shaderLoader.CreateProgram("shaders/shader_disco.vert", "shaders/shader_disco.frag");

	initSkyBox();

	terrain.createTerrainFromNoise(20,20,3.0);
	//terrain.createTerrainFromPng(5.0);

	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);

	loadModelToContext("./models/spaceship.obj", models::spaceshipContext);
	loadModelToContext("./models/sphere.obj", models::sphereContext);
	loadModelToContext("./models/aquarium.obj", models::aquariumContext);
	loadModelToContext("./models/test.obj", models::testContext);
	loadModelToContext("./models/goldie.obj", models::goldfishContext);
	loadModelToContext("./models/golden_fish.obj", models::leaderContext);
	loadModelToContext("./models/piranha.obj", models::sharkContext);
	loadModelToContext("./models/cube.obj", models::cubeContext);

	//textures:
	texture::ship = Core::LoadTexture("./textures/spaceship.jpg");
	texture::fish = Core::LoadTexture("./textures/Stylized_Scales_003_basecolor.png");
	texture::leaderFish = Core::LoadTexture("./textures/golden_fish.jpg");
	texture::brickwall = Core::LoadTexture("./textures/brickwall.jpg");
	texture::blueTest = Core::LoadTexture("./textures/blueTest.jpg");
	texture::shark = Core::LoadTexture("./textures/Pool_Water_Texture_Diff.jpg");
	//normal maps:
	texture::shipNormal = Core::LoadTexture("./textures/spaceship_normal.jpg");
	texture::fishNormal = Core::LoadTexture("./textures/Stylized_Scales_003_normal.png");
	texture::brickwall_normal = Core::LoadTexture("./textures/brickwall_normal.jpg");
	texture::shark_normal = Core::LoadTexture("./textures/Pool_Water_Texture_nrml.jpg");

	// fish --> red
	for (int i = 0; i <= 4; i++) {
		boids.push_back(new Boid(randomVec3(), false, 0, false));
		boids.push_back(new Boid(randomVec3(), false, 1, false));
	}


	// shark
	boids.push_back(new Boid(glm::vec3(3.5f,10.2f,4.5f), true, 2, false));


	for (Boid* b : boids) {
		b->initParticles();
	}

	obstacle.center = obstacleTransformation;
	obstacle.radious = 1.5f;


}
void shutdown(GLFWwindow* window)
{
	shaderLoader.DeleteProgram(programPhSh);
}



void processInput(GLFWwindow* window)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.05f * deltaTime * 60;
	float moveSpeed = 0.05f * deltaTime * 60;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		spaceshipPos += spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		spaceshipPos -= spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		spaceshipPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		spaceshipPos -= spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));

	cameraPos = spaceshipPos - 0.5 * spaceshipDir + glm::vec3(0, 1, 0) * 0.2f;
	cameraDir = spaceshipDir;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		exposition -= 0.05;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		exposition += 0.05;

	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
		printf("spaceshipPos = glm::vec3(%ff, %ff, %ff);\n", spaceshipPos.x, spaceshipPos.y, spaceshipPos.z);
		printf("spaceshipDir = glm::vec3(%ff, %ff, %ff);\n", spaceshipDir.x, spaceshipDir.y, spaceshipDir.z);
	}


	//for entering fullscreen mode:
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
	{
		fullScreen = !fullScreen;

		if (fullScreen) 
		{
			glfwSetWindowMonitor(window, nullptr, 50, 50, WIDTH, HEIGHT, GLFW_DONT_CARE);
		}
		else {
			monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode * mode = glfwGetVideoMode(monitor);
			WIDTH = mode->width;
			HEIGHT = mode->height;
			glfwSetWindowMonitor(window, monitor, 0, 0, WIDTH, HEIGHT, mode->refreshRate);
		}
		
		
	}

	//enable/disable cursor:
	bool iKeyPressedNow = glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS;
	if (iKeyPressedNow && !iKeyPressedLastFrame)
	{
		cursorDisabled = !cursorDisabled;
		if (cursorDisabled)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			glfwSetCursorPos(window, lastMouseX, lastMouseY);
		}
	}
		iKeyPressedLastFrame = iKeyPressedNow;


}

void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		renderScene(window);
		glfwPollEvents();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}