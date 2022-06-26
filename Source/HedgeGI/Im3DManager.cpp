#include "Im3DManager.h"

#include "Input.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Viewport.h"
#include "ViewportWindow.h"

constexpr size_t IM3D_VERTEX_BUFFER_SIZE = 3 * 1024 * 1024;

Im3DManager::Im3DManager() :
    billboardShader(ShaderProgram::get("Billboard")),
    triangleShader(ShaderProgram::get("Im3d_Triangles")),
    lineShader(ShaderProgram::get("Im3d_Lines")),
    pointShader(ShaderProgram::get("Im3d_Points")),
    vertexArray(IM3D_VERTEX_BUFFER_SIZE * sizeof(Im3d::VertexData), nullptr,
    {
        { 0, 4, GL_FLOAT, false, sizeof(Im3d::VertexData), offsetof(Im3d::VertexData, m_positionSize) },
        { 1, 4, GL_UNSIGNED_BYTE, true, sizeof(Im3d::VertexData), offsetof(Im3d::VertexData, m_color) },
    })
{
}

const Camera& Im3DManager::getCamera() const
{
    return camera;
}

const Vector3& Im3DManager::getRayPosition() const
{
    return rayPosition;
}

const Vector3& Im3DManager::getRayDirection() const
{
    return rayDirection;
}

const Billboard* Im3DManager::drawBillboard(const Vector3& position, const float size, const Color4& color, const Texture& texture)
{
    const Vector4 viewPos = camera.view * Vector4(position.x(), position.y(), position.z(), 1.0f);

    if (viewPos.z() >= 0.0f)
        return nullptr;

    const Vector4 projPos = camera.projection * viewPos;

    billboards.push_back(
    {
        projPos.head<2>() / projPos.w(),

        Vector2(
            (float)texture.width / (float)viewportWindow->getWidth() * size / -viewPos.z(),
            (float)texture.height / (float)viewportWindow->getHeight() * size / -viewPos.z()),

        color,

        &texture,

        -viewPos.z()
    });

    return &billboards.back();
}

bool Im3DManager::checkCursorOnBillboard(const Billboard& billboard) const
{
    return abs(viewportWindow->getMouseX() * 2.0f - 1.0f - billboard.rectPos.x()) < billboard.rectSize.x() &&
        abs(viewportWindow->getMouseY() * -2.0f + 1.0f - billboard.rectPos.y()) < billboard.rectSize.y();
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
    glEnable(GL_PROGRAM_POINT_SIZE);

    if (!billboards.empty())
    {
        std::stable_sort(billboards.begin(), billboards.end(), 
            [](const Billboard& left, const Billboard& right) { return left.depth > right.depth; });

        billboardShader.use();
        billboardShader.set("uView", camera.view);
        billboardShader.set("uProjection", camera.projection);
        billboardShader.set("uTexture", 0);

        billboardQuad.bind();

        for (auto& billboard : billboards)
        {
            billboardShader.set("uRectPos", billboard.rectPos);
            billboardShader.set("uRectSize", billboard.rectSize);
            billboardShader.set("uColor", billboard.color);
            billboard.texture->bind(0);

            billboardQuad.draw();
        }

        billboards.clear();
    }

    vertexArray.buffer.bind();
    vertexArray.bind();

    for (Im3d::U32 i = 0; i < Im3d::GetDrawListCount(); i++)
    {
        const auto& drawList = Im3d::GetDrawLists()[i];

        GLenum mode;
        const ShaderProgram* shader;

        switch (drawList.m_primType)
        {
        case Im3d::DrawPrimitive_Triangles:
            mode = GL_TRIANGLES;
            shader = &triangleShader;
            break;

        case Im3d::DrawPrimitive_Lines:
            mode = GL_LINES;
            shader = &lineShader;
            break;

        case Im3d::DrawPrimitive_Points:
            mode = GL_POINTS;
            shader = &pointShader;
            break;

        default:
            continue;
        }

        shader->use();
        shader->set("uView", camera.view);
        shader->set("uProjection", camera.projection);
        shader->set("uCamPosition", camera.position);
        shader->set("uDiscardFactor", drawList.m_layerId == IM3D_TRANSPARENT_DISCARD_ID ? 0.5f : 0.0f);
        shader->set("uViewport", Vector2(viewportWindow->getWidth(), viewportWindow->getHeight()));

        if (const Texture* texture = viewport->getInitialTexture(); texture)
        {
            shader->set("uTexture", 0);
            shader->set("uRect", Vector4(0, 1 - viewport->getNormalizedHeight(),
                viewport->getNormalizedWidth(), viewport->getNormalizedHeight()));

            texture->bind(0);
        }

        for (Im3d::U32 j = 0; j < drawList.m_vertexCount; j += IM3D_VERTEX_BUFFER_SIZE)
        {
            const GLsizei vertexCount = std::min<GLsizei>(drawList.m_vertexCount - j, IM3D_VERTEX_BUFFER_SIZE);

            glBufferSubData(vertexArray.buffer.target, 0, vertexCount * sizeof(Im3d::VertexData), &drawList.m_vertexData[j]);
            glDrawArrays(mode, 0, vertexCount);
        }
    }
}

void Im3DManager::update(const float deltaTime)
{
    const auto input = get<Input>();
    viewportWindow = get<ViewportWindow>();

    camera = get<Viewport>()->getCamera();

    const float xNormalized = viewportWindow->getMouseX() * 2 - 1;
    const float yNormalized = viewportWindow->getMouseY() * -2 + 1;
    const float tanFovy = tanf(camera.fieldOfView / 2.0f);

    rayPosition = camera.position;
    rayDirection = (camera.rotation * Vector3(xNormalized * tanFovy * camera.aspectRatio, yNormalized * tanFovy, -1)).normalized();

    Im3d::AppData& appData = Im3d::GetAppData();
    appData.m_keyDown[Im3d::Mouse_Left] = input->heldMouseButtons[GLFW_MOUSE_BUTTON_LEFT];
    appData.m_keyDown[Im3d::Key_L] = input->tappedKeys['L'];
    appData.m_keyDown[Im3d::Key_R] = input->tappedKeys['R'];
    appData.m_keyDown[Im3d::Key_S] = input->tappedKeys['S'];
    appData.m_keyDown[Im3d::Key_T] = input->tappedKeys['T'];
    appData.m_cursorRayOrigin = { camera.position.x(), camera.position.y(), camera.position.z() };
    appData.m_cursorRayDirection = { rayDirection.x(), rayDirection.y(), rayDirection.z() };
    appData.m_viewOrigin = { camera.position.x(), camera.position.y(), camera.position.z() };
    appData.m_viewDirection = { camera.direction.x(), camera.direction.y(), camera.direction.z() };
    appData.m_viewportSize = { (float)viewportWindow->getWidth(), (float)viewportWindow->getHeight() };
    appData.m_projScaleY = 1.5f * (tanFovy * 2.0f);
    appData.m_deltaTime = deltaTime;

    Im3d::NewFrame();
}
