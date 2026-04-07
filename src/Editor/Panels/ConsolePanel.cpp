#include "ConsolePanel.h"

#include <imgui.h>
#include <cstring>

#include <Editor/Core/EditorContext.h>
#include "Editor/Logs/QEEditorConsole.h"

ConsolePanel::ConsolePanel(EditorContext* editorContext, QEEditorConsole* console)
    : _editorContext(editorContext)
    , _console(console)
{
    _filter[0] = '\0';
}

bool ConsolePanel::PassesLevelFilter(QELogLevel level) const
{
    switch (level)
    {
    case QELogLevel::Info:    return _showInfo;
    case QELogLevel::Warning: return _showWarnings;
    case QELogLevel::Error:   return _showErrors;
    case QELogLevel::Debug:   return _showDebug;
    default:                  return true;
    }
}

const char* ConsolePanel::GetLevelText(QELogLevel level) const
{
    switch (level)
    {
    case QELogLevel::Info:    return "Info";
    case QELogLevel::Warning: return "Warning";
    case QELogLevel::Error:   return "Error";
    case QELogLevel::Debug:   return "Debug";
    default:                  return "Unknown";
    }
}

void ConsolePanel::Draw()
{
    if (!_editorContext || !_editorContext->ShowConsole)
        return;

    if (!ImGui::Begin("Console", &_editorContext->ShowConsole))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear"))
    {
        if (_console)
        {
            _console->Clear();
        }
    }

    if (_console)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &_console->AutoScroll);
    }

    ImGui::Separator();

    ImGui::Checkbox("Info", &_showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &_showWarnings);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &_showErrors);
    ImGui::SameLine();
    ImGui::Checkbox("Debug", &_showDebug);

    ImGui::InputTextWithHint("##ConsoleFilter", "Filter...", _filter, sizeof(_filter));

    ImGui::Separator();
    ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (_console)
    {
        const auto entries = _console->GetEntriesSnapshot();

        for (const auto& entry : entries)
        {
            if (!PassesLevelFilter(entry.Level))
                continue;

            if (_filter[0] != '\0')
            {
                const std::string filter = _filter;
                const bool matchMessage = entry.Message.find(filter) != std::string::npos;
                const bool matchCategory = entry.Category.find(filter) != std::string::npos;

                if (!matchMessage && !matchCategory)
                    continue;
            }

            switch (entry.Level)
            {
            case QELogLevel::Info:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
                break;
            case QELogLevel::Warning:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                break;
            case QELogLevel::Error:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
                break;
            case QELogLevel::Debug:
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                break;
            }

            if (!entry.Category.empty())
            {
                ImGui::TextWrapped("[%s] [%s] %s",
                    GetLevelText(entry.Level),
                    entry.Category.c_str(),
                    entry.Message.c_str());
            }
            else
            {
                ImGui::TextWrapped("[%s] %s",
                    GetLevelText(entry.Level),
                    entry.Message.c_str());
            }

            ImGui::PopStyleColor();
        }

        if (_console->AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 10.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }

    ImGui::EndChild();
    ImGui::End();
}
