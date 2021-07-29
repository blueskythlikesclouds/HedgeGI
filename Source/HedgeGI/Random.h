#pragma once

class alignas(std::hardware_destructive_interference_size) Random
{
    std::default_random_engine engine;
    std::uniform_real_distribution<float> distribution;

    Random() : engine(std::random_device {}()), distribution(0, 1) {}

public:
    static float next()
    {
        thread_local Random random;
        return random.distribution(random.engine);
    }
};
