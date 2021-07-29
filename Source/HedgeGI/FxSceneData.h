// From LWAPI: https://github.com/Sajidur78/LWAPI/blob/master/Sonic2013/Hedgehog/Graphics/Resource/hhFxSceneParmeters.h

#pragma once

namespace sonic2013
{
	enum GIModeTypes : char
	{
		GIMODE_NORMAL = 0,
		GIMODE_ONLY = 1,
		GIMODE_NONE = 2,
		GIMODE_SHADOW = 3,
		GIMODE_SEPARATED = 4,
	};

	enum LightFieldModeTypes : char
	{
		LFMODE_NORMAL = 0,
		LFMODE_ONLY = 1,
		LFMODE_NONE = 2,
	};

	enum PeepingPlayerType : char
	{
		TYPE_DEFAULT = 0,
		TYPE_EDGE = 1,
	};

	enum EyeLightMode : char
	{
		LIGHTMODE_DIR = 0,
		LIGHTMODE_POINT = 1,
	};

	enum SaturationScalingType : char
	{
		SATURATION_KEEPING_LUMINANCE = 0,
		SATURATION_KEEPING_BRIGHTNESS = 1,
		SATURATION_NONE = 2,
	};

	enum TimeType : char
	{
		TIME_NONE = 0,
		TIME_MORNING = 1,
		TIME_DAY = 2,
		TIME_EVENING = 3,
		TIME_NIGHT = 4,
	};

	struct FxSceneConfig
	{
		float gammaTVWiiU{};
		float gammaDRCWiiU{};
		bool fixedLdr{};
		GIModeTypes giMode{};
		LightFieldModeTypes lightFieldMode{};
		bool drawLightFieldSamplingPoints{};
		bool updateLightFieldEachFrame{};
		bool drawLightFieldRegion{};
		int  screenshotLargeScale{};
		bool drawFxColGeom{};
		bool drawFxColName{};
		bool drawLocalLightSphere{};
	};

	struct FxCullingSettings
	{
		float rangeDefault{};
		float rangeNear{};
		float rangeMiddle{};
		float rangeFar{};
	};

	struct FxGrassSettings
	{
		bool grassIsHide{};
		float grassHeightMin{};
		float grassHeight{};
		float grassWidth{};
		float grassFar{};
		float grassFarEnd{};
		float grassWindAxis{};
		float grassWindSpeed{};
		float grassWindCycle{};
		float grassWindStrength{};
		uint32_t grassDupCount{};
		float grassDupRange{};
	};

	struct FxStageDistortion
	{
		bool distortionIsUse{};
		float distortionSpeed{};
		float distortionPower{};
		float distortionDensity{};
		float distortionDepthDensity{};
		float distortionPowerBloom{};
		float distortionPowerDepth{};
		float distortionPowerFront{};
		float distortionDensityFront{};
	};

	struct FxSceneCasinoLight
	{
		bool isUseCasinoLight{};
		Vector3 casinoLightAABBMin{};
		Vector3 casinoLightAABBMax{};
		float casinoLightMoveRatio{};
		float casinoLightStrengthMax{};
		float casinoLightRadMin{};
		float casinoLightRadMax{};
	};

	struct alignas(16) FxSceneSettings
	{
		float skyIntensityScale{};
		float skyFollowupRatioY{};
		bool pseudoFogEnable{};
		bool pseudoFogWithoutFar{};
		bool pseudoDof{};
		bool deepBlurEnable{};
		bool noBlurEnable{};
		float blurScale{};
		bool peepingPlayerEnable{};
		float occCheckedPlayerTime{};
		PeepingPlayerType peepingPlayerType{};
		bool clearFirstSurface{};
		bool useManualZPrepass{};
		bool useCaptureFramebufferColor{};
		bool useCaptureFramebufferDepth{};
		bool playerDrawOverlay{};
	};

