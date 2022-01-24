#pragma once

#include "Document.h"

class Document;

class Component
{
    friend class Document;

protected:
    Document* document;
    bool initialized;

    template<typename T>
    T* get()
    {
        return document->get<T>();
    }

public:
    Component();
    virtual ~Component();

    Document* getDocument() const;

    virtual void initialize() {}
    virtual void update(float deltaTime) {}
};