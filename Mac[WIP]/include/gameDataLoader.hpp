#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>

class gameDataLoader
{
	class gameData *_data;

	std::string getFileName(std::string filepath);

public:
	gameDataLoader(class gameData *_d);
	~gameDataLoader();

	void load(unsigned int &gsize, std::vector<class buildingObject*> &blist);
};