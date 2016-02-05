#pragma once

#include <cstdlib>
#include <cstdio>
#include <vector>

#include <glm/glm.hpp>

#include "gameObject.hpp"
#include "Debug.hpp"

/*
#include "game-framework/gameWindow.hpp"
#include "game-framework/OpenGLImport.hpp"
#include "game-framework/ShaderProgram.hpp"
#include "game-framework/MeshConsolidator.hpp"*/

class meshObject : public gameObject
{
	std::string name;
	std::string filepath;
	unsigned int id;

	std::vector<glm::vec3> VertexPositions;
	std::vector<glm::vec2> UVPositions;
	std::vector<glm::vec3> VertexNormals;

	class mLock *dataLock;

	//MeshConsolidator *meshConsolidator;

public:
	meshObject(std::string n, std::string file);
	~meshObject();
	
	gameObjectType getType();

	std::vector<glm::vec2> getUVPositions();
	std::vector<glm::vec3> getVertexPositions();
	std::vector<glm::vec3> getVertexNormals();
	const float *getVertexPositionsPtr();
	const float *getUVPositionsPtr();
	const float *getVertexNormalsPtr();
	size_t getVertexPositionsNum();
	size_t getUVPositionsNum();
	size_t getVertexNormalsNum();

	std::string getName();
	void setName(std::string n);
	void setID(unsigned int i);

	void load();
	void reload();

	void render(glm::mat4 baseMat, class tWindow *_window);

	std::string getTexturePath();

	void onSave(std::string savePath);
	void onLoad(std::string savePath);

	void upgrade();
	bool checkupgrade();
};