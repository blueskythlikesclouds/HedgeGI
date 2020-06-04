#pragma once

class Random
{
public:
    static float next()
    {
        thread_local std::default_random_engine engine(std::random_device {}());
        const std::uniform_real_distribution<float> distribution(0, 1);
        return distribution(engine);
    }
};
