#pragma once

class Component;

template<typename T>
class ComponentTypeId
{
protected:
    static inline uint8_t dummy;

public:
    static uintptr_t get()
    {
        return (uintptr_t)&dummy;
    }
};

class Document
{
protected:
    std::unordered_map<uintptr_t, Component*> componentMap;
    std::vector<std::unique_ptr<Component>> components;
    Document* parent;
    std::unordered_set<Document*> children;

    template<typename T>
    T* get(Document* prev)
    {
        T* component = static_cast<T*>(componentMap[ComponentTypeId<T>::get()]);
        if (component)
        {
            if (!component->initialized)
            {
                component->initialize();
                component->initialized = true;
            }
        }
        else
        {
            for (auto& document : children)
            {
                if (document == prev)
                    continue;

                component = document->get<T>(this);
                if (component)
                    return component;
            }

            if (parent && parent != prev)
                component = parent->get<T>(this);
        }

        return component;
    }

public:
    Document();
    Document(const Document& document) = delete;
    Document(Document&& document);
    ~Document();

    template<typename T>
    void add(std::unique_ptr<T>&& component)
    {
        component->document = this;
        componentMap[ComponentTypeId<T>::get()] = component.get();
        components.push_back(std::move(component));
    }

    template<typename T>
    T* get()
    {
        return get<T>(nullptr);
    }

    void setParent(Document* document);

    void initialize() const;

    void update(float deltaTime) const;
};
