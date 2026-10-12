#pragma once

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/random_device.hpp>

template<typename T>
T random(T min, T max) {
    boost::random::random_device rd;
    boost::random::mt19937 gen(rd());
    boost::random::uniform_int_distribution<T> dist(min, max);

    return dist(gen);
}