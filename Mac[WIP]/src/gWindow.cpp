#include "gWindow.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <chrono>
#include <cmath>
#include <ctgmath>

#include "game-framework/GlErrorCheck.hpp"
#include "game-framework/MathUtils.hpp"
#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "INIReader.hpp"
#include "mLock.hpp"
#include "gameControl.hpp"
#include "gameData.hpp"
#include "gameEvent.hpp"
#include "Debug.hpp"
#include "meshObject.hpp"
#include "Grid.hpp"
#include "common.hpp"
#include "selectbarObject.hpp"
#include "buildingObject.hpp"
#include "gameObject.hpp"
#include "coreObject.hpp"

using namespace std;
using namespace glm;
using namespace irrklang;

const float FPS = 60;
const double daynightLightDelta = 10;
const vec3 DNlightDelta = vec3(0, 0, 0.4f/((float)FullTick / 2));
const vec3 nooncolor = vec3(1, 1, 1);
const vec3 nightcolor = vec3(0.2, 0.2, 0.2);
const float lightDistance = 100;
const vec3 olightPos = lightDistance * vec3(0.0f, 1.0f, 0.0f);
const vec3 firstlightPos = lightDistance * vec3(1.0f, 0.0f, -1.0f);
const vec4 backgroundColor(1.0f, 178.0f/255.0f, 102.0f/255.0f, 1);
const vec4 fogColor(142.0f/255.0f, 120.0f/255.0f, 106.0f/255.0f, 1);
const string backgoundMusic = "GameData/sounds/bg.mp3";
const string buttonSound = "GameData/sounds/button.mp3";
const string upgradeSound = "GameData/sounds/upgrade.mp3";
const string linkSound = "GameData/sounds/link.mp3";
const string loadingtexture = "GameData/overlay/loading.bmp";
const string progressbartexture = "GameData/overlay/progressbar.bmp";
const string selectedtexture = "GameData/textures/selected.bmp";
const string overlayBackgroundtexture = "GameData/overlay/background.bmp";
const string popupoverlayBackgroundtexture = "GameData/overlay/background.bmp";
const string overlayUpgradetexture = "GameData/overlay/upgrade.bmp";
const string overlaySavetexture = "GameData/overlay/save.bmp";
const string overlayLoadtexture = "GameData/overlay/load.bmp";
const string overlayExittexture = "GameData/overlay/exit.bmp";
const string overlayB1texture = "GameData/overlay/b1.bmp";
const string overlayB2texture = "GameData/overlay/b2.bmp";
const string overlayB3texture = "GameData/overlay/b3.bmp";
const string popupoverlayEnergytexture = "GameData/overlay/energy.bmp";
const string popupoverlayWatertexture = "GameData/overlay/water.bmp";
const string popupoverlayOretexture = "GameData/overlay/ore.bmp";
const string plussigntexture = "GameData/overlay/+.bmp";
const string placeholdertexture = "GameData/overlay/placeholder.bmp";

gWindow::gWindow() : _window(new tWindow()), _control(NULL)
{
	init();
}

gWindow::~gWindow() 
{

	// intentionally leaking memory from _window
	// or it crashes when GL tries to delete the shaders
	// there is only one window, so this is no a problem
}

void gWindow::init() 
{
	_window->setgWindow(this);
}

void gWindow::main(gWindow *_window, mLock *main_thread_ready_lock) 
{
	// thread main
	// ready check
	main_thread_ready_lock->acquire();
	main_thread_ready_lock->release();
	// launch window
	Debug("gWindow thread running..." << endl;);
	gameWindow::launch(_window->get_tWindow(), 1280, 720, "Colonization I");
	// send a shutdown event to gameControl
	gameEvent shutdownevent;
	Debug("gWindow thread shutting down..." << endl;);
	shutdownevent._type = gameEventType::Event_Shutdown;
	_window->getController()->pushEvent(shutdownevent);
}

void gWindow::run(mLock *main_thread_ready_lock)
{
	gWindow::main(this, main_thread_ready_lock);
}

void gWindow::pushEvent(gameEvent _e)
{
	_window->pushEvent(_e);
}

tWindow *gWindow::get_tWindow()
{
	return _window;
}

void gWindow::setController(gameControl *ctrl)
{
	_control = ctrl;
}

gameControl *gWindow::getController()
{
	return _control;
}

void gWindow::shutdown()
{
	_window->shutdown();
}

void gWindow::setSelectBar(class selectbarObject *_s)
{
	_window->setSelectBar(_s);
}

void tWindow::init() 
{
	
	createShaderProgram();

	initGridVertex();

	loadSkybox();

	loadingOverlay();
	loadgameplayOverlay();
	loadpopupOverlay();
	loadNumber();
	loadextra();

	initPerspectiveMatrix();

	initOverlayMVP();
	initpopupOverlayMVP();

	initMatrix();

	initLightSources();

	generateFactors();

	// init sound engine
	soundEngine = createIrrKlangDevice();
	soundEngine->play2D(backgoundMusic.c_str(), true);

	// let move time forward to cast longer shadow
	for (int i = ticks; i < (3 * (float)FullTick / 4); ++i)
	{
		tick();
	}

}

tWindow::tWindow() 
: showmenu(true),
eventQueue_lock(new mLock()), 
eventQueue(queue<gameEvent>()), 
MeshLoadingProgress(0),
mesh_texture(map<unsigned int, GLuint>()),
vao_meshData(map<unsigned int, GLuint>()),
vbo_UVPositions(map<unsigned int, GLuint>()),
vbo_vertexPositions(map<unsigned int, GLuint>()),
vbo_vertexNormals(map<unsigned int, GLuint>()),
positionAttribLocation(map<unsigned int, GLint>()),
s_positionAttribLocation(map<unsigned int, GLint>()),
normalAttribLocation(map<unsigned int, GLint>()),
UVAttribLocation(map<unsigned int, GLint>()),
meshDataIdxSize(map<unsigned int, unsigned int>()),
mat_ks(map<unsigned int, vec3>()),
mat_shininess(map<unsigned int, double>()),
finishedLoading(false),
scale(1),
drag_start_x(-1),
drag_start_y(-1),
rot_x(0),
rot_y(90),
mx(0),
my(0),
mouseOver_x(-256),
mouseOver_y(-256),
mouseClick_x(-256),
mouseClick_y(-256),
ticks((float)FullTick / 2),
castingShadow(false),
daynight(0),
soundEngine(NULL),
o_width(1600),
o_height(900),
button_width(133),
button_height(450),
aspect(16.0f/9.0f),
selectbar(NULL),
barid(0),
buildingID(-1),
popupWidth(300),
popupHeight(300),
p_width(300),
p_height(300),
number_width(30),
number_height(30)
{}

tWindow::~tWindow() 
{
	soundEngine->drop();
	delete eventQueue_lock;
}

void tWindow::generateFactors()
{
	scale_fact = 0.25;
	max_scale = 10;
	min_scale = 0.25;

	max_camera_rotation_y = 178;
	min_camera_rotation_y = 2;

	rot_fact_x = 180.0f / m_windowWidth;
	rot_fact_y = 180.0f / m_windowHeight;

	shadowResw = 1920;
	shadowResh = 1080;
}

void tWindow::pushEvent(gameEvent _e)
{
	eventQueue_lock->acquire();

	eventQueue.push(_e);

	eventQueue_lock->release();
}

void tWindow::createShaderProgram()
{
	u_shader.generateProgramObject();
	u_shader.attachVertexShader( string("GameData/Shader/GeneralVertexShader.vs").c_str() );
	u_shader.attachFragmentShader( string("GameData/Shader/GeneralFragmentShader.fs").c_str() );
	u_shader.link();

	p_shader.generateProgramObject();
	p_shader.attachVertexShader( string("GameData/Shader/PickingVertexShader.vs").c_str() );
	p_shader.attachFragmentShader( string("GameData/Shader/PickingFragmentShader.fs").c_str() );
	p_shader.link();

	s_shader.generateProgramObject();
	s_shader.attachVertexShader( string("GameData/Shader/ShadowMapVertexShader.vs").c_str() );
	s_shader.attachFragmentShader( string("GameData/Shader/ShadowMapFragmentShader.fs").c_str() );
	s_shader.link();

	b_shader.generateProgramObject();
	b_shader.attachVertexShader( string("GameData/Shader/SkyboxVertexShader.vs").c_str() );
	b_shader.attachFragmentShader( string("GameData/Shader/SkyboxFragmentShader.fs").c_str() );
	b_shader.link();

	o_shader.generateProgramObject();
	o_shader.attachVertexShader( string("GameData/Shader/OverlayVertexShader.vs").c_str() );
	o_shader.attachFragmentShader( string("GameData/Shader/OverlayFragmentShader.fs").c_str() );
	o_shader.link();
}

void tWindow::loadNumber()
{
	// load number textures
	for (int i = 0; i < 10; ++i)
	{
		string texturefile = "GameData/overlay/number" + std::to_string(i) + ".bmp";
		//string texturefile = placeholdertexture;

		glGenTextures(1, &number_texture[i]);
	    glBindTexture(GL_TEXTURE_2D, number_texture[i]);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
	}
}

