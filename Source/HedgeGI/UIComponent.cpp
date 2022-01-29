#include "UIComponent.h"

char UIComponent::tmp[256];

const char* UIComponent::makeName(const char* name)
{
    strcpy(tmp, "##");
    strcpy(tmp + 2, name);
    return tmp;
}

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
    return ImGui::InputScalar(makeName(label), dataType, data);
}

bool UIComponent::property(const char* label, bool& data)
{
    beginProperty(label);
    return ImGui::Checkbox(makeName(label), &data);
}

bool UIComponent::property(const char* label, Color3& data)
{
    beginProperty(label);
    return ImGui::ColorEdit3(makeName(label), data.data());
}

bool UIComponent::property(const char* label, char* data, size_t dataSize, const float width)
{
    beginProperty(label, width);
    return ImGui::InputText(makeName(label), data, dataSize);
}

bool UIComponent::property(const char* label, Eigen::Array3i& data)
{
    beginProperty(label);
    return ImGui::InputInt3(label, data.data());
}

bool UIComponent::dragProperty(const char* label, float& data, float speed, float min, float max)
{
    beginProperty(label);
    return ImGui::DragFloat(makeName(label), &data, speed, min, max);
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