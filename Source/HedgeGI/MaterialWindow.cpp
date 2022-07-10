#include "MaterialWindow.h"

#include "AppWindow.h"
#include "ImGuiManager.h"
#include "MenuBar.h"

template <typename T>
void MaterialWindow::drawParams(const char* name, std::vector<hl::hh::mirage::material_param<T>>& params)
{
    if (!ImGui::CollapsingHeader(name))
        return;

    if (params.empty())
    {
        ImGui::TextUnformatted("There are no parameters.");
        return;
    }

    if (!beginProperties(name))
        return;

    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Value");

    for (auto& param : params)
    {
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);

        char buf[1024];
        strcpy(buf, param.name.c_str());

        if (ImGui::InputText(makeName(buf), buf, sizeof(buf)))
            param.name = buf;

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1);

        if constexpr (std::is_same_v<T, hl::vec4>)
            ImGui::DragFloat4(makeName(buf), &param.values[0].x, 0.1f, 0, 0, "%g");

        else if constexpr (std::is_same_v<T, hl::ivec4>)
            ImGui::InputInt4(makeName(buf), &param.values[0].x);

        else if constexpr (std::is_same_v<T, hl::bvec4>)
            ImGui::Checkbox(makeName(buf), (bool*) &param.values[0].w);
    }

    endProperties();

    if (ImGui::Button("Add"))
    {
        
    }
}

MaterialWindow::MaterialWindow(const bool fullscreen) : fullscreen(fullscreen), visible(true)
{
}

void MaterialWindow::initialize()
{
    for (auto& mat : std::filesystem::directory_iterator("D:/Steam/steamapps/common/Sonic Generations/disk/bb3/Sonic"))
    {
        if (wcsstr(mat.path().c_str(), L".material"))
        {
            materials.push_back(std::make_unique<hl::hh::mirage::material>(mat.path().c_str()));
        }
    }
}

void MaterialWindow::update(const float deltaTime)
{
    const auto window = get<AppWindow>();

    if (fullscreen)
    {
        const auto menuBar = get<MenuBar>();

        ImGui::SetNextWindowPos({ 0, menuBar->getDockSpaceY() });
        ImGui::SetNextWindowSize({ (float)window->getWidth(), (float)window->getHeight() - menuBar->getDockSpaceY() });

        if (!ImGui::Begin("##Materials Fullscreen", nullptr, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoBackground))
            return ImGui::End();
    }
    else
    {
        if (!visible)
            return;

        if (!ImGui::Begin("Materials", &visible))
            return ImGui::End();
    }

    if (ImGui::BeginTable("##Materials Table", 2))
    {
        if (ImGui::TableNextColumn())
        {
            const float width = std::max(384.0f, window->getWidth() / 4.0f);

            ImGui::SetNextItemWidth(width);

            const bool doSearch = ImGui::InputText("##Material Search", search, sizeof(search)) || strlen(search) > 0;

            if (ImGui::BeginListBox("##Material Names", { width, -30 }))
            {
                for (auto& material : materials)
                {
                    if (doSearch && material->name.find(search) == std::string::npos)
                        continue;

                    if (ImGui::Selectable(material->name.c_str(), selection == material.get()))
                        selection = material.get();
                }

                ImGui::EndListBox();
            }

            if (ImGui::Button("Add"))
            {
                
            }

            ImGui::SameLine();

            if (ImGui::Button("Clone"))
            {

            }

            ImGui::SameLine();

            if (ImGui::Button("Remove"))
            {

            }
        }

        if (ImGui::TableNextColumn() && selection != nullptr)
        {
            if (beginProperties("##Material Properties"))
            {
                if (property("Shader", selection->shaderName))
                    selection->subShaderName = selection->shaderName;

                property("Alpha Threshold", ImGuiDataType_Float, &selection->alphaThreshold);
                property("Double Sided", selection->noBackfaceCulling);
                property("Additive", selection->useAdditiveBlending);

                endProperties();
            }

            if (ImGui::CollapsingHeader("Textures"))
            {
            }

            drawParams("Float4 Parameters", selection->float4Params);
            drawParams("Int4 Parameters", selection->int4Params);
            drawParams("Bool Parameters", selection->bool4Params);

            if (ImGui::CollapsingHeader("SCA Parameters"))
            {
                
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}
