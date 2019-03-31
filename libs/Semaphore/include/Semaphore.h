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
    Semaphore(std::int64_t n = 0);
    Semaphore(Semaphore&&);
    Semaphore& operator=(Semaphore&&);
    ~Semaphore();

    void release(std::uint64_t n = 1);
    void acquire(std::uint64_t n = 1);
    void acquireAndDo(std::function<void()> f, std::uint64_t n = 1);
    void waitFor(std::function<bool(std::int64_t)> p) const;
    void setTo(std::int64_t n);
    bool when(std::function<bool(std::int64_t)> p, std::function<std::int64_t(std::int64_t)> f);
    bool ifThenSetTo(std::function<bool(std::int64_t)> p, std::int64_t n);
    void waitAndDo(std::function<bool(std::int64_t)> p, std::function<std::int64_t(std::int64_t)> f);
    void waitAndSetTo(std::function<bool(std::int64_t)> p, std::int64_t n);
    std::int64_t get() const;
};
