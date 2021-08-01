#pragma once

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

    void load(const tinyxml2::XMLDocument& doc);
    void load(const sonic2013::FxSceneData& data);
    void load(const wars::NeedleFxSceneData& data);
};
