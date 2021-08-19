#include "SceneEffect.h"

#include "FxSceneData.h"
#include "Light.h"
#include "NeedleFxSceneData.h"

#include "Math.h"
#include "Utilities.h"

void SceneEffect::LightScattering::computeGpuValues()
{
    g_LightScattering_Ray_Mie_Ray2_Mie2.x() = rayleigh;
    g_LightScattering_Ray_Mie_Ray2_Mie2.y() = mie;
    g_LightScattering_Ray_Mie_Ray2_Mie2.z() = rayleigh * 3.0f / (PI * 16.0f);
    g_LightScattering_Ray_Mie_Ray2_Mie2.w() = mie / (PI * 4.0f);
    g_LightScattering_ConstG_FogDensity.x() = (1.0f - g) * (1.0f - g);
    g_LightScattering_ConstG_FogDensity.y() = g * g + 1.0f;
    g_LightScattering_ConstG_FogDensity.z() = g * -2.0f;
    g_LightScatteringFarNearScale.x() = 1.0f / (zFar - zNear);
    g_LightScatteringFarNearScale.y() = zNear;
    g_LightScatteringFarNearScale.z() = depthScale;
    g_LightScatteringFarNearScale.w() = inScatteringScale;
}

Vector2 SceneEffect::LightScattering::compute(const Vector3& position, const Vector3& viewPosition, const Vector3& eyePosition, const Light& light) const
{
    Vector4 r0 {}, r3 {}, r4 {};

    r0.x() = -viewPosition.z() + -g_LightScatteringFarNearScale.y();
    r0.x() = saturate(r0.x() * g_LightScatteringFarNearScale.x());
    r0.x() = r0.x() * g_LightScatteringFarNearScale.z();
    r0.y() = g_LightScattering_Ray_Mie_Ray2_Mie2.y() + g_LightScattering_Ray_Mie_Ray2_Mie2.x();
    r0.z() = 1.0f / r0.y();
    r0.x() = r0.x() * -r0.y();
    r0.x() = r0.x() * LOG2E;
    r0.x() = exp2(r0.x());
    r0.y() = -r0.x() + 1;
    r3.head<3>() = -position + eyePosition;
    r4.head<3>() = r3.head<3>().normalized();
    r3.x() = -light.position.dot(r4.head<3>());
    r3.y() = g_LightScattering_ConstG_FogDensity.z() * r3.x() + g_LightScattering_ConstG_FogDensity.y();
    r4.x() = pow(abs(r3.y()), 1.5f);
    r3.y() = 1.0f / r4.x();
    r3.y() = r3.y() * g_LightScattering_ConstG_FogDensity.x();
    r3.y() = r3.y() * g_LightScattering_Ray_Mie_Ray2_Mie2.w();
    r3.x() = r3.x() * r3.x() + 1;
    r3.x() = g_LightScattering_Ray_Mie_Ray2_Mie2.z() * r3.x() + r3.y();
    r0.z() = r0.z() * r3.x();
    r0.y() = r0.y() * r0.z();

    return Vector2(r0.x(), r0.y() * g_LightScatteringFarNearScale.w());
}