void tWindow::loadSkybox()
{
	// upload skybox mesh
	vector<float> vs;
	vs.clear();

	float scale = 500;
	float current_x = -0.5;
	float current_y = 0.5;
	float current_z = -0.5;
	float cube_size = 1;

	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z);
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z);                              // front, t1
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z);                              // front, t2
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);                  // back, t1
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);                  // back, t2
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z);
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);                  // left, t1
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z);                              // left, t2
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z);
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);                  // right, t1
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z);                              // right, t2
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z);
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z);                              // down, t1
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z);
	vs.push_back(current_x + cube_size);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x);
	vs.push_back(-current_y);
	vs.push_back(current_z + cube_size);                  // down, t2
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z);                              // up, t1
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z);
	vs.push_back(current_x + cube_size);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);
	vs.push_back(current_x);
	vs.push_back(current_y);
	vs.push_back(current_z + cube_size);                  // up, t2

	size_t sz = vs.size();
	float *verts = new float[sz];

	for (unsigned int i = 0; i < sz; ++i)
	{
		verts[i] = vs[i] * scale;
	}

	glGenVertexArrays(1, &skybox_vao);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(skybox_vao);

		// Enable the vertex shader attribute location for "position" when rendering.
		skybox_vertexPos = b_shader.getAttribLocation("position");
		glEnableVertexAttribArray(skybox_vertexPos);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &skybox_vbo);

		glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);

		glBufferData(GL_ARRAY_BUFFER, sz * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// generate skybox texture

	vector<GLenum> vtype;
	vector<string> texturefiles;
	vtype.clear();
	vtype.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	vtype.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
	vtype.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
	vtype.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
	vtype.push_back(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
	vtype.push_back(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	texturefiles.clear();
	texturefiles.push_back("GameData/textures/sb1.bmp");
	texturefiles.push_back("GameData/textures/sb2.bmp");
	texturefiles.push_back("GameData/textures/sb3.bmp");
	texturefiles.push_back("GameData/textures/sb4.bmp");
	texturefiles.push_back("GameData/textures/sb5.bmp");
	texturefiles.push_back("GameData/textures/sb6.bmp");

	glGenTextures(1, &skybox_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERRORS;

	for (int i = 0; i < vtype.size(); ++i)
	{
		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefiles[i].c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);

		glTexImage2D(vtype[i], 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	    CHECK_GL_ERRORS;
	    delete [] data;
	}

	//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void tWindow::loadextra()
{
	{
		// upload selected textures
		// loading texture
		string texturefile = selectedtexture;

		glGenTextures(1, &selected_texture);
	    glBindTexture(GL_TEXTURE_2D, selected_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
	}
	{
		// upload selected textures
		// loading texture
		string texturefile = plussigntexture;

		glGenTextures(1, &plus_texture);
	    glBindTexture(GL_TEXTURE_2D, plus_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
	}
	number_texture[-1] = plus_texture;
}

void tWindow::loadingOverlay()
{
	{
		float *verts = new float[18];
		verts[0] = -o_width / 2;
		verts[1] = -o_height / 2;
		verts[2] = 1;
		verts[3] = -o_width / 2;
		verts[4] = o_height / 2;
		verts[5] = 1;
		verts[6] = o_width / 2;
		verts[7] = -o_height / 2;
		verts[8] = 1;
		verts[9] = o_width / 2;
		verts[10] = -o_height / 2;
		verts[11] = 1;
		verts[12] = o_width / 2;
		verts[13] = o_height / 2;
		verts[14] = 1;
		verts[15] = -o_width / 2;
		verts[16] = o_height / 2;
		verts[17] = 1;

		glGenVertexArrays(1, &vao_overlay_loading);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_overlay_loading);

			// Enable the vertex shader attribute location for "position" when rendering.
			overlay_positionAttribLocation = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(overlay_positionAttribLocation);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_overlay_loading);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_overlay_loading);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		string texturefile = loadingtexture;

		glGenTextures(1, &overlay_texture_loading);
	    glBindTexture(GL_TEXTURE_2D, overlay_texture_loading);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
	}

    {
		float *verts = new float[18];
		verts[0] = -o_width / 2;
		verts[1] = -o_height / 2;
		verts[2] = 0;
		verts[3] = -o_width / 2;
		verts[4] = -o_height / 2 + o_height / 50;
		verts[5] = 0;
		verts[6] = o_width / 2;
		verts[7] = -o_height / 2;
		verts[8] = 0;
		verts[9] = o_width / 2;
		verts[10] = -o_height / 2;
		verts[11] = 0;
		verts[12] = o_width / 2;
		verts[13] = -o_height / 2 + o_height / 50;
		verts[14] = 0;
		verts[15] = -o_width / 2;
		verts[16] = -o_height / 2 + o_height / 50;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_overlay_progressbar);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_overlay_progressbar);

			// Enable the vertex shader attribute location for "position" when rendering.
			overlay_progressbar_positionAttribLocation = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(overlay_progressbar_positionAttribLocation);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_overlay_progressbar);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_overlay_progressbar);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		string texturefile = progressbartexture;

		glGenTextures(1, &overlay_texture_progressbar);
	    glBindTexture(GL_TEXTURE_2D, overlay_texture_progressbar);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

}

void tWindow::loadpopupOverlay()
{
	// overlay background
	{
		float *verts = new float[18];
		verts[0] = -p_width / 2;
		verts[1] = -p_height / 2;
		verts[2] = -0.1;
		verts[3] = -p_width / 2;
		verts[4] = p_height / 2;
		verts[5] = -0.1;
		verts[6] = p_width / 2;
		verts[7] = -p_height / 2;
		verts[8] = -0.1;
		verts[9] = p_width / 2;
		verts[10] = -p_height / 2;
		verts[11] = -0.1;
		verts[12] = p_width / 2;
		verts[13] = p_height / 2;
		verts[14] = -0.1;
		verts[15] = -p_width / 2;
		verts[16] = p_height / 2;
		verts[17] = -0.1;

		glGenVertexArrays(1, &vao_popupOverlay_background);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_background);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_background_positionAttribLocation = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_background_positionAttribLocation);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_background);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_background);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayBackgroundtexture;

			glGenTextures(1, &popupOverlay_background_texture);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_background_texture);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}

	// double v_split = 30;
	// double h_split = 15;

	// energy
	{
		float *verts = new float[18];
		verts[0] = -(p_width / 2 - 15);
		verts[1] = 50;
		verts[2] = -0.2;
		verts[3] = -(p_width / 2 - 15);
		verts[4] = 135;
		verts[5] = -0.2;
		verts[6] = -(p_width / 2 - 15) + 90;
		verts[7] = 50;
		verts[8] = -0.2;
		verts[9] = -(p_width / 2 - 15) + 90;
		verts[10] = 50;
		verts[11] = -0.2;
		verts[12] = -(p_width / 2 - 15) + 90;
		verts[13] = 135;
		verts[14] = -0.2;
		verts[15] = -(p_width / 2 - 15);
		verts[16] = 135;
		verts[17] = -0.2;

		glGenVertexArrays(1, &vao_popupOverlay_energy);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_energy);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_energy_positionAttribLocation = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_energy_positionAttribLocation);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_energy);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayEnergytexture;

			glGenTextures(1, &popupOverlay_energy_texture);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_energy_texture);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}
	// energy number
	for (int i = 0; i < 6; ++i)
	{
		float *verts = new float[18];
		verts[0] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[1] = 50;
		verts[2] = -0.2;
		verts[3] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[4] = 80;
		verts[5] = -0.2;
		verts[6] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[7] = 50;
		verts[8] = -0.2;
		verts[9] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[10] = 50;
		verts[11] = -0.2;
		verts[12] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[13] = 80;
		verts[14] = -0.2;
		verts[15] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[16] = 80;
		verts[17] = -0.2;

		glGenVertexArrays(1, &vao_popupOverlay_energy_number[i]);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_energy_number[i]);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_energy_number_positionAttribLocation[i] = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_energy_number_positionAttribLocation[i]);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_energy_number[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_number[i]);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;
	}

	// water
	{
		float *verts = new float[18];
		verts[0] = -(p_width / 2 - 15);
		verts[1] = 50 - 85;
		verts[2] = -0.2;
		verts[3] = -(p_width / 2 - 15);
		verts[4] = 135 - 85;
		verts[5] = -0.2;
		verts[6] = -(p_width / 2 - 15) + 90;
		verts[7] = 50 - 85;
		verts[8] = -0.2;
		verts[9] = -(p_width / 2 - 15) + 90;
		verts[10] = 50 - 85;
		verts[11] = -0.2;
		verts[12] = -(p_width / 2 - 15) + 90;
		verts[13] = 135 - 85;
		verts[14] = -0.2;
		verts[15] = -(p_width / 2 - 15);
		verts[16] = 135 - 85;
		verts[17] = -0.2;

		glGenVertexArrays(1, &vao_popupOverlay_water);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_water);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_water_positionAttribLocation = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_water_positionAttribLocation);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_water);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayWatertexture;

			glGenTextures(1, &popupOverlay_water_texture);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_water_texture);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}
	// water number
	for (int i = 0; i < 6; ++i)
	{
		float *verts = new float[18];
		verts[0] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[1] = 50 - 85;
		verts[2] = -0.2;
		verts[3] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[4] = 80 - 85;
		verts[5] = -0.2;
		verts[6] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[7] = 50 - 85;
		verts[8] = -0.2;
		verts[9] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[10] = 50 - 85;
		verts[11] = -0.2;
		verts[12] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[13] = 80 - 85;
		verts[14] = -0.2;
		verts[15] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[16] = 80 - 85;
		verts[17] = -0.2;

		glGenVertexArrays(1, &vao_popupOverlay_water_number[i]);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_water_number[i]);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_water_number_positionAttribLocation[i] = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_water_number_positionAttribLocation[i]);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_water_number[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_number[i]);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;
	}
	//ore
	{
		float *verts = new float[18];
		verts[0] = -(p_width / 2 - 15);
		verts[1] = 50 - 85 - 85;
		verts[2] = -0.2;
		verts[3] = -(p_width / 2 - 15);
		verts[4] = 135 - 85 - 85;
		verts[5] = -0.2;
		verts[6] = -(p_width / 2 - 15) + 90;
		verts[7] = 50 - 85 - 85;
		verts[8] = -0.2;
		verts[9] = -(p_width / 2 - 15) + 90;
		verts[10] = 50 - 85 - 85;
		verts[11] = -0.2;
		verts[12] = -(p_width / 2 - 15) + 90;
		verts[13] = 135 - 85 - 85;
		verts[14] = -0.2;
		verts[15] = -(p_width / 2 - 15);
		verts[16] = 135 - 85 - 85;
		verts[17] = -0.2;

		glGenVertexArrays(1, &vao_popupOverlay_ore);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_ore);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_ore_positionAttribLocation = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_ore_positionAttribLocation);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_ore);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayOretexture;

			glGenTextures(1, &popupOverlay_ore_texture);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_ore_texture);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}
	// ore number
	for (int i = 0; i < 6; ++i)
	{
		float *verts = new float[18];
		verts[0] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[1] = 50 - 85 - 85;
		verts[2] = -0.2;
		verts[3] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[4] = 80 - 85 - 85;
		verts[5] = -0.2;
		verts[6] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[7] = 50 - 85 - 85;
		verts[8] = -0.2;
		verts[9] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[10] = 50 - 85 - 85;
		verts[11] = -0.2;
		verts[12] = -(p_width / 2 - 15) + 90 + 30 + 30 * i;
		verts[13] = 80 - 85 - 85;
		verts[14] = -0.2;
		verts[15] = -(p_width / 2 - 15) + 90 + 30 * i;
		verts[16] = 80 - 85 - 85;
		verts[17] = -0.2;

		glGenVertexArrays(1, &vao_popupOverlay_ore_number[i]);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_ore_number[i]);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_ore_number_positionAttribLocation[i] = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_ore_number_positionAttribLocation[i]);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_ore_number[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_number[i]);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;
	}
}

