#include "meshObject.hpp"

#include <cstdlib>
#include <cstdio>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "gameObject.hpp"
#include "gWindow.hpp"
#include "mLock.hpp"
#include "Debug.hpp"

using namespace std;
using namespace glm;

meshObject::meshObject(std::string n, std::string file)
:name(n), filepath(file), dataLock(new mLock()), id(0),
VertexPositions(vector<vec3>()),
VertexNormals(vector<vec3>()),
UVPositions(vector<vec2>())
{}

meshObject::~meshObject() 
{
	delete dataLock;
}

gameObjectType meshObject::getType()
{
	return gameObjectType::Object_Mesh;
}

vector<vec3> meshObject::getVertexPositions() 
{
	return VertexPositions;
}

vector<vec2> meshObject::getUVPositions() 
{
	return UVPositions;
}

vector<vec3> meshObject::getVertexNormals() 
{
	return VertexNormals;
}

const float *meshObject::getVertexPositionsPtr() 
{
	return &(VertexPositions[0].x);
	//return meshConsolidator->getVertexPositionDataPtr();
}

const float *meshObject::getUVPositionsPtr() 
{
	return &(UVPositions[0].x);
	//return meshConsolidator->getVertexPositionDataPtr();
}

const float *meshObject::getVertexNormalsPtr() 
{
	return &(VertexNormals[0].x);
	//return meshConsolidator->getVertexNormalDataPtr();
}

string meshObject::getName() 
{
	string r = "";
	dataLock->acquire();
	r = name;
	dataLock->release();

	return r;
}

void meshObject::setName(std::string n) 
{
	dataLock->acquire();
	name = n;
	dataLock->release();
}

void meshObject::setID(unsigned int i)
{
	dataLock->acquire();
	id = i;
	dataLock->release();
}

void meshObject::load() 
{
	
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile( filepath, 
		aiProcess_CalcTangentSpace| 
		aiProcess_Triangulate|
		aiProcess_JoinIdenticalVertices|
        aiProcess_SortByPType);
	if (scene)
	{
		for (int i = 0; i < scene->mNumMeshes; ++i)
		{
			aiMesh *currnet_mesh = scene->mMeshes[i];
			for (int j = 0; j < (currnet_mesh->mNumFaces); ++j)
			{
				aiFace currentFace = currnet_mesh->mFaces[j];
				for (int z = 0; z < currentFace.mNumIndices; ++z)
				{
					unsigned int idx = currentFace.mIndices[z];
					vec2 uv(0, 0);
					if (currnet_mesh->HasTextureCoords(0))
					{
						uv.x = (currnet_mesh->mTextureCoords[0][idx].x);
						uv.y = (currnet_mesh->mTextureCoords[0][idx].y);
					}
					UVPositions.push_back(uv);
					// cout << name << ": " << to_string(uv) << endl;
					// load vertex
					VertexPositions.push_back(
						vec3(
						double(currnet_mesh->mVertices[idx].x),
						double(currnet_mesh->mVertices[idx].z),
						double(currnet_mesh->mVertices[idx].y))
					);
					// load normals
					VertexNormals.push_back(
						vec3(
						double(currnet_mesh->mNormals[idx].x),
						double(currnet_mesh->mNormals[idx].z),
						double(currnet_mesh->mNormals[idx].y))
					);
				}
			}
		}
	}
	
	/*
	meshConsolidator = new MeshConsolidator{
			("GameData/cube.obj")
	};
	*/
}

void meshObject::reload() 
{
	load();
}

size_t meshObject::getVertexPositionsNum()
{
	return VertexPositions.size() * sizeof(vec3);
	//return meshConsolidator->getNumVertexPositionBytes();
}

size_t meshObject::getUVPositionsNum()
{
	return UVPositions.size() * sizeof(vec3);
	//return meshConsolidator->getNumVertexPositionBytes();
}

size_t meshObject::getVertexNormalsNum()
{
	return VertexNormals.size() * sizeof(vec3);
	//return meshConsolidator->getNumVertexNormalBytes();
}

void meshObject::render(glm::mat4 baseMat, tWindow *_window)
{
	// meshObject does not render directly
}

string meshObject::getTexturePath()
{
	return "GameData/textures/" + name + ".bmp";
}

void meshObject::onSave(string savePath) {}

void meshObject::onLoad(string savePath) {}

void meshObject::upgrade() {}

bool meshObject::checkupgrade()
{
	return false;
}