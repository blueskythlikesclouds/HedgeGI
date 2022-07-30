#include "SHLightFieldEditor.h"

#include "BakeParams.h"
#include "CameraController.h"
#include "Im3DManager.h"
#include "Stage.h"
#include "StageParams.h"
#include "SHLightField.h"
#include "Math.h"

const char* const SHLF_SEARCH_DESC = "Type in a light name to search for.";
const char* const SHLF_ADD_DESC = "Adds a new light field probe.";
const char* const SHLF_CLONE_DESC = "Clones the selected light field probe.";
const char* const SHLF_REMOVE_DESC = "Removes the selected light field probe.";

const Label SHLF_NAME_LABEL = { "Name",
    "Name of the light field probe." };

const Label SHLF_POSITION_LABEL = { "Position",
    "Position of the light field probe.\n\n"
    "Press T in the viewport to move it around using a gizmo." };

const Label SHLF_ROTATION_LABEL = { "Rotation",
    "Rotation of the light field probe in degrees.\n\n"
    "Press R in the viewport to rotate it using a gizmo." };

const Label SHLF_SCALE_LABEL = { "Scale",
    "Scaling of the light field probe.\n\n"
    "Press S in the viewport to scale it using a gizmo." };

const Label SHLF_RES_LABEL = { "Resolution",
    "Density of the light field probe.\n\n"
    "Higher values are going to have higher quality at the cost of worse bake times and disk/memory space requirements." };

void SHLightFieldEditor::update(float deltaTime)
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    if (params->targetEngine != TargetEngine::HE2 || !ImGui::CollapsingHeader("Light Fields"))
        return;

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchSHLFs", search, sizeof(search)) || strlen(search) > 0;

    tooltip(SHLF_SEARCH_DESC);

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginListBox("##SHLFs"))
    {
        for (auto& shlf : stage->getScene()->shLightFields)
        {
            if (doSearch && shlf->name.find(search) == std::string::npos)
                continue;

            if (ImGui::Selectable(shlf->name.c_str(), selection == shlf.get()))
                selection = shlf.get();
        }

        ImGui::EndListBox();
    }

    if (ImGui::Button("Add"))
    {
        std::unique_ptr<SHLightField> shlf = std::make_unique<SHLightField>();
        shlf->position = get<CameraController>()->getNewObjectPosition() * 10;
        shlf->rotation = Vector3::Zero();
        shlf->scale = { 80, 120, 750 };
        shlf->resolution = { 2, 2, 10 };

        char name[0x100];
        sprintf(name, "%s_shLF_%03d", stage->getName().c_str(), (int)stage->getScene()->shLightFields.size());
        shlf->name = name;

        selection = shlf.get();
        stage->getScene()->shLightFields.push_back(std::move(shlf));
    }

    tooltip(SHLF_ADD_DESC);

    ImGui::SameLine();

    if (ImGui::Button("Clone"))
    {
        if (selection != nullptr)
        {
            std::unique_ptr<SHLightField> shlf = std::make_unique<SHLightField>(*selection);

            char name[0x100];
            sprintf(name, "%s_shLF_%03d", stage->getName().c_str(), (int)stage->getScene()->shLightFields.size());
            shlf->name = name;

            selection = shlf.get();
            stage->getScene()->shLightFields.push_back(std::move(shlf));
        }
    }

    tooltip(SHLF_CLONE_DESC);

    ImGui::SameLine();

    if (ImGui::Button("Remove"))
    {
        if (selection != nullptr)
        {
            for (auto it = stage->getScene()->shLightFields.begin(); it != stage->getScene()->shLightFields.end(); ++it)
            {
                if ((*it).get() != selection)
                    continue;

                stage->getScene()->shLightFields.erase(it);
                break;
            }

            selection = nullptr;
        }
    }

    tooltip(SHLF_REMOVE_DESC);

    if (selection == nullptr)
        return;

    ImGui::Separator();

    if (!beginProperties("##SHLF Properties"))
        return;

    const Matrix4 matrix = selection->getMatrix();

    Im3d::PushColor(Im3d::Color(153.0f / 255.0f, 217.0f / 255.0f, 234.0f / 255.0f, 0.5f));
    drawOrientedBoxFilled(matrix, 0.1f);
    Im3d::PopColor();

    Im3d::PushSize(IM3D_LINE_SIZE);
    drawOrientedBox(matrix, 0.1f);
    Im3d::PopSize();

    const float radius = (selection->scale.array() / selection->resolution.cast<float>()).minCoeff() / 40.0f;

    const Vector3 lineOffsets[] =
    {
        matrix.col(0).head<3>().normalized() * radius,
        matrix.col(1).head<3>().normalized() * radius,
        matrix.col(2).head<3>().normalized() * radius
    };

    for (size_t z = 0; z < selection->resolution.z(); z++)
    {
        const float zNormalized = (z + 0.5f) / selection->resolution.z() - 0.5f;

        for (size_t y = 0; y < selection->resolution.y(); y++)
        {
            const float yNormalized = (y + 0.5f) / selection->resolution.y() - 0.5f;

            for (size_t x = 0; x < selection->resolution.x(); x++)
            {
                const float xNormalized = (x + 0.5f) / selection->resolution.x() - 0.5f;
                const Vector3 position = (matrix * Vector4(xNormalized, yNormalized, zNormalized, 1)).head<3>() / 10.0f;

                for (size_t i = 0; i < 3; i++)
                {
                    const Vector3 a = position + lineOffsets[i];
                    const Vector3 b = position - lineOffsets[i];

                    Im3d::DrawLine({ a.x(), a.y(), a.z() }, { b.x(), b.y(), b.z() }, IM3D_LINE_SIZE, Im3d::Color_White);
                }
            }
        }
    }

    char name[0x400];
    strcpy(name, selection->name.c_str());

    if (property(SHLF_NAME_LABEL, name, sizeof(name)))
        selection->name = name;

    Vector3 position = selection->position / 10.0f;
    Matrix3 rotation = selection->getRotationMatrix();
    Vector3 scale = selection->scale / 10.0f;

    Im3d::PushLayerId(IM3D_TRANSPARENT_DISCARD_ID);

    if (Im3d::Gizmo(selection->name.c_str(), position.data(), rotation.data(), scale.data()))
    {
        selection->position = position * 10.0f;
        selection->setFromRotationMatrix(rotation);
        selection->scale = scale * 10.0f;
    }

    Im3d::PopLayerId();

    if (dragProperty(SHLF_POSITION_LABEL, position))
        selection->position = position * 10.0f;

    Vector3 rotAngles = selection->rotation / PI * 180.0f;

    if (dragProperty(SHLF_ROTATION_LABEL, rotAngles))
        selection->rotation = rotAngles / 180.0f * PI;

    if (dragProperty(SHLF_SCALE_LABEL, scale))
        selection->scale = scale * 10.0f;

    property(SHLF_RES_LABEL, selection->resolution);

    endProperties();
}
