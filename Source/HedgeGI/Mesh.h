﻿#pragma once

class FileStream;
class Material;
class Scene;

struct Vertex
{
    Vector3 position;
    Vector3 normal;
    Vector3 tangent;
    Vector3 binormal;
    Vector2 uv;
    Vector2 vPos;
    Color4 color;

    Matrix3 getTangentToWorldMatrix() const;
};

struct Triangle
{
    uint32_t a{};
    uint32_t b{};
    uint32_t c{};
};

enum class MeshType
{
    Opaque,
    Transparent,
    Punch,
    Special
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
    AABB aabb;

    void buildAABB();
    RTCGeometry createRTCGeometry() const;
    void generateTangents() const;

    void read(const FileStream& file, const Scene& scene);
    void write(const FileStream& file, const Scene& scene) const;
};