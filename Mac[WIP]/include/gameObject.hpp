#pragma once

#include <glm/glm.hpp>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>

enum class gameObjectType {
	Object_None,
	Object_Mesh,
	Object_Render
};

class gameObject
{
protected:
	unsigned int oid;
public:
	gameObject();
	virtual ~gameObject() = 0;

	virtual gameObjectType getType() = 0;

	virtual std::string getName() = 0;
	virtual void setName(std::string n) = 0;

	virtual void load() = 0;
	virtual void reload() = 0;
	virtual void render(glm::mat4 baseMat, class tWindow *_window) = 0;

	void setoid(unsigned int id);
	unsigned int getoid();

	virtual void onLoad(std::string) = 0;
	virtual void onSave(std::string) = 0;

	virtual void upgrade() = 0;
	virtual bool checkupgrade() = 0;
};