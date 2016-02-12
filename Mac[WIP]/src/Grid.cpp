#include "Grid.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "game-framework/MathUtils.hpp"

#include "INIReader.hpp"
#include "gameObject.hpp"
#include "mLock.hpp"
#include "Debug.hpp"
#include "gWindow.hpp"
#include "common.hpp"
#include "buildingObject.hpp"
#include "gameData.hpp"
#include "pipeObject.hpp"
#include "coreObject.hpp"

using namespace std;
using namespace glm;

Grid::Grid(unsigned int s) 
: gsize(s), _grid(NULL), dataLock(new mLock()), _data(NULL), unitpipe(NULL),
energy(200), water(200), ore(200),
energyCap(99999), waterCap(99999), oreCap(99999)
{
	// allocate _grid
	_grid = new cell*[gsize + gsize + 1];
	for (int i = 0; i < (gsize + gsize + 1); ++i)
	{
		_grid[i] = new cell[gsize + gsize + 1];
	}

	for (int i = 0; i < (gsize + gsize + 1); ++i)
	{
		for (int j = 0; j < (gsize + gsize + 1); ++j)
		{
			_grid[i][j].gx = i;
			_grid[i][j].gy = j;
		}
	}
}

Grid::~Grid() 
{
	for (int i = 0; i < (gsize + gsize + 1); ++i)
	{
		delete [] _grid[i];
	}

	delete [] _grid;
	delete dataLock;
}

gameObject *Grid::getObject(int x, int y)
{
	x = x + gsize;
	y = y + gsize;

	gameObject *r = NULL;


	dataLock->acquire();

	r = _grid[x][y].holding;

	dataLock->release();

	return r;
}

bool Grid::setObject(int x, int y, class gameObject *_o)
{
	x = x + gsize;
	y = y + gsize;

	bool ret = false;

	dataLock->acquire();

	if (_grid[x][y].holding == NULL)
	{
		_grid[x][y].holding = _o;
		if ((x - gsize == 0) && (y - gsize == 0))
		{
			_core = _o;
		} else {
			// find a cloest building to link to
			float mindst = -2;
			int m2x = gsize;
			int m2y = gsize;
			for (int l2x = 0; l2x < (gsize + gsize + 1); ++l2x)
			{
				for (int l2y = 0; l2y < (gsize + gsize + 1); ++l2y)
				{
					if ((_grid[l2x][l2y].holding == NULL) || ((l2x == x) && (l2y == y)))
					{
						continue;
					}
					float c1x = float(x) - float(gsize);
					float c1y = float(y) - float(gsize);
					float c2x = float(l2x) - float(gsize);
					float c2y = float(l2y) - float(gsize);

					vec3 direction = vec3(c1x, 0, c1y) - vec3(c2x, 0, c2y);
					float length = glm::length(direction);
					if ((mindst <= -1) || (length < mindst))
					{
						mindst = length;
						m2x = l2x;
						m2y = l2y;
					}
				}
			}
			_grid[x][y].link.push_back(&(_grid[m2x][m2y]));
			_grid[m2x][m2y].link.push_back(&(_grid[x][y]));
		}
		ret = true;
	} else {
		ret = false;
	}

	dataLock->release();

	return ret;
}

