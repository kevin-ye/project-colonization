#pragma once

#include <mutex>

class mLock
{	
	std::mutex *_lock;
public:
	mLock();
	~mLock();
	
	void acquire();
	void release();
	int tryacquire();
};