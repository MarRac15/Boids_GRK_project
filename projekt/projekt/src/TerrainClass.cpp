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
# include <random>

#include "SOIL/SOIL.h"

class Terrain {


	unsigned int NUM_OF_STRIPS;
	unsigned int NUM_VER_PER_STRIP;

	//GenerateHeightmap GenerateHeightmap();
	GLuint terrainVAO, terrainVBO, terrainEBO;
public:
	Terrain(){ 
		NUM_OF_STRIPS = 0;NUM_VER_PER_STRIP = 0;

	}

	float random(glm::vec2 st,float random1, float random2, float random3) {
		return glm::fract(
			sin(
				glm::dot(
					st,
					glm::vec2(random1, random2)
				)
				* random3)
		);
	}

	float noise(glm::vec2 pos,float random1, float random2, float random3) {
		glm::vec2 i = floor(pos);
		glm::vec2 f = fract(pos);

		float a = random(i,random1,random2, random3);
		float b = random(i + glm::vec2(1.0, 0.0), random1, random2, random3);
		float c = random(i + glm::vec2(0.0, 1.0), random1, random2, random3);
		float d = random(i + glm::vec2(1.0, 1.0), random1, random2, random3);

		//cubic curve for smoothstep interpolation
		glm::vec2 u = f * f * (3.0f - 2.0f * f);

		return glm::mix(a, b, u.x) +
			(c - a) * u.y * (1.0f - u.x) +
			(d - b) * u.x * u.y;
	}
	
	float calculateYFromNoise(glm::vec2 pos,int width,int height,float random1, float random2, float random3) {

			glm::vec2 resolution = glm::vec2((float)width, (float)height);
			//pos = pos / resolution;
			glm::vec2 st = pos * 5.0;

			float n = noise(st, random1, random2,random3);
		
			float y =  n;
			
		return y;
	}

	//temporrary
	

	std::vector<float> vertices;
	int terrainHeight, terrainWidth;

