#pragma once
#include <memory>
#include <cstdint>
#include <functional>

class Semaphore { // TODO: This should be split into a simple release / acquire semaphore and a "ThreadsafeStateMachine"
private:
    struct impl;
    std::unique_ptr<impl> pimpl;

    Semaphore(const Semaphore&);
    Semaphore& operator=(const Semaphore&);
public:
    Semaphore(int64_t n = 0);
    Semaphore(Semaphore&&);
    Semaphore& operator=(Semaphore&&);
    ~Semaphore();

    void release(uint64_t n = 1);
    void acquire(uint64_t n = 1);
    void acquireAndDo(std::function<void()> f, uint64_t n = 1);
    void waitFor(std::function<bool(int64_t)> p) const;
    void setTo(int64_t n);
    bool when(std::function<bool(int64_t)> p, std::function<int64_t(int64_t)> f);
    bool ifThenSetTo(std::function<bool(int64_t)> p, int64_t n);
    void waitAndDo(std::function<bool(int64_t)> p, std::function<int64_t(int64_t)> f);
    void waitAndSetTo(std::function<bool(int64_t)> p, int64_t n);
    int64_t get() const;
};
