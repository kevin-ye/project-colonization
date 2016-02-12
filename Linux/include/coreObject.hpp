#pragma once

#include <glm/glm.hpp>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "gameObject.hpp"

class coreObject : public gameObject
{
	std::string name;
	std::string filepath;

	class mLock *dataLock;

	std::vector<unsigned int> meshes;
	glm::mat4 trans;
	unsigned int buildingLevel;

public:
	coreObject(std::string n, std::string file);
	~coreObject();

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

	int neededEnergy;
	int neededWater;
	int neededOre;

	int runningEnergy;
	int runningWater;
	int runningOre;
};