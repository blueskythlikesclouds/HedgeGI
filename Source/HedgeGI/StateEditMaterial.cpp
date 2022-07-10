#include "StateEditMaterial.h"

#include "AppWindow.h"
#include "MaterialWindow.h"

void StateEditMaterial::enter()
{
    document.setParent(getContext());
    document.add(std::make_unique<MaterialWindow>(true));
    document.initialize();
}

void StateEditMaterial::update(const float deltaTime)
{
    if (getContext()->get<AppWindow>()->isFocused())
        document.update(deltaTime);
}
