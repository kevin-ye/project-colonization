#include "objectDefLoader.hpp"

#include <cstdlib>
#include <cstdio>
#include <cctype>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "game-framework/MathUtils.hpp"

#include "gameObject.hpp"
#include "INIReader.hpp"
#include "mLock.hpp"
#include "Debug.hpp"

using namespace std;
using namespace glm;

objectDefLoader::objectDefLoader(string file) : filepath(file) {}

objectDefLoader::~objectDefLoader() {}

void objectDefLoader::load(vector<unsigned int> &meshes, mat4 &trans,
		int &neededEnergy, int &neededWater, int &neededOre,
		int &runningEnergy, int &runningWater, int &runningOre)
{
	meshes.clear();
	trans = mat4(1);
	INIReader reader(filepath);

	// load meshes
	int MeshCount = reader.GetInteger("common", "MeshCount", 0);
	for (int i = 0; i < MeshCount; ++i)
	{
		meshes.push_back(reader.GetInteger("Meshes", "m" + std::to_string(i), 0));
	}

	// load transformation
	int transCount = reader.GetInteger("common", "transCount", 0);
	for (int i = 0; i < transCount; ++i)
	{
		string opearation = reader.Get("TransOP", "t" + std::to_string(i), "none");
		float argv1 = reader.GetReal("TransArgv", "t" + std::to_string(i), 0);
		if (opearation != "none")
		{
			char op = toupper(opearation[0]);
			char argv0 = toupper(opearation[1]);
			
			switch (op) {
				case 'T' :
					if (argv0 == 'X')
					{
						trans = glm::translate(mat4(1), vec3(argv1, 0, 0)) * trans;
					} else if (argv0 == 'Y')
					{
						trans = glm::translate(mat4(1), vec3(0, argv1, 0)) * trans;
					} else if (argv0 == 'Z')
					{
						trans = glm::translate(mat4(1), vec3(0, 0, argv1)) * trans;
					} 
					break;
				case 'R' :
					if (argv0 == 'X')
					{
						trans = glm::rotate(mat4(1), degreesToRadians(argv1), vec3(1, 0, 0)) * trans;
					} else if (argv0 == 'Y')
					{
						trans = glm::rotate(mat4(1), degreesToRadians(argv1), vec3(0, 1, 0)) * trans;
					} else if (argv0 == 'Z')
					{
						trans = glm::rotate(mat4(1), degreesToRadians(argv1), vec3(0, 0, 1)) * trans;
					} 
					break;
				case 'S' :
					if (argv0 == 'X')
					{
						trans = glm::scale(mat4(1), vec3(argv1, 1, 1)) * trans;
					} else if (argv0 == 'Y')
					{
						trans = glm::scale(mat4(1), vec3(1, argv1, 1)) * trans;
					} else if (argv0 == 'Z')
					{
						trans = glm::scale(mat4(1), vec3(1, 1, argv1)) * trans;
					} 
					break;
				default:
					break;
			}
		}
	}

	// load building settings
	neededEnergy = reader.GetInteger("Settings", "neededEnergy", 0);
	neededWater = reader.GetInteger("Settings", "neededWater", 0);
	neededOre = reader.GetInteger("Settings", "neededOre", 0);
	runningEnergy = reader.GetInteger("Settings", "runningEnergy", 0);
	runningWater = reader.GetInteger("Settings", "runningWater", 0);
	runningOre = reader.GetInteger("Settings", "runningOre", 0);
}