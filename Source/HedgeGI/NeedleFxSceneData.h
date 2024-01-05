#pragma once

namespace wars
{
    enum BloomRenderTargetSize : int8_t
    {
        RTSIZE_ONE_FOURTH = 0,
        RTSIZE_ONE_EIGHTH = 1,
        RTSIZE_ONE_SIXTEENTH = 2,
    };

    enum DOFRenderTargetSize : int32_t
    {
        RTSIZE_FULL_SCALE = 0,
        RTSIZE_HALF_SCALE = 1,
        RTSIZE_QUARTER_SCALE = 2,
    };

    struct FxRenderTargetSetting
    {
        BloomRenderTargetSize bloomRenderTargetScale;
        DOFRenderTargetSize dofRenderTargetScale;
        int32_t shadowMapWidth;
        int32_t shadowMapHeight;
    };

    enum AntiAliasingType : int8_t
    {
        AATYPE_NONE = 0,
        AATYPE_TAA = 1,
        AATYPE_FXAA = 2,
        AATYPE_LAST = 3,
    };

    struct FxAntiAliasing
    {
        AntiAliasingType aaType;
    };

    struct NeedleFxSceneConfig
    {
        FxRenderTargetSetting rendertarget;
        FxAntiAliasing antialiasing;
    };

    enum ToneMapType : int8_t
    {
        TONEMAPTYPE_MANUAL_EXPOSURE = 0,
        TONEMAPTYPE_AUTO = 1,
    };

    enum ExposureMode : int8_t
    {
        FXEXPOSUREMODE_DISNEY = 0,
        FXEXPOSUREMODE_FILMIC = 1,
    };

    struct FxHDROption
    {
        bool enable;
        uint8_t padding;
    };

    enum DebugViewType : int8_t
    {
        DEBUG_VIEW_DEFAULT = 0,
        DEBUG_VIEW_DIR_DIFFUSE = 1,
        DEBUG_VIEW_DIR_SPECULAR = 2,
        DEBUG_VIEW_AMB_DIFFUSE = 3,
        DEBUG_VIEW_AMB_SPECULAR = 4,
        DEBUG_VIEW_ONLY_IBL = 5,
        DEBUG_VIEW_ONLY_IBL_SURF_NORMAL = 6,
        DEBUG_VIEW_SHADOW = 7,
        DEBUG_VIEW_WHITE_ALBEDO = 8,
        DEBUG_VIEW_USER0 = 9,
        DEBUG_VIEW_USER1 = 10,
        DEBUG_VIEW_USER2 = 11,
        DEBUG_VIEW_USER3 = 12,
        DEBUG_VIEW_ALBEDO = 13,
        DEBUG_VIEW_ALBEDO_CHECK_OUTLIER = 14,
        DEBUG_VIEW_OPACITY = 15,
        DEBUG_VIEW_NORMAL = 16,
        DEBUG_VIEW_ROUGHNESS = 17,
        DEBUG_VIEW_AMBIENT = 18,
        DEBUG_VIEW_CAVITY = 19,
        DEBUG_VIEW_REFLECTANCE = 20,
        DEBUG_VIEW_METALLIC = 21,
        DEBUG_VIEW_LOCAL_LIGHT = 22,
        DEBUG_VIEW_SCATTERING_FEX = 23,
        DEBUG_VIEW_SCATTERING_LIN = 24,
        DEBUG_VIEW_SSAO = 25,
        DEBUG_VIEW_RLR = 26,
        DEBUG_VIEW_IBL_DIFFUSE = 27,
        DEBUG_VIEW_IBL_SPECULAR = 28,
        DEBUG_VIEW_ENV_BRDF = 29,
        DEBUG_VIEW_WORLD_POSITION = 30,
        DEBUG_VIEW_SHADING_MODEL_ID = 31,
        DEBUG_VIEW_IBL_CAPTURE = 32,
        DEBUG_VIEW_IBL_SKY_TERRAIN = 33,
        DEBUG_VIEW_WRITE_DEPTH_TO_ALPHA = 34,
    };

    enum LocalLightCullingType : int8_t
    {
        LOCAL_LIGHT_CULLING_TYPE_NONE = 0,
        LOCAL_LIGHT_CULLING_TYPE_CPU_TILE = 1,
        LOCAL_LIGHT_CULLING_TYPE_GPU_TILE = 2,
    };

