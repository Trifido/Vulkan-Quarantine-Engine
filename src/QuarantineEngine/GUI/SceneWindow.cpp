#include "SceneWindow.h"

void SceneWindow::renderScene()
{
    ImGui::Begin("Scene");

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    mSize = { viewportPanelSize.x, viewportPanelSize.y };

    // add rendered texture to ImGUI scene window
    ImTextureID textureID = 1;// mFrameBuffer->get_texture();
    ImGui::Image(textureID, ImVec2{ mSize.x, mSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

    ImGui::End();
}
