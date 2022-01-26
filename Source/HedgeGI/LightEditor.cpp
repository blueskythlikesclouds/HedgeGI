#include "LightEditor.h"

#include "CameraController.h"
#include "Im3DManager.h"
#include "Stage.h"
#include "StageParams.h"
#include "Light.h"
#include "Math.h"
#include "PackService.h"
#include "StateManager.h"

void LightEditor::update(float deltaTime)
{
    if (!ImGui::CollapsingHeader("Lights"))
        return;

    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchLights", search, sizeof(search)) || strlen(search) > 0;

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginListBox("##Lights"))
    {
        for (auto& light : stage->getScene()->lights)
        {
            if (doSearch && light->name.find(search) == std::string::npos)
                continue;

            if (ImGui::Selectable(light->name.c_str(), selection == light.get()))
                selection = light.get();
        }

        ImGui::EndListBox();
    }

    if (ImGui::Button("Add"))
    {
        std::unique_ptr<Light> light = std::make_unique<Light>();
        light->type = LightType::Point;
        light->position = get<CameraController>()->current.getNewObjectPosition();
        light->color = Color3::Ones();
        light->range = Vector4(0, 0, 0, 3);

        char name[16];
        sprintf(name, "Omni%03d", (int)stage->getScene()->lights.size());
        light->name = name;

        selection = light.get();
        stage->getScene()->lights.push_back(std::move(light));
        params->dirtyBVH = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("Clone"))
    {
        if (selection != nullptr)
        {
            std::unique_ptr<Light> light = std::make_unique<Light>(*selection);

            char name[16];
            sprintf(name, "Omni%03d", (int)stage->getScene()->lights.size());
            light->name = name;

            selection = light.get();
            stage->getScene()->lights.push_back(std::move(light));
            params->dirtyBVH = true;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove"))
    {
        if (selection != nullptr)
        {
            for (auto it = stage->getScene()->lights.begin(); it != stage->getScene()->lights.end(); ++it)
            {
                if ((*it).get() != selection)
                    continue;

                stage->getScene()->lights.erase(it);
                params->dirtyBVH = true;

                break;
            }

            selection = nullptr;
        }
    }

    if (selection != nullptr)
    {
        Im3d::DrawSphere(*(const Im3d::Vec3*)selection->position.data(), selection->range.w());

        Im3d::PushLayerId(IM3D_TRANSPARENT_DISCARD_ID);
        {
            params->dirtyBVH |= Im3d::GizmoTranslation(selection->name.c_str(), selection->position.data());
        }
        Im3d::PopLayerId();

        if (beginProperties("##Light Settings"))
        {
            char name[0x400];
            strcpy(name, selection->name.c_str());

            if (property("Name", name, sizeof(name)))
                selection->name = name;

            if (selection->type == LightType::Point)
                params->dirtyBVH |= dragProperty("Position", selection->position);

            else if (selection->type == LightType::Directional)
                params->dirty |= dragProperty("Direction", selection->position);

            // Store color/intensity in the property bag, however we should reset the values
            // if the light values were modified in the stage files by the user
            Color3 color =
            {
                params->propertyBag.get<float>(selection->name + ".color.x()"),
                params->propertyBag.get<float>(selection->name + ".color.y()"),
                params->propertyBag.get<float>(selection->name + ".color.z()"),
            };

            float intensity = params->propertyBag.get<float>(selection->name + ".intensity");

            if (color.maxCoeff() > 1.0f || !nearlyEqual<Color3>(color * intensity, selection->color))
            {
                intensity = selection->color.maxCoeff();

                if (intensity > 1.0f)
                    intensity *= 2.0f;
                else
                    intensity = 1.0f;

                color = selection->color / intensity;
            }

            // Convert to SRGB when dealing with HE2
            if (params->bakeParams.targetEngine == TargetEngine::HE2)
                color = color.pow(1.0f / 2.2f);

            params->dirty |= property("Color", color);
            params->dirty |= dragProperty("Color Intensity", intensity, 0.01f, 0, INFINITY);

            if (params->bakeParams.targetEngine == TargetEngine::HE2)
                color = color.pow(2.2f);

            selection->color = color * intensity;

            params->propertyBag.set(selection->name + ".color.x()", color.x());
            params->propertyBag.set(selection->name + ".color.y()", color.y());
            params->propertyBag.set(selection->name + ".color.z()", color.z());
            params->propertyBag.set(selection->name + ".intensity", intensity);

            if (selection->type == LightType::Point)
            {
                if (params->bakeParams.targetEngine == TargetEngine::HE1)
                {
                    params->dirtyBVH |= dragProperty("Inner Radius", selection->range.z(), 0.1f, 0.0f, selection->range.w());
                    params->dirtyBVH |= dragProperty("Outer Radius", selection->range.w(), 0.1f, selection->range.z(), INFINITY);
                }
                else if (params->bakeParams.targetEngine == TargetEngine::HE2)
                {
                    if (dragProperty("Radius", selection->range.w()))
                    {
                        selection->range.z() = selection->range.w();
                        selection->range.y() = selection->range.w() / 2.0f;

                        params->dirtyBVH |= true;
                    }
                }
            }

            endProperties();
        }

        if (ImGui::Button("Save Changes"))
            get<StateManager>()->packResources(PackResourceMode::Light);
    }

    params->dirty |= params->dirtyBVH;
}
