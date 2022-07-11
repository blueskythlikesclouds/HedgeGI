#pragma once

#include "Component.h"

struct Label
{
    const char* name;
    const char* desc;

    Label(const char* name) : name(name), desc(nullptr) {}
    Label(const char* name, const char* desc) : name(name), desc(desc) {}
};

class UIComponent : public Component
{
protected:
    static char tmp[256];
    static const char* makeName(const char* name);

public:
    static bool beginProperties(const char* name, ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame);
    static void beginProperty(const char* name, float width = -1);
    static bool property(const Label& label, enum ImGuiDataType_ dataType, void* data);
    static bool property(const Label& label, bool& data);
    static bool property(const Label& label, Color3& data);
    static bool property(const Label& label, char* data, size_t dataSize, float width = -1);
    static bool property(const Label& label, std::string& data);
    static bool property(const Label& label, Eigen::Array3i& data);

    template <typename T>
    bool property(const char* label, const std::initializer_list<std::pair<const Label, T>>& values, T& data)
    {
        beginProperty(label);

        const char* preview = "None";
        for (auto& value : values)
        {
            if (value.second != data)
                continue;

            preview = value.first.name;
            break;
        }

        if (!ImGui::BeginCombo(makeName(label), preview))
            return false;

        bool any = false;

        for (auto& value : values)
        {
            const bool result = ImGui::Selectable(value.first.name);

            tooltip(value.first.desc);

            if (!result)
                continue;

            data = value.second;
            any = true;
        }

        ImGui::EndCombo();
        return any;
    }

    static bool dragProperty(const Label& label, float& data, float speed = 0.1f, float min = 0, float max = 0);
    static bool dragProperty(const Label& label, Vector3& data, float speed = 0.1f, float min = 0, float max = 0);
    static void endProperties();

    static void tooltip(const char* desc, bool editing = false);
};
