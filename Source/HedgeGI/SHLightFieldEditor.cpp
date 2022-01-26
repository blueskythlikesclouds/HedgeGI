#include "SHLightFieldEditor.h"

#include "BakeParams.h"
#include "CameraController.h"
#include "Im3DManager.h"
#include "Stage.h"
#include "StageParams.h"
#include "SHLightField.h"
#include "Math.h"

void SHLightFieldEditor::update(float deltaTime)
{
    const auto stage = get<Stage>();
    const auto params = get<StageParams>();

    if (params->bakeParams.targetEngine != TargetEngine::HE2 || !ImGui::CollapsingHeader("Light Fields"))
        return;

    ImGui::SetNextItemWidth(-1);
    const bool doSearch = ImGui::InputText("##SearchSHLFs", search, sizeof(search)) || strlen(search) > 0;

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

    if (selection == nullptr)
        return;

    ImGui::Separator();

    if (!beginProperties("##SHLF Properties"))
        return;

    const Matrix4 matrix = selection->getMatrix();

    Im3d::PushColor(Im3d::Color(153.0f / 255.0f, 217.0f / 255.0f, 234.0f / 255.0f, 0.5f));
    drawOrientedBoxFilled(matrix, 0.1f);
    Im3d::PopColor();

    drawOrientedBox(matrix, 0.1f);

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

                    Im3d::DrawLine({ a.x(), a.y(), a.z() }, { b.x(), b.y(), b.z() }, 1.0f, Im3d::Color_White);
                }
            }
        }
    }

    char name[0x400];
    strcpy(name, selection->name.c_str());

    if (property("Name", name, sizeof(name)))
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

    if (dragProperty("Position", position))
        selection->position = position * 10.0f;

    Vector3 rotAngles = selection->rotation / PI * 180.0f;

    if (dragProperty("Rotation", rotAngles))
        selection->rotation = rotAngles / 180.0f * PI;

    if (dragProperty("Scale", scale))
        selection->scale = scale * 10.0f;

    property("Resolution", selection->resolution);

    endProperties();
}