void tWindow::renderpopupOverlay()
{
	if ((mx == -1) || (my == -1) || (mouseOver_x == -256) || (mouseOver_y == -256))
	{
		if  ((showmenu) && (my <= (m_windowWidth, m_windowHeight / 5))) {
			// convert mouse coords to button index
			int cx = (mx / (double(m_windowWidth) / (o_width / button_width)));
			int cy = (my / (double(m_windowHeight / 5) / (o_height / button_height)));
			//cout << cx << "," << cy << endl;
			// triger button click event
			bool playsound = false;
			if ((cy == 0))
			{
				return;
			}
			if ((cx < 0) || (cx >= gameData::OriginBuildings.size()))
			{
				return;
			}
			//cout << cx << endl;
			gameObject *obj = gameData::OriginBuildings[cx];
			glViewport(float(mx), float(m_windowHeight) - float(my) - float(popupHeight), float(popupWidth), float(popupHeight));

			// render background
			{
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_background);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_background);
				glVertexAttribPointer(popupOverlay_background_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_background);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, p_width);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, p_height);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -p_width/2);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, -p_height/2);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, popupOverlay_background_texture);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
			// render energy
			{
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_energy);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy);
				glVertexAttribPointer(popupOverlay_energy_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_energy);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, 90);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, 85);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -(p_width / 2 - 15));
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, 50);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, popupOverlay_energy_texture);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
			// energy number
			bool flag_nonzero = false;
			int needed = 0;
			if ((mouseOver_x == 0) && (mouseOver_y == 0)) {
				coreObject *building = (coreObject*)obj;
				needed = building->neededEnergy;
			} else {
				buildingObject *building = (buildingObject*)obj;
				needed = building->neededEnergy;
			}
			for (int i = 0; i < 6; ++i)
			{
				// number
				int d = 0;
				if ((i == 0) && (needed < 0))
				{
					d = -1;
					flag_nonzero = true;
				}  else if (i > 0) {
					d = std::to_string(abs((int)needed))[i - 1] - '0';
					flag_nonzero = flag_nonzero || (d != 0);
					if (!flag_nonzero)
					{
						continue;
					}
				} else {
					continue;
				}
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_energy_number[i]);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_number[i]);
				glVertexAttribPointer(popupOverlay_energy_number_positionAttribLocation[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_energy_number[i]);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, 30);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, 30);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -(p_width / 2 - 15) + 90 + 30 * i);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, 50);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, number_texture[d]);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
			// water
			{
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_water);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water);
				glVertexAttribPointer(popupOverlay_water_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_water);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, 90);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, 85);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -(p_width / 2 - 15));
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, 50 - 85);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, popupOverlay_water_texture);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
			// water number
			flag_nonzero = false;
			needed = 0;
			if ((mouseOver_x == 0) && (mouseOver_y == 0)) {
				coreObject *building = (coreObject*)obj;
				needed = building->neededWater;
			} else {
				buildingObject *building = (buildingObject*)obj;
				needed = building->neededWater;
			}
			for (int i = 0; i < 6; ++i)
			{
				// number
				int d = 0;
				if ((i == 0) && (needed < 0))
				{
					d = -1;
					flag_nonzero = true;
				} else if (i > 0) {
					d = std::to_string(abs((int)needed))[i - 1] - '0';
					flag_nonzero = flag_nonzero || (d != 0);
					if (!flag_nonzero)
					{
						continue;
					}
				} else {
					continue;
				}
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_water_number[i]);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_number[i]);
				glVertexAttribPointer(popupOverlay_water_number_positionAttribLocation[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_water_number[i]);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, 30);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, 30);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -(p_width / 2 - 15) + 90 + 30 * i);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, 50 - 85);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, number_texture[d]);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
			// ore energy
			{
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_ore);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore);
				glVertexAttribPointer(popupOverlay_water_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_ore);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, 90);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, 85);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -(p_width / 2 - 15));
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, 50 - 85 - 85);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, popupOverlay_ore_texture);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
			// ore number
			flag_nonzero = false;
			needed = 0;
			if ((mouseOver_x == 0) && (mouseOver_y == 0)) {
				coreObject *building = (coreObject*)obj;
				needed = building->neededOre;
			} else {
				buildingObject *building = (buildingObject*)obj;
				needed = building->neededOre;
			}
			for (int i = 0; i < 6; ++i)
			{
				// number
				int d = 0;
				if ((i == 0) && (needed < 0))
				{
					d = -1;
					flag_nonzero = true;
				} else if (i > 0) {
					d = std::to_string(abs((int)needed))[i - 1] - '0';
					flag_nonzero = flag_nonzero || (d != 0);
					if (!flag_nonzero)
					{
						continue;
					}
				} else {
					continue;
				}
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_popupOverlay_ore_number[i]);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_number[i]);
				glVertexAttribPointer(popupOverlay_ore_number_positionAttribLocation[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;
				glEnable(GL_DEPTH_TEST);

				glBindVertexArray(vao_popupOverlay_ore_number[i]);
				o_shader.enable();
				
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = o_shader.getUniformLocation("MVP");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
					CHECK_GL_ERRORS;
					
					// shadow map
					location = o_shader.getUniformLocation("TextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("width");
					glUniform1f(location, 30);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("height");
					glUniform1f(location, 30);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_x");
					glUniform1f(location, -(p_width / 2 - 15) + 90 + 30 * i);
					CHECK_GL_ERRORS;

					location = o_shader.getUniformLocation("buttom_left_y");
					glUniform1f(location, 50 - 85 - 85);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, number_texture[d]);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				o_shader.disable();
				
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			}
		} else {
			return;
		}
	} else {
		gameObject *obj = _window->getController()->getgameData()->getGrid()->getObject(mouseOver_x, mouseOver_y);
		if (obj == NULL)
		{
			return;
		}
		glViewport(float(mx), float(m_windowHeight) - float(my), float(popupWidth), float(popupHeight));

		// render background
		{
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_background);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_background);
			glVertexAttribPointer(popupOverlay_background_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_background);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, p_width);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, p_height);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -p_width/2);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, -p_height/2);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, popupOverlay_background_texture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
		// render energy
		{
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_energy);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy);
			glVertexAttribPointer(popupOverlay_energy_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_energy);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, 90);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, 85);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -(p_width / 2 - 15));
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, 50);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, popupOverlay_energy_texture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
		// energy number
		bool flag_nonzero = false;
		int needed = 0;
		if ((mouseOver_x == 0) && (mouseOver_y == 0)) {
			coreObject *building = (coreObject*)obj;
			needed = building->runningEnergy;
		} else {
			buildingObject *building = (buildingObject*)obj;
			needed = building->runningEnergy;
		}
		for (int i = 0; i < 6; ++i)
		{
			// number
			int d = 0;
			if ((i == 0) && (needed < 0))
			{
				d = -1;
				flag_nonzero = true;
			}  else if (i > 0) {
				d = std::to_string(abs((int)needed))[i - 1] - '0';
				flag_nonzero = flag_nonzero || (d != 0);
				if (!flag_nonzero)
				{
					continue;
				}
			} else {
				continue;
			}
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_energy_number[i]);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_number[i]);
			glVertexAttribPointer(popupOverlay_energy_number_positionAttribLocation[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_energy_number[i]);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, 30);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, 30);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -(p_width / 2 - 15) + 90 + 30 * i);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, 50);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, number_texture[d]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
		// water
		{
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_water);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water);
			glVertexAttribPointer(popupOverlay_water_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_water);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, 90);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, 85);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -(p_width / 2 - 15));
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, 50 - 85);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, popupOverlay_water_texture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
		// water number
		flag_nonzero = false;
		needed = 0;
		if ((mouseOver_x == 0) && (mouseOver_y == 0)) {
			coreObject *building = (coreObject*)obj;
			needed = building->runningWater;
		} else {
			buildingObject *building = (buildingObject*)obj;
			needed = building->runningWater;
		}
		for (int i = 0; i < 6; ++i)
		{
			// number
			int d = 0;
			if ((i == 0) && (needed < 0))
			{
				d = -1;
				flag_nonzero = true;
			} else if (i > 0) {
				d = std::to_string(abs((int)needed))[i - 1] - '0';
				flag_nonzero = flag_nonzero || (d != 0);
				if (!flag_nonzero)
				{
					continue;
				}
			} else {
				continue;
			}
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_water_number[i]);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_number[i]);
			glVertexAttribPointer(popupOverlay_water_number_positionAttribLocation[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_water_number[i]);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, 30);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, 30);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -(p_width / 2 - 15) + 90 + 30 * i);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, 50 - 85);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, number_texture[d]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
		// ore energy
		{
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_ore);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore);
			glVertexAttribPointer(popupOverlay_water_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_ore);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, 90);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, 85);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -(p_width / 2 - 15));
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, 50 - 85 - 85);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, popupOverlay_ore_texture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
		// ore number
		flag_nonzero = false;
		needed = 0;
		if ((mouseOver_x == 0) && (mouseOver_y == 0)) {
			coreObject *building = (coreObject*)obj;
			needed = building->runningOre;
		} else {
			buildingObject *building = (buildingObject*)obj;
			needed = building->runningOre;
		}
		for (int i = 0; i < 6; ++i)
		{
			// number
			int d = 0;
			if ((i == 0) && (needed < 0))
			{
				d = -1;
				flag_nonzero = true;
			} else if (i > 0) {
				d = std::to_string(abs((int)needed))[i - 1] - '0';
				flag_nonzero = flag_nonzero || (d != 0);
				if (!flag_nonzero)
				{
					continue;
				}
			} else {
				continue;
			}
			// Bind VAO in order to record the data mapping.
			glBindVertexArray(vao_popupOverlay_ore_number[i]);
			// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
			// "position" vertex attribute location for any bound vertex shader program.
			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_number[i]);
			glVertexAttribPointer(popupOverlay_ore_number_positionAttribLocation[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			//-- Unbind target, and restore default values:
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);

			CHECK_GL_ERRORS;
			glEnable(GL_DEPTH_TEST);

			glBindVertexArray(vao_popupOverlay_ore_number[i]);
			o_shader.enable();
			
			{
				//-- Set Perpsective matrix uniform for the scene:
				GLint location = o_shader.getUniformLocation("MVP");
				glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(popupoverlayMVP));
				CHECK_GL_ERRORS;
				
				// shadow map
				location = o_shader.getUniformLocation("TextureSampler");
				glUniform1i(location, 0);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("width");
				glUniform1f(location, 30);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("height");
				glUniform1f(location, 30);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_x");
				glUniform1f(location, -(p_width / 2 - 15) + 90 + 30 * i);
				CHECK_GL_ERRORS;

				location = o_shader.getUniformLocation("buttom_left_y");
				glUniform1f(location, 50 - 85 - 85);
				CHECK_GL_ERRORS;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, number_texture[d]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			o_shader.disable();
			
			glBindVertexArray(0);
			glDisable( GL_DEPTH_TEST );
		}
	}
}

