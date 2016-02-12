#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include "gameDataLoader.hpp"
#include "gameData.hpp"
#include "INIReader.hpp"
#include "gameObject.hpp"
#include "meshObject.hpp"
#include "gameControl.hpp"
#include "gameEvent.hpp"
#include "buildingObject.hpp"

using namespace std;
using namespace glm;

const int defaultGsize = 10; 

gameDataLoader::gameDataLoader(gameData *_d)
:_data(_d)
{}

gameDataLoader::~gameDataLoader() {}

void gameDataLoader::load(unsigned int &gsize, vector<buildingObject*> &blist)
{
	INIReader reader("GameData/define.ini");
	int MeshCount = reader.GetInteger("common", "MeshCount", 0);
	gsize = reader.GetInteger("common", "GridSize", defaultGsize);

	float totalprogress = 29;
	vector<unsigned int> loadingMeshObjID;
	vector<vec3> loadingMeshObjID_ks;
	vector<double> loadingMeshObjID_shininess;
	loadingMeshObjID.clear();
	// load meshObjects
	for (int i = 0; i < MeshCount; ++i)
	{
		string filepath = reader.Get("Meshes", "m" + std::to_string(i), "NOTFOUND");
		if (filepath == "NOTFOUND")
		{
			continue;
		}
		double r = reader.GetReal("MaterialKS", "m" + std::to_string(i) + "r", 0);
		double g = reader.GetReal("MaterialKS", "m" + std::to_string(i) + "g", 0);
		double b = reader.GetReal("MaterialKS", "m" + std::to_string(i) + "b", 0);
		loadingMeshObjID_ks.push_back(vec3(r, g, b));
		double s = reader.GetReal("MaterialShininess", "m" + std::to_string(i), 0);
		loadingMeshObjID_shininess.push_back(s);
		filepath = "GameData/" + filepath;
		meshObject *newobj = new meshObject(getFileName(filepath), filepath);
		newobj->load();
		unsigned int id = _data->addgameObject(newobj);
		newobj->setID(id);
		loadingMeshObjID.push_back(id);

		_data->setLoadingProgress(float(i) / float(MeshCount) * totalprogress);
	}

	_data->setLoadingProgress(30);

	// load buildings
	totalprogress = 19;
	blist.clear();
	int buildingCount = reader.GetInteger("common", "BuildingCount", 0);
	for (int i = 0; i < buildingCount; ++i)
	{
		string filepath = reader.Get("Buildings", "b" + std::to_string(i), "NOTFOUND");
		if (filepath == "NOTFOUND")
		{
			continue;
		}
		buildingObject *newobj = new buildingObject(getFileName(filepath), filepath);
		newobj->load();
		blist.push_back(newobj);

		_data->setLoadingProgress(30 + float(i) / float(MeshCount) * totalprogress);
	}

	// upload meshObjects
	for (unsigned int i = 0; i < loadingMeshObjID.size(); ++i)
	{
		gameEvent uploadEvent;
		uploadEvent._type = gameEventType::Event_MeshUpload;
		uploadEvent.info.clear();
		uploadEvent.info.push_back(loadingMeshObjID[i]);
		uploadEvent.info.push_back(loadingMeshObjID_ks[i].x);
		uploadEvent.info.push_back(loadingMeshObjID_ks[i].y);
		uploadEvent.info.push_back(loadingMeshObjID_ks[i].z);
		uploadEvent.info.push_back(loadingMeshObjID_shininess[i]);

		// send an event to let window upload mesh to GPU
		_data->getController()->toWindow(uploadEvent);

		_data->setLoadingProgress(50 + float(i) / float(loadingMeshObjID.size()) * totalprogress);
	}

	_data->setLoadingProgress(100);

}

string gameDataLoader::getFileName(string filepath)
{
	string r = "";

	unsigned int endi = filepath.size() - 1;
	while ((endi > 0) && (filepath[endi] == '.')) endi--;

	unsigned int i = endi;
	while ((i > 0) && (filepath[i] != '/')) i--;
	
	if (filepath[i] == '/')
	{
		r = filepath.substr(i + 1, endi - 1);
	} else {
		r = filepath.substr(i, endi - 1);
	}

	return r;
}