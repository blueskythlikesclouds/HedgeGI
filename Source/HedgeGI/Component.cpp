#include "Component.h"

Component::Component() : document(nullptr), initialized(false)
{
}

Component::~Component() = default;

Document* Component::getDocument() const
{
    return document;
}