void Grid::render(glm::mat4 baseMat, tWindow *_window)
{
	// render surface mesh
	dataLock->acquire();
	surface->render(baseMat, _window);
	// render grid 
	_window->renderGrid(false);
	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			// render buildings
			if (_grid[x][y].holding != NULL)
			{
				gameObject *obj = _grid[x][y].holding;
				if (obj->getType() == gameObjectType::Object_Render)
				{
					// render-able object
					mat4 passingbaseMat = baseMat * glm::translate(
						vec3(cubeSize * (float(x) - float(gsize)), 
						0, 
						cubeSize * (float(y) - float(gsize))));
					obj->render(passingbaseMat, _window);

				}
			}
			// find a cloest building to link to

			// render pipes
			if (_grid[x][y].link.size() != 0) 
			{
				// link with pipes

				for (int i = 0; i < _grid[x][y].link.size(); ++i)
				{
					float c1x = float(x) - float(gsize);
					float c1y = float(y) - float(gsize);
					float c2x = float(_grid[x][y].link[i]->gx) - float(gsize);
					float c2y = float(_grid[x][y].link[i]->gy) - float(gsize);

					vec3 direction = vec3(c1x, 0, c1y) - vec3(c2x, 0, c2y);

					float length = glm::length(direction);
					float angle = getAngle(vec3(1, 0, 0), direction);
					float dotp = dot(vec3(0, 0, 1), direction);
					//cout << angle << "," << dotp << endl;

					if (dotp <= 0) {
						angle = angle + degreesToRadians(180.0f);
					} else {
						continue;
					}

					for (int t = 0; t <= int(length + 0.5); ++t)
					{
						mat4 pipemat = mat4(1);
						
						if (t == 0)
						{
							pipemat = baseMat * 
							glm::translate(
								vec3(cubeSize * (float(c1x)), 
								0, 
								cubeSize * (float(c1y)))) * 
							glm::rotate(mat4(1), angle, vec3(0, 1, 0));

							pipemat = pipemat * glm::translate(
							vec3(cubeSize * float(0.25), 
							0, 
							0)) * glm::scale(vec3(0.5, 1, 1));
							
						} else if (t == int(length + 0.5)){
							pipemat = baseMat * 
							glm::translate(
								vec3(cubeSize * (float(c1x)), 
								0, 
								cubeSize * (float(c1y)))) * 
							glm::rotate(mat4(1), angle, vec3(0, 1, 0));

							pipemat = pipemat * glm::translate(
							vec3(cubeSize * float(t - 0.25), 
							0, 
							0)) * glm::scale(vec3(0.5, 1, 1));

						} else {
							pipemat = baseMat * 
							glm::translate(
								vec3(cubeSize * (float(c1x)), 
								0, 
								cubeSize * (float(c1y)))) * 
							glm::rotate(mat4(1), angle, vec3(0, 1, 0));

							pipemat = pipemat * glm::translate(
							vec3(cubeSize * float(t), 
							0, 
							0));

						}
						/*
						pipemat = baseMat * 
							glm::translate(
								vec3(cubeSize * (float(c1x)), 
								0, 
								cubeSize * (float(c1y)))) * 
							glm::rotate(mat4(1), angle, vec3(0, 1, 0));

							pipemat = pipemat * glm::translate(
							vec3(cubeSize * float(t), 
							0, 
							0));*/

						unitpipe->render(pipemat, _window);
					}
				}
			}
		}
	}
	dataLock->release();
}

void Grid::setunitpipe(pipeObject *pipe)
{
	unitpipe = pipe;
}

void Grid::setgameData(gameData *_d)
{
	_data = _d;
}

void Grid::setSurface(class gameObject *_o)
{
	surface = _o;
}

unsigned int Grid::getGsize()
{
	return gsize;
}

void Grid::onSave(std::string savePath) 
{
	ofstream savefile;
	savefile.open(savePath, ios::app);
	vector<gameObject*> savequeue;
	savequeue.clear();
	savefile << "[Grid]" << endl;
	savefile << "energy=" << energy << endl;
	savefile << "water=" << water << endl;
	savefile << "ore=" << ore << endl;
	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			if (_grid[x][y].holding != NULL)
			{
				gameObject *obj = _grid[x][y].holding;
				if (_core != obj)
				{
					savefile << x << "," << y << "=" << obj->getoid() << endl;
					savequeue.push_back(obj);
				} else {
					savefile << x << "," << y << "=NULL" << endl;
					savequeue.push_back(obj);
				}
			} else {
				savefile << x << "," << y << "=NULL" << endl;
			}
		}
	}
	// save link
	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			savefile << "[GridLink" << x <<"_"<< y<<"]" << endl;
			savefile << "LinkCount=" << _grid[x][y].link.size() << endl;
			for (int i = 0; i < _grid[x][y].link.size(); ++i)
			{
				savefile << i << "x=" << (_grid[x][y].link[i])->gx << endl;
				savefile << i << "y=" << (_grid[x][y].link[i])->gy << endl;
			}
		}
	}
	for (int i = 0; i < savequeue.size(); ++i)
	{
		savequeue[i]->onSave(savePath);
	}

	savefile.close();
}

