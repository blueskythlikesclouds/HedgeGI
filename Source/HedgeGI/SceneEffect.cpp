#include "SceneEffect.h"

#include "FxSceneData.h"
#include "NeedleFxSceneData.h"

#include "Utilities.h"

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
}

void SceneEffect::load(const sonic2013::FxSceneData& data)
{
    hdr.middleGray = data.items[0].hdr.middleGray;
    hdr.lumMin = data.items[0].hdr.luminanceLow;
    hdr.lumMax = data.items[0].hdr.luminanceHigh;
    def.skyIntensityScale = data.items[0].scene.skyIntensityScale;
}

void SceneEffect::load(const wars::NeedleFxSceneData& data)
{
    hdr.middleGray = data.items[0].tonemap.middleGray;
    hdr.lumMin = data.items[0].tonemap.lumMin;
    hdr.lumMax = data.items[0].tonemap.lumMax;
}