    enum TextureViewType : int8_t
    {
        TEXTURE_VIEW_NONE = 0,
        TEXTURE_VIEW_DEPTH = 1,
        TEXTURE_VIEW_BLOOM_POWER = 2,
        TEXTURE_VIEW_BLOOM_BRIGHT = 3,
        TEXTURE_VIEW_BLOOM_FINAL = 4,
        TEXTURE_VIEW_GLARE = 5,
        TEXTURE_VIEW_LUMINANCE = 6,
        TEXTURE_VIEW_DOF_BOKEH = 7,
        TEXTURE_VIEW_DOF_BOKEH_NEAR = 8,
        TEXTURE_VIEW_SSAO_SOURCE = 9,
        TEXTURE_VIEW_DOWNSAMPLE = 10,
        TEXTURE_VIEW_CASCADED_SHADOW_MAPS_0 = 11,
        TEXTURE_VIEW_CASCADED_SHADOW_MAPS_1 = 12,
        TEXTURE_VIEW_CASCADED_SHADOW_MAPS_2 = 13,
        TEXTURE_VIEW_CASCADED_SHADOW_MAPS_3 = 14,
    };

    enum AmbientSpecularType : int8_t
    {
        AMBIENT_SPECULAR_NONE = 0,
        AMBIENT_SPECULAR_SG = 1,
        AMBIENT_SPECULAR_IBL = 2,
        AMBIENT_SPECULAR_BLEND = 3,
    };

    enum DebugScreenView : int8_t
    {
        DEBUG_SCREEN_VIEW_DEFAULT = 0,
        DEBUG_SCREEN_VIEW_ALL_ENABLE = 1,
        DEBUG_SCREEN_VIEW_ALL_DISABLE = 2,
    };

    enum ViewMode : int8_t
    {
        VIEWMODE_RGB = 0,
        VIEWMODE_RRR = 1,
        VIEWMODE_GGG = 2,
        VIEWMODE_BBB = 3,
        VIEWMODE_AAA = 4,
    };

    enum DebugScreenType : int8_t
    {
        DEBUG_SCREEN_GBUFFER0 = 0,
        DEBUG_SCREEN_GBUFFER1 = 1,
        DEBUG_SCREEN_GBUFFER2 = 2,
        DEBUG_SCREEN_GBUFFER3 = 3,
        DEBUG_SCREEN_DEPTHBUFFER = 4,
        DEBUG_SCREEN_CSM0 = 5,
        DEBUG_SCREEN_CSM1 = 6,
        DEBUG_SCREEN_CSM2 = 7,
        DEBUG_SCREEN_CSM3 = 8,
        DEBUG_SCREEN_HDR = 9,
        DEBUG_SCREEN_BLOOM = 10,
        DEBUG_SCREEN_RLR = 11,
        DEBUG_SCREEN_GODRAY = 12,
        DEBUG_SCREEN_SSAO = 13,
        DEBUG_SCREEN_CUSTOM0 = 14,
        DEBUG_SCREEN_CUSTOM1 = 15,
        DEBUG_SCREEN_CUSTOM2 = 16,
        DEBUG_SCREEN_CUSTOM3 = 17,
    };

    enum ErrorCheckType : int8_t
    {
        ERROR_CHECK_NONE = 0,
        ERROR_CHECK_NAN = 1,
        ERROR_CHECK_ALBEDO = 2,
        ERROR_CHECK_NORMAL = 3,
    };

    struct FxDebugScreenOption
    {
        bool enable;
        bool fullScreen;
        ViewMode viewMode;
        float exposure;
        DebugScreenType screenType;
        ErrorCheckType errorCheck;
    };

