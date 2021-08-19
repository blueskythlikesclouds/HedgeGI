#pragma once

class Light;

namespace wars
{
    struct NeedleFxSceneData;
}

namespace sonic2013
{
    struct FxSceneData;
}

struct SceneEffect
{
    struct Default
    {
        float skyIntensityScale;
    } def { 1.0f };

    struct HDR
    {
        float middleGray;
        float lumMin;
        float lumMax;
    } hdr { 0.37f, 0.15f, 1.74f };

    struct LightScattering
    {
        bool enable;
        Color3 color; // ms_Color
        float depthScale; // ms_FarNearScale.z
        float inScatteringScale; // ms_FarNearScale.w
        float rayleigh; // ms_Ray_Mie_Ray2_Mie2.x
        float mie; // ms_Ray_Mie_Ray2_Mie2.y
        float g; // ms_G
        float zNear; // ms_FarNearScale.y
        float zFar; // ms_FarNearScale.x

        // Values passed to GPU
        Vector4 g_LightScattering_Ray_Mie_Ray2_Mie2;
        Vector4 g_LightScattering_ConstG_FogDensity;
        Vector4 g_LightScatteringFarNearScale;

        void computeGpuValues();

        Vector2 compute(const Vector3& position, const Vector3& viewPosition, const Vector3& eyePosition, const Light& light) const;

    } lightScattering { false, {0.1f, 0.21f, 0.3f }, 9.1f, 50.0f, 0.1f, 0.01f, 0.7f, 60.0f, 700.0f };

    void load(const tinyxml2::XMLDocument& doc);
    void load(const sonic2013::FxSceneData& data);
    void load(const wars::NeedleFxSceneData& data);
};
