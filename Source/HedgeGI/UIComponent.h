#pragma once

#include "Component.h"

class UIComponent : public Component
{
protected:
    static char tmp[256];
    static const char* makeName(const char* name);

public:
    static bool beginProperties(const char* name, ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame);
    static void beginProperty(const char* label, float width = -1);
    static bool property(const char* label, enum ImGuiDataType_ dataType, void* data);
    static bool property(const char* label, bool& data);
    static bool property(const char* label, Color3& data);
    static bool property(const char* label, char* data, size_t dataSize, float width = -1);
    static bool property(const char* label, std::string& data);
    static bool property(const char* label, Eigen::Array3i& data);

    template <typename T>
    bool property(const char* label, const std::initializer_list<std::pair<const char*, T>>& values, T& data)
    {
        beginProperty(label);

        const char* preview = "None";
        for (auto& value : values)
        {
            if (value.second != data)
                continue;

            preview = value.first;
            break;
        }

        if (!ImGui::BeginCombo(makeName(label), preview))
            return false;

        bool any = false;

        for (auto& value : values)
        {
            if (!ImGui::Selectable(value.first))
                continue;

            data = value.second;
            any = true;
        }

        ImGui::EndCombo();
        return any;
    }

    static bool dragProperty(const char* label, float& data, float speed = 0.1f, float min = 0, float max = 0);
    static bool dragProperty(const char* label, Vector3& data, float speed = 0.1f, float min = 0, float max = 0);

    static void endProperties();
};
