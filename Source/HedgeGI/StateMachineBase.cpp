#include "StateMachineBase.h"
#include "StateBase.h"

StateMachineBase::StateMachineBase() : context(nullptr)
{
}

StateMachineBase::~StateMachineBase()
{
}

StateBase* StateMachineBase::getCurrentState() const
{
    return !states.empty() ? states.top().get() : nullptr;
}

void StateMachineBase::setState(std::unique_ptr<StateBase>&& state)
{
    popState();
    pushState(std::move(state));
}

void StateMachineBase::pushState(std::unique_ptr<StateBase>&& state)
{
    states.push(std::move(state));
    enter = states.top().get();
}

void StateMachineBase::popState()
{
    if (states.empty())
        return;

    leave = std::move(states.top());
    states.pop();
}

void StateMachineBase::setContextBase(void* context)
{
    this->context = context;
}

void* StateMachineBase::getContextBase() const
{
    return context;
}

void StateMachineBase::update(const float deltaTime)
{
    if (leave)
    {
        leave->leave();
        leave->stateMachine = nullptr;
        leave = nullptr;
    }

    if (enter)
    {
        enter->stateMachine = this;
        enter->enter();
        enter = nullptr;
    }

    if (!states.empty()) 
        states.top()->update(deltaTime);
}