    struct FxRenderOption
    {
        DebugViewType debugViewType;
        bool clearRenderTarget;
        int32_t maxCubeProbe;
        bool enableDrawCubeProbe;
        bool enableDirectionalLight;
        bool enablePointLight;
        bool enableEffectDeformation;
        bool enableReverseDepth;
        float cullingTooSmallThrehold;
        LocalLightCullingType localLightCullingType;
        float localLightScale;
        bool debugEnableDrawLocalLight;
        TextureViewType debugTextureViewType;
        bool debugEnableOutputTextureView;
        float debugViewDepthNear;
        float debugViewDepthFar;
        AmbientSpecularType debugAmbientSpecularType;
        bool debugIBLPlusDirectionalSpecular;
        bool debugEnableSGGIVer2nd;
        bool debugEnableOcclusionCullingView;
        int32_t debugOccluderVertThreshold;
        FxDebugScreenOption debugScreen[16];
        DebugScreenView debugScreenView;
    };

    struct FxSGGIParameter
    {
        float sgStartSmoothness;
        float sgEndSmoothness;
    };

    struct FxRLRParameter
    {
        bool enable;
        float num;
        float travelFadeStart;
        float travelFadeEnd;
        float borderFadeStart;
        float borderFadeEnd;
        float overrideRatio;
        float maxRoughness;
        float hizStartLevel;
    };

    enum GlareType : int8_t
    {
        GLARE_DISABLE = 0,
        GLARE_CAMERA = 1,
        GLARE_NATURAL = 2,
        GLARE_CHEAP_LENS = 3,
        GLARE_FILTER_CROSS_SCREEN = 4,
        GLARE_FILTER_CROSS_SCREEN_SPECTRAL = 5,
        GLARE_FILTER_SNOW_CROSS = 6,
        GLARE_FILTER_SNOW_CROSS_SPECTRAL = 7,
        GLARE_FILTER_SUNNY_CROSS = 8,
        GLARE_FILTER_SUNNY_CROSS_SPECTRAL = 9,
        GLARE_CINECAM_VERTICAL_SLITS = 10,
        GLARE_CINECAM_HORIZONTAL_SLITS = 11,
    };

    struct FxBloomParameter
    {
        bool enable;
        float bloomThreshold;
        float bloomMax;
        float bloomScale;
        float starScale;
        int32_t ghostCount;
        float ghostScale;
        float haloScale;
        float sampleRadiusScale;
        int32_t blurQuality;
        GlareType glareType;
    };

    struct FxToneMapParameter
    {
        float middleGray;
        float lumMax;
        float lumMin;
        float adaptedRatio;
    };

    struct FxExposureParameter
    {
        float exposureValue;
    };

    enum LutIndex : int32_t
    {
        LUT_INDEX_DEFAULT = 0,
        LUT_INDEX_WB = 1,
        LUT_INDEX_USER_0 = 2,
        LUT_INDEX_USER_1 = 3,
        LUT_INDEX_USER_2 = 4,
        LUT_INDEX_USER_3 = 5,
        LUT_INDEX_USER_4 = 6,
        LUT_INDEX_USER_5 = 7,
    };

    struct FxColorContrastParameter
    {
        bool enable;
        float contrast;
        float dynamicRange;
        float crushShadows;
        float crushHilights;
        bool useLut;
        LutIndex lutIndex0;
        LutIndex lutIndex1;
        float blendRatio;
        float minContrast;
        bool useHlsCorrection;
        float hlsHueOffset;
        float hlsLightnessOffset;
        float hlsSaturationOffset;
        int32_t hlsColorOffset[3];
        uint8_t padding;
    };

    struct FxLightScatteringParameter
    {
        bool enable;
        Vector3 color;
        float depthScale;
        float inScatteringScale;
        float rayleigh;
        float mie;
        float g;
        float znear;
        float zfar;
    };

    struct FxDOFParameter
    {
        bool enable;
        bool useFocusLookAt;
        float foregroundBokehMaxDepth;
        float foregroundBokehStartDepth;
        float backgroundBokehStartDepth;
        float backgroundBokehMaxDepth;
        bool enableCircleDOF;
        float cocMaxRadius;
        float bokehRadiusScale;
        int32_t bokehSampleCount;
        float skyFocusDistance;
        float bokehBias;
        bool drawFocalPlane;
        bool enableSWA;
        float swaFocus;
        float swaFocusRange;
        float swaNear;
        float swaFar;
    };

