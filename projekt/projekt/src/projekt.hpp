#include "glew.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "Shader_Loader.h"
#include "Render_Utils.h"
//#include "Texture.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include "Boids.hpp"
#include "TerrainClass.cpp"

#include <cstdlib>
#include <ctime> 
#include <vector>

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

int WIDTH = 500, HEIGHT = 500;

namespace models {
	Core::RenderContext spaceshipContext;
	Core::RenderContext sphereContext;
	Core::RenderContext aquariumContext;
	Core::RenderContext goldfishContext;
	Core::RenderContext testContext;
	Core::RenderContext terrainContext;
	Core::RenderContext sharkContext;
}

//GLuint terrainVAO, terrainVBO, terrainEBO;
Terrain terrain;
unsigned int NUM_STRIPS;
unsigned int NUM_VERTS_PER_STRIP;


GLuint depthMapFBO;
GLuint depthMap;


GLuint programPBR;
GLuint programPhSh;
GLuint programTest;
GLuint programDepth;

Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext sphereContext;
Core::RenderContext goldfishContext;
Core::RenderContext sharkContext;

glm::vec3 sunPos = glm::vec3(-4.740971f, 2.149999f, 0.369280f);
glm::vec3 sunDir = glm::vec3(-0.93633f, 0.351106, 0.003226f);
glm::vec3 sunColor = glm::vec3(0.9f, 0.9f, 0.7f) * 0.5;

glm::vec3 cameraPos = glm::vec3(0.479490f, 1.250000f, -2.124680f);
glm::vec3 cameraDir = glm::vec3(-0.354510f, 0.000000f, 0.935054f);


glm::vec3 spaceshipPos = glm::vec3(0.065808f, 1.250000f, -2.189549f);
glm::vec3 spaceshipDir = glm::vec3(-0.490263f, 0.000000f, 0.871578f);
GLuint VAO, VBO;

float aspectRatio = 1.f;

float exposition = 1.f;


glm::vec3 pointlightPos = glm::vec3(5.f,5.f,-5.f);
glm::vec3 pointlightDir = glm::vec3(0., -0.3, 0.5);
glm::vec3 pointlightColor = glm::vec3(0.9, 0.6, 0.6);

glm::vec3 spotlightPos = glm::vec3(0, 0, 0);
glm::vec3 spotlightConeDir = glm::vec3(0, 0, 0);
glm::vec3 spotlightColor = glm::vec3(0.4, 0.4, 0.9) * 3;
float spotlightPhi = 3.14 / 4;


std::vector<Boid*> boids;

float lastTime = -1.f;
float deltaTime = 0.f;

// global variables needed for imgui:
glm::vec3 aquarium_color = glm::vec3(1.0f);


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
	float f = 20.;
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
	glm::mat4 view = glm::lookAt(pointlightPos, pointlightPos - pointlightDir, glm::vec3(0, 1, 0));
	//jak ustawić w którą stronę patrzy światło... 
	//zeby zgadzalo sie z światłem kierunkowym

	glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.f, 50.f);
	return projection * view;
}



void drawObjectPhong(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {
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

	glm::mat4 lightVP = createLightViewProjection();
	glUniformMatrix4fv(glGetUniformLocation(programPhSh, "lightVP"), 1, GL_FALSE, (float*)&lightVP);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);



	Core::DrawContext(context);
}

