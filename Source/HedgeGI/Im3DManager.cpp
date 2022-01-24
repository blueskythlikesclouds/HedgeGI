#include "Im3DManager.h"

#include "Input.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Viewport.h"
#include "ViewportWindow.h"

constexpr size_t IM3D_VERTEX_BUFFER_SIZE = 3 * 1024 * 1024;

Im3DManager::Im3DManager() : shader(ShaderProgram::get("Im3d")),
    vertexArray(IM3D_VERTEX_BUFFER_SIZE * sizeof(Im3d::VertexData), nullptr,
    {
        { 0, 4, GL_FLOAT, false, sizeof(Im3d::VertexData), offsetof(Im3d::VertexData, m_positionSize) },
        { 1, 4, GL_UNSIGNED_BYTE, true, sizeof(Im3d::VertexData), offsetof(Im3d::VertexData, m_color) },
    })
{
}

void Im3DManager::endFrame()
{
    Im3d::EndFrame();

    const auto viewportWindow = get<ViewportWindow>();
    const auto viewport = get<Viewport>();

    glViewport(
        viewportWindow->getX(), 
        (int)(ImGui::GetIO().DisplaySize.y - (float)viewportWindow->getY() - (float)viewportWindow->getHeight()), 
        viewportWindow->getWidth(),
        viewportWindow->getHeight());

    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0f);
    glPointSize(2.0f);

    vertexArray.buffer.bind();
    vertexArray.bind();

    const Matrix4 view = camera.getView();

    shader.use();
    shader.set("uView", view);
    shader.set("uProjection", camera.getProjection());
    shader.set("uCamPosition", camera.position);

    if (const Texture* texture = viewport->getInitialTexture(); texture)
    {
        shader.set("uTexture", 0);
        shader.set("uRect", Vector4(0, 1 - viewport->getNormalizedHeight(),
            viewport->getNormalizedWidth(), viewport->getNormalizedHeight()));

        texture->bind(0);
    }

    for (Im3d::U32 i = 0; i < Im3d::GetDrawListCount(); i++)
    {
        const auto& drawList = Im3d::GetDrawLists()[i];

        shader.set("uDiscardFactor", drawList.m_layerId == IM3D_TRANSPARENT_DISCARD_ID ? 0.5f : 0.0f);

        for (Im3d::U32 j = 0; j < drawList.m_vertexCount; j += IM3D_VERTEX_BUFFER_SIZE)
        {
            const GLsizei vertexCount = std::min<GLsizei>(drawList.m_vertexCount - j, IM3D_VERTEX_BUFFER_SIZE);

            glBufferSubData(vertexArray.buffer.target, 0,
                vertexCount * sizeof(Im3d::VertexData), &drawList.m_vertexData[j]);

            GLenum mode;

            switch (drawList.m_primType)
            {
            case Im3d::DrawPrimitive_Triangles:
                mode = GL_TRIANGLES;
                break;

            case Im3d::DrawPrimitive_Lines:
                mode = GL_LINES;
                break;

            case Im3d::DrawPrimitive_Points:
                mode = GL_POINTS;
                break;

            default:
                continue;
            }

            glDrawArrays(mode, 0, vertexCount);
        }
    }
}

void Im3DManager::update(const float deltaTime)
{
    const auto input = get<Input>();
    const auto viewportWindow = get<ViewportWindow>();

    camera = get<Viewport>()->getCamera();

    const Vector3 viewDirection = camera.getDirection();

    const float xNormalized = viewportWindow->getMouseX() * 2 - 1;
    const float yNormalized = viewportWindow->getMouseY() * -2 + 1;
    const float tanFovy = tanf(camera.fieldOfView / 2.0f);
    const Vector3 rayDirection = (camera.rotation * Vector3(xNormalized * tanFovy * camera.aspectRatio, yNormalized * tanFovy, -1)).normalized();

    Im3d::AppData& appData = Im3d::GetAppData();
    appData.m_keyDown[Im3d::Mouse_Left] = input->heldMouseButtons[GLFW_MOUSE_BUTTON_LEFT];
    appData.m_keyDown[Im3d::Key_L] = input->tappedKeys['L'];
    appData.m_keyDown[Im3d::Key_R] = input->tappedKeys['R'];
    appData.m_keyDown[Im3d::Key_S] = input->tappedKeys['S'];
    appData.m_keyDown[Im3d::Key_T] = input->tappedKeys['T'];
    appData.m_cursorRayOrigin = { camera.position.x(), camera.position.y(), camera.position.z() };
    appData.m_cursorRayDirection = { rayDirection.x(), rayDirection.y(), rayDirection.z() };
    appData.m_viewOrigin = { camera.position.x(), camera.position.y(), camera.position.z() };
    appData.m_viewDirection = { viewDirection.x(), viewDirection.y(), viewDirection.z() };
    appData.m_viewportSize = { (float)viewportWindow->getWidth(), (float)viewportWindow->getHeight() };
    appData.m_projScaleY = tanFovy * 2.0f;
    appData.m_deltaTime = deltaTime;

    Im3d::NewFrame();
}
