#include "SceneWindow.h"
#include "InstanceEditor.h"
#include "LightEditor.h"
#include "SHLightFieldEditor.h"
#include "AppData.h"

SceneWindow::SceneWindow() : visible(true)
{
}

SceneWindow::~SceneWindow()
{
    get<AppData>()->getPropertyBag().set(PROP("application.showScene"), visible);
}

void SceneWindow::initialize()
{
    visible = get<AppData>()->getPropertyBag().get<bool>(PROP("application.showScene"), true);

    editorDocument.setParent(document);
    editorDocument.add(std::make_unique<InstanceEditor>());
    editorDocument.add(std::make_unique<LightEditor>());
    editorDocument.add(std::make_unique<SHLightFieldEditor>());
    editorDocument.initialize();
}

void SceneWindow::update(float deltaTime)
{
    if (!visible)
        return;

    if (!ImGui::Begin("Scene", &visible))
        return ImGui::End();

    editorDocument.update(deltaTime);

    ImGui::End();
}
