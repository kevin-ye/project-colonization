#pragma once

#include "game-framework/gameWindow.hpp"
#include "game-framework/OpenGLImport.hpp"
#include "game-framework/ShaderProgram.hpp"
#include "game-framework/MeshConsolidator.hpp"
#include <glm/glm.hpp>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <map>
#include <queue>
#include <irrKlang.h>

#include "gameEvent.hpp"

class gWindow
{
	class tWindow *_window;
	std::thread *_thread;
	class gameControl *_control;

	static void main(class gWindow *_window, class mLock *main_thread_ready_lock);

	void run(class mLock *main_thread_ready_lock);
	void init();

protected:

public:
	gWindow(class mLock *main_thread_ready_lock);
	~gWindow();

	class tWindow *get_tWindow();

	void setController(class gameControl *ctrl);
	class gameControl *getController();

	void shutdown();

	void pushEvent(gameEvent _e);

	void onSave(std::string savePath);
	void onLoad(std::string savePath);

	void setSelectBar(class selectbarObject *_s);
};

struct LightSource 
{
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

class tWindow : public gameWindow {
	class gWindow *_window;

	bool finishedLoading;

	GLint P_uni; // Uniform location for Projection matrix.
	GLint MV_uni; // Uniform location for ModelView matrix.

	// Matrices controlling the camera and projection.
	glm::mat4 m_perpsective;
	glm::mat4 m_view;

	// extra load
	// selected 
	GLuint selected_texture;
	GLuint plus_texture;
	void loadextra();

	//-- GL resources for mesh geometry data:
	std::map<unsigned int, GLuint> mesh_texture;
	std::map<unsigned int, GLuint> vao_meshData;
	std::map<unsigned int, unsigned int> meshDataIdxSize;
	std::map<unsigned int, GLuint> vbo_vertexPositions;
	std::map<unsigned int, GLuint> vbo_UVPositions;
	std::map<unsigned int, GLuint> vbo_vertexNormals;
	std::map<unsigned int, GLint> positionAttribLocation;
	std::map<unsigned int, GLint> s_positionAttribLocation;
	std::map<unsigned int, GLint> normalAttribLocation;
	std::map<unsigned int, GLint> UVAttribLocation;

	std::map<int, GLuint> number_texture;
	void loadNumber();

	//material
	std::map<unsigned int, glm::vec3> mat_ks;
	std::map<unsigned int, double> mat_shininess;

	//-- GL resources for grid data:
	GLuint vao_grid;
	GLuint vbo_grid;
	GLint grid_positionAttribLocation;

	// general shader
	ShaderProgram u_shader;
	// picking shader
	ShaderProgram p_shader;
	// Shadow map shader
	ShaderProgram s_shader;
	// Skybox shader
	ShaderProgram b_shader;
	// Overlay shader
	ShaderProgram o_shader;

	// overlay
	double o_width;
	double o_height;
	double button_height;
	double button_width;

	//-- GL resources for overlay
	GLuint vao_overlay_loading;
	GLuint vbo_overlay_loading;
	GLuint overlay_texture_loading;
	GLint overlay_positionAttribLocation;
	GLuint vao_overlay_progressbar;
	GLuint vbo_overlay_progressbar;
	GLuint overlay_texture_progressbar;
	GLint overlay_progressbar_positionAttribLocation;
	// popup overlay

	glm::mat4 overlayMVP;
	glm::mat4 popupoverlayMVP;

	void initOverlayMVP();
	void initpopupOverlayMVP();
	void loadingOverlay();
	void renderLoadingOverlay();

	//-- GL resources for gameplayOverlay
	GLuint vao_gameplayOverlay_background;
	GLuint vbo_gameplayOverlay_background;
	GLuint gameplayOverlay_background_texture;
	GLint gameplayOverlay_background_positionAttribLocation;
	// upgrade button
	GLuint vao_gameplayOverlay_upgrade;
	GLuint vbo_gameplayOverlay_upgrade;
	GLuint gameplayOverlay_upgrade_texture;
	GLint gameplayOverlay_upgrade_positionAttribLocation;
	// save button
	GLuint vao_gameplayOverlay_save;
	GLuint vbo_gameplayOverlay_save;
	GLuint gameplayOverlay_save_texture;
	GLint gameplayOverlay_save_positionAttribLocation;
	// load button
	GLuint vao_gameplayOverlay_load;
	GLuint vbo_gameplayOverlay_load;
	GLuint gameplayOverlay_load_texture;
	GLint gameplayOverlay_load_positionAttribLocation;
	// exit button
	GLuint vao_gameplayOverlay_exit;
	GLuint vbo_gameplayOverlay_exit;
	GLuint gameplayOverlay_exit_texture;
	GLint gameplayOverlay_exit_positionAttribLocation;
	// b1 button
	GLuint vao_gameplayOverlay_b1;
	GLuint vbo_gameplayOverlay_b1;
	GLuint gameplayOverlay_b1_texture;
	GLint gameplayOverlay_b1_positionAttribLocation;
	// b2 button
	GLuint vao_gameplayOverlay_b2;
	GLuint vbo_gameplayOverlay_b2;
	GLuint gameplayOverlay_b2_texture;
	GLint gameplayOverlay_b2_positionAttribLocation;
	// b3 button
	GLuint vao_gameplayOverlay_b3;
	GLuint vbo_gameplayOverlay_b3;
	GLuint gameplayOverlay_b3_texture;
	GLint gameplayOverlay_b3_positionAttribLocation;
	// main resource display
	GLuint vao_popupOverlay_energy_main;
	GLuint vbo_popupOverlay_energy_main;
	GLuint popupOverlay_energy_texture_main;
	GLint popupOverlay_energy_positionAttribLocation_main;
	std::map<unsigned int, GLuint> vao_popupOverlay_energy_number_main;
	std::map<unsigned int, GLuint> vbo_popupOverlay_energy_number_main;
	std::map<unsigned int, GLint> popupOverlay_energy_number_positionAttribLocation_main;
	GLuint vao_popupOverlay_water_main;
	GLuint vbo_popupOverlay_water_main;
	GLuint popupOverlay_water_texture_main;
	GLint popupOverlay_water_positionAttribLocation_main;
	std::map<unsigned int, GLuint> vao_popupOverlay_water_number_main;
	std::map<unsigned int, GLuint> vbo_popupOverlay_water_number_main;
	std::map<unsigned int, GLint> popupOverlay_water_number_positionAttribLocation_main;
	GLuint vao_popupOverlay_ore_main;
	GLuint vbo_popupOverlay_ore_main;
	GLuint popupOverlay_ore_texture_main;
	GLint popupOverlay_ore_positionAttribLocation_main;
	std::map<unsigned int, GLuint> vao_popupOverlay_ore_number_main;
	std::map<unsigned int, GLuint> vbo_popupOverlay_ore_number_main;
	std::map<unsigned int, GLint> popupOverlay_ore_number_positionAttribLocation_main;
	void loadgameplayOverlay();
	void rendergameplayOverlay();

