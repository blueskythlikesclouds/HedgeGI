#include "StateEditStage.h"

#include "AppWindow.h"
#include "LogListener.h"
#include "StateIdle.h"
#include "Viewport.h"

StateEditStage::StateEditStage(Document&& document) : document(std::move(document))
{
}

void StateEditStage::update(const float deltaTime)
{
    if (getContext()->get<AppWindow>()->isFocused())
        document.update(deltaTime);
}

void StateEditStage::leave()
{
    getContext()->get<Viewport>()->waitForBake();
}
