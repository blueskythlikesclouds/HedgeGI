#include "UIComponent.h"

char UIComponent::tmp[256];

const char* UIComponent::makeName(const char* name)
{
    strcpy(tmp, "##");
    strcpy(tmp + 2, name);
    return tmp;
}

bool UIComponent::beginProperties(const char* name, ImGuiTableFlags flags)
{
    const bool result = ImGui::BeginTable(name, 2, flags, { ImGui::GetContentRegionAvail().x, 0 });

    if (result)
        ImGui::PushTextWrapPos();

    return result;
}

void UIComponent::beginProperty(const char* name, const float width)
{
    ImGui::TableNextColumn();
    const ImVec2 cursorPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({ cursorPos.x + 4, cursorPos.y + 4 });
    ImGui::TextUnformatted(name);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(width);
}

bool UIComponent::property(const Label& label, const enum ImGuiDataType_ dataType, void* data)
{
    beginProperty(label.name);
    const bool result = ImGui::InputScalar(makeName(label.name), dataType, data);
    tooltip(label.desc, result);
    return result;
}

bool UIComponent::property(const Label& label, bool& data)
{
    beginProperty(label.name);
    const bool result = ImGui::Checkbox(makeName(label.name), &data);
    tooltip(label.desc, result);
    return result;
}

bool UIComponent::property(const Label& label, Color3& data)
{
    beginProperty(label.name);
    const bool result = ImGui::ColorEdit3(makeName(label.name), data.data());
    tooltip(label.desc, result);
    return result;
}

bool UIComponent::property(const Label& label, char* data, size_t dataSize, const float width)
{
    beginProperty(label.name, width);
    const bool result = ImGui::InputText(makeName(label.name), data, dataSize);
    tooltip(label.desc, result);
    return result;
}

bool UIComponent::property(const Label& label, std::string& data)
{
    char buf[1024];
    strcpy(buf, data.c_str());

    if (!property(label, buf, sizeof buf))
        return false;

    data = buf;
    return true;
}

bool UIComponent::property(const Label& label, Eigen::Array3i& data)
{
    beginProperty(label.name);
    const bool result = ImGui::InputInt3(makeName(label.name), data.data());
    tooltip(label.desc, result);
    return result;
}

bool UIComponent::dragProperty(const Label& label, float& data, float speed, float min, float max)
{
    beginProperty(label.name);
    const bool result = ImGui::DragFloat(makeName(label.name), &data, speed, min, max);
    tooltip(label.desc, result);
    return result;
}

bool UIComponent::dragProperty(const Label& label, Vector3& data, float speed, float min, float max)
{
    beginProperty(label.name);
    const bool result = ImGui::DragFloat3(makeName(label.name), data.data(), speed, min, max);
    tooltip(label.desc, result);
    return result;
}

void UIComponent::endProperties()
{
    ImGui::PopTextWrapPos();
    ImGui::EndTable();
}

void UIComponent::tooltip(const char* desc, const bool editing)
{
    if (!desc || !ImGui::IsItemHovered() || GImGui->HoveredIdTimer <= 0.5f)
        return;

    if (editing)
    {
        GImGui->HoveredIdTimer = 0.0f;
        return;
    }

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}
