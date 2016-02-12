#include "buildingObject.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "gameObject.hpp"
#include "mLock.hpp"
#include "INIReader.hpp"
#include "Debug.hpp"
#include "gWindow.hpp"
#include "objectDefLoader.hpp"

using namespace std;
using namespace glm;

buildingObject::buildingObject(buildingObject &copyObj) 
{
	this->name = copyObj.name;
	this->filepath = copyObj.filepath;
	this->dataLock = new mLock();
	this->buildingID = copyObj.buildingID;
	this->buildingLevel = copyObj.buildingLevel;
	this->neededEnergy = copyObj.neededEnergy;
	this->neededWater = copyObj.neededWater;
	this->neededOre = copyObj.neededOre;
	this->runningEnergy = copyObj.runningEnergy;
	this->runningWater = copyObj.runningWater;
	this->runningOre = copyObj.runningOre;

	this->meshes.clear();
	for (unsigned int i = 0; i < copyObj.meshes.size(); ++i)
	{
		this->meshes.push_back(copyObj.meshes[i]);
	}

	this->trans = copyObj.trans;
}

buildingObject::buildingObject(string n, string file) : 
name(n), filepath(file), dataLock(new mLock()), buildingID(0),buildingLevel(0),
meshes(vector<unsigned int>())
{}

buildingObject::buildingObject() : dataLock(new mLock()), buildingID(0),
meshes(vector<unsigned int>())
{}

buildingObject::~buildingObject() 
{
	delete dataLock;
}

gameObjectType buildingObject::getType() 
{
	return gameObjectType::Object_Render;
}

string buildingObject::getName() 
{
	string r = "";
	dataLock->acquire();
	r = name;
	dataLock->release();

	return r;
}

void buildingObject::setName(string n) 
{
	dataLock->acquire();
	name = n;
	dataLock->release();
}

void buildingObject::load() 
{
	objectDefLoader loader("GameData/" + filepath);
	loader.load(meshes, trans, 
		neededEnergy, neededWater, neededOre,
		runningEnergy, runningWater, runningOre);
}

void buildingObject::reload() 
{
	load();
}

void buildingObject::render(glm::mat4 baseMat, tWindow *_window)
{
	// render
	baseMat = baseMat * trans;
	_window->renderMesh(baseMat, meshes[buildingLevel], true);
}

void buildingObject::onSave(std::string savePath) 
{
	ofstream savefile;
	savefile.open(savePath, ios::app);

	savefile << "[" << oid << "]" << endl;
	savefile << "BuildingInfo=BUILDING" << endl;
	savefile << "name=" << name << endl;
	savefile << "filepath=" << filepath << endl;
	savefile << "meshesCount=" << meshes.size() << endl;

	for (int i = 0; i < meshes.size(); ++i)
	{
		savefile << "m" << i << "=" << meshes[i] << endl;
	}

	savefile << "buildingID=" << buildingID << endl;
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

void buildingObject::onLoad(std::string savePath) 
{
	INIReader reader(savePath);

	name = reader.Get(std::to_string(oid), "name", "name");
	filepath = reader.Get(std::to_string(oid), "filepath", "filepath");

	unsigned int meshesCount = reader.GetInteger(std::to_string(oid), "meshesCount", 0);

	for (int i = 0; i < meshesCount; ++i)
	{
		meshes.push_back(reader.GetInteger(std::to_string(oid), "m" + std::to_string(i), 0));
	}

	buildingID = reader.GetInteger(std::to_string(oid), "buildingID", 0);
	buildingLevel = reader.GetInteger(std::to_string(oid), "buildingLevel", 0);

	trans[0][0] = reader.GetReal(std::to_string(oid), "trans00", 0);
	trans[0][1] = reader.GetReal(std::to_string(oid), "trans01", 0);
	trans[0][2] = reader.GetReal(std::to_string(oid), "trans02", 0);
	trans[0][3] = reader.GetReal(std::to_string(oid), "trans03", 0);
	trans[1][0] = reader.GetReal(std::to_string(oid), "trans10", 0);
	trans[1][1] = reader.GetReal(std::to_string(oid), "trans11", 0);
	trans[1][2] = reader.GetReal(std::to_string(oid), "trans12", 0);
	trans[1][3] = reader.GetReal(std::to_string(oid), "trans13", 0);
	trans[2][0] = reader.GetReal(std::to_string(oid), "trans20", 0);
	trans[2][1] = reader.GetReal(std::to_string(oid), "trans21", 0);
	trans[2][2] = reader.GetReal(std::to_string(oid), "trans22", 0);
	trans[2][3] = reader.GetReal(std::to_string(oid), "trans23", 0);
	trans[3][0] = reader.GetReal(std::to_string(oid), "trans30", 0);
	trans[3][1] = reader.GetReal(std::to_string(oid), "trans31", 0);
	trans[3][2] = reader.GetReal(std::to_string(oid), "trans32", 0);
	trans[3][3] = reader.GetReal(std::to_string(oid), "trans33", 0);

	neededEnergy = reader.GetInteger(std::to_string(oid), "neededEnergy", neededEnergy);
	neededWater = reader.GetInteger(std::to_string(oid), "neededWater", neededWater);
	neededOre = reader.GetInteger(std::to_string(oid), "neededOre", neededOre);

	runningEnergy = reader.GetInteger(std::to_string(oid), "runningEnergy", runningEnergy);
	runningWater = reader.GetInteger(std::to_string(oid), "runningWater", runningWater);
	runningOre = reader.GetInteger(std::to_string(oid), "runningOre", runningOre);
}

void buildingObject::upgrade()
{
	if (buildingLevel < (meshes.size() - 1))
	{
		buildingLevel++;
		neededEnergy *= 2;
		neededWater *= 2;
		neededOre *= 2;
		runningEnergy *= 2;
		runningWater *= 2;
		runningOre *= 2;
	}
}

bool buildingObject::checkupgrade()
{
	return true;
}