void tWindow::loadgameplayOverlay()
{
	// overlay background
	float *verts = new float[18];
	verts[0] = -o_width / 2;
	verts[1] = -o_height / 2;
	verts[2] = 1;
	verts[3] = -o_width / 2;
	verts[4] = o_height / 2;
	verts[5] = 1;
	verts[6] = o_width / 2;
	verts[7] = -o_height / 2;
	verts[8] = 1;
	verts[9] = o_width / 2;
	verts[10] = -o_height / 2;
	verts[11] = 1;
	verts[12] = o_width / 2;
	verts[13] = o_height / 2;
	verts[14] = 1;
	verts[15] = -o_width / 2;
	verts[16] = o_height / 2;
	verts[17] = 1;

	glGenVertexArrays(1, &vao_gameplayOverlay_background);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_background);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_background_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_background_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_background);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_background);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// upload overlay textures
	// loading texture
	{
		string texturefile = overlayBackgroundtexture;

		glGenTextures(1, &gameplayOverlay_background_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_background_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // upgrade button
    verts = new float[18];
	verts[0] = -o_width / 2;
	verts[1] = -o_height / 2 + button_height;
	verts[2] = 0;
	verts[3] = -o_width / 2;
	verts[4] = -o_height / 2 + button_height + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width;
	verts[7] = -o_height / 2 + button_height;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width;
	verts[10] = -o_height / 2 + button_height;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width;
	verts[13] = -o_height / 2 + button_height + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2;
	verts[16] = -o_height / 2 + button_height + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_upgrade);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_upgrade);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_upgrade_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_upgrade_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_upgrade);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_upgrade);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlayUpgradetexture;

		glGenTextures(1, &gameplayOverlay_upgrade_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_upgrade_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // save button
    verts = new float[18];
	verts[0] = -o_width / 2 + button_width * 9;
	verts[1] = -o_height / 2 + button_height;
	verts[2] = 0;
	verts[3] = -o_width / 2 + button_width * 9;
	verts[4] = -o_height / 2 + button_height + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width + button_width * 9;
	verts[7] = -o_height / 2 + button_height;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width + button_width * 9;
	verts[10] = -o_height / 2 + button_height;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width + button_width * 9;
	verts[13] = -o_height / 2 + button_height + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2 + button_width * 9;
	verts[16] = -o_height / 2 + button_height + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_save);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_save);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_save_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_save_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_save);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_save);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlaySavetexture;

		glGenTextures(1, &gameplayOverlay_save_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_save_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // load button
    verts = new float[18];
	verts[0] = -o_width / 2 + button_width * 10;
	verts[1] = -o_height / 2 + button_height;
	verts[2] = 0;
	verts[3] = -o_width / 2 + button_width * 10;
	verts[4] = -o_height / 2 + button_height + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width + button_width * 10;
	verts[7] = -o_height / 2 + button_height;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width + button_width * 10;
	verts[10] = -o_height / 2 + button_height;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width + button_width * 10;
	verts[13] = -o_height / 2 + button_height + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2 + button_width * 10;
	verts[16] = -o_height / 2 + button_height + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_load);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_load);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_load_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_load_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_load);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_load);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlayLoadtexture;

		glGenTextures(1, &gameplayOverlay_load_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_load_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // quit button
    verts = new float[18];
	verts[0] = -o_width / 2 + button_width * 11;
	verts[1] = -o_height / 2 + button_height;
	verts[2] = 0;
	verts[3] = -o_width / 2 + button_width * 11;
	verts[4] = -o_height / 2 + button_height + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width + button_width * 11;
	verts[7] = -o_height / 2 + button_height;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width + button_width * 11;
	verts[10] = -o_height / 2 + button_height;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width + button_width * 11;
	verts[13] = -o_height / 2 + button_height + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2 + button_width * 11;
	verts[16] = -o_height / 2 + button_height + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_exit);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_exit);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_exit_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_exit_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_exit);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_exit);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlayExittexture;

		glGenTextures(1, &gameplayOverlay_exit_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_exit_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // b1 button
    verts = new float[18];
	verts[0] = -o_width / 2;
	verts[1] = -o_height/2;
	verts[2] = 0;
	verts[3] = -o_width / 2;
	verts[4] = -o_height/2 + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width;
	verts[7] = -o_height/2;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width;
	verts[10] = -o_height/2;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width;
	verts[13] = -o_height/2 + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2;
	verts[16] = -o_height/2 + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_b1);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_b1);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_b1_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_b1_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_b1);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_b1);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlayB1texture;

		glGenTextures(1, &gameplayOverlay_b1_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_b1_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // b2 button
    verts = new float[18];
	verts[0] = -o_width / 2 + button_width * 1;
	verts[1] = -o_height/2;
	verts[2] = 0;
	verts[3] = -o_width / 2 + button_width * 1;
	verts[4] = -o_height/2 + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width + button_width * 1;
	verts[7] = -o_height/2;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width + button_width * 1;
	verts[10] = -o_height/2;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width + button_width * 1;
	verts[13] = -o_height/2 + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2 + button_width * 1;
	verts[16] = -o_height/2 + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_b2);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_b2);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_b2_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_b2_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_b2);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_b2);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlayB2texture;

		glGenTextures(1, &gameplayOverlay_b2_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_b2_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }
    // b3 button
    verts = new float[18];
	verts[0] = -o_width / 2 + button_width * 2;
	verts[1] = -o_height/2;
	verts[2] = 0;
	verts[3] = -o_width / 2 + button_width * 2;
	verts[4] = -o_height/2 + button_height;
	verts[5] = 0;
	verts[6] = -o_width / 2 + button_width + button_width * 2;
	verts[7] = -o_height/2;
	verts[8] = 0;
	verts[9] = -o_width / 2 + button_width + button_width * 2;
	verts[10] = -o_height/2;
	verts[11] = 0;
	verts[12] = -o_width / 2 + button_width + button_width * 2;
	verts[13] = -o_height/2 + button_height;
	verts[14] = 0;
	verts[15] = -o_width / 2 + button_width * 2;
	verts[16] = -o_height/2 + button_height;
	verts[17] = 0;

	glGenVertexArrays(1, &vao_gameplayOverlay_b3);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_gameplayOverlay_b3);

		// Enable the vertex shader attribute location for "position" when rendering.
		gameplayOverlay_b3_positionAttribLocation = o_shader.getAttribLocation("position");
		glEnableVertexAttribArray(gameplayOverlay_b3_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_gameplayOverlay_b3);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_b3);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;

	// textures
	{
		string texturefile = overlayB3texture;

		glGenTextures(1, &gameplayOverlay_b3_texture);
	    glBindTexture(GL_TEXTURE_2D, gameplayOverlay_b3_texture);

		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char * data;
		FILE * file = fopen(texturefile.c_str(), "rb");
		fread(header, 1, 54, file);
		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);
		if (imageSize==0) imageSize=width*height*3;
	 	if (dataPos==0) dataPos=54;
	 	data = new unsigned char [imageSize];
	 	fread(data, 1, imageSize, file);
	 	fclose(file);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	    glGenerateMipmap(GL_TEXTURE_2D);
	    glBindTexture(GL_TEXTURE_2D, 0);
	    delete [] data;
    }

    // energy
	{
		float *verts = new float[18];
		verts[0] = -o_width/8;
		verts[1] = -o_height/6 + o_height/3;
		verts[2] = 0;
		verts[3] = -o_width/8;
		verts[4] = -o_height/6 + o_height/3 + o_height/3;
		verts[5] = 0;
		verts[6] = -o_width/8 + 90;
		verts[7] = -o_height/6 + o_height/3;
		verts[8] = 0;
		verts[9] = -o_width/8 + 90;
		verts[10] = -o_height/6 + o_height/3;
		verts[11] = 0;
		verts[12] = -o_width/8 + 90;
		verts[13] = -o_height/6 + o_height/3 + o_height/3;
		verts[14] = 0;
		verts[15] = -o_width/8;
		verts[16] = -o_height/6 + o_height/3 + o_height/3;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_popupOverlay_energy_main);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_energy_main);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_energy_positionAttribLocation_main = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_energy_positionAttribLocation_main);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_energy_main);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_main);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayEnergytexture;

			glGenTextures(1, &popupOverlay_energy_texture_main);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_energy_texture_main);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}
	// energy number
	for (int i = 0; i < 6; ++i)
	{
		float *verts = new float[18];
		verts[0] = -o_width/8 + 90 + 30 * i;
		verts[1] = -o_height/6 + o_height/3;
		verts[2] = 0;
		verts[3] = -o_width/8 + 90 + 30 * i;
		verts[4] = -o_height/6 + o_height/3 + 159;
		verts[5] = 0;
		verts[6] = -o_width/8 + 90 + 30 + 30 * i;
		verts[7] = -o_height/6 + o_height/3;
		verts[8] = 0;
		verts[9] = -o_width/8 + 90 + 30 + 30 * i;
		verts[10] = -o_height/6 + o_height/3;
		verts[11] = 0;
		verts[12] = -o_width/8 + 90 + 30 + 30 * i;
		verts[13] = -o_height/6 + o_height/3 + 159;
		verts[14] = 0;
		verts[15] = -o_width/8 + 90 + 30 * i;
		verts[16] = -o_height/6 + o_height/3 + 159;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_popupOverlay_energy_number_main[i]);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_energy_number_main[i]);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_energy_number_positionAttribLocation_main[i] = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_energy_number_positionAttribLocation_main[i]);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_energy_number_main[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_number_main[i]);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;
	}
	// water
	{
		float *verts = new float[18];
		verts[0] = -o_width/8;
		verts[1] = -o_height/6;
		verts[2] = 0;
		verts[3] = -o_width/8;
		verts[4] = -o_height/6 + o_height/3;
		verts[5] = 0;
		verts[6] = -o_width/8 + 90;
		verts[7] = -o_height/6;
		verts[8] = 0;
		verts[9] = -o_width/8 + 90;
		verts[10] = -o_height/6;
		verts[11] = 0;
		verts[12] = -o_width/8 + 90;
		verts[13] = -o_height/6 + o_height/3;
		verts[14] = 0;
		verts[15] = -o_width/8;
		verts[16] = -o_height/6 + o_height/3;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_popupOverlay_water_main);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_water_main);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_water_positionAttribLocation_main = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_water_positionAttribLocation_main);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_water_main);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_main);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayWatertexture;

			glGenTextures(1, &popupOverlay_water_texture_main);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_water_texture_main);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}
	// water number
	for (int i = 0; i < 6; ++i)
	{
		float *verts = new float[18];
		verts[0] = -o_width/8 + 90 + 30 * i;
		verts[1] = -o_height/6;
		verts[2] = 0;
		verts[3] = -o_width/8 + 90 + 30 * i;
		verts[4] = -o_height/6 + 159;
		verts[5] = 0;
		verts[6] = -o_width/8 + 90 + 30 + 30 * i;
		verts[7] = -o_height/6;
		verts[8] = 0;
		verts[9] = -o_width/8 + 90 + 30 + 30 * i;
		verts[10] = -o_height/6;
		verts[11] = 0;
		verts[12] = -o_width/8 + 90 + 30 + 30 * i;
		verts[13] = -o_height/6 + 159;
		verts[14] = 0;
		verts[15] = -o_width/8 + 90 + 30 * i;
		verts[16] = -o_height/6 + 159;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_popupOverlay_water_number_main[i]);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_water_number_main[i]);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_water_number_positionAttribLocation_main[i] = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_water_number_positionAttribLocation_main[i]);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_water_number_main[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_number_main[i]);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;
	}

	// ore
	{
		float *verts = new float[18];
		verts[0] = -o_width/8;
		verts[1] = -o_height/6 - o_height/3;
		verts[2] = 0;
		verts[3] = -o_width/8;
		verts[4] = -o_height/6 + o_height/3 - o_height/3;
		verts[5] = 0;
		verts[6] = -o_width/8 + 90;
		verts[7] = -o_height/6 - o_height/3;
		verts[8] = 0;
		verts[9] = -o_width/8 + 90;
		verts[10] = -o_height/6 - o_height/3;
		verts[11] = 0;
		verts[12] = -o_width/8 + 90;
		verts[13] = -o_height/6 + o_height/3 - o_height/3;
		verts[14] = 0;
		verts[15] = -o_width/8;
		verts[16] = -o_height/6 + o_height/3 - o_height/3;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_popupOverlay_ore_main);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_ore_main);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_ore_positionAttribLocation_main = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_ore_positionAttribLocation_main);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_ore_main);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_main);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;

		// upload overlay textures
		// loading texture
		{
			string texturefile = popupoverlayOretexture;

			glGenTextures(1, &popupOverlay_ore_texture_main);
		    glBindTexture(GL_TEXTURE_2D, popupOverlay_ore_texture_main);

			unsigned char header[54];
			unsigned int dataPos;
			unsigned int width, height;
			unsigned int imageSize;
			unsigned char * data;
			FILE * file = fopen(texturefile.c_str(), "rb");
			fread(header, 1, 54, file);
			dataPos = *(int*)&(header[0x0A]);
			imageSize = *(int*)&(header[0x22]);
			width = *(int*)&(header[0x12]);
			height = *(int*)&(header[0x16]);
			if (imageSize==0) imageSize=width*height*3;
		 	if (dataPos==0) dataPos=54;
		 	data = new unsigned char [imageSize];
		 	fread(data, 1, imageSize, file);
		 	fclose(file);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		    glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);
		    delete [] data;
	    }
	}
	// ore number
	for (int i = 0; i < 6; ++i)
	{
		float *verts = new float[18];
		verts[0] = -o_width/8 + 90 + 30 * i;
		verts[1] = -o_height/6 - o_height/3;
		verts[2] = 0;
		verts[3] = -o_width/8 + 90 + 30 * i;
		verts[4] = -o_height/6 + 159 - o_height/3;
		verts[5] = 0;
		verts[6] = -o_width/8 + 90 + 30 + 30 * i;
		verts[7] = -o_height/6 - o_height/3;
		verts[8] = 0;
		verts[9] = -o_width/8 + 90 + 30 + 30 * i;
		verts[10] = -o_height/6 - o_height/3;
		verts[11] = 0;
		verts[12] = -o_width/8 + 90 + 30 + 30 * i;
		verts[13] = -o_height/6 + 159 - o_height/3;
		verts[14] = 0;
		verts[15] = -o_width/8 + 90 + 30 * i;
		verts[16] = -o_height/6 + 159 - o_height/3;
		verts[17] = 0;

		glGenVertexArrays(1, &vao_popupOverlay_ore_number_main[i]);
		//-- Enable input slots for m_vao_meshData:
		{
			glBindVertexArray(vao_popupOverlay_ore_number_main[i]);

			// Enable the vertex shader attribute location for "position" when rendering.
			popupOverlay_ore_number_positionAttribLocation_main[i] = o_shader.getAttribLocation("position");
			glEnableVertexAttribArray(popupOverlay_ore_number_positionAttribLocation_main[i]);

			CHECK_GL_ERRORS;
		}
		glBindVertexArray(0);
		// Generate VBO to store all vertex position data
		{
			glGenBuffers(1, &vbo_popupOverlay_ore_number_main[i]);

			glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_number_main[i]);

			glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
					verts, GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			CHECK_GL_ERRORS;
		}

		delete [] verts;
	}

}

