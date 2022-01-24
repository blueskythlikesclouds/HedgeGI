#include "StateBase.h"
#include "StateMachineBase.h"

StateBase::StateBase() : stateMachine(nullptr)
{
}

StateBase::~StateBase()
{
}

void StateBase::drop()
{
    if (stateMachine->getCurrentState() == this)
        stateMachine->popState();
}

void* StateBase::getContextBase() const
{
    return stateMachine->getContextBase();
}

void StateBase::enter()
{
}

void StateBase::update(float deltaTime)
{
}

void StateBase::leave()
{
}
