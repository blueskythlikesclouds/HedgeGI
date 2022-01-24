#pragma once

class StateBase;

class StateMachineBase
{
protected:
    std::stack<std::unique_ptr<StateBase>> states;
    void* context;

    StateBase* enter{};
    std::unique_ptr<StateBase> leave;

public:
    StateMachineBase();
    ~StateMachineBase();

    StateBase* getCurrentState() const;
    void setState(std::unique_ptr<StateBase>&& state);

    void pushState(std::unique_ptr<StateBase>&& state);
    void popState();

    void setContextBase(void* context);
    void* getContextBase() const;

    void update(float deltaTime);
};
