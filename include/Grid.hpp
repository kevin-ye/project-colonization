#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class cell
{
public:
	cell() : holding(NULL), link(std::vector<cell *>()) {}
	~cell() {}

	class gameObject *holding;
	std::vector<cell *> link;
	int gx;
	int gy;
	
};

class Grid
{
	unsigned int gsize;
	cell **_grid;
	class gameData *_data;
	class gameObject *_core;
	class pipeObject *unitpipe;
	class mLock *dataLock;

	class gameObject *surface;

	float getAngle(glm::vec3 v1, glm::vec3 v2);

public:
	Grid(unsigned int s);
	~Grid();

	class gameObject *getObject(int x, int y);
	bool setObject(int x, int y, class gameObject *_o);

	void setSurface(class gameObject *_o);

	void render(glm::mat4 baseMat, class tWindow *_window);

	void setgameData(class gameData *_d);
	void setunitpipe(class pipeObject *pipe);

	unsigned int getGsize();

	void onSave(std::string savePath);
	void onLoad(std::string savePath);

	void upgrade(int x, int y);
	bool checkupgrade(int x, int y);
	bool checkbuild(int neededenergy, int neededwater, int neededore);
	void reduceResource(int neededenergy, int neededwater, int neededore);

	void resourceTick();

	// in game resource
	int energy;
	int water;
	int ore;
	int energyCap;
	int waterCap;
	int oreCap;

	int getenergyCap();
	int getwaterCap();
	int getoreCap();
};