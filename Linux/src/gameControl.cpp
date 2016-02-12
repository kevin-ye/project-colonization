#include "gameControl.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>

#include "mLock.hpp"
#include "gWindow.hpp"
#include "gameData.hpp"
#include "gameEvent.hpp"
#include "Debug.hpp"
#include "common.hpp"

using namespace std;

gameControl::gameControl(): eventqueue_lock(new mLock())
{
	// block all access to gameControl thread
	// until init is finished 
	init();
}

gameControl::~gameControl() {
	if ((_thread != NULL) && (_thread->joinable()))
	{
		_thread->join();
	}

	delete _thread;
	delete eventqueue_lock;
}

void gameControl::run()
{	
	_thread = new thread(gameControl::main, this);
}

// thread entry
void gameControl::main(gameControl *_control)
{
	mLock *ready_lock = new mLock();
	ready_lock->acquire();

	// create gWindow
	gWindow *_window = new gWindow(ready_lock);
	// create gameData
	gameData *_data = new gameData(ready_lock);
	_window->setController(_control);
	_data->setController(_control);

	_control->setgWindow(_window);
	_control->setgameData(_data);

	Debug("gameControl thread running..." << endl;);
	bool runFlag = true;
	auto start_time = std::chrono::high_resolution_clock::now();

	ready_lock->release();
	while (runFlag) {
		auto end_time = std::chrono::high_resolution_clock::now();
		auto timeinterval = end_time - start_time;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(timeinterval).count() >= Tick)
		{
			start_time = std::chrono::high_resolution_clock::now();
			gameEvent _tick;
			_tick._type = gameEventType::Event_TimeTick;
			_window->pushEvent(_tick);
		}
		if (_control->getEventCount() != 0)
		{
			gameEvent _e = _control->popEvent();
			switch (_e._type) 
			{
				case gameEventType::Event_None :
					break;
				case gameEventType::Event_Shutdown:
				_control->toData(_e);
					runFlag = false;
					break;
				default:
					break;
			}
		}
	}
	Debug("gameControl thread terminating..." << endl;);
	delete _window;
	delete _data;
	delete ready_lock;
	Debug("gameControl thread terminated..." << endl;);
}

void gameControl::init()
{
	_window = NULL;
	_data = NULL;
}

void gameControl::setgWindow(class gWindow *_w)
{
	_window = _w;
}

gWindow *gameControl::getgWindow()
{
	return _window;
}

void gameControl::setgameData(class gameData *_d)
{
	_data = _d;
}

gameData *gameControl::getgameData()
{
	return _data;
}

void gameControl::pushEvent(gameEvent _e)
{
	eventqueue_lock->acquire();

	eventQueue.push(_e);

	eventqueue_lock->release();
}

gameEvent gameControl::frontEvent()
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

gameEvent gameControl::backEvent()
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

gameEvent gameControl::popEvent()
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

size_t gameControl::getEventCount()
{
	return eventQueue.size();
}

void gameControl::toWindow(gameEvent _e)
{
	_window->pushEvent(_e);
}

void gameControl::toData(gameEvent _e)
{
	_data->pushEvent(_e);
}