void Grid::onLoad(std::string savePath) 
{
	INIReader reader(savePath);
	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			string oidcheck = reader.Get("Grid", std::to_string(x) + "," + std::to_string(y), "NULL");
			if (oidcheck != "NULL")
			{
				unsigned int oid = reader.GetInteger("Grid", std::to_string(x) + "," + std::to_string(y), 0);
				// recreate this building
				string buildingCheck = reader.Get(oidcheck, "BuildingInfo", "NOINFO");
				if (buildingCheck != "BUILDING")
				{
					if (_grid[x][y].holding != _core)
					{
						_grid[x][y].holding = NULL;
					} else {
						_core->onLoad(savePath);
					}
				}
				buildingObject *recreate = new buildingObject();
				recreate->setoid(oid);
				recreate->onLoad(savePath);

				_grid[x][y].holding = recreate;
				_data->addgameObject(recreate);
			} else {
				if (_grid[x][y].holding != _core)
				{
					_grid[x][y].holding = NULL;
				}
			}
		}
	}
	// load link
	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			_grid[x][y].link.clear();
			int LinkCount = reader.GetInteger("GridLink" + std::to_string(x) + "_" + std::to_string(y), 
				"LinkCount", 0);
			for (int i = 0; i < LinkCount; ++i)
			{
				int rx = reader.GetInteger("GridLink" + std::to_string(x) + "_" + std::to_string(y), 
				std::to_string(i) + "x", 0);
				int ry = reader.GetInteger("GridLink" + std::to_string(x) + "_" + std::to_string(y), 
				std::to_string(i) + "y", 0);
				_grid[x][y].link.push_back(&(_grid[rx][ry]));
			}
		}
	}

	energy = reader.GetInteger("Grid", "energy", energy);
	water = reader.GetInteger("Grid", "water", water);
	ore = reader.GetInteger("Grid", "ore", ore);
}

float Grid::getAngle(vec3 v1, vec3 v2)
{

	return acos(dot(v1,v2) / (length(v1) * length(v2)));
}

void Grid::upgrade(int x, int y)
{
	x = x + gsize;
	y = y + gsize;

	if (_grid[x][y].holding != NULL)
	{
		if (_grid[x][y].holding->checkupgrade())
		{
			_grid[x][y].holding->upgrade();
			if (((x - gsize) != 0) && ((y - gsize) != 0))
			{
				buildingObject *bobj = (buildingObject*)(_grid[x][y].holding);
				reduceResource(bobj->neededEnergy * 2, bobj->neededWater * 2, bobj->neededOre * 2);
			} else {
				coreObject *bobj = (coreObject*)(_grid[x][y].holding);
				reduceResource(bobj->neededEnergy * 2, bobj->neededWater * 2, bobj->neededOre * 2);
			}
		}
	}
}

bool Grid::checkupgrade(int x, int y)
{
	x = x + gsize;
	y = y + gsize;

	if (_grid[x][y].holding != NULL)
	{
		return (_grid[x][y].holding->checkupgrade());
	} else {
		return false;
	}
}

bool Grid::checkbuild(int neededenergy, int neededwater, int neededore)
{
	bool flag = false;
	dataLock->acquire();

	flag = (neededenergy <= energy) && (neededwater <= water) && (neededore <= ore);

	dataLock->release();
	return flag;
}

void Grid::reduceResource(int neededenergy, int neededwater, int neededore)
{
	bool flag = false;
	dataLock->acquire();

	energy -= neededenergy;
	water -= neededwater;
	ore -= neededore;

	dataLock->release();
}

int Grid::getenergyCap()
{
	return energyCap;
}

int Grid::getwaterCap()
{
	return waterCap;
}

int Grid::getoreCap()
{
	return oreCap;
}

void Grid::resourceTick()
{
	dataLock->acquire();

	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			if ((x - gsize == 0) && (y - gsize == 0))
			{
				if (_grid[x][y].holding != NULL)
				{
					coreObject *obj = (coreObject*)(_grid[x][y].holding); 
					bool applying = true;
					applying = applying &&
					((energy - obj->runningEnergy) >= 0) && ((energy - obj->runningEnergy) <= energyCap) &&
					((water - obj->runningWater) >= 0) && ((water - obj->runningWater) <= waterCap) &&
					((ore - obj->runningOre) >= 0) && ((ore - obj->runningOre) <= oreCap);
					if (!applying)
					{
						continue;
					}
					energy -= obj->runningEnergy;
					water -= obj->runningWater;
					ore -= obj->runningOre;
				}
			} else {
				if (_grid[x][y].holding != NULL)
				{
					buildingObject *obj = (buildingObject*)(_grid[x][y].holding); 
					bool applying = true;
					applying = applying &&
					((energy - obj->runningEnergy) >= 0) && ((energy - obj->runningEnergy) <= energyCap) &&
					((water - obj->runningWater) >= 0) && ((water - obj->runningWater) <= waterCap) &&
					((ore - obj->runningOre) >= 0) && ((ore - obj->runningOre) <= oreCap);
					if (!applying)
					{
						continue;
					}
					energy -= obj->runningEnergy;
					water -= obj->runningWater;
					ore -= obj->runningOre;
				}
			}
		}

	}

	dataLock->release();
}