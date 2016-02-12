#pragma once

#include <cstdlib>
#include <cstdio>
#include <thread>
#include <vector>
#include <queue>

#include "gameEvent.hpp"

class gameData
{
	class gameControl *_control;
	std::thread *_thread;
	class Grid *_grid;
	class gameObject *_core;

	class mLock *obj_lock;
	std::vector<class gameObject*> objects;

	class mLock *pro_lock;
	float loadingProgress;

	class mLock *eventqueue_lock;
	std::queue<gameEvent> eventQueue;

	static void main(class gameData *_data, class mLock *main_thread_ready_lock);

	void run(class mLock *main_thread_ready_lock);
	void init();

public:
	gameData(class mLock *main_thread_ready_lock);
	~gameData();
	
	void setController(class gameControl *ctrl);
	class gameControl *getController();

	unsigned int addgameObject(class gameObject *_o);
	class gameObject *getgameObject(unsigned int idx);
	class gameObject *removegameObject(unsigned int idx);

	float getLoadingProgress();
	void setLoadingProgress(float p);

	void pushEvent(gameEvent _e);
	gameEvent frontEvent();
	gameEvent backEvent();
	gameEvent popEvent();
	size_t getEventCount();

	class Grid *getGrid();
	void setGrid(class Grid *_g);

	void onSave();
	void onLoad();

	static std::vector<class buildingObject*> OriginBuildings;
};