void tWindow::initOverlayMVP()
{
	overlayMVP = glm::ortho(float(o_width/2), float(-o_width/2), float(-o_height/2), float(o_height/2), 0.0f, 3.0f);
	//overlayMVP = m_perpsective;
	overlayMVP = overlayMVP * glm::lookAt(vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}

void tWindow::initpopupOverlayMVP()
{
	popupoverlayMVP = glm::ortho(float(p_width/2), float(-p_width/2), float(-p_height/2), float(p_height/2), 0.0f, 3.0f);
	//overlayMVP = m_perpsective;
	popupoverlayMVP = popupoverlayMVP * glm::lookAt(vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
}


void tWindow::renderSkybox() 
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(skybox_vao);
	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, skybox_vao);
	glVertexAttribPointer(skybox_vertexPos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
	glEnable( GL_DEPTH_TEST );

	glBindVertexArray(skybox_vao);
	b_shader.enable();
	
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = b_shader.getUniformLocation("MVP");
		mat4 mvp = m_perpsective * m_view * getModelMatrix();
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(mvp));
		CHECK_GL_ERRORS;

		// shadow map
		location = b_shader.getUniformLocation("skyboxTexture");
		glUniform1i(location, 0);
		CHECK_GL_ERRORS;

		location = b_shader.getUniformLocation("colorratio");
		float r;
		if (daynight == 1)
		{
			// night
			if ((ticks >= 0) && (ticks <= ((float)FullTick/4)))
			{
				r = 0.4 * (float)ticks / ((float)FullTick/4);
			} else if ((ticks >= (ticks <= (3 * (float)FullTick/4))) && (ticks <= ((float)FullTick))) {
				r = 0.4 * (1 - (float)ticks / ((float)FullTick));
			}
		} else {
			r = 0;
		}
		glUniform1f(location, r);
		CHECK_GL_ERRORS;
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 6);

	b_shader.disable();
	
	glBindVertexArray(0);
	glDisable( GL_DEPTH_TEST ); 
}

void tWindow::renderLoadingOverlay()
{
	glViewport(0, 0, m_windowWidth, m_windowHeight);
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(vao_overlay_loading);
	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, vbo_overlay_loading);
	glVertexAttribPointer(overlay_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
	glEnable( GL_DEPTH_TEST );

	glBindVertexArray(vao_overlay_loading);
	o_shader.enable();
	
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = o_shader.getUniformLocation("MVP");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
		CHECK_GL_ERRORS;
		
		// shadow map
		location = o_shader.getUniformLocation("TextureSampler");
		glUniform1i(location, 0);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("width");
		glUniform1f(location, o_width);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("height");
		glUniform1f(location, o_height);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("buttom_left_x");
		glUniform1f(location, -o_width/2);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("buttom_left_y");
		glUniform1f(location, -o_height/2);
		CHECK_GL_ERRORS;
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overlay_texture_loading);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	o_shader.disable();
	
	glBindVertexArray(0);
	glDisable( GL_DEPTH_TEST );

	// progressbar
	glBindVertexArray(vao_overlay_progressbar);
	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, vbo_overlay_progressbar);
	glVertexAttribPointer(overlay_progressbar_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
	glEnable( GL_DEPTH_TEST );

	glBindVertexArray(vao_overlay_progressbar);
	o_shader.enable();
	
	{
		//-- Set Perpsective matrix uniform for the scene:
		GLint location = o_shader.getUniformLocation("MVP");
		mat4 progressmat = overlayMVP;
		progressmat = progressmat * glm::translate(mat4(1), vec3(-o_width/2, 0, 0))
		* glm::scale(mat4(1), vec3(MeshLoadingProgress / 100, 1, 1))
		* glm::translate(mat4(1), vec3(o_width/2, 0, 0));
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(progressmat));
		CHECK_GL_ERRORS;
		
		// shadow map
		location = o_shader.getUniformLocation("TextureSampler");
		glUniform1i(location, 0);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("width");
		glUniform1f(location, o_width);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("height");
		glUniform1f(location, o_height/50);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("buttom_left_x");
		glUniform1f(location, -o_width/2);
		CHECK_GL_ERRORS;

		location = o_shader.getUniformLocation("buttom_left_y");
		glUniform1f(location, -o_height/2);
		CHECK_GL_ERRORS;
	}
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, overlay_texture_progressbar);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	o_shader.disable();
	
	glBindVertexArray(0);
	glDisable( GL_DEPTH_TEST );
}

void tWindow::rendergameplayOverlay()
{
	glViewport(0, float(m_windowHeight) * 4 / 5, m_windowWidth, float(m_windowHeight) / 5);

	// render background
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_background);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_background);
		glVertexAttribPointer(gameplayOverlay_background_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_background);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, o_width);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, o_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/2);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_background_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}

	// render upgrade button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_upgrade);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_upgrade);
		glVertexAttribPointer(gameplayOverlay_upgrade_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_upgrade);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 1);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, 0);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_upgrade_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}

	// render save button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_save);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_save);
		glVertexAttribPointer(gameplayOverlay_save_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_save);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 9);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, 0);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_save_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}

	// render load button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_load);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_load);
		glVertexAttribPointer(gameplayOverlay_load_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_load);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 10);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, 0);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_load_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}

	// render quit button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_exit);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_exit);
		glVertexAttribPointer(gameplayOverlay_exit_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_exit);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 11);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, 0);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_exit_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// render b1 button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_b1);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_b1);
		glVertexAttribPointer(gameplayOverlay_b1_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_b1);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 1);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/2);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_b1_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// render b2 button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_b2);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_b2);
		glVertexAttribPointer(gameplayOverlay_b2_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_b2);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 2);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/2);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_b2_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// render b3 button
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_gameplayOverlay_b3);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_gameplayOverlay_b3);
		glVertexAttribPointer(gameplayOverlay_b3_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_gameplayOverlay_b3);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("width");
			glUniform1f(location, button_width);
			CHECK_GL_ERRORS;

			// shadow map
			location = o_shader.getUniformLocation("height");
			glUniform1f(location, button_height);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/2 + button_width * 3);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/2);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gameplayOverlay_b3_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}

	// render energy
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_popupOverlay_energy_main);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_main);
		glVertexAttribPointer(popupOverlay_energy_positionAttribLocation_main, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_popupOverlay_energy_main);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, 90);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, o_height/3);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/8);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/6 + o_height/3);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, popupOverlay_energy_texture_main);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// energy number
	bool flag_nonzero = false;
	for (int i = 0; i < 6; ++i)
	{
		// number
		int needed = abs((int)(_window->getController()->getgameData()->getGrid()->energy));
		int d = 0;
		if ((i == 0) && (_window->getController()->getgameData()->getGrid()->energy < 0))
		{
			d = -1;
			flag_nonzero = true;
		}  else if (i > 0) {
			d = std::to_string(needed)[i - 1] - '0';
			flag_nonzero = flag_nonzero || (d != 0);
			if (!flag_nonzero)
			{
				continue;
			}
		} else {
			continue;
		}
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_popupOverlay_energy_number_main[i]);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_energy_number_main[i]);
		glVertexAttribPointer(popupOverlay_energy_number_positionAttribLocation_main[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_popupOverlay_energy_number_main[i]);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, 30);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, 159);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/8 + 90 + 30 * i);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/6 + o_height/3);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, number_texture[d]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// water energy
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_popupOverlay_water_main);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_main);
		glVertexAttribPointer(popupOverlay_water_positionAttribLocation_main, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_popupOverlay_water_main);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, 90);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, o_height/3);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/8);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/6);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, popupOverlay_water_texture_main);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// water number
	flag_nonzero = false;
	for (int i = 0; i < 6; ++i)
	{
		// number
		int needed = abs((int)(_window->getController()->getgameData()->getGrid()->water));
		int d = 0;
		if ((i == 0) && (_window->getController()->getgameData()->getGrid()->water < 0))
		{
			d = -1;
			flag_nonzero = true;
		}  else if (i > 0) {
			d = std::to_string(needed)[i - 1] - '0';
			flag_nonzero = flag_nonzero || (d != 0);
			if (!flag_nonzero)
			{
				continue;
			}
		} else {
			continue;
		}
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_popupOverlay_water_number_main[i]);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_water_number_main[i]);
		glVertexAttribPointer(popupOverlay_water_number_positionAttribLocation_main[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_popupOverlay_water_number_main[i]);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, 30);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, 159);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/8 + 90 + 30 * i);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/6);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, number_texture[d]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}

	// ore
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_popupOverlay_ore_main);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_main);
		glVertexAttribPointer(popupOverlay_ore_positionAttribLocation_main, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_popupOverlay_ore_main);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, 90);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, o_height/3);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/8);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/6 - o_height/3);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, popupOverlay_ore_texture_main);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
	// ore number
	flag_nonzero = false;
	for (int i = 0; i < 6; ++i)
	{
		// number
		int needed = abs((int)(_window->getController()->getgameData()->getGrid()->ore));
		int d = 0;
		if ((i == 0) && (_window->getController()->getgameData()->getGrid()->ore < 0))
		{
			d = -1;
			flag_nonzero = true;
		}  else if (i > 0) {
			d = std::to_string(needed)[i - 1] - '0';
			flag_nonzero = flag_nonzero || (d != 0);
			if (!flag_nonzero)
			{
				continue;
			}
		} else {
			continue;
		}
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_popupOverlay_ore_number_main[i]);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_popupOverlay_ore_number_main[i]);
		glVertexAttribPointer(popupOverlay_ore_number_positionAttribLocation_main[i], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(vao_popupOverlay_ore_number_main[i]);
		o_shader.enable();
		
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = o_shader.getUniformLocation("MVP");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(overlayMVP));
			CHECK_GL_ERRORS;
			
			// shadow map
			location = o_shader.getUniformLocation("TextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("width");
			glUniform1f(location, 30);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("height");
			glUniform1f(location, 159);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_x");
			glUniform1f(location, -o_width/8 + 90 + 30 * i);
			CHECK_GL_ERRORS;

			location = o_shader.getUniformLocation("buttom_left_y");
			glUniform1f(location, -o_height/6 - o_height/3);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, number_texture[d]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		o_shader.disable();
		
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
}

