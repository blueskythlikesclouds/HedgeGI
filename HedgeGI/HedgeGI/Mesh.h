#pragma once

class Material;
class Scene;

struct Vertex
{
    Eigen::Vector3f position;
    Eigen::Vector3f normal;
    Eigen::Vector3f tangent;
    Eigen::Vector3f binormal;
    Eigen::Vector2f uv;
    Eigen::Vector2f vPos;
    Eigen::Array4f color;

    Eigen::Matrix3f getTangentToWorldMatrix() const;
};

struct Triangle
{
    uint32_t a{};
    uint32_t b{};
    uint32_t c{};
};

enum MeshType
{
    MESH_TYPE_OPAQUE,
    MESH_TYPE_TRANSPARENT,
    MESH_TYPE_PUNCH,
    MESH_TYPE_SPECIAL
};

class Mesh
{
public:
    MeshType type{};
    uint32_t vertexCount{};
    uint32_t triangleCount{};
    std::unique_ptr<Vertex[]> vertices;
    std::unique_ptr<Triangle[]> triangles;
    const Material* material{};
    Eigen::AlignedBox3f aabb;

    void buildAABB();
    RTCGeometry createRTCGeometry() const;
    void generateTangents() const;

    void read(const FileStream& file, const Scene& scene);
    void write(const FileStream& file, const Scene& scene) const;
};
