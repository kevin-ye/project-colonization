#include "pipeObject.hpp"

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

pipeObject::pipeObject(pipeObject &copyObj) 
{
	this->name = copyObj.name;
	this->filepath = copyObj.filepath;
	this->dataLock = new mLock();
	this->buildingID = copyObj.buildingID;

	this->meshes.clear();
	for (unsigned int i = 0; i < copyObj.meshes.size(); ++i)
	{
		this->meshes.push_back(copyObj.meshes[i]);
	}

	this->trans = copyObj.trans;
}

pipeObject::pipeObject(string n, string file) : 
name(n), filepath(file), dataLock(new mLock()), buildingID(0),
meshes(vector<unsigned int>())
{}

pipeObject::pipeObject() : dataLock(new mLock()), buildingID(0),
meshes(vector<unsigned int>())
{}

pipeObject::~pipeObject() 
{
	delete dataLock;
}

gameObjectType pipeObject::getType() 
{
	return gameObjectType::Object_Render;
}

string pipeObject::getName() 
{
	string r = "";
	dataLock->acquire();
	r = name;
	dataLock->release();

	return r;
}

void pipeObject::setName(string n) 
{
	dataLock->acquire();
	name = n;
	dataLock->release();
}

void pipeObject::load() 
{
	objectDefLoader loader("GameData/" + filepath);
	int nouse;
	loader.load(meshes, trans,
		nouse, nouse, nouse,
		nouse, nouse, nouse);
}

void pipeObject::reload() 
{
	load();
}

void pipeObject::render(glm::mat4 baseMat, tWindow *_window)
{
	// render
	baseMat = baseMat * trans;
	for (int i = 0; i < meshes.size(); ++i)
	{
		_window->renderMesh(baseMat, meshes[i], true);
	}
}

void pipeObject::onSave(std::string savePath) 
{
}

void pipeObject::onLoad(std::string savePath) 
{
}

void pipeObject::upgrade() {}

bool pipeObject::checkupgrade()
{
	return false;
}