#include "coreObject.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "INIReader.hpp"
#include "gameObject.hpp"
#include "mLock.hpp"
#include "INIReader.hpp"
#include "Debug.hpp"
#include "gWindow.hpp"
#include "objectDefLoader.hpp"

using namespace std;
using namespace glm;

coreObject::coreObject(string n, string file) : 
name(n), filepath(file), dataLock(new mLock()),
meshes(vector<unsigned int>()), buildingLevel(0)
{}

coreObject::~coreObject() 
{
	delete dataLock;
}

gameObjectType coreObject::getType() 
{
	return gameObjectType::Object_Render;
}

string coreObject::getName() 
{
	string r = "";
	dataLock->acquire();
	r = name;
	dataLock->release();

	return r;
}

void coreObject::setName(string n) 
{
	dataLock->acquire();
	name = n;
	dataLock->release();
}

void coreObject::load() 
{
	objectDefLoader loader("GameData/" + filepath);
	loader.load(meshes, trans, 
		neededEnergy, neededWater, neededOre,
		runningEnergy, runningWater, runningOre);
}

void coreObject::reload() 
{
	load();
}

void coreObject::render(glm::mat4 baseMat, tWindow *_window)
{
	// render
	baseMat = baseMat * trans;
	_window->renderMesh(baseMat, meshes[buildingLevel], true);
}

void coreObject::onSave(std::string savePath) 
{
	ofstream savefile;
	savefile.open(savePath, ios::app);

	savefile << "[core]" << endl;
	savefile << "BuildingInfo=CORE" << endl;
	savefile << "name=" << name << endl;
	savefile << "filepath=" << filepath << endl;
	savefile << "meshesCount=" << meshes.size() << endl;

	for (int i = 0; i < meshes.size(); ++i)
	{
		savefile << "m" << i << "=" << meshes[i] << endl;
	}

	savefile << "buildingLevel=" << buildingLevel << endl;

	savefile << "trans00=" << trans[0][0] << endl;
	savefile << "trans01=" << trans[0][1] << endl;
	savefile << "trans02=" << trans[0][2] << endl;
	savefile << "trans03=" << trans[0][3] << endl;
	savefile << "trans10=" << trans[1][0] << endl;
	savefile << "trans11=" << trans[1][1] << endl;
	savefile << "trans12=" << trans[1][2] << endl;
	savefile << "trans13=" << trans[1][3] << endl;
	savefile << "trans20=" << trans[2][0] << endl;
	savefile << "trans21=" << trans[2][1] << endl;
	savefile << "trans22=" << trans[2][2] << endl;
	savefile << "trans23=" << trans[2][3] << endl;
	savefile << "trans30=" << trans[3][0] << endl;
	savefile << "trans31=" << trans[3][1] << endl;
	savefile << "trans32=" << trans[3][2] << endl;
	savefile << "trans33=" << trans[3][3] << endl;

	savefile << "neededEnergy=" << neededEnergy << endl;
	savefile << "neededWater=" << neededWater << endl;
	savefile << "neededOre=" << neededOre << endl;
	savefile << "runningEnergy=" << runningEnergy << endl;
	savefile << "runningWater=" << runningWater << endl;
	savefile << "runningOre=" << runningOre << endl;


	savefile.close();
}

void coreObject::onLoad(std::string savePath) 
{
	INIReader reader(savePath);

	name = reader.Get("core", "name", "name");
	filepath = reader.Get("core", "filepath", "filepath");

	unsigned int meshesCount = reader.GetInteger("core", "meshesCount", 0);

	for (int i = 0; i < meshesCount; ++i)
	{
		meshes.push_back(reader.GetInteger("core", "m" + std::to_string(i), 0));
	}

	trans[0][0] = reader.GetReal("core", "trans00", 0);
	trans[0][1] = reader.GetReal("core", "trans01", 0);
	trans[0][2] = reader.GetReal("core", "trans02", 0);
	trans[0][3] = reader.GetReal("core", "trans03", 0);
	trans[1][0] = reader.GetReal("core", "trans10", 0);
	trans[1][1] = reader.GetReal("core", "trans11", 0);
	trans[1][2] = reader.GetReal("core", "trans12", 0);
	trans[1][3] = reader.GetReal("core", "trans13", 0);
	trans[2][0] = reader.GetReal("core", "trans20", 0);
	trans[2][1] = reader.GetReal("core", "trans21", 0);
	trans[2][2] = reader.GetReal("core", "trans22", 0);
	trans[2][3] = reader.GetReal("core", "trans23", 0);
	trans[3][0] = reader.GetReal("core", "trans30", 0);
	trans[3][1] = reader.GetReal("core", "trans31", 0);
	trans[3][2] = reader.GetReal("core", "trans32", 0);
	trans[3][3] = reader.GetReal("core", "trans33", 0);

	buildingLevel = reader.GetInteger("core", "buildingLevel", 0);

	neededEnergy = reader.GetInteger(std::to_string(oid), "neededEnergy", neededEnergy);
	neededWater = reader.GetInteger(std::to_string(oid), "neededWater", neededWater);
	neededOre = reader.GetInteger(std::to_string(oid), "neededOre", neededOre);

	runningEnergy = reader.GetInteger(std::to_string(oid), "runningEnergy", runningEnergy);
	runningWater = reader.GetInteger(std::to_string(oid), "runningWater", runningWater);
	runningOre = reader.GetInteger(std::to_string(oid), "runningOre", runningOre);
}

void coreObject::upgrade()
{
	if (buildingLevel < (meshes.size() - 1))
	{
		buildingLevel++;
	}
}

bool coreObject::checkupgrade()
{
	return true;
}