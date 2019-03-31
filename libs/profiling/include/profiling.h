#pragma once
#include <chrono>
#include <iostream>

template<typename Rep = std::chrono::high_resolution_clock::duration::rep, typename Period = std::chrono::high_resolution_clock::duration::period, typename Functor>
std::chrono::duration<Rep, Period> time(Functor f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
	
    return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(std::chrono::high_resolution_clock::now() - start);
}

#define TIME(STMT) do { \
    auto t = time<float,std::milli>([&] { STMT }); \
    std::cout << #STMT << ": " << t.count() << "ms\n"; \
} while(0)
