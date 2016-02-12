#include "gameData.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <thread>
#include <queue>

#include "mLock.hpp"
#include "Debug.hpp"
#include "gameControl.hpp"
#include "gameObject.hpp"
#include "gameDataLoader.hpp"
#include "gameEvent.hpp"
#include "Grid.hpp"
#include "INIReader.hpp"
#include "coreObject.hpp"
#include "surfaceObject.hpp"
#include "buildingObject.hpp"
#include "gWindow.hpp"
#include "pipeObject.hpp"
#include "selectbarObject.hpp"

using namespace std;

const string savePath = "Save/save.ini";

vector<buildingObject*> gameData::OriginBuildings;

gameData::gameData(mLock *main_thread_ready_lock) 
: _control(NULL), objects(vector<gameObject*>()), obj_lock(new mLock()),
loadingProgress(0), eventqueue_lock(new mLock()),
_grid(NULL),
eventQueue(queue<gameEvent>())
{
	init();
	run(main_thread_ready_lock);
}

gameData::~gameData() 
{
	if ((_thread != NULL) && (_thread->joinable()))
	{
		_thread->join();
	}

	delete _thread;	
	delete obj_lock;
	delete eventqueue_lock;
	delete _grid;
}

void gameData::init() 
{
	objects.clear();
}

void gameData::run(class mLock *main_thread_ready_lock)
{
	_thread = new thread(gameData::main, this, main_thread_ready_lock);
}

void gameData::main(gameData *_data, mLock *main_thread_ready_lock)
{
	// ready check
	main_thread_ready_lock->acquire();
	main_thread_ready_lock->release();
	//
	Debug("gameData thread running..." << endl;);

	// load GameData
	OriginBuildings.clear();
	float progress = 0;
	_data->setLoadingProgress(progress);
	gameDataLoader loader(_data);
	unsigned int gsize = 0;
	loader.load(gsize, OriginBuildings);

	// core, at least
	INIReader reader("GameData/define.ini");
	string coreFilepath = reader.Get("Objects", "core", "NOTFOUND");
	coreObject *core = new coreObject("core", coreFilepath);
	core->load();
	_data->addgameObject(core);
	// surface
	string surfaceFilepath = reader.Get("Objects", "surface", "NOTFOUND");
	surfaceObject *surface = new surfaceObject("surface", surfaceFilepath);
	surface->load();
	_data->addgameObject(surface);
	// pipe object
	string pipeFilepath = reader.Get("Objects", "pipe", "NOTFOUND");
	pipeObject *pipe = new pipeObject("pipe", pipeFilepath);
	pipe->load();
	// selectbar
	string selectbarFilepath = reader.Get("Objects", "selectbar", "NOTFOUND");
	selectbarObject *bar = new selectbarObject("selectbar", selectbarFilepath);
	bar->load();
	unsigned int barid = _data->addgameObject(bar);
	// build Grid
	Grid *_grid = new Grid(gsize);
	_grid->setgameData(_data);
	_grid->setunitpipe(pipe);
	_grid->setSurface(surface);
	_grid->setObject(0, 0, core);
	_data->setGrid(_grid);
	_data->getController()->getgWindow()->setSelectBar(bar);
	// load save
	_data->onLoad();
	bool runFlag = true;
	gameEvent finishedLoad;
	finishedLoad._type = gameEventType::Event_FinishedDataLoading;
	_data->getController()->toWindow(finishedLoad);
	int x = 0;
	int y = 0;
	unsigned int bid = 0;
	buildingObject *newbuilding = NULL;
	while (runFlag) {
		if (_data->getEventCount() != 0)
		{
			gameEvent _e = _data->popEvent();
			switch (_e._type) 
			{
				case gameEventType::Event_None :
					break;
				case gameEventType::Event_Shutdown:
					runFlag = false;
					break;
				case gameEventType::Event_setBuilding:
					x = _e.info[0];
					y = _e.info[1];
					bid = _e.info[2];
					if (_grid->checkbuild(OriginBuildings[bid]->neededEnergy, OriginBuildings[bid]->neededWater, OriginBuildings[bid]->neededOre))
					{
						newbuilding = new buildingObject(*OriginBuildings[bid]);
						if (!(_grid->setObject(x, y, newbuilding))) {
							delete newbuilding;
						} else {
							_data->addgameObject(newbuilding);
						}
						// reduce resource
						_grid->reduceResource(newbuilding->neededEnergy, newbuilding->neededWater, newbuilding->neededOre);
						// send a build suc event
						gameEvent buildsuc;
						buildsuc._type = gameEventType::Event_BuildSuc;
						_data->getController()->toWindow(buildsuc);
					} else {
						// send a build failed event
						gameEvent buildfailed;
						buildfailed._type = gameEventType::Event_BuildFailed;
						_data->getController()->toWindow(buildfailed);
					}
					break;
				default:
					break;
			}
		}
	}

	Debug("gameData thread shutting down..." << endl;);
}

