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
    if (ImGui::IsKeyDown('1'))
    {
        Notify(1);
    }
    else if(ImGui::IsKeyDown('2'))
    {
        Notify(2);
    }
    else if (ImGui::IsKeyDown('3'))
    {
        Notify(3);
    }
}

KeyboardController::~KeyboardController() {
    std::cout << "Goodbye, I was the Subject.\n";
}

void KeyboardController::Attach(IObserver* observer)
{
    list_observer_.push_back(observer);
}

void KeyboardController::Detach(IObserver* observer)
{
    list_observer_.remove(observer);
}

void KeyboardController::Notify(__int8 keyNum)
{
    std::list<IObserver*>::iterator iterator = list_observer_.begin();

    while (iterator != list_observer_.end()) {
        (*iterator)->Update(keyNum);
        ++iterator;
    }
}