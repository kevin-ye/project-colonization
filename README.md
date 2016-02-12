About the game
==============
The purpose of this project is to create a city builder/resource management game with as many gaming graphical features as possible.
The scenario of this game is that the player is managing a space colonization program on the surface of another planet, whose main goal is to collect and utilize the resource in order to survive and extend the base of colonization.

Features
========
* Time-based skybox/lighting
* Shadow mapping
* Distance fog (GLSL shader)
* UV texture mapping
* GameObject, texture, sound, model and game balance definition files are under GameData/ folder
* UI overlay (orthogonal projection)
* Mouseover overlay
* Buildings auto connection
* MVC pattern
* Async event handling
* Save/load game state

Note
====
* Shadow mapping is not working on Mac OS
* Game is not balanced yet, but everything can be modified in GameData/ folder

Environment/Requirement
===========
Graphics card supporting OpenGL 4
* Linux: stable
* Mac: unstable, WIP, no shadow mapping

Build/Compile
==============
under project/ folder
```shell
$ premake4
$ make clean
$ make
$ ./run
```

Libraries/Credits
==================
* Assimp
  * Loading various formats of 3D model files
* irrKlang
  * Basic sound engine library(no source, dynamically linked)

KeyBind
=======
* F9
  * Toggle in game Overlay
* Mouse
  - Left click to select a type of building, then left click to place it on the grid
  - Mouse over a building on the Overlay shows the needed resource to build it
  - Mouse over an existing building on the grid shows the running resource of it
 
