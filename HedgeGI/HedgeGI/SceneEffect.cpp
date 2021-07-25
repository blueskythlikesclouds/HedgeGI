#include "SceneEffect.h"
#include "FxSceneData.h"
#include "NeedleFxSceneData.h"

void SceneEffect::load(const tinyxml2::XMLDocument& doc)
{
    const tinyxml2::XMLElement* element = doc.FirstChildElement("SceneEffect.prm.xml");
    if (!element) return;

    element = element->FirstChildElement("HDR");
    if (!element) return;

    element = element->FirstChildElement("Category");
    if (!element) return;

    element = element->FirstChildElement("Basic");
    if (!element) return;

    element = element->FirstChildElement("Param");
    if (!element) return;

    const tinyxml2::XMLElement* subElement = element->FirstChildElement("Middle_Gray");
    if (subElement)
        hdr.middleGray = subElement->FloatText();

    subElement = element->FirstChildElement("Luminance_Low");
    if (subElement)
        hdr.lumMin = subElement->FloatText();

    subElement = element->FirstChildElement("Luminance_High");
    if (subElement)
        hdr.lumMax = subElement->FloatText();
}

void SceneEffect::load(const sonic2013::FxSceneData& data)
{
    hdr.middleGray = data.items[0].hdr.middleGray;
    hdr.lumMin = data.items[0].hdr.luminanceLow;
    hdr.lumMax = data.items[0].hdr.luminanceHigh;
}

void SceneEffect::load(const wars::NeedleFxSceneData& data)
{
    hdr.middleGray = data.items[0].tonemap.middleGray;
    hdr.lumMin = data.items[0].tonemap.lumMin;
    hdr.lumMax = data.items[0].tonemap.lumMax;
}
