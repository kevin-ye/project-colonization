#pragma once

#include <glm/glm.hpp>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

class objectDefLoader
{
	std::string filepath;
public:
	objectDefLoader(std::string file);
	~objectDefLoader();

	void load(std::vector<unsigned int> &meshes, glm::mat4 &trans,
		int &neededEnergy, int &neededWater, int &neededOre,
		int &runningEnergy, int &runningWater, int &runningOre);
	
};