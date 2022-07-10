#pragma once

#include "UIComponent.h"

class MaterialWindow : public UIComponent
{
    bool fullscreen;

    std::vector<std::unique_ptr<hl::hh::mirage::material>> materials;
    hl::hh::mirage::material* selection {};

    char search[1024]{};

    template<typename T>
    static void drawParams(const char* name, std::vector<hl::hh::mirage::material_param<T>>& params);
        
public:
    bool visible;

    MaterialWindow(bool fullscreen);

    void initialize() override;
    void update(float deltaTime) override;
};