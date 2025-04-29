#include "LightEditor.h"

#include "BakingFactory.h"
#include "CameraController.h"
#include "Im3DManager.h"
#include "Input.h"
#include "Light.h"
#include "Math.h"
#include "PackService.h"
#include "resource.h"
#include "Stage.h"
#include "StageParams.h"
#include "StateManager.h"
#include "ViewportWindow.h"

const char* const LIGHT_SEARCH_DESC = "Type in a light name to search for.";
const char* const LIGHT_ADD_DESC = "Adds a new omni light.";
const char* const LIGHT_CLONE_DESC = "Clones the selected light.";
const char* const LIGHT_REMOVE_DESC = "Removes the selected light.";

const Label LIGHT_NAME_LABEL = { "Name",
    "Name of the light." };

const Label LIGHT_POSITION_LABEL = { "Position",
    "Position of the light." };

const Label LIGHT_DIRECTION_LABEL = { "Direction",
    "Direction of the light.\n\n"
    "Do not edit this manually. Use the gizmo in the viewport." };

const Label LIGHT_COLOR_LABEL = { "Color",
    "Color of the light." };

const Label LIGHT_COLOR_INTENSITY_LABEL = { "Color Intensity",
    "Intensity of the light.\n\n"
    "For HE1, do not have very high intensity values as light maps are unable to store such values properly.\n\n"
    "For HE2, you are going to need very intense lights to get good results." };

const Label LIGHT_INNER_RADIUS_LABEL = { "Inner Radius",
    "Controls the sharpness of the falloff." };

const Label LIGHT_OUTER_RADIUS_LABEL = { "Outer Radius",
    "Radius of the light." };

const Label LIGHT_RADIUS_LABEL = { "Radius",
    "Radius of the light." };

const Label SHADOW_RADIUS_LABEL = { "Shadow Radius",
    "Radius of the shadow. Smaller values will produce sharper shadows whereas higher values will soften them." };

const Label CAST_SHADOW_LABEL = { "Cast Shadow",
    "Whether the light is going to cast a shadow." };

const char* const LIGHT_SAVE_CHANGES_DESC = "Packs every changed light into stage files.";

void LightEditor::drawBillboardsAndUpdateSelection()
{
    const auto viewportWindow = get<ViewportWindow>();
    if (!viewportWindow->show)
        return;

    const auto camera = get<CameraController>();
    const auto im3d = get<Im3DManager>();
    const auto input = get<Input>();
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    const Light* current = nullptr;
    float currentDepth = INFINITY;

    stage->getScene()->getLightBVH().traverse(camera->frustum, [&](const Light* light)
    {
        if (light->type != LightType::Point)
            return;

        const Color3 color = light->color / light->computeIntensity();

        const Billboard* billboard = im3d->drawBillboard(light->position, 1.0f, Color4(color.x(), color.y(), color.z(), 1.0f), lightBulb);

        if (!billboard || !viewportWindow->isFocused() || params->dirtyBVH ||
            !input->tappedMouseButtons[GLFW_MOUSE_BUTTON_LEFT] || !im3d->checkCursorOnBillboard(*billboard))
            return;

        bool proceed;

        if (nearlyEqual(billboard->depth, currentDepth))
            proceed = light == selection || current != selection;
        else
            proceed = billboard->depth < currentDepth;

        if (!proceed)
            return;

        current = light;
        currentDepth = billboard->depth;
    });

    if (current)
        selection = const_cast<Light*>(current);
}

LightEditor::LightEditor() : lightBulb(ResRawData_LightBulb)
{
}

static void makeName(const Scene& scene, const char* originalName, char* destName, size_t destNameSize)
{
    char prefix[0x100];
    strcpy_s(prefix, originalName);
    size_t length = strlen(prefix);
    size_t index = 0;
    size_t digits = 1;
    while (length > 0 && std::isdigit(prefix[length - 1]))
    {
        --length;
        index += (prefix[length] - '0') * digits;
        digits *= 10;
        prefix[length] = '\0';
    }
    if (digits == 1)
        index = scene.lights.size();

    while (true)
    {
        sprintf_s(destName, destNameSize, "%s%03lld", prefix, index);

        bool found = false;
        for (auto& light : scene.lights)
        {
            if (light->name == destName)
            {
                found = true;
                break;
            }
        }

        if (!found)
            break;

        ++index;
    }
}

