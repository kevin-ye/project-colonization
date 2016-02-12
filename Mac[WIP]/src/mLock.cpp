#include "mLock.hpp"

#include <cstdlib>
#include <cstdio>
#include <mutex>

using namespace std;

mLock::mLock():
_lock(new mutex())
{}

mLock::~mLock() {
	delete _lock;
}

void mLock::acquire()
{
	_lock->lock();
}

void mLock::release()
{
	_lock->unlock();
}

int mLock::tryacquire()
{
	return _lock->try_lock();
}