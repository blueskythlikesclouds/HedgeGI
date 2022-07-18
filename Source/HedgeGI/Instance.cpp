#include "Instance.h"

#include "Mesh.h"
#include "PropertyBag.h"

void Instance::buildAABB()
{
    aabb.setEmpty();

    for (auto& mesh : meshes)
        aabb.extend(mesh->aabb);
}

uint16_t Instance::getResolution(const PropertyBag& propertyBag) const
{
    return propertyBag.get<uint16_t>(name + ".resolution", originalResolution > 0 ? originalResolution : 256);
}

void Instance::setResolution(PropertyBag& propertyBag, const uint16_t resolution)
{
    if (resolution > 0)
        propertyBag.set(name + ".resolution", resolution);
}
