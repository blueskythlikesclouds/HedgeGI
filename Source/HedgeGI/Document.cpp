#include "Document.h"
#include "Component.h"

Document::Document() : parent(nullptr)
{
}

Document::Document(Document&& document)
{
    componentMap = std::move(document.componentMap);
    components = std::move(document.components);

    for (auto& component : components)
        component->document = this;

    setParent(document.parent);
    document.setParent(nullptr);

    children = std::move(document.children);

    for (auto& child : children)
        child->parent = this;
}

Document::~Document()
{
    components.clear();

    if (parent)
        parent->children.erase(this);

    for (auto& child : children)
        child->parent = nullptr;
}

void Document::setParent(Document* document)
{
    if (parent)
        parent->children.erase(this);

    parent = document;

    if (parent)
        parent->children.insert(this);
}

void Document::initialize() const
{
    for (auto& component : components)
    {
        if (component->initialized)
            continue;

        component->initialize();
        component->initialized = true;
    }
}

void Document::update(const float deltaTime) const
{
    for (auto& component : components) 
        component->update(deltaTime);
}
