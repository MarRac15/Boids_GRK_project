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


#include "SOIL/SOIL.h"

class Terrain {



	int width, height, nChannels;
	unsigned int NUM_OF_STRIPS;
	unsigned int NUM_VER_PER_STRIP;
	GLuint terrainVAO, terrainVBO, terrainEBO;
public:
	Terrain(){ 
		width = 0; height = 0; nChannels = 0; NUM_OF_STRIPS = 0;NUM_VER_PER_STRIP = 0;

	}


	//temporrary
	unsigned char* loadHeightMap() {

		unsigned char* data = SOIL_load_image("img/heightmap/mini.png", &width, &height, &nChannels, 0);
		return data;
	}

	
	void createTerrain() {


		float scaleY = 5.0f / 256.0f;
		//znormalizowanie do 0,1 oraz wysokosc jaka chce ortzymac czyli 20
		float shiftY = 0.f;

		float scalePixel = 0.5f;

		unsigned char* data = loadHeightMap();
		//close soil image?

		std::vector<float> vertices;
		for (unsigned int i = 0;i < height;i++) {
			for (unsigned int j = 0;j < width;j++) {
				unsigned char* texel = data + (j + width * i) * nChannels;

				unsigned char y = texel[0];
				//koordynat y w heightmapie
				vertices.push_back(scalePixel*( - height / 2.0f + i));	//vertices v.x
				vertices.push_back((int)y * scaleY - shiftY); // v.y
				vertices.push_back(scalePixel*( - width / 2.0f + j));       //v.z
			}
		}
		std::vector<unsigned int> indices;

		for (unsigned int i = 0;i < height - 1; i++) {
			for (unsigned int j = 0; j < width; j++) {
				for (unsigned int k = 0; k < 2;k++) {
					indices.push_back(j + width * (i + k));
				}
			}
		}
		SOIL_free_image_data(data);

		NUM_OF_STRIPS = height - 1;
		NUM_VER_PER_STRIP = width * 2;

		glGenVertexArrays(1, &terrainVAO);
		glBindVertexArray(terrainVAO);

		glGenBuffers(1, &terrainVBO);
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(0);

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

};