void SceneEffect::load(const tinyxml2::XMLDocument& doc)
{
    const auto element = doc.FirstChildElement("SceneEffect.prm.xml");
    if (!element) 
        return;

    if (const auto hdrElement = getElement(element, { "HDR", "Category", "Basic", "Param" }); hdrElement)
    {
        getElementValue(hdrElement->FirstChildElement("Middle_Gray"), hdr.middleGray);
        getElementValue(hdrElement->FirstChildElement("Luminance_Low"), hdr.lumMin);
        getElementValue(hdrElement->FirstChildElement("Luminance_High"), hdr.lumMax);
    }

    if (const auto defaultElement = getElement(element, { "Default", "Category", "Basic", "Param" }); defaultElement)
        getElementValue(defaultElement->FirstChildElement("CFxSceneRenderer::m_skyIntensityScale"), def.skyIntensityScale);

    if (const auto lightScatteringElement = getElement(element, { "LightScattering", "Category" }); lightScatteringElement)
    {
        lightScattering.enable = true;

        if (const auto commonElement = getElement(lightScatteringElement, { "Common", "Param" }); commonElement)
        {
            getElementValue(commonElement->FirstChildElement("ms_Color.x"), lightScattering.color.x());
            getElementValue(commonElement->FirstChildElement("ms_Color.y"), lightScattering.color.y());
            getElementValue(commonElement->FirstChildElement("ms_Color.z"), lightScattering.color.z());
            getElementValue(commonElement->FirstChildElement("ms_FarNearScale.z"), lightScattering.depthScale);
        }

        if (const auto lightScatteringElem = getElement(lightScatteringElement, { "LightScattering", "Param" }); lightScatteringElem)
        {
            getElementValue(lightScatteringElem->FirstChildElement("ms_FarNearScale.w"), lightScattering.inScatteringScale);
            getElementValue(lightScatteringElem->FirstChildElement("ms_Ray_Mie_Ray2_Mie2.x"), lightScattering.rayleigh);
            getElementValue(lightScatteringElem->FirstChildElement("ms_Ray_Mie_Ray2_Mie2.y"), lightScattering.mie);
            getElementValue(lightScatteringElem->FirstChildElement("ms_G"), lightScattering.g);
        }

        if (const auto fogElement = getElement(lightScatteringElement, { "Fog", "Param" }); fogElement)
        {
            getElementValue(fogElement->FirstChildElement("ms_FarNearScale.y"), lightScattering.zNear);
            getElementValue(fogElement->FirstChildElement("ms_FarNearScale.x"), lightScattering.zFar);
        }

        lightScattering.computeGpuValues();
    }
}

void SceneEffect::load(const sonic2013::FxSceneData& data)
{
    hdr.middleGray = data.items[0].hdr.middleGray;
    hdr.lumMin = data.items[0].hdr.luminanceLow;
    hdr.lumMax = data.items[0].hdr.luminanceHigh;
    def.skyIntensityScale = data.items[0].scene.skyIntensityScale;

    lightScattering.enable = data.items[0].olsNear.enable;
    lightScattering.color = data.items[0].olsNear.color;
    lightScattering.depthScale = data.items[0].olsNear.depthScale;
    lightScattering.inScatteringScale = data.items[0].olsNear.inScatteringScale;
    lightScattering.rayleigh = data.items[0].olsNear.rayleigh;
    lightScattering.mie = data.items[0].olsNear.mie;
    lightScattering.g = data.items[0].olsNear.g;
    lightScattering.zNear = data.items[0].olsNear.znear / 10.0f;
    lightScattering.zFar = data.items[0].olsNear.zfar / 10.0f;
    lightScattering.computeGpuValues();
}

void SceneEffect::load(const wars::NeedleFxSceneData& data)
{
    hdr.middleGray = data.items[0].tonemap.middleGray;
    hdr.lumMin = data.items[0].tonemap.lumMin;
    hdr.lumMax = data.items[0].tonemap.lumMax;

    lightScattering.enable = data.items[0].lightscattering.enable;
    lightScattering.color = data.items[0].lightscattering.color;
    lightScattering.depthScale = data.items[0].lightscattering.depthScale;
    lightScattering.inScatteringScale = data.items[0].lightscattering.inScatteringScale;
    lightScattering.rayleigh = data.items[0].lightscattering.rayleigh;
    lightScattering.mie = data.items[0].lightscattering.mie;
    lightScattering.g = data.items[0].lightscattering.g;
    lightScattering.zNear = data.items[0].lightscattering.znear / 10.0f;
    lightScattering.zFar = data.items[0].lightscattering.zfar / 10.0f;
    lightScattering.computeGpuValues();
}
