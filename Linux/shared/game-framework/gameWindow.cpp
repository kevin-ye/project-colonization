#include "gameWindow.hpp"
#include "game-framework/Exception.hpp"
#include "game-framework/OpenGLImport.hpp"

#include <sstream>
#include <iostream>
#include <thread>

#include <imgui/imgui.h>
#include <imgui_impl_glfw_gl3.h>

using namespace std;

//-- Forward Declarations:
extern "C" {
	int gl3wInit(void);
}
static void renderImGui (int framebufferWidth, int framebufferHeight);

//-- Static member initialization:
string gameWindow::m_exec_dir = ".";
shared_ptr<gameWindow> gameWindow::m_instance = nullptr;


static void printGLInfo();


//----------------------------------------------------------------------------------------
// Constructor
gameWindow::gameWindow()
 : m_window(nullptr),
   m_monitor(nullptr),
   m_windowTitle(),
   m_windowWidth(0),
   m_windowHeight(0),
   m_framebufferWidth(0),
   m_framebufferHeight(0),
   m_paused(false),
   m_fullScreen(false)
{

}

//----------------------------------------------------------------------------------------
// Destructor
gameWindow::~gameWindow() {
	// Free all GLFW resources.
	glfwTerminate();
}

//----------------------------------------------------------------------------------------
/*
 * Error callback to be registered with GLFW.
 */
void gameWindow::errorCallback(
		int error,
		const char * description
) {
	stringstream msg;
	msg << "GLFW Error Code: " << error << "\n" <<
			"GLFW Error Description: " << description << "\n";
	throw Exception(msg.str());
}

//----------------------------------------------------------------------------------------
/*
 * Window resize event callback to be registered with GLFW.
 */
void gameWindow::windowResizeCallBack (
		GLFWwindow * window,
		int width,
		int height
) {
	getInstance()->gameWindow::windowResizeEvent(width, height);
	getInstance()->windowResizeEvent(width, height);
}

//----------------------------------------------------------------------------------------
/*
 * Key input event callback to be registered with GLFW.
 */
void gameWindow::keyInputCallBack (
		GLFWwindow * window,
		int key,
		int scancode,
		int action,
		int mods
) {
	if(!getInstance()->keyInputEvent(key, action, mods)) {
		// Send event to parent class for processing.
		getInstance()->gameWindow::keyInputEvent(key, action, mods);
	}
}

//----------------------------------------------------------------------------------------
/*
 * Mouse scroll event callback to be registered with GLFW.
 */
void gameWindow::mouseScrollCallBack (
		GLFWwindow * window,
		double xOffSet,
		double yOffSet
) {
	getInstance()->mouseScrollEvent(xOffSet, yOffSet);
}


//----------------------------------------------------------------------------------------
/*
 * Mouse button event callback to be registered with GLFW.
 */
void gameWindow::mouseButtonCallBack (
		GLFWwindow * window,
		int button,
		int actions,
		int mods
) {
	getInstance()->mouseButtonInputEvent(button, actions, mods);
}

//----------------------------------------------------------------------------------------
/*
 *  Mouse move event callback to be registered with GLFW.
 */
void gameWindow::mouseMoveCallBack (
		GLFWwindow * window,
		double xPos,
		double yPos
) {
	getInstance()->mouseMoveEvent(xPos, yPos);
}

//----------------------------------------------------------------------------------------
/*
 * Cursor enter window event callback to be registered with GLFW.
 */
void gameWindow::cursorEnterWindowCallBack (
		GLFWwindow * window,
		int entered
) {
	getInstance()->cursorEnterWindowEvent(entered);
}

//----------------------------------------------------------------------------------------
/*
 * Event handler. Handles window resize events.
 */
bool gameWindow::windowResizeEvent (
		int width,
		int height
) {
	m_windowWidth = width;
	m_windowHeight = height;

	return false;
}


//----------------------------------------------------------------------------------------
/*
 * Event handler. Handles key input events.
 */
