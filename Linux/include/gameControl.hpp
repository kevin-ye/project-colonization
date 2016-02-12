#pragma once

#include <cstdlib>
#include <cstdio>
#include <thread>
#include <queue>

#include "gameEvent.hpp"

class gameControl
{
	class gWindow *_window;
	class gameData *_data;

	std::queue<gameEvent> eventQueue;

	std::thread *_thread;
	class mLock *eventqueue_lock;

	static void main(class gameControl *_control);

public:
	gameControl();
	~gameControl();

	void run();
	void init();

	void setgWindow(class gWindow *_w);
	class gWindow *getgWindow();
	void setgameData(class gameData *_d);
	class gameData *getgameData();

	void pushEvent(gameEvent _e);
	gameEvent frontEvent();
	gameEvent backEvent();
	gameEvent popEvent();
	size_t getEventCount();

	void toWindow(gameEvent _e);
	void toData(gameEvent _e);
};