    enum ShadowFilter : int8_t
    {
        SHADOW_FILTER_POINT = 0,
        SHADOW_FILTER_PCF = 1,
        SHADOW_FILTER_ESM = 2,
        SHADOW_FILTER_MSM = 3,
        SHADOW_FILTER_VSM_POINT = 4,
        SHADOW_FILTER_VSM_LINEAR = 5,
        SHADOW_FILTER_VSM_ANISO_2 = 6,
        SHADOW_FILTER_VSM_ANISO_4 = 7,
        SHADOW_FILTER_VSM_ANISO_8 = 8,
        SHADOW_FILTER_VSM_ANISO_16 = 9,
    };

    enum ShadowRangeType : int8_t
    {
        SHADOW_RANGE_TYPE_CAMERA_LOOKAT = 0,
        SHADOW_RANGE_TYPE_POSITION_MANUAL = 1,
        SHADOW_RANGE_TYPE_FULL_MANUAL = 2,
    };

    enum FitProjection : int8_t
    {
        FIT_PROJECTION_TO_CASCADES = 0,
        FIT_PROJECTION_TO_SCENE = 1,
    };

    enum FitNearFar : int8_t
    {
        FIT_NEARFAR_ZERO_ONE = 0,
        FIT_NEARFAR_AABB = 1,
        FIT_NEARFAR_SCENE_AABB = 2,
    };

    enum CascadeSelection : int8_t
    {
        CASCADE_SELECTION_MAP = 0,
        CASCADE_SELECTION_INTERVAL = 1,
    };

    struct FxShadowMapParameter
    {
        bool enable;
        ShadowFilter shadowFilter;
        ShadowRangeType shadowRangeType;
        FitProjection fitProjection;
        FitNearFar fitNearFar;
        CascadeSelection cascadeSelection;
        float sceneRange;
        float sceneCenter[3];
        float manualLightPos[3];
        int32_t cascadeLevel;
        float cascadeSplits[4];
        int32_t blurQuality;
        int32_t blurSize;
        float bias;
        float fadeoutDistance;
        Matrix4 shadowCameraViewMatrix;
        Matrix4 shadowCameraProjectionMatrix;
        float shadowCameraNearDepth;
        float shadowCameraFarDepth;
        float shadowCameraLookAtDepth;
        bool enableShadowCamera;
        bool enableMoveLightTexelSize;
        bool enableDrawSceneAABB;
        bool enableDrawShadowFrustum;
        bool enableDrawCascade;
        bool enableDrawCameraFrustum;
        bool enablePauseCamera;
    };

    struct FxSSAOParameter
    {
        bool enable;
        float intensity;
        float radius;
        float fadeoutDistance;
        float fadeoutRadius;
        float power;
        float bias;
        float occlusionDistance;
        int32_t renderTargetSizeEnumIndex;
    };

    enum ScalingType : int8_t
    {
        SCALING_PHOTOSHOP_FILETER = 2,
        SCALING_NONE = 3,
        SCALING_IGNORE_DATA = 4,
    };

    enum DebugDrawType : int8_t
    {
        DEBUG_DRAW_TYPE_NONE = 0,
        DEBUG_DRAW_TYPE_LOOKAT = 1,
        DEBUG_DRAW_TYPE_NODE = 2,
    };

    enum DebugColorType : int8_t
    {
        DEBUG_COLOR_TYPE_COLOR = 0,
        DEBUG_COLOR_TYPE_SHADOW = 1,
        DEBUG_COLOR_TYPE_LUMINANCE = 2,
    };

    struct FxLightFieldParameter
    {
        ScalingType saturationScalingType;
        float saturationScalingRate;
        float luminanceScalingRate;
        bool forceUpdate;
        bool ignoreData;
        bool ignoreFinalLightColorAdjestment;
        float intensityThreshold;
        float intensityBias;
        float luminanceMax;
        float luminanceMin;
        float luminanceCenter;
        Vector3 defaultColorUp;
        Vector3 defaultColorDown;
        Vector3 offsetColorUp;
        Vector3 offsetColorDown;
        DebugDrawType debugDrawType;
        DebugColorType debugColorType;
        int32_t drawNodeStart;
        int32_t drawNodeCount;
        float debugColorScale;
        float debugBoxScale;
        float debugLookAtRange;
        bool lockLookAt;
    };

    struct FxSHLightFieldParameter
    {
        bool enable;
        DebugDrawType debugDrawType;
        bool showSkyVisibility;
    };