void tWindow::uploadMeshData(unsigned int meshID)
{
	gameData *theData = _window->getController()->getgameData();
	meshObject *dataObj = (meshObject *)(theData->getgameObject(meshID));

	if (dataObj->getName() == "select.blend")
	{
		barid = meshID;
	}

	// load texture
	string texturepath = dataObj->getTexturePath();
    glGenTextures(1, &(mesh_texture[meshID]));
    glBindTexture(GL_TEXTURE_2D, mesh_texture[meshID]);

	unsigned char header[54];
	unsigned int dataPos;
	unsigned int width, height;
	unsigned int imageSize;
	unsigned char * data;
	FILE * file = fopen(texturepath.c_str(), "rb");
	fread(header, 1, 54, file);
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);
	if (imageSize==0) imageSize=width*height*3;
 	if (dataPos==0) dataPos=54;
 	data = new unsigned char [imageSize];
 	fread(data, 1, imageSize, file);
 	fclose(file);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    delete [] data;

	glGenVertexArrays(1, &(vao_meshData[meshID]));
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_meshData[meshID]);

		// Enable the vertex shader attribute location for "position" when rendering.
		positionAttribLocation[meshID] = u_shader.getAttribLocation("position");
		glEnableVertexAttribArray(positionAttribLocation[meshID]);

		// for shadow shader
		s_positionAttribLocation[meshID] = s_shader.getAttribLocation("position");
		glEnableVertexAttribArray(s_positionAttribLocation[meshID]);

		// Enable the vertex shader attribute location for "normal" when rendering.
		normalAttribLocation[meshID] = u_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(normalAttribLocation[meshID]);
		
		UVAttribLocation[meshID] = u_shader.getAttribLocation("vertexUV");
		glEnableVertexAttribArray(UVAttribLocation[meshID]);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &(vbo_vertexPositions[meshID]));

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexPositions[meshID]);

		glBufferData(GL_ARRAY_BUFFER, dataObj->getVertexPositionsNum(),
				dataObj->getVertexPositionsPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// uv coords
	{
		glGenBuffers(1, &(vbo_UVPositions[meshID]));

		glBindBuffer(GL_ARRAY_BUFFER, vbo_UVPositions[meshID]);

		glBufferData(GL_ARRAY_BUFFER, dataObj->getUVPositionsNum(),
				dataObj->getUVPositionsPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex normal data
	{
		glGenBuffers(1, &(vbo_vertexNormals[meshID]));

		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexNormals[meshID]);

		glBufferData(GL_ARRAY_BUFFER, dataObj->getVertexNormalsNum(),
				dataObj->getVertexNormalsPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	meshDataIdxSize[meshID] = dataObj->getVertexPositionsNum();
}

void tWindow::initGridVertex()
{
	// generate vertex
	float *verts = new float[18];
	float dy = -0.7;

	verts[0] = -cubeSize / 2;
	verts[1] = dy;
	verts[2] = -cubeSize / 2;
	verts[3] = -cubeSize / 2;
	verts[4] = dy;
	verts[5] = cubeSize / 2;
	verts[6] = cubeSize / 2;
	verts[7] = dy;
	verts[8] = -cubeSize / 2;

	verts[9] = cubeSize / 2;
	verts[10] = dy;
	verts[11] = -cubeSize / 2;
	verts[12] = cubeSize / 2;
	verts[13] = dy;
	verts[14] = cubeSize / 2;
	verts[15] = -cubeSize / 2;
	verts[16] = dy;
	verts[17] = cubeSize / 2;

	glGenVertexArrays(1, &vao_grid);
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(vao_grid);

		// Enable the vertex shader attribute location for "position" when rendering.
		grid_positionAttribLocation = p_shader.getAttribLocation("position");
		glEnableVertexAttribArray(grid_positionAttribLocation);

		CHECK_GL_ERRORS;
	}
	glBindVertexArray(0);
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &vbo_grid);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_grid);

		glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float),
				verts, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	delete [] verts;
}

void tWindow::initPerspectiveMatrix()
{
	aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perpsective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 5000.0f);
}

void tWindow::initMatrix() 
{
	m_view = glm::lookAt(vec3(0.0f, 0.0f, 70.0f), vec3(0.0f, 0.0f, 0.0f),
			vec3(0.0f, 1.0f, 0.0f));
	//m_view = glm::translate(vec3(0.0f, 0.0f, -70.0f)) * m_view;
	//m_viewModel = glm::rotate(mat4(1), degreesToRadians(-90.0f), vec3(1, 0, 0));
}

void tWindow::initLightSources()
{
	// World-space position
	m_light.position = olightPos;
	m_light.rgbIntensity = vec3(1.0f); // White light
}

void tWindow::setgWindow(gWindow *_w)
{
	_window = _w;
}

void tWindow::shutdown()
{
	glfwSetWindowShouldClose(m_window, GL_TRUE);
}

void tWindow::queueProcessing()
{
	// time between two frames

	float duringTime = 1000 * float(1) / FPS;
	auto start_time = std::chrono::high_resolution_clock::now();

	eventQueue_lock->acquire();

	while (eventQueue.size() > 0) {

		// process an event
		gameEvent frontEvent = eventQueue.front();
		eventQueue.pop();
		eventQueue_lock->release();
		switch (frontEvent._type) 
		{
			case gameEventType::Event_None :
				break;
			case gameEventType::Event_MeshLoadingProgressUpdate:
				if (MeshLoadingProgress <= frontEvent.info[0])
				{
					// update MeshLoadingProgress
					MeshLoadingProgress = frontEvent.info[0];
				}
				//cout << "Loading: " << MeshLoadingProgress << endl;
				break;
			case gameEventType::Event_MeshUpload:
				mat_ks[frontEvent.info[0]] = vec3(frontEvent.info[1],frontEvent.info[2],frontEvent.info[3]);
				mat_shininess[frontEvent.info[0]] = frontEvent.info[4];
				uploadMeshData(frontEvent.info[0]);
				break;
			case gameEventType::Event_FinishedDataLoading:
				finishedLoading = true;
				break;
			case gameEventType::Event_TimeTick:
				tick();
				break;
			case gameEventType::Event_BuildFailed:
				// play build failed sound
				break;
			case gameEventType::Event_BuildSuc:
				soundEngine->play2D(linkSound.c_str(), false);
			default:
				break;
		}

		auto end_time = std::chrono::high_resolution_clock::now();
  		auto timeinterval = end_time - start_time;
  		if (std::chrono::duration_cast<std::chrono::milliseconds>(timeinterval).count() >= duringTime)
  		{
  			// abort to draw next frame
  			break;
  		} else {
  			eventQueue_lock->acquire();
  		}
	}

	eventQueue_lock->release();
}

void tWindow::appLogic() 
{
	queueProcessing();
}

void tWindow::guiLogic() 
{
	static bool firstRun(true);
	if (firstRun) {
		ImGui::SetNextWindowPos(ImVec2(50, 500));
		firstRun = false;
	}

	static bool showDebugWindow(true);
	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Menu", &showDebugWindow, ImVec2(200,200), opacity,
			windowFlags);
		if( ImGui::Button( "Load game" ) ) 
		{
			load();
		}
		if( ImGui::Button( "Save game" ) ) {
			save();
		}

		if( ImGui::Button( "Cheat in 500 unit of Energy" ) ) {
			_window->getController()->getgameData()->getGrid()->energy += 500;
		}

		if( ImGui::Button( "Cheat in 500 unit of Water" ) ) {
			_window->getController()->getgameData()->getGrid()->water += 500;
		}

		if( ImGui::Button( "Cheat in 500 unit of Ore" ) ) {
			_window->getController()->getgameData()->getGrid()->ore += 500;
		}

		if( ImGui::Button( "Fast forward TimeTicks" ) ) {
			for (int i = 0; i < ((float)FullTick / 4); ++i)
			{
				tick();
			}
		}


		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );
	ImGui::End();
}

void tWindow::draw() 
{
	if (finishedLoading)
	{
		// mouse picking
		offscreenDraw();
		glClearColor(0, 0, 0, 1);
		//glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], 1);
		renderSkybox();
		// call Grid to render
		renderScene();
		renderpopupOverlay();
		if (showmenu)
		{
			rendergameplayOverlay();
		}
	} else {
		renderLoadingOverlay();
	}
}

void tWindow::tick() 
{
	ticks++;
	appliedDelta = DNlightDelta;
	if ((ticks >= 0) && (ticks <= ((float)FullTick / 2)))
	{
		appliedDelta = DNlightDelta;
	}
	if (ticks >= ((float)FullTick / 2)) {
		appliedDelta = -DNlightDelta;
	}
	vec4 p(m_light.position, 1);
	p = glm::rotate(mat4(1), float(M_PI/(((float)FullTick) + 1)), vec3(1, 0, 1)) * p;
	m_light.position = vec3(p.x, p.y, p.z);
	//cout << to_string(DNlightDelta) << endl;
	if (ticks >= FullTick)
	{
		// switch day night cycle
		daynight++;
		daynight = daynight % 2;
		m_light.position = firstlightPos;
		ticks = 0;
	}
	if ((daynight == 0) && (ticks == 0)) {
		m_light.rgbIntensity = vec3(1.0f, 1.0f, 0.6f);
	}
	if (daynight == 0)
	{
		m_light.rgbIntensity += appliedDelta;
	}
	if (daynight == 1)
	{
		// night
		m_light.rgbIntensity = nightcolor;
	}

	if (finishedLoading && (ticks % resourceTicks == 0))
	{
		_window->getController()->getgameData()->getGrid()->resourceTick();
	}
}

void tWindow::cleanup() 
{

}

bool tWindow::cursorEnterWindowEvent(int entered) 
{
	bool eventHandled = false;
	return eventHandled;
}

bool tWindow::mouseMoveEvent(double xPos, double yPos) 
{
	bool eventHandled = false;

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		mx = xPos;
		my = yPos;
		ImGuiIO& io = ImGui::GetIO();
		float dx = 0;
		float dy = 0;
		float dz = 0;

		if (io.MouseDown[1])
		{
			// right mouse held
			if (drag_start_x == -1)
			{
				drag_start_x = xPos;
			} else {
				double dis_x = abs(xPos - drag_start_x);
				int qu = int(dis_x * rot_fact_x) / 360;
				double r = dis_x * rot_fact_x - qu;
				double nrot_x = rot_x;
				if (xPos < drag_start_x)
				{
					nrot_x = rot_x - (float)r;
				} else {
					nrot_x = rot_x + (float)r;
				}
				if (nrot_x >= 360)
				{
					nrot_x -= 360;
				}
				
				if (nrot_x <= 0)
				{
					nrot_x += 360;
				}
				rot_x = nrot_x;
				drag_start_x = -1;
			}
			if (drag_start_y == -1)
			{
				drag_start_y = yPos;
			} else {
				double dis_y = abs(yPos - drag_start_y);
				int qu = int(dis_y * rot_fact_y) / 360;
				double r = dis_y * rot_fact_y - qu;
				double nrot_y = rot_y;
				if (yPos < drag_start_y)
				{
					nrot_y = rot_y - (float)r;
				} else {
					nrot_y = rot_y + (float)r;
				}
				if (nrot_y >= 360)
				{
					nrot_y -= 360;
				}
				
				if (nrot_y <= 0)
				{
					nrot_y += 360;
				}
				if ((nrot_y >= min_camera_rotation_y) && (nrot_y <= max_camera_rotation_y))
				{
					rot_y = nrot_y;
				}
				
				drag_start_y = -1;
			}
		} else {
			drag_start_x = -1;
			drag_start_y = -1;
		}
	}

	return eventHandled;
}

bool tWindow::mouseButtonInputEvent(int button, int actions, int mods) 
{
	bool eventHandled = false;
	if (!ImGui::IsMouseHoveringAnyWindow())
	{
		ImGuiIO& io = ImGui::GetIO();
		// left click
		if (io.MouseDown[0])
		{
			if ((showmenu) && (my <= (m_windowWidth, m_windowHeight / 5)))
			{
				// convert mouse coords to button index
				int cx = (mx / (double(m_windowWidth) / (o_width / button_width)));
				int cy = (my / (double(m_windowHeight / 5) / (o_height / button_height)));
				//cout << cx << "," << cy << endl;
				// triger button click event
				bool playsound = false;
				if ((cy == 0))
				{
					if (cx == 0)
					{
						// upgrade button
						upgradeBuildings();
					} else if (cx == 9)
					{
						// save
						save();
					} else if (cx == 10) {
						// load
						playsound = true;
						load();
					} else if (cx == 11) {
						playsound = true;
						quit();
					}
				} else {
					if (cx == 0)
					{
						playsound = true;
						buildingID = cx;
					} else if (cx == 1) {
						playsound = true;
						buildingID = cx;
					} else if (cx == 2) {
						playsound = true;
						buildingID = cx;
					} else {
						playsound = true;
						buildingID = -1;
					}
				}
				// play button sound
				if (playsound)
				{
					soundEngine->play2D(buttonSound.c_str(), false);
				}
			} else {
				// mouse on game view
				soundEngine->play2D(buttonSound.c_str(), false);

				if ((buildingID != -1) && (mouseOver_x != -256) && (mouseOver_y != -256))
				{
					gameEvent event;
					event._type = gameEventType::Event_setBuilding;
					event.info.clear();
					event.info.push_back(mouseOver_x);
					event.info.push_back(mouseOver_y);
					event.info.push_back(buildingID);
					_window->getController()->toData(event);
					buildingID = -1;
				}
				if ((mouseOver_x != -256) && (mouseOver_y != -256) && (mouseClick_x == mouseOver_x) && (mouseClick_y == mouseOver_y))
				{
					mouseClick_x = -256;
					mouseClick_y = -256;
					return eventHandled;
				} else {
					mouseClick_x = mouseOver_x;
					mouseClick_y = mouseOver_y;
				}
			}
		}
	}
	return eventHandled;
}

bool tWindow::mouseScrollEvent(double xOffSet, double yOffSet) 
{
	bool eventHandled = false;
	vec4 v;

	if ((yOffSet > 0) && (scale + 1 <= max_scale))
	{
		scale += scale_fact;

	} else if ((yOffSet < 0) && (scale - 1 >= min_scale)) {
		scale -= scale_fact;
	}
	return eventHandled;
}

bool tWindow::windowResizeEvent(int width, int height) 
{
	bool eventHandled = false;
	
	initPerspectiveMatrix();
	initOverlayMVP();
	initpopupOverlayMVP();

	generateFactors();

	return eventHandled;
}

bool tWindow::keyInputEvent(int key, int action, int mods)
{
	bool eventHandled = false;

	if( action == GLFW_PRESS ) {
		if (key == 298) {
			// F9
			showmenu = !showmenu;
		}
	}

	return eventHandled;
}

