#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <random>
#include <ctime>
#include <chrono>

template<typename T>
class Random {
private:
    unsigned seed;
    std::default_random_engine generator;
public:
    Random()
    {
        seed = std::chrono::system_clock::now().time_since_epoch().count();
        generator = std::default_random_engine(seed);
    }

    T operator()(T lowerBound, T upperBound)
    {
        std::uniform_real_distribution<T> distribution(lowerBound, upperBound);
        return distribution(this->generator);
    }
};

#endif