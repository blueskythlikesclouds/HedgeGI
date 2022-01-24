#pragma once

#include "StateBase.h"

template<typename TContext>
class State : public StateBase
{
public:
    TContext* getContext() const
    {
        return static_cast<TContext*>(getContextBase());
    }
};