bool gameWindow::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
			eventHandled = true;

		} else if (key == GLFW_KEY_P) {
			m_paused = !m_paused;
			if(m_paused) {
				static bool showPauseWindow(true);
				ImGui::SetNextWindowPosCenter();
				ImGui::SetNextWindowSize(ImVec2(m_framebufferWidth*0.5,
						m_framebufferHeight*0.5));
				ImGui::Begin("PAUSED", &showPauseWindow, ImVec2(m_framebufferWidth*0.5,
								m_framebufferHeight*0.5), 0.1f, ImGuiWindowFlags_NoTitleBar);
				ImGui::Text("Paused");
				ImGui::Text("Press P to continue...");
				ImGui::End();
				renderImGui(m_framebufferWidth, m_framebufferHeight);
				glfwSwapBuffers(m_window);
			}
			eventHandled = true;
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler. Handles mouse scroll events.
 */
bool gameWindow::mouseScrollEvent (
		double xOffSet,
		double yOffSet
) {
	return false;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler. Handles mouse button events.
 */
bool gameWindow::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	return false;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler. Handles mouse move events.
 */
bool gameWindow::mouseMoveEvent (
		double xPos,
		double yPos
) {
	return false;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler. Handles mouse cursor entering window area events.
 */
bool gameWindow::cursorEnterWindowEvent (
		int entered
) {
	return false;
}

//----------------------------------------------------------------------------------------
void gameWindow::centerWindow() {
	int windowWidth, windowHeight;
	glfwGetWindowSize(m_window, &windowWidth, &windowHeight);

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	if (monitor == NULL) {
		return;
	}

	int x, y;
	const GLFWvidmode *videoMode = glfwGetVideoMode(monitor);
	x = videoMode->width;
	y = videoMode->height;

	x = (x - windowWidth) / 2;
	y = (y - windowHeight) / 2;

	glfwSetWindowPos(m_window, x, y);
}

//----------------------------------------------------------------------------------------
/*
 * Register callbacks with GLFW, and associate events with the current GLFWwindow.
 */
void gameWindow::registerGlfwCallBacks() {
	glfwSetKeyCallback(m_window, keyInputCallBack);
	glfwSetWindowSizeCallback(m_window, windowResizeCallBack);
	glfwSetScrollCallback(m_window, mouseScrollCallBack);
	glfwSetMouseButtonCallback(m_window, mouseButtonCallBack);
	glfwSetCursorPosCallback(m_window, mouseMoveCallBack);
	glfwSetCursorEnterCallback(m_window, cursorEnterWindowCallBack);
}
//----------------------------------------------------------------------------------------
/*
 * Returns the static instance of class gameWindow.
 */
shared_ptr<gameWindow> gameWindow::getInstance()
{
    static gameWindow * instance = new gameWindow();
    if (m_instance == nullptr) {
        // Pass ownership of instance to shared_ptr.
        m_instance = shared_ptr<gameWindow>(instance);
    }
    return m_instance;
}

//----------------------------------------------------------------------------------------
void gameWindow::launch (
		gameWindow *window,
		int width,
		int height,
		const std::string& title, 
		float fps
) {

	if( m_instance == nullptr ) {
        m_instance = shared_ptr<gameWindow>(window);
		m_instance->run( width, height, title, fps );
	}
}

//----------------------------------------------------------------------------------------
static void renderImGui (
		int framebufferWidth,
		int framebufferHeight
) {
	// Set viewport to full window size.
	glViewport(0, 0, framebufferWidth, framebufferHeight);
	ImGui::Render();
}

//----------------------------------------------------------------------------------------
void gameWindow::run (
		int width,
		int height,
		const string &windowTitle,
		float desiredFramesPerSecond
) {
	m_windowTitle = windowTitle;
    m_windowWidth = width;
    m_windowHeight = height;
	glfwSetErrorCallback(errorCallback);

    if (glfwInit() == GL_FALSE) {
	    throw Exception("Call to glfwInit() failed.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);

    m_monitor = glfwGetPrimaryMonitor();
    if (m_monitor == NULL) {
        glfwTerminate();
        throw Exception("Error retrieving primary m_monitor.");
    }

    m_window = glfwCreateWindow(width, height, windowTitle.c_str(), NULL, NULL);
    if (m_window == NULL) {
        glfwTerminate();
        throw Exception("Call to glfwCreateWindow failed.");
    }

    // Get default framebuffer dimensions in order to support high-definition
    // displays.
    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);

    centerWindow();
    glfwMakeContextCurrent(m_window);
	gl3wInit();
    
#ifdef DEBUG_GL
    printGLInfo();
#endif

    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    registerGlfwCallBacks();

	// Setup ImGui binding.  Tell the ImGui subsystem not to
	// bother setting up its callbacks -- ours will do just fine here.
	ImGui_ImplGlfwGL3_Init( m_window, false );

    // Clear error buffer.
    while(glGetError() != GL_NO_ERROR);

    try {
        // Wait until m_monitor refreshes before swapping front and back buffers.
        // To prevent tearing artifacts.
        glfwSwapInterval(1);

		// Call client-defined startup code.
        init();

        // steady_clock::time_point frameStartTime;

        // Main Program Loop:
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
			ImGui_ImplGlfwGL3_NewFrame();

            if (!m_paused) {
				// Apply application-specific logic
	            appLogic();

	            guiLogic();

				// Ask the derived class to do the actual OpenGL drawing.
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                draw();

	            // In case of a window resize, get new framebuffer dimensions.
	            glfwGetFramebufferSize(m_window, &m_framebufferWidth,
			            &m_framebufferHeight);

	            // Draw any UI controls specified in guiLogic() by derived class.
	            renderImGui(m_framebufferWidth, m_framebufferHeight);

				// Finally, blast everything to the screen.
                glfwSwapBuffers(m_window);
            }

        }
        
    } catch (const  std::exception & e) {
        std::cerr << "Exception Thrown: ";
        std::cerr << e.what() << endl;
    } catch (...) {
        std::cerr << "Uncaught exception thrown!  Terminating Program." << endl;
    }

    cleanup();
    glfwDestroyWindow(m_window);
}


//----------------------------------------------------------------------------------------
void gameWindow::init() {
	// Render only the front face of geometry.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// Setup depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	glEnable(GL_DEPTH_CLAMP);

	glClearDepth(1.0f);
	glClearColor(0.3, 0.5, 0.7, 1.0);

}

//----------------------------------------------------------------------------------------
void gameWindow::appLogic() {

}

//----------------------------------------------------------------------------------------
void gameWindow::draw() {

}

//----------------------------------------------------------------------------------------
void gameWindow::guiLogic() {

}

//----------------------------------------------------------------------------------------
void gameWindow::cleanup() {

}

//----------------------------------------------------------------------------------------
/*
 * Used to print OpenGL version and supported extensions to standard output stream.
 */
static void printGLInfo() {
	const GLubyte * renderer = glGetString( GL_RENDERER );
	const GLubyte * vendor = glGetString( GL_VENDOR );
	const GLubyte * version = glGetString( GL_VERSION );
	const GLubyte * glsl_version = glGetString( GL_SHADING_LANGUAGE_VERSION );

	GLint majorVersion, minorVersion;

	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);

	stringstream message;

	if (vendor)
		message << "GL Vendor          : " << vendor << endl;

	if (renderer)
		message << "GL Renderer        : " << renderer << endl;

	if (version)
		message << "GL Version         : " << version << endl;

	message << "GL Version         : " << majorVersion << "." << minorVersion << endl
			<< "GLSL Version       : " << glsl_version << endl;

	cout << message.str();

	cout << "Supported Extensions: " << endl;
	const GLubyte * extension;
	GLint nExtensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);

	for (GLuint i = 0; i < nExtensions; ++i) {
		extension = glGetStringi(GL_EXTENSIONS, i);
		if (extension) {
			cout << extension << endl;
		}
	}
}

//----------------------------------------------------------------------------------------
std::string gameWindow::getAssetFilePath(const char *base)
{
	return m_exec_dir + "/Assets/" + base;
}
