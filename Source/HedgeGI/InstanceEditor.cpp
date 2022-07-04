#include "InstanceEditor.h"
#include "Stage.h"
#include "StageParams.h"
#include "Instance.h"
#include "Math.h"

void InstanceEditor::update(float deltaTime)
{
    if (!ImGui::CollapsingHeader("Instances"))
        return;

    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchInstances", search, sizeof(search)) || strlen(search) > 0;

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginListBox("##Instances"))
    {
        for (auto& instance : stage->getScene()->instances)
        {
            const uint16_t resolution = params->propertyBag.get<uint16_t>(instance->name + ".resolution", 256);

            char name[1024];
            sprintf(name, "%s (%dx%d)", instance->name.c_str(), resolution, resolution);

            if (doSearch && !strstr(name, search))
                continue;

            if (ImGui::Selectable(name, selection == instance.get()))
                selection = instance.get();
        }

        ImGui::EndListBox();
    }

    if (selection != nullptr)
    {
        uint16_t resolution = params->propertyBag.get<uint16_t>(selection->name + ".resolution", 256);
        if (beginProperties("##Instance Settings"))
        {
            if (property("Selected Instance Resolution", ImGuiDataType_U16, &resolution))
                params->propertyBag.set(selection->name + ".resolution", nextPowerOfTwo(resolution));

            endProperties();
            ImGui::Separator();
        }
    }

    if (beginProperties("##Resolution Settings"))
    {
        property("Resolution Base", ImGuiDataType_Float, &params->bakeParams.resolution.base);
        property("Resolution Bias", ImGuiDataType_Float, &params->bakeParams.resolution.bias);
        property("Resolution Minimum", ImGuiDataType_U16, &params->bakeParams.resolution.min);
        property("Resolution Maximum", ImGuiDataType_U16, &params->bakeParams.resolution.max);
        endProperties();
    }

    if (ImGui::Button("Compute New Resolutions"))
    {
        for (auto& instance : stage->getScene()->instances)
        {
            uint16_t resolution = nextPowerOfTwo((int)exp2f(params->bakeParams.resolution.bias + logf(getRadius(instance->aabb)) / logf(params->bakeParams.resolution.base)));
            resolution = std::max(params->bakeParams.resolution.min, std::min(params->bakeParams.resolution.max, resolution));

            params->propertyBag.set(instance->name + ".resolution", resolution);
        }
    }
}
