#pragma once

class StateMachineBase;

class StateBase
{
    friend class StateMachineBase;

protected:
    StateMachineBase* stateMachine;

public:
    StateBase();
    virtual ~StateBase();

    void drop();

    void* getContextBase() const;

    virtual void enter();
    virtual void update(float deltaTime);
    virtual void leave();
};