void gameData::setController(class gameControl *ctrl)
{
	_control = ctrl;
}

gameControl *gameData::getController()
{
	return _control;
}

unsigned int gameData::addgameObject(class gameObject *_o)
{
	unsigned int idx = 0;
	obj_lock->acquire();

	idx = objects.size();
	objects.push_back(_o);
	_o->setoid(idx);

	obj_lock->release();

	return idx;
}

gameObject *gameData::getgameObject(unsigned int idx)
{
	gameObject *ret = NULL;
	obj_lock->acquire();

	if (idx < objects.size())
	{
		ret = objects[idx];
	}

	obj_lock->release();

	return ret;
}

gameObject *gameData::removegameObject(unsigned int idx)
{
	obj_lock->acquire();

	if (idx < objects.size())
	{
		gameObject *ptr = objects[idx];
		objects.erase(objects.begin() + idx);
		delete ptr;
	}

	obj_lock->release();
}

float gameData::getLoadingProgress()
{
	return loadingProgress;
}

void gameData::setLoadingProgress(float p)
{
	// update progress to window

	gameEvent _e;
	_e._type = gameEventType::Event_MeshLoadingProgressUpdate;
	_e.info.clear();
	_e.info.push_back(p);

	_control->toWindow(_e);
}

void gameData::pushEvent(gameEvent _e)
{
	eventqueue_lock->acquire();

	eventQueue.push(_e);

	eventqueue_lock->release();
}

gameEvent gameData::frontEvent()
{
	eventqueue_lock->acquire();
	gameEvent _r;
	if (eventQueue.size() == 0)
	{
		_r._type = gameEventType::Event_None;	
	} else {
		_r = eventQueue.front();
	}
	eventqueue_lock->release();

	return _r;
}

gameEvent gameData::backEvent()
{
	eventqueue_lock->acquire();
	gameEvent _r;
	if (eventQueue.size() == 0)
	{
		_r._type = gameEventType::Event_None;	
	} else {
		_r = eventQueue.back();
	}
	eventqueue_lock->release();

	return _r;
}

gameEvent gameData::popEvent()
{
	eventqueue_lock->acquire();
	gameEvent _r;
	if (eventQueue.size() == 0)
	{
		_r._type = gameEventType::Event_None;	
	} else {
		_r = eventQueue.front();
		eventQueue.pop();
	}
	eventqueue_lock->release();

	return _r;
}

size_t gameData::getEventCount()
{
	return eventQueue.size();
}

Grid *gameData::getGrid()
{
	return _grid;
}

void gameData::setGrid(class Grid *_g)
{
	_grid = _g;
}

void gameData::onSave()
{
	// flush old save
	ofstream savefile;
	savefile.open(savePath);
	savefile.close();
	// save game data state
	// save window's state
	_control->getgWindow()->onSave(savePath);
	// save grid
	_grid->onSave(savePath);
}

void gameData::onLoad()
{
	// load game data state
	// load window's state
	_control->getgWindow()->onLoad(savePath);
	// load grid
	_grid->onLoad(savePath);
}