	void createTerrainFromNoise(int width,int height, float heightScale) {

		terrainWidth = width;
		terrainHeight = height;

		float scaleY = heightScale;
		//znormalizowanie do 0,1 oraz wysokosc jaka chce ortzymac czyli 20
		float shiftY = -1.5f;

		float scalePixel = 10.f/(float)width;//do zmiany zeby bylo 10.0/(float)width
		 
		srand(time(0));
		float random1 =  999999.0f; // 1.1 + (float)(std::rand() % (99999 - 1 + 1));
		float random2 = 1.1 + (float)(std::rand() % (99999 - 1 + 1));
		float random3 = 1.1 + (float)(std::rand() % (99999 - 1 + 1));
		
		random1 += 1.0 / (float)(std::rand() % (99999 - 1 + 1));
		random2 += 1.0 / (float)(std::rand() % (99999 - 1 + 1));
		random3 += 1.0 / (float)(std::rand() % (99999 - 1 + 1));

		//close soil image?

		//std::vector<float> verticesNormals;

		for (unsigned int i = 0;i < height;i++) {
			for (unsigned int j = 0;j < width;j++) {
				

				float y = calculateYFromNoise(glm::vec2((float)(-height / 2.0f + i), (float)(-width / 2.0f + j)), width, height,random1,random2,random3);
				//przykladowo!
				//koordynat y w heightmapie
				vertices.push_back(scalePixel * (-height / 2.0f + i));	//vertices v.x
				vertices.push_back(y * scaleY - shiftY); // v.y
				vertices.push_back(scalePixel * (-width / 2.0f + j));       //v.z
				//DODANIE NORMALNYCH WEKTOROW
				//x y z wartosci normalnej
				vertices.push_back(0.f);	//x normalnej
				vertices.push_back(0.f);	//y normalnej
				vertices.push_back(0.f);	//z normalnej 
				//
			}
		}
		std::vector<unsigned int> indices;

		for (unsigned int i = 0;i < height - 1; i++) {//tu sa tworzone indeksy czyli tu przy trojkatach mozna obliczac normalne
			for (unsigned int j = 0; j < width; j++) {
				for (unsigned int k = 0; k < 2;k++) {
					indices.push_back(j + width * (i + k));//??? czy tak?
					//czy powinno byc *3 skoro mam dodatkowe 3 atrybuty 

				}
			}
		}
		for (unsigned int i = 0;i < indices.size()-2;i = i + 3) {
			unsigned int index0 = indices[i] * 6;
			unsigned int index1 = indices[i + 1] * 6;
			unsigned int index2 = indices[i + 2] * 6;	//bo wierzcholek ma 6 atrybutow
			//teraz dostalismy indeksy wierzcholkow trojkata
			//wierzcholki
			glm::vec3 vertice0 = glm::vec3(vertices[index0], vertices[index0 + 1], vertices[index0 + 2]);
			glm::vec3 vertice1 = glm::vec3(vertices[index1], vertices[index1 + 1], vertices[index1 + 2]);
			glm::vec3 vertice2 = glm::vec3(vertices[index2], vertices[index2 + 1], vertices[index2 + 2]);

			glm::vec3 normal = glm::cross(vertice0 - vertice1, vertice2 - vertice1);
			vertices[index0 + 3] += normal.x;
			vertices[index0 + 4] += normal.y;
			vertices[index0 + 5] += normal.z;
			vertices[index1 + 3] += normal.x;
			vertices[index1 + 4] += normal.y;
			vertices[index1 + 5] += normal.z;
			vertices[index2 + 3] += normal.x;
			vertices[index2 + 4] += normal.y;
			vertices[index2 + 5] += normal.z;


		}

		NUM_OF_STRIPS = height - 1;
		NUM_VER_PER_STRIP = width * 2;

		glGenVertexArrays(1, &terrainVAO);
		glBindVertexArray(terrainVAO);

		glGenBuffers(1, &terrainVBO);
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));


		glGenBuffers(1, &terrainEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		//koniec funkcji
	}

	void createTerrainFromPng(float heightScale) {

		int width, height, nChannels;
		float scaleY = heightScale / 256.0f;
		//znormalizowanie do 0,1 oraz wysokosc jaka chce ortzymac czyli 20
		float shiftY = -1.f;


		

		unsigned char* data = SOIL_load_image("img/heightmap/mini.png", &width, &height, &nChannels, 0);
		
		float scalePixel = 0.5;//10.0f / (float)width;
		
		//terrainHeight = height;
		//terrainWidth = width;
		//close soil image?
		
		//std::vector<float> verticesNormals;

		std::vector<float> vertices;
		for (unsigned int i = 0;i < height;i++) {
			for (unsigned int j = 0;j < width;j++) {
				unsigned char* texel = data + (j + width * i) * nChannels;

				unsigned char y = texel[0];
				//koordynat y w heightmapie
				vertices.push_back(scalePixel*( - height / 2.0f + i));	//vertices v.x
				vertices.push_back((int)y * scaleY - shiftY); // v.y
				vertices.push_back(scalePixel*( - width / 2.0f + j));       //v.z
				//DODANIE NORMALNYCH WEKTOROW
				//x y z wartosci normalnej
				vertices.push_back(0.f);	//x normalnej
				vertices.push_back(0.f);	//y normalnej
				vertices.push_back(0.f);	//z normalnej 
				//DLA TESTU DALAM 0.5 dla kazdej
			}
		}
		std::vector<unsigned int> indices;

		for (unsigned int i = 0;i < height - 1; i++) {//tu sa tworzone indeksy czyli tu przy trojkatach mozna obliczac normalne
			for (unsigned int j = 0; j < width; j++) {
				for (unsigned int k = 0; k < 2;k++) {
					indices.push_back(j + width * (i + k));//??? czy tak?
					//czy powinno byc *3 skoro mam dodatkowe 3 atrybuty 
					
				}
			}
		}
		for (unsigned int i = 0;i < indices.size()-2;i = i + 3) {
			unsigned int index0 = indices[i]*6;
			unsigned int index1 = indices[i + 1]*6;
			unsigned int index2 = indices[i + 2]*6;	//bo wierzcholek ma 6 atrybutow
			//teraz dostalismy indeksy wierzcholkow trojkata
			//wierzcholki
			glm::vec3 vertice0 = glm::vec3(vertices[index0],vertices[index0+1],vertices[index0+2]);
			glm::vec3 vertice1 = glm::vec3(vertices[index1],vertices[index1+1],vertices[index1+2]);
			glm::vec3 vertice2 = glm::vec3(vertices[index2],vertices[index2+1],vertices[index2+2]);

			glm::vec3 normal = glm::cross(vertice0 - vertice1, vertice2 - vertice1);
			vertices[index0 + 3] += normal.x;
			vertices[index0 + 4] += normal.y;
			vertices[index0 + 5] += normal.z;
			vertices[index1 + 3] += normal.x;
			vertices[index1 + 4] += normal.y;
			vertices[index1 + 5] += normal.z;
			vertices[index2 + 3] += normal.x;
			vertices[index2 + 4] += normal.y;
			vertices[index2 + 5] += normal.z;

			
		}
		SOIL_free_image_data(data);

		NUM_OF_STRIPS = height - 1;
		NUM_VER_PER_STRIP = width * 2;

		glGenVertexArrays(1, &terrainVAO);
		glBindVertexArray(terrainVAO);

		glGenBuffers(1, &terrainVBO);
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3 * sizeof(float)));


		glGenBuffers(1, &terrainEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	//koniec funkcji
	}
	
	void drawTerrain() {
		
		glBindVertexArray(terrainVAO);

		for (unsigned int strip = 0;strip < NUM_OF_STRIPS;strip++) {
			glDrawElements(GL_TRIANGLE_STRIP, NUM_VER_PER_STRIP, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * NUM_VER_PER_STRIP * strip));
		}
		glBindVertexArray(0);
	}


	float getHeight(float x, float z) {
		float scalePixel = 10.f / (float)terrainWidth;

		// liczenie indeksu siatki, w której le¿y punkt
		int i = (int)((x / scalePixel) + (terrainHeight / 2.0f));  // pion
		int j = (int)((z / scalePixel) + (terrainWidth / 2.0f));   // poziom

		std::cout << " " << scalePixel << std::endl;


		if (i < 0 || i >= terrainHeight || j < 0 || j >= terrainWidth) {
			return 0.0f;
		}

		int index = (i * terrainWidth + j) * 6;  // indeks w vertices


		return vertices[index + 1];
	}

};

