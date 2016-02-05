#include "gameObject.hpp"

#include <cstdlib>
#include <cstdio>

using namespace std;

gameObject::gameObject() : oid(0) {}

gameObject::~gameObject() {}

void gameObject::setoid(unsigned int id)
{
	oid = id;
}

unsigned int gameObject::getoid()
{
	return oid;
}