void LightEditor::update(float deltaTime)
{
    if (!ImGui::CollapsingHeader("Lights"))
        return;

    const auto stage = get<Stage>();
    const auto params = get<StageParams>();
    const auto im3d = get<Im3DManager>();

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchLights", search, sizeof(search)) || strlen(search) > 0;

    tooltip(LIGHT_SEARCH_DESC);

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

        if (stage->getScene()->getLightBVH().getSunLight() == nullptr)
        {
            light->type = LightType::Directional;
            light->position = -Vector3::UnitY();
            light->color = Color3::Ones();
            light->name = "Direct01";
        }
        else
        {
            light->type = LightType::Point;
            light->position = get<CameraController>()->getNewObjectPosition();
            light->color = Color3::Ones();
            light->range = Vector4(0, 0, 0, 3);

            char name[0x100];
            ::makeName(*stage->getScene(), "Omni", name, sizeof(name));
            light->name = name;
        }

        selection = light.get();
        stage->getScene()->lights.push_back(std::move(light));
        params->dirtyBVH = true;
    }

    tooltip(LIGHT_ADD_DESC);

    ImGui::SameLine();

    if (ImGui::Button("Clone"))
    {
        if (selection != nullptr)
        {
            std::unique_ptr<Light> light = std::make_unique<Light>(*selection);

            char name[0x100];
            ::makeName(*stage->getScene(), light->name.c_str(), name, sizeof(name));
            light->name = name;

            selection = light.get();
            stage->getScene()->lights.push_back(std::move(light));
            params->dirtyBVH = true;
        }
    }

    tooltip(LIGHT_CLONE_DESC);

    ImGui::SameLine();

    if (ImGui::Button("Remove"))
    {
        if (selection != nullptr)
        {
            for (auto it = stage->getScene()->lights.begin(); it != stage->getScene()->lights.end(); ++it)
            {
                if ((*it).get() != selection)
                    continue;

                it = stage->getScene()->lights.erase(it);

                if (it != stage->getScene()->lights.end())
                    selection = it->get();
                else if (!stage->getScene()->lights.empty())
                    selection = stage->getScene()->lights.back().get();
                else
                    selection = nullptr;

                params->dirtyBVH = true;

                break;
            }
        }
    }

    tooltip(LIGHT_REMOVE_DESC);

    if (selection != nullptr)
    {
        Im3d::PushSize(IM3D_LINE_SIZE);
        Im3d::DrawSphere(*(const Im3d::Vec3*)selection->position.data(), selection->range.w());
        Im3d::PopSize();

        Im3d::PushLayerId(IM3D_TRANSPARENT_DISCARD_ID);
        {
            if (selection->type == LightType::Point)
                params->dirtyBVH |= Im3d::GizmoTranslation(selection->name.c_str(), selection->position.data());

            else
            {
                Affine3 position;
                position = Eigen::Translation3f(im3d->getCamera().getNewObjectPosition());

                Im3d::PushMatrix(*(const Im3d::Mat4*)position.data());

                Im3d::PushSize(IM3D_LINE_SIZE * 2.0f);
                Im3d::DrawArrow(Im3d::Vec3(0), *(const Im3d::Vec3*)selection->position.data());
                Im3d::PopSize();

                Matrix3 rotation;
                rotation.col(0) = selection->position.cross(Vector3::UnitX());
                rotation.col(1) = selection->position;
                rotation.col(2) = selection->position.cross(Vector3::UnitZ());

                if (Im3d::GizmoRotation(selection->name.c_str(), rotation.data()))
                {
                    selection->position = rotation.col(1).normalized();
                    params->dirty = true;
                }

                Im3d::PopMatrix();
            }
        }
        Im3d::PopLayerId();

        if (beginProperties("##Light Settings"))
        {
            char name[0x400];
            strcpy(name, selection->name.c_str());

            if (property(LIGHT_NAME_LABEL, name, sizeof(name)))
                selection->name = name;

            if (selection->type == LightType::Point)
                params->dirtyBVH |= dragProperty(LIGHT_POSITION_LABEL, selection->position);

            else if (selection->type == LightType::Directional)
                params->dirty |= dragProperty(LIGHT_DIRECTION_LABEL, selection->position);

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
            if (params->targetEngine == TargetEngine::HE2)
                color = color.pow(1.0f / 2.2f);

            params->dirty |= property(LIGHT_COLOR_LABEL, color);
            params->dirty |= dragProperty(LIGHT_COLOR_INTENSITY_LABEL, intensity, 0.01f, 0, INFINITY);

            if (params->targetEngine == TargetEngine::HE2)
                color = color.pow(2.2f);

            selection->color = color * intensity;

            params->propertyBag.set(selection->name + ".color.x()", color.x());
            params->propertyBag.set(selection->name + ".color.y()", color.y());
            params->propertyBag.set(selection->name + ".color.z()", color.z());
            params->propertyBag.set(selection->name + ".intensity", intensity);

            if (selection->type == LightType::Point)
            {
                if (params->targetEngine == TargetEngine::HE1)
                {
                    params->dirtyBVH |= dragProperty(LIGHT_INNER_RADIUS_LABEL, selection->range.z(), 0.1f, 0.0f, selection->range.w());
                    params->dirtyBVH |= dragProperty(LIGHT_OUTER_RADIUS_LABEL, selection->range.w(), 0.1f, selection->range.z(), INFINITY);
                    params->dirty |= dragProperty(SHADOW_RADIUS_LABEL, selection->shadowRadius, 0.001f, 0.0f, 1.0f);
                    params->dirty |= property(CAST_SHADOW_LABEL, selection->castShadow);
                }
                else if (params->targetEngine == TargetEngine::HE2)
                {
                    if (dragProperty(LIGHT_RADIUS_LABEL, selection->range.w()))
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

        tooltip(LIGHT_SAVE_CHANGES_DESC);
    }

    params->dirty |= params->dirtyBVH;

    drawBillboardsAndUpdateSelection();
}
