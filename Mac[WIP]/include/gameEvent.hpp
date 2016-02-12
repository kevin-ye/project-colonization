#pragma once

#include <cstdlib>
#include <cstdio>
#include <vector>

enum class gameEventType {
	Event_None,
	Event_TimeTick,
	Event_setBuilding,
	Event_Shutdown,
    Event_keyboard,
    Event_mouseClick,
    Event_mouseMove,
    Event_mouseDrag,
    Event_controlFlow,
    Event_MeshLoadingProgressUpdate,
    Event_MeshUpload,
    Event_FinishedDataLoading,
    Event_BuildFailed,
    Event_BuildSuc
};

class gameEvent
{
public:
	gameEventType _type;
	std::vector<double> info;
};