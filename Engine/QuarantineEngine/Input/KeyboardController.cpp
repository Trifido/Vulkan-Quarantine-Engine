#include "KeyboardController.h"
#include <iostream>
#include <imgui.h>

KeyboardController* KeyboardController::instance = nullptr;

KeyboardController* KeyboardController::getInstance()
{
	if (instance == NULL)
		instance = new KeyboardController();
	else
		std::cout << "Getting existing KeyboardController instance" << std::endl;

	return instance;
}

void KeyboardController::ReadKeyboardEvents()
{
    if (isEditorProfile)
    {
        ReadPolygonModeKeys();
    }
}

void KeyboardController::cleanup()
{
    if (instance != NULL)
    {
        delete instance;
        instance = NULL;
    }
}

void KeyboardController::ReadPolygonModeKeys()
{
    if (ImGui::IsKeyPressed('1'))
    {
        __raise this->PolygonModeEvent(1);
    }
    else if(ImGui::IsKeyPressed('2'))
    {
        __raise this->PolygonModeEvent(2);
    }
    else if (ImGui::IsKeyPressed('3'))
    {
        __raise this->PolygonModeEvent(3);
    }
}
