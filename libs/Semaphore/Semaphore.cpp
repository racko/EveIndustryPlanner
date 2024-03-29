#include "libs/Semaphore/Semaphore.h"

#include <cassert>
#include <condition_variable>
#include <mutex>

namespace {
int64_t to_int(std::uint64_t n) {
    assert(n <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    return static_cast<std::int64_t>(n);
}
} // namespace

struct Semaphore::impl {
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    std::int64_t count;

    impl(std::int64_t n) : count(n) {}

    void release(std::uint64_t n) {
        std::lock_guard<std::mutex> lock(mutex);
        count += to_int(n);
        condition.notify_all();
    }

    void acquire(std::uint64_t n) {
        auto n_ = to_int(n);
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this, n_]() { return count >= n_; });
        count -= n_;
    }

    void acquireAndDo(std::function<void()> f, std::uint64_t n) {
        auto n_ = to_int(n);
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this, n_]() { return count >= n_; });
        count -= n_;
        f();
    }

    void waitFor(std::function<bool(std::int64_t)> p) const {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this, p]() { return p(count); });
    }

    void setTo(std::int64_t n) {
        std::lock_guard<std::mutex> lock(mutex);
        count = n;
        condition.notify_all();
    }

    bool when(std::function<bool(std::int64_t)> p, std::function<std::int64_t(std::int64_t)> f) {
        std::lock_guard<std::mutex> lock(mutex);
        if (p(count)) {
            count = f(count);
            condition.notify_all();
            return true;
        }

        return false;
    }

    bool ifThenSetTo(std::function<bool(std::int64_t)> p, std::int64_t n) {
        std::lock_guard<std::mutex> lock(mutex);
        if (p(count)) {
            count = n;
            condition.notify_all();
            return true;
        }

        return false;
    }

    void waitAndDo(std::function<bool(std::int64_t)> p, std::function<std::int64_t(std::int64_t)> f) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this, p]() { return p(count); });
        count = f(count);
        condition.notify_all();
    }

    void waitAndSetTo(std::function<bool(std::int64_t)> p, std::int64_t n) {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this, p]() { return p(count); });
        count = n;
        condition.notify_all();
    }
};

Semaphore::Semaphore(std::int64_t n) : pimpl(new impl(n)) {}

Semaphore::Semaphore(Semaphore&& rhs) = default;
Semaphore& Semaphore::operator=(Semaphore&&) = default;

Semaphore::~Semaphore() = default;

void Semaphore::release(std::uint64_t n) { pimpl->release(n); }

void Semaphore::acquire(std::uint64_t n) { pimpl->acquire(n); }

void Semaphore::acquireAndDo(std::function<void()> f, std::uint64_t n) { pimpl->acquireAndDo(f, n); }

void Semaphore::waitFor(std::function<bool(std::int64_t)> p) const { pimpl->waitFor(p); }

void Semaphore::setTo(std::int64_t n) { pimpl->setTo(n); }

bool Semaphore::when(std::function<bool(std::int64_t)> p, std::function<std::int64_t(std::int64_t)> f) {
    return pimpl->when(p, f);
}

bool Semaphore::ifThenSetTo(std::function<bool(std::int64_t)> p, std::int64_t n) { return pimpl->ifThenSetTo(p, n); }

void Semaphore::waitAndDo(std::function<bool(std::int64_t)> p, std::function<std::int64_t(std::int64_t)> f) {
    pimpl->waitAndDo(p, f);
}

void Semaphore::waitAndSetTo(std::function<bool(std::int64_t)> p, std::int64_t n) { pimpl->waitAndSetTo(p, n); }

int64_t Semaphore::get() const { return pimpl->count; }
