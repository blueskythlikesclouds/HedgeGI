#include "UIComponent.h"

bool UIComponent::beginProperties(const char* name)
{
    return ImGui::BeginTable(name, 2);
}

void UIComponent::beginProperty(const char* label, const float width)
{
    ImGui::TableNextColumn();
    const ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ cursorPos.x + 4, cursorPos.y + 4 });
    ImGui::TextUnformatted(label);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(width);
}

bool UIComponent::property(const char* label, const enum ImGuiDataType_ dataType, void* data)
{
    beginProperty(label);
    return ImGui::InputScalar((std::string("##") + label).c_str(), dataType, data);
}

bool UIComponent::property(const char* label, bool& data)
{
    beginProperty(label);
    return ImGui::Checkbox((std::string("##") + label).c_str(), &data);
}

bool UIComponent::property(const char* label, Color3& data)
{
    beginProperty(label);
    return ImGui::ColorEdit3((std::string("##") + label).c_str(), data.data());
}

bool UIComponent::property(const char* label, char* data, size_t dataSize, const float width)
{
    beginProperty(label, width);
    return ImGui::InputText((std::string("##") + label).c_str(), data, dataSize);
}

bool UIComponent::property(const char* label, Eigen::Array3i& data)
{
    beginProperty(label);
    return ImGui::InputInt3(label, data.data());
}

bool UIComponent::dragProperty(const char* label, float& data, float speed, float min, float max)
{
    beginProperty(label);
    return ImGui::DragFloat((std::string("##") + label).c_str(), &data, speed, min, max);
}

bool UIComponent::dragProperty(const char* label, Vector3& data, float speed, float min, float max)
{
    beginProperty(label);
    return ImGui::DragFloat3(label, data.data(), speed, min, max);
}

void UIComponent::endProperties()
{
    ImGui::EndTable();
}