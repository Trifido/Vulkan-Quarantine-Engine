#include "KeyboardController.h"
#include <iostream>
#include <imgui.h>

void KeyboardController::ReadKeyboardEvents()
{
    if (isEditorProfile)
    {
        ReadPolygonModeKeys();
    }
}

void KeyboardController::CleanLastResources()
{
    for (auto var : list_observer_)
    {
        delete var;
        var = nullptr;
    }

    list_observer_.clear();
}

void KeyboardController::ReadPolygonModeKeys()
{
    if (ImGui::IsKeyDown(ImGuiKey_1))
    {
        Notify(1);
    }
    else if(ImGui::IsKeyDown(ImGuiKey_2))
    {
        Notify(2);
    }
    else if (ImGui::IsKeyDown(ImGuiKey_3))
    {
        Notify(3);
    }
}

KeyboardController::~KeyboardController() {

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