	struct FxLightSettings
	{
		bool globalLightEnable{};
		bool ambLightEnable{};
		bool allLocalLightEnable{};
		bool eyeLightEnable{};
		EyeLightMode eyeLightMode{};
		Vector3 eyeLightDiffuse{};
		Vector3 eyeLightSpecular{};
		float eyeLightRangeStart{};
		float eyeLightRangeEnd{};
	};

	struct FxLightFieldSettings
	{
		bool ignoreData{};
		char defaultUpdateInterval{};
		Vector3 offsetColorUp{};
		Vector3 offsetColorDown{};
		SaturationScalingType saturationScalingType{};
		float saturationScalingRate{};
		float luminanceScalingRate{};
		bool disableFinalAdjustColor{};
		float luminanceMin{};
		float luminanceMax{};
		float luminanceMidium{};
		float intensityThreshold{};
		float intensityBias{};
		float defaultInterruption{};
		Vector3 defaultColorUp{};
		Vector3 defaultColorDown{};
	};

	struct alignas(16) FxLightScatteringSettings
	{
		bool enable{};
		Vector3 color{};
		float depthScale{};
		float inScatteringScale{};
		float rayleigh{};
		float mie{};
		float g{};
		float znear{};
		float zfar{};
	};

	struct FxHdrSettings
	{
		bool enable{};
		bool adaptationEnable{};
		float adaptationRatio{};
		float middleGray{};
		float luminanceLow{};
		float luminanceHigh{};
	};

	struct FxGlareSettings
	{
		bool enable{};
		float brightPassThreshold{};
		float brightPassInvScale{};
		float persistent{};
		float bloomScale{};
	};

	struct FxDofSettings
	{
		bool enable{};
		bool ignoreSky{};
		float focus{};
		float znear{};
		float zfar{};
		float focusRange{};
	};

	struct FxHourSettings
	{
		float middleGray{};
		Vector3 baseColor{};
		Vector3 light{};
		float skyIntensity{};
		Vector3 sky{};
		Vector3 ambient{};
		Vector3 lightScattering{};
	};

	struct alignas(16) FxTimeChangeSettings
	{
		bool enable{};
		bool ignoreSky{};
		TimeType timeDebugIndex{};
		float morning{};
		float day{};
		float evening{};
		float night{};
		FxHourSettings hourParams[4]{};
	};

	struct FxInShadowShadowScaleSettings
	{
		float shadowScaleX{};
		float shadowScaleY{};
		float shadowScaleLightFieldY{};
	};

	struct FxOnSceneShadowScaleSettings
	{
		float shadowScaleZ{};
		float shadowScaleW{};
		float shadowScaleLightFieldW{};
	};

	struct FxShadowScale
	{
		FxInShadowShadowScaleSettings shadowScaleInShadow{};
		FxOnSceneShadowScaleSettings shadowScaleOnScene{};
	};

	struct FxStencilShadow
	{
		bool enable{};
		Vector3 shadowColor{};
		float shadowAlpha{};
	};

	struct FxEffectSettings
	{
		float lightScale{};
		float shadowScale{};
	};

	struct FxParameter
	{
		FxCullingSettings culling{};
		FxSceneSettings scene{};
		FxLightSettings light{};
		FxLightFieldSettings lightField{};
		FxLightScatteringSettings olsNear{};
		bool separateOlsLayer{};
		FxLightScatteringSettings olsFar{};
		FxHdrSettings hdr{};
		FxGlareSettings glare{};
		FxDofSettings dof{};
		FxTimeChangeSettings timeChange{};
		FxShadowScale shadowScale{};
		FxGrassSettings grassSetting{};
		FxStageDistortion stageDistortion{};
		FxStencilShadow stencilShadow{};
		FxEffectSettings effect{};
		FxSceneCasinoLight casinoLight{};
	};

	struct alignas(16) FxSceneData
	{
		FxSceneConfig config{};
		FxParameter items[4]{};
	};
}