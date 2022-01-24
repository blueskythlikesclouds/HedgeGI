#pragma once

#include "StateMachineBase.h"

template<typename TContext>
class StateMachine : public StateMachineBase
{
public:
    void setContext(TContext* context)
    {
        setContextBase(context);
    }

    TContext* getContext() const
    {
        return static_cast<TContext*>(getContextBase());
    }
};