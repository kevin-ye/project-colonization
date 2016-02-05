#include "surfaceObject.hpp"

#include <cstdlib>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "gameObject.hpp"
#include "mLock.hpp"
#include "Debug.hpp"
#include "gWindow.hpp"
#include "objectDefLoader.hpp"

using namespace std;
using namespace glm;

surfaceObject::surfaceObject(string n, string file) : 
name(n), filepath(file), dataLock(new mLock()),
meshes(vector<unsigned int>())
{}

surfaceObject::~surfaceObject() 
{
	delete dataLock;
}

gameObjectType surfaceObject::getType() 
{
	return gameObjectType::Object_Render;
}

string surfaceObject::getName() 
{
	string r = "";
	dataLock->acquire();
	r = name;
	dataLock->release();

	return r;
}

void surfaceObject::setName(string n) 
{
	dataLock->acquire();
	name = n;
	dataLock->release();
}

void surfaceObject::load() 
{
	objectDefLoader loader("GameData/" + filepath);
	int nouse;
	loader.load(meshes, trans, 
		nouse, nouse, nouse,
		nouse, nouse, nouse);
}

void surfaceObject::reload() 
{
	load();
}

void surfaceObject::render(glm::mat4 baseMat, tWindow *_window)
{
	// render
	baseMat = baseMat * trans;
	for (unsigned int i = 0; i < meshes.size(); ++i)
	{
		_window->renderMesh(baseMat, meshes[i], false);
	}
}

void surfaceObject::onSave(std::string savePath) {}

void surfaceObject::onLoad(std::string savePath) {}

void surfaceObject::upgrade() {}

bool surfaceObject::checkupgrade()
{
	return false;
}