void drawObjectPBR(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color, float roughness, float metallic) {

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programPBR, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programPBR, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	glUniform1f(glGetUniformLocation(programPBR, "exposition"), exposition);

	glUniform1f(glGetUniformLocation(programPBR, "roughness"), roughness);
	glUniform1f(glGetUniformLocation(programPBR, "metallic"), metallic);

	glUniform3f(glGetUniformLocation(programPBR, "color"), color.x, color.y, color.z);

	glUniform3f(glGetUniformLocation(programPBR, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUniform3f(glGetUniformLocation(programPBR, "sunDir"), sunDir.x, sunDir.y, sunDir.z);
	glUniform3f(glGetUniformLocation(programPBR, "sunColor"), sunColor.x, sunColor.y, sunColor.z);

	glUniform3f(glGetUniformLocation(programPBR, "lightPos"), pointlightPos.x, pointlightPos.y, pointlightPos.z);
	glUniform3f(glGetUniformLocation(programPBR, "lightColor"), pointlightColor.x, pointlightColor.y, pointlightColor.z);

	glUniform3f(glGetUniformLocation(programPBR, "spotlightConeDir"), spotlightConeDir.x, spotlightConeDir.y, spotlightConeDir.z);
	glUniform3f(glGetUniformLocation(programPBR, "spotlightPos"), spotlightPos.x, spotlightPos.y, spotlightPos.z);
	glUniform3f(glGetUniformLocation(programPBR, "spotlightColor"), spotlightColor.x, spotlightColor.y, spotlightColor.z);
	glUniform1f(glGetUniformLocation(programPBR, "spotlightPhi"), spotlightPhi);
	Core::DrawContext(context);

}


void drawObjectDepth(Core::RenderContext& context, glm::mat4 viewProjection, glm::mat4 modelMatrix) {
	glUniformMatrix4fv(glGetUniformLocation(programDepth, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniformMatrix4fv(glGetUniformLocation(programDepth, "lightViewProjection"), 1, GL_FALSE, (float*)&viewProjection);
	Core::DrawContext(context);
}



void renderShadowmapPointLight() {
	glUseProgram(programDepth);
	float time = glfwGetTime();
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::mat4 viewProjection = createLightViewProjection();

	//drawObjectDepth(models::sphereContext, viewProjection,glm::translate(glm::vec3(0.f, 2.f, 0.f)));


	//drawObjectDepth(models::aquariumContext, viewProjection, glm::mat4() * glm::scale(glm::vec3(0.3)) * glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

	//drawObjectDepth(models::terrainContext, viewProjection,glm::scale(glm::vec3(0.2, 0.2, 0.2)));


	for (Boid* b : boids) {
		drawObjectDepth(models::goldfishContext, viewProjection, b->getMatrix());

	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
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

	glm::mat4 lightVP = createLightViewProjection();
	glUniformMatrix4fv(glGetUniformLocation(programPhSh, "lightVP"), 1, GL_FALSE, (float*)&lightVP);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	terrain.drawTerrain();

}

void renderScene(GLFWwindow* window)
{
	glClearColor(0.4f, 0.4f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//ImGUI new frame:
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	float time = glfwGetTime();
	updateDeltaTime(time);
	glUseProgram(programPhSh);
	
	renderShadowmapPointLight();


	//drawObjectPhong(models::aquariumContext, glm::mat4() * glm::scale(glm::vec3(0.4)) * glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
	//	aquarium_color);

	
	//drawObjectPhong(models::terrainContext, glm::scale(glm::vec3(0.2, 0.2, 0.2)), glm::vec3(0.8, 0.3, 0.3));
	drawTerrain(glm::vec3(0.3, 0.3, 0.3), glm::translate(glm::vec3(0.f,5.f,0.f)));
	//IMGUI WINDOWS:
	ImGui::Begin("Main testing window");
	ImGui::Text("Application average framerate: %.1f FPS", 1000.0 / double(ImGui::GetIO().Framerate), double(ImGui::GetIO().Framerate));
	ImGui::Text("Set aquarium color:");
	ImGui::ColorEdit3("change color", (float*)&aquarium_color);
	ImGui::Text("Enable or disable the rules:");

	// Seperation ON/OFF:
	static bool seperationEnabled = true;
	ImGui::Checkbox("Seperation", &seperationEnabled);
	float newSeparationWeight = seperationEnabled ? 2.0f : 0.0f;
	for (Boid* b : boids) {
		b->setSeparationWeight(newSeparationWeight);
	}



	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glUseProgram(programTest);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, depthMap);
	//Core::DrawContext(models::testContext);

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

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Boids & Particles
	for (Boid* b : boids) {
		b->update(boids);
		b->updateParticles(deltaTime);
		if (b->isShark) {
			if (fleeEnabled) {
				drawObjectPhong(models::sharkContext, b->getMatrix() * glm::scale(glm::vec3(0.4,0.4,0.3))*
					glm::rotate(glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f)), b->getGroupColor());
			}
		}
		else {
			drawObjectPhong(models::goldfishContext, b->getMatrix(), b->getGroupColor());
		}
	}
	renderParticles(boids);

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
		randomFloat(-4.0f, 4.0),
		randomFloat(1.f, 4.0f),
		randomFloat(-1.0f, 1.0f)
	);
}

void initDepthMap() {
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void init(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//ImGUI initialization:
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");


	glEnable(GL_DEPTH_TEST);
	programPBR = shaderLoader.CreateProgram("shaders/shader_9_1.vert", "shaders/shader_9_1.frag");
	programPhSh = shaderLoader.CreateProgram("shaders/with_shadow_mapping.vert", "shaders/with_shadow_mapping.frag");
	programTest = shaderLoader.CreateProgram("shaders/test.vert", "shaders/test.frag");

	initDepthMap();
	terrain.createTerrain();

	loadModelToContext("./models/sphere.obj", sphereContext);
	loadModelToContext("./models/spaceship.obj", shipContext);

	loadModelToContext("./models/spaceship.obj", models::spaceshipContext);
	loadModelToContext("./models/sphere.obj", models::sphereContext);
	loadModelToContext("./models/aquarium.obj", models::aquariumContext);
	loadModelToContext("./models/test.obj", models::testContext);
	loadModelToContext("./models/goldie.obj", models::goldfishContext);
	loadModelToContext("./models/mountain.obj", models::terrainContext);

	loadModelToContext("./models/shark.obj", models::sharkContext);

	// fish --> red
	for (int i = 0; i <= 4; i++) {
		boids.push_back(new Boid(randomVec3(), false, 0));
		boids.push_back(new Boid(randomVec3(), false, 1));
	}


	// shark
	boids.push_back(new Boid(randomVec3(), true, 2));

	for (Boid* b : boids) {
		b->initParticles();
	}


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