void tWindow::renderMesh(glm::mat4 baseMat, unsigned int meshID, bool castShadow)
{
	if (!castingShadow)
	{
		// Bind VAO in order to record the data mapping.
		glBindVertexArray(vao_meshData[meshID]);
		// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
		// "position" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexPositions[meshID]);
		glVertexAttribPointer(positionAttribLocation[meshID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
		// "normal" vertex attribute location for any bound vertex shader program.
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexNormals[meshID]);
		glVertexAttribPointer(normalAttribLocation[meshID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_UVPositions[meshID]);
		glVertexAttribPointer(UVAttribLocation[meshID], 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;

		glEnable( GL_DEPTH_TEST );

		glBindVertexArray(vao_meshData[meshID]);
		u_shader.enable();
		{
			//-- Set Perpsective matrix uniform for the scene:
			GLint location = u_shader.getUniformLocation("Perspective");
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
			CHECK_GL_ERRORS;


			//-- Set LightSource uniform for the scene:
			{
				location = u_shader.getUniformLocation("light.position");
				glUniform3fv(location, 1, value_ptr(m_light.position));
				location = u_shader.getUniformLocation("light.rgbIntensity");
				glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
				CHECK_GL_ERRORS;
			}

			//-- Set background light ambient intensity
			{
				location = u_shader.getUniformLocation("ambientIntensity");
				vec3 ambientIntensity(0.05f);
				glUniform3fv(location, 1, value_ptr(ambientIntensity));
				CHECK_GL_ERRORS;
			}

			//-- Set ModelView matrix:
			location = u_shader.getUniformLocation("ModelView");
			mat4 modelView = m_view * getModelMatrix() * baseMat;
			// mat4 modelView = getViewMatrixfromLight() * baseMat;
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
			CHECK_GL_ERRORS;

			location = u_shader.getUniformLocation("NormalMatrix");
			mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
			glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
			CHECK_GL_ERRORS;

			// shadow matrix
			location = u_shader.getUniformLocation("DepthMVP");
			mat4 DepthMVP = glm::ortho(float(shadowResw/50), float(-shadowResw/50), float(-shadowResh/50), float(shadowResh/50), 0.0f, 800.0f)
			 * getViewMatrixfromLight() * baseMat;
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(DepthMVP));
			CHECK_GL_ERRORS;

			//-- Set Material values:
			location = u_shader.getUniformLocation("material.ks");
			vec3 ks = mat_ks[meshID];
			glUniform3fv(location, 1, value_ptr(ks));
			CHECK_GL_ERRORS;
			location = u_shader.getUniformLocation("material.shininess");
			double s = mat_shininess[meshID];
			glUniform1f(location, s);
			CHECK_GL_ERRORS;

			location = u_shader.getUniformLocation("fogColor");
			glUniform3fv(location, 1, value_ptr(fogColor));
			CHECK_GL_ERRORS;

			// mesh texture:
			location = u_shader.getUniformLocation("MeshTextureSampler");
			glUniform1i(location, 0);
			CHECK_GL_ERRORS;

			// shadow map
			location = u_shader.getUniformLocation("ShadowMapTextureSampler");
			glUniform1i(location, 1);
			CHECK_GL_ERRORS;

			// enable shadow
			location = u_shader.getUniformLocation("enableShadow");
			glUniform1f(location, true);
			CHECK_GL_ERRORS;
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mesh_texture[meshID]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glDrawArrays(GL_TRIANGLES, 0, meshDataIdxSize[meshID]);

		u_shader.disable();
		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	} else {
		if (!castShadow)
		{
			return;
		}
		glEnable( GL_DEPTH_TEST );

		glBindVertexArray(vao_meshData[meshID]);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexPositions[meshID]);
		glVertexAttribPointer(s_positionAttribLocation[meshID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		//-- Unbind target, and restore default values:
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		CHECK_GL_ERRORS;
		glBindVertexArray(vao_meshData[meshID]);
		// use shadow shader
		s_shader.enable();
		{
			//-- Set Perspective matrix uniform for the scene:
			GLint location = s_shader.getUniformLocation("Perspective");
			mat4 p = glm::ortho(float(shadowResw/50), float(-shadowResw/50), float(-shadowResh/50), float(shadowResh/50), 0.0f, 800.0f);
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(p));
			CHECK_GL_ERRORS;

			//-- Set ModelView matrix:
			location = s_shader.getUniformLocation("ModelView");
			mat4 modelView = getViewMatrixfromLight() * baseMat;
			glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
			CHECK_GL_ERRORS;
		}

		// draw depth buffer
		glDrawArrays(GL_TRIANGLES, 0, meshDataIdxSize[meshID]);
		s_shader.disable();

		glBindVertexArray(0);
		glDisable( GL_DEPTH_TEST );
	}
		
}

void tWindow::renderGrid(bool ColorID)
{
	if (castingShadow) 
	{
		// Grid doesnt cast shadow!
		return;
	}
	unsigned int gsize = _window->getController()->getgameData()->getGrid()->getGsize();
	// Set the background colour.
	if (ColorID)
	{
		glClearColor(0, 0, 0, 1.0);
	}

	for (int x = 0; x < (gsize + gsize + 1); ++x)
	{
		for (int y = 0; y < (gsize + gsize + 1); ++y)
		{
			if (((!ColorID) && (mouseOver_x != -256) && (mouseOver_y != -256) && (mouseOver_x + gsize == x) && (mouseOver_y + gsize == y)) ||
				((!ColorID) && (mouseClick_x != -256) && (mouseClick_y != -256) && (mouseClick_x + gsize == x) && (mouseClick_y + gsize == y)))
			{
				// Bind VAO in order to record the data mapping.
				unsigned int meshID = barid;
				glBindVertexArray(vao_meshData[meshID]);
				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexPositions[meshID]);
				glVertexAttribPointer(positionAttribLocation[meshID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
				// "normal" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexNormals[meshID]);
				glVertexAttribPointer(normalAttribLocation[meshID], 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				glBindBuffer(GL_ARRAY_BUFFER, vbo_UVPositions[meshID]);
				glVertexAttribPointer(UVAttribLocation[meshID], 2, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;

				glEnable( GL_DEPTH_TEST );

				glBindVertexArray(vao_meshData[meshID]);
				u_shader.enable();
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = u_shader.getUniformLocation("Perspective");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
					CHECK_GL_ERRORS;


					//-- Set LightSource uniform for the scene:
					{
						location = u_shader.getUniformLocation("light.position");
						glUniform3fv(location, 1, value_ptr(m_light.position));
						location = u_shader.getUniformLocation("light.rgbIntensity");
						glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
						CHECK_GL_ERRORS;
					}

					//-- Set background light ambient intensity
					{
						location = u_shader.getUniformLocation("ambientIntensity");
						vec3 ambientIntensity(0.05f);
						glUniform3fv(location, 1, value_ptr(ambientIntensity));
						CHECK_GL_ERRORS;
					}

					//-- Set ModelView matrix:
					location = u_shader.getUniformLocation("ModelView");
					mat4 m_model = glm::translate(
						vec3(cubeSize * (float(x) - float(gsize)), 
							0, 
							cubeSize * (float(y) - float(gsize))));
					mat4 mv = m_view * getModelMatrix(m_model);
					// mat4 modelView = getViewMatrixfromLight() * baseMat;
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(mv));
					CHECK_GL_ERRORS;

					location = u_shader.getUniformLocation("NormalMatrix");
					mat3 normalMatrix = glm::transpose(glm::inverse(mat3(mv)));
					glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
					CHECK_GL_ERRORS;

					// shadow matrix
					location = u_shader.getUniformLocation("DepthMVP");
					mat4 DepthMVP = mat4(1);
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(DepthMVP));
					CHECK_GL_ERRORS;

					//-- Set Material values:
					location = u_shader.getUniformLocation("material.ks");
					vec3 ks = mat_ks[meshID];
					glUniform3fv(location, 1, value_ptr(ks));
					CHECK_GL_ERRORS;
					location = u_shader.getUniformLocation("material.shininess");
					double s = mat_shininess[meshID];
					glUniform1f(location, s);
					CHECK_GL_ERRORS;

					location = u_shader.getUniformLocation("fogColor");
					glUniform3fv(location, 1, value_ptr(fogColor));
					CHECK_GL_ERRORS;

					// mesh texture:
					location = u_shader.getUniformLocation("MeshTextureSampler");
					glUniform1i(location, 0);
					CHECK_GL_ERRORS;

					// shadow map
					location = u_shader.getUniformLocation("ShadowMapTextureSampler");
					glUniform1i(location, 1);
					CHECK_GL_ERRORS;

					// enable shadow
					location = u_shader.getUniformLocation("enableShadow");
					glUniform1f(location, false);
					CHECK_GL_ERRORS;
				}
				glActiveTexture(GL_TEXTURE0);
				if ((!ColorID) && (mouseClick_x != -256) && (mouseClick_y != -256) && (mouseClick_x + gsize == x) && (mouseClick_y + gsize == y))
				{
					glBindTexture(GL_TEXTURE_2D, selected_texture);
				} else {
					glBindTexture(GL_TEXTURE_2D, mesh_texture[meshID]);
				}
				glDrawArrays(GL_TRIANGLES, 0, meshDataIdxSize[meshID]);

				u_shader.disable();
				glBindVertexArray(0);
				glDisable( GL_DEPTH_TEST );
			} else if (ColorID) {
				// Bind VAO in order to record the data mapping.
				glBindVertexArray(vao_grid);

				// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
				// "position" vertex attribute location for any bound vertex shader program.
				glBindBuffer(GL_ARRAY_BUFFER, vbo_grid);
				glVertexAttribPointer(grid_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

				//-- Unbind target, and restore default values:
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				CHECK_GL_ERRORS;

				glEnable( GL_DEPTH_TEST );

				glBindVertexArray(vao_grid);
				p_shader.enable();
				{
					//-- Set Perpsective matrix uniform for the scene:
					GLint location = p_shader.getUniformLocation("Perspective");
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perpsective));
					CHECK_GL_ERRORS;

					//-- Set ModelView matrix:
					location = p_shader.getUniformLocation("ModelView");
					mat4 m_model = glm::translate(
						vec3(cubeSize * (float(x) - float(gsize)), 
							0, 
							cubeSize * (float(y) - float(gsize))));
					mat4 mv = m_view * getModelMatrix(m_model);
					glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(mv));
					CHECK_GL_ERRORS;

					location = p_shader.getUniformLocation("colorID");
					vec3 cid = CoordToColorID(x, y);
					glUniform3fv(location, 1, value_ptr(cid));

				}
				glDrawArrays(GL_TRIANGLES, 0, 18);
				p_shader.disable();

				glBindVertexArray(0);

				glDisable( GL_DEPTH_TEST );
			}
		}
	}
}

void tWindow::renderScene()
{
	// render shadow
	castingShadow = true;

	// set up offscreen frame buffer

	// call Grid to render under Shadow shader on offscreen frame buffer

	// store image as Shadow map
	GLuint m_fbo;
	glGenFramebuffers(1, &m_fbo);
	glViewport(0, 0, shadowResw, shadowResh);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowResw, shadowResh, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

    glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glClear(GL_DEPTH_BUFFER_BIT);

	_window->getController()->getgameData()->getGrid()->render(mat4(1), this);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	castingShadow = false;

	// render normally
	glViewport(0, 0, m_windowWidth, m_windowHeight);
	_window->getController()->getgameData()->getGrid()->render(mat4(1), this);

	glDeleteTextures(1, &depthTexture);
	glDeleteFramebuffers(1, &m_fbo);
}

mat4 tWindow::getModelMatrix()
{
	mat4 m_viewModel;
	// horizontal rotation
	m_viewModel = glm::rotate(m_viewModel, degreesToRadians(rot_x), vec3(0, 1, 0));
	// vertical rotation, rotate arount (view x rot_x)
	vec3 rot_axis(m_viewModel[0][0], m_viewModel[1][0], m_viewModel[2][0]);
	//rot_axis = glm::cross(rot_axis, vec3(0, rot_x, 0));

	m_viewModel = glm::rotate(m_viewModel, degreesToRadians(rot_y), rot_axis);
	m_viewModel = glm::scale(m_viewModel, vec3(scale, scale, scale));
	return m_viewModel;
}

mat4 tWindow::getViewMatrixfromLight()
{
	vec3 direction = normalize(vec3(0, 0, 0) - m_light.position);
	vec4 pos = vec4(m_light.position, 1);
	pos = glm::translate(mat4(1), direction * 40) * pos;
	vec3 pos3 = vec3(pos.x, pos.y, pos.z);
	mat4 n_view = glm::lookAt(pos3, vec3(0.0f, 0.0f, 0.0f),
			vec3(0.0f, 1.0f, 0.0f));

	return n_view;
}

mat4 tWindow::getModelMatrix(mat4 translation)
{
	mat4 m_viewModel;
	// horizontal rotation
	m_viewModel = glm::rotate(m_viewModel, degreesToRadians(rot_x), vec3(0, 1, 0));
	// vertical rotation, rotate arount (view x rot_x)
	vec3 rot_axis(m_viewModel[0][0], m_viewModel[1][0], m_viewModel[2][0]);
	//rot_axis = glm::cross(rot_axis, vec3(0, rot_x, 0));

	m_viewModel = glm::rotate(m_viewModel, degreesToRadians(rot_y), rot_axis);
	m_viewModel = glm::scale(m_viewModel, vec3(scale, scale, scale));
	m_viewModel = m_viewModel * translation;
	return m_viewModel;
}

void tWindow::offscreenDraw()
{
	glClearColor(1, 0, 0, 0);
	// render buffer
	GLuint m_fbo;
	GLuint colorrenderBuffer;
	GLuint depthrenderBuffer;
	glGenRenderbuffers(1, &colorrenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorrenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, m_windowWidth, m_windowHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// depth buffer
	glGenRenderbuffers(1, &depthrenderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_windowWidth, m_windowHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorrenderBuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderGrid(true);

	// picking color
	glFlush();
	glFinish(); 

	GLubyte pixels[3];
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glReadPixels(mx, m_windowHeight - my - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixels);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteRenderbuffers(1, &colorrenderBuffer);
	glDeleteRenderbuffers(1, &depthrenderBuffer);
	glDeleteFramebuffers(1, &m_fbo);

	if (pixels[0] == 0)
	{
		ColorIDToCoord(mouseOver_x, mouseOver_y, vec3(pixels[0], pixels[1], pixels[2]));
	} else {
		mouseOver_x = -256;
		mouseOver_y = -256;
	}

	// cout << mouseOver_x << "," << mouseOver_y << endl;

	glDisable(GL_DEPTH_TEST);
}

vec3 tWindow::CoordToColorID(int x, int y)
{
	unsigned int gsize = _window->getController()->getgameData()->getGrid()->getGsize();

	vec3 cid(0, 0, 0);
	cid[1] = float(x) / 255;
	cid[2] = float(y) / 255;

	return cid;
}
void tWindow::ColorIDToCoord(int &x, int &y, glm::vec3 ColorID)
{
	unsigned int gsize = _window->getController()->getgameData()->getGrid()->getGsize();

	int nx = ColorID[1] - float(gsize);
	int ny = ColorID[2] - float(gsize);

	x = nx;
	y = ny;
}

void gWindow::onSave(string savePath)
{
	_window->onSave(savePath);
}

void gWindow::onLoad(string savePath)
{
	_window->onLoad(savePath);
}

void tWindow::onSave(std::string savePath)
{
	ofstream savefile;
	savefile.open(savePath, ios::app);
	savefile << "[tWindow]" << endl;
	savefile << "scale=" << scale << endl;

	savefile << "rot_x=" << rot_x << endl;
	savefile << "rot_y=" << rot_y << endl;

	savefile << "ticks=" << ticks << endl;
	savefile << "buildingID=" << buildingID << endl;
	savefile << "daynight=" << daynight << endl;  
	savefile << "appliedDelta0=" << appliedDelta[0] << endl;
	savefile << "appliedDelta1=" << appliedDelta[1] << endl;
	savefile << "appliedDelta2=" << appliedDelta[2] << endl;

	savefile.close();
}

void tWindow::onLoad(std::string savePath)
{
	INIReader reader(savePath);
	scale = reader.GetReal("tWindow", "scale", scale);

	rot_x = reader.GetReal("tWindow", "rot_x", rot_x);
	rot_y = reader.GetReal("tWindow", "rot_y", rot_y);

	ticks = reader.GetInteger("tWindow", "ticks", ticks);
	buildingID = reader.GetInteger("tWindow", "buildingID", buildingID);
	daynight = reader.GetInteger("tWindow", "daynight", daynight);  
	appliedDelta[0] = reader.GetReal("tWindow", "appliedDelta0", appliedDelta[0]);
	appliedDelta[1] = reader.GetReal("tWindow", "appliedDelta1", appliedDelta[1]);
	appliedDelta[2] = reader.GetReal("tWindow", "appliedDelta2", appliedDelta[2]);
}

void tWindow::save()
{
	_window->getController()->getgameData()->onSave();
}

void tWindow::load()
{
	_window->getController()->getgameData()->onLoad();
}

void tWindow::quit()
{
	glfwSetWindowShouldClose(m_window, GL_TRUE);
}

void tWindow::upgradeBuildings()
{
	if ((mouseClick_x != -256) || (mouseClick_y != -256))
	{
		if (_window->getController()->getgameData()->getGrid()->checkupgrade(mouseClick_x, mouseClick_y))
		{
			_window->getController()->getgameData()->getGrid()->upgrade(mouseClick_x, mouseClick_y);
			soundEngine->play2D(upgradeSound.c_str(), false);
		}
	}
	
}

void tWindow::setSelectBar(class selectbarObject *_s)
{
	selectbar = _s;
}