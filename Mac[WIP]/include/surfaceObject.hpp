#pragma once

#include <glm/glm.hpp>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "gameObject.hpp"

class surfaceObject : public gameObject
{
	std::string name;
	std::string filepath;

	class mLock *dataLock;

	std::vector<unsigned int> meshes;
	glm::mat4 trans;

public:
	surfaceObject(std::string n, std::string file);
	~surfaceObject();

	gameObjectType getType();

	std::string getName();
	void setName(std::string n);

	void load();
	void reload();

	void render(glm::mat4 baseMat, class tWindow *_window);

	void onSave(std::string savePath);
	void onLoad(std::string savePath);

	void upgrade();
	bool checkupgrade();
};