	//-- GL resources for popupOverlay
	GLuint vao_popupOverlay_background;
	GLuint vbo_popupOverlay_background;
	GLuint popupOverlay_background_texture;
	GLint popupOverlay_background_positionAttribLocation;
	GLuint vao_popupOverlay_energy;
	GLuint vbo_popupOverlay_energy;
	GLuint popupOverlay_energy_texture;
	GLint popupOverlay_energy_positionAttribLocation;
	std::map<unsigned int, GLuint> vao_popupOverlay_energy_number;
	std::map<unsigned int, GLuint> vbo_popupOverlay_energy_number;
	std::map<unsigned int, GLint> popupOverlay_energy_number_positionAttribLocation;
	GLuint vao_popupOverlay_water;
	GLuint vbo_popupOverlay_water;
	GLuint popupOverlay_water_texture;
	GLint popupOverlay_water_positionAttribLocation;
	std::map<unsigned int, GLuint> vao_popupOverlay_water_number;
	std::map<unsigned int, GLuint> vbo_popupOverlay_water_number;
	std::map<unsigned int, GLint> popupOverlay_water_number_positionAttribLocation;
	GLuint vao_popupOverlay_ore;
	GLuint vbo_popupOverlay_ore;
	GLuint popupOverlay_ore_texture;
	GLint popupOverlay_ore_positionAttribLocation;
	std::map<unsigned int, GLuint> vao_popupOverlay_ore_number;
	std::map<unsigned int, GLuint> vbo_popupOverlay_ore_number;
	std::map<unsigned int, GLint> popupOverlay_ore_number_positionAttribLocation;
	float popupWidth;
	float popupHeight;
	float p_width;
	float p_height;
	float number_height;
	float number_width;
	void loadpopupOverlay();
	void renderpopupOverlay();

	bool showmenu;

	LightSource m_light;
	// shadow texture
	GLuint depthTexture;

	// sound engine
	irrklang::ISoundEngine *soundEngine;

	void createShaderProgram();
	void uploadMeshData(unsigned int meshID);
	void initGridVertex();
	void initPerspectiveMatrix();
	void initMatrix();
	void initLightSources();
	void generateFactors();

	void loadSkybox();
	void renderSkybox();
	GLuint skybox_vao;
	GLuint skybox_vbo;
	GLint skybox_vertexPos;
	GLuint skybox_texture;

	void upgradeBuildings();

	class mLock *eventQueue_lock;
	std::queue<gameEvent> eventQueue;

	double MeshLoadingProgress;

	void queueProcessing();
	void renderScene();

	float aspect;

	float scale;
	float scale_fact;
	int max_scale;
	int min_scale;

	float rot_x;
	float rot_y;
	double drag_start_x;
	double drag_start_y;
	float rot_fact_x;
	float rot_fact_y;
	double max_camera_rotation_y;
	double min_camera_rotation_y;

	double mx;
	double my;

	int mouseOver_x;
	int mouseOver_y;
	int mouseClick_x;
	int mouseClick_y;
	class selectbarObject *selectbar;
	unsigned int barid;

	unsigned int ticks;
	int buildingID;
	int daynight;  // 0 = day, 1 = night
	glm::vec3 appliedDelta;

	glm::mat4 getModelMatrix();
	glm::mat4 getViewMatrixfromLight();
	glm::mat4 getModelMatrix(glm::mat4 translation);

	glm::vec3 CoordToColorID(int x, int y);
	void ColorIDToCoord(int &x, int &y, glm::vec3 ColorID);

	void offscreenDraw();
	bool castingShadow;
	double shadowResw;
	double shadowResh;

	void tick();
	void save();
	void load();
	void quit();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

public:
	tWindow();
	~tWindow();

	void setgWindow(class gWindow *_w);
	void shutdown();

	void pushEvent(gameEvent _e);

	void renderMesh(glm::mat4 baseMat, unsigned int meshID, bool castShadow);
	void renderGrid(bool ColorID);

	void onSave(std::string savePath);
	void onLoad(std::string savePath);

	void setSelectBar(class selectbarObject *_s);
};