    enum BlurType : int8_t
    {
        BLURTYPE_PREV_SURFACE = 0,
        BLURTYPE_RADIAL = 1,
        BLURTYPE_CAMERA = 2,
    };

    enum FocusType : int8_t
    {
        FOCUSTYPE_CENTER = 0,
        FOCUSTYPE_LOOKAT = 1,
        FOCUSTYPE_USER_SETTING = 2,
    };

    struct FxScreenBlurParameter
    {
        bool enable;
        BlurType blurType;
        float blurPower;
        FocusType focusType;
        Vector3 focusPosition;
        float focusRange;
        float alphaSlope;
        int32_t sampleNum;
    };

    struct FxOcclusionCapsuleParameter
    {
        bool enable;
        bool enableOcclusion;
        uint8_t occlusionColor[4];
        float occlusionPower;
        bool enableSpecularOcclusion;
        float specularOcclusionPower;
        float specularOcclusionConeAngle;
        bool enableShadow;
        uint8_t shadowColor[4];
        float shadowPower;
        float shadowConeAngle;
        float cullingDistance;
        bool enableManualLight;
        int32_t manualLightCount;
        Vector3 manualLightPos[4];
        bool debugDraw;
    };

    struct FxEffectParameter
    {
        float lightFieldColorCoefficient;
        Vector3 shadowColor;
        Vector3 directionalLightOverwrite;
        float directionalLightIntensityOverwrite;
        bool overwriteDirectionalLight;
        bool renderWireframe;
    };

    struct FxScreenSpaceGodrayParameter
    {
        bool enable;
        float num;
        float density;
        float decay;
        float threshold;
        float lumMax;
        float intensity;
        bool enableDither;
    };

    struct FxGodrayParameter
    {
        bool enable;
        Matrix4 box;
        Vector3 color;
        float num;
    };

    struct FxHeatHazeParameter
    {
        bool enable;
        float speed;
        float scale;
        float cycle;
        float nearDepth;
        float farDepth;
        float maxHeight;
        float parallaxCorrectFactor;
    };

    struct FxSceneEnvironmentParameter
    {
        float wind_rotation_y;
        float wind_strength;
        float wind_noise;
        float wind_amplitude;
        float wind_frequencies[4];
        Vector4 grass_lod_distance;
        bool enable_tread_grass;
        bool enableHighLight;
        float highLightThreshold;
        float highLightObjectAmbientScale;
        float highLightObjectAlbedoHeighten;
        float highLightCharaAmbientScale;
        float highLightCharaAlbedoHeighten;
        float highLightCharaFalloffScale;
    };

    struct FxTAAParameter
    {
        float blendRatio;
        float sharpnessPower;
    };

    struct NeedleFxParameter
    {
        FxHDROption hdrOption;
        FxRenderOption renderOption;
        FxSGGIParameter sggi;
        FxRLRParameter rlr;
        FxBloomParameter bloom;
        ToneMapType tonemapType;
        ExposureMode exposureMode;
        FxToneMapParameter tonemap;
        FxExposureParameter exposure;
        FxColorContrastParameter colorContrast;
        FxLightScatteringParameter lightscattering;
        FxDOFParameter dof;
        FxShadowMapParameter shadowmap;
        FxSSAOParameter ssao;
        FxLightFieldParameter lightfield;
        FxSHLightFieldParameter shlightfield;
        FxScreenBlurParameter blur;
        FxOcclusionCapsuleParameter occlusionCapsule;
        FxEffectParameter effect;
        FxScreenSpaceGodrayParameter ssGodray;
        FxGodrayParameter godray;
        FxHeatHazeParameter heatHaze;
        FxSceneEnvironmentParameter sceneEnv;
        FxTAAParameter taa;
    };

    struct StageCommonParameter
    {
        float deadline;
    };

    struct StageCameraParameter
    {
        float zNear;
        float zFar;
        float fovy;
    };

    struct StageConfig
    {
        StageCommonParameter common;
        StageCameraParameter camera;
    };

    struct NeedleFxSceneData
    {
        NeedleFxSceneConfig config;
        NeedleFxParameter items[16];
        StageConfig stageConfig;

        void fix(hl::bina::endian_flag flag)
        {
        }
    };
};