#pragma once

class Bitmap;

enum class MaterialType : uint32_t
{
    Common,
    Blend,
    IgnoreLight,
    Sky
};

class Material
{
public:
    std::string name;
    MaterialType type {};
    size_t skyType {};
    bool ignoreVertexColor {}; // HE1 only
    bool hasMetalness {}; // HE2 only

    struct Parameters
    {
        Color4 diffuse { 1, 1, 1, 1 };
        Color4 specular { 1, 1, 1, 1 };
        Color4 ambient { 1, 1, 1, 1 };
        Color4 powerGlossLevel { 0, 0, 0, 0 };
        Color4 opacityReflectionRefractionSpecType { 1, 0, 0, 0 };
        Color4 luminanceRange { 0, 0, 0, 0 };
        Color4 luminance { 1, 1, 1, 1 };
        Color4 pbrFactor { 0.04, 0.5, 0, 0 };
        Color4 pbrFactor2 { 0.04, 0.5, 0, 0 };
        Color4 emissionParam { 0, 0, 0, 1 };
        Color4 emissive { 0, 0, 0, 0 };
        bool doubleSided{};
        bool additive{};
    } parameters{};

    struct Textures
    {
        const Bitmap* diffuse {};
        const Bitmap* specular {};
        const Bitmap* gloss {};
        const Bitmap* normal {};
        const Bitmap* alpha {};
        const Bitmap* diffuseBlend {};
        const Bitmap* specularBlend {};
        const Bitmap* glossBlend {};
        const Bitmap* normalBlend {};
        const Bitmap* emission {};
        const Bitmap* environment {};
    } textures{};
};
