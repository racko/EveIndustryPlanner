#pragma once

#include <type_traits>
#include <utility>

template <typename Functor>
class Finalizer {
    static_assert(std::is_invocable_v<Functor> && std::is_move_constructible_v<Functor>);

  public:
    Finalizer(Functor f) : func(std::move(f)) {}
    ~Finalizer() { finalize(); }
    Finalizer(const Finalizer&) = delete;
    Finalizer(Finalizer&&) = delete;
    Finalizer& operator=(const Finalizer&) = delete;
    Finalizer& operator=(Finalizer&&) = delete;
    void abort() { finalized = true; }
    void finalize() {
        if (!finalized) {
            func();
            finalized = true;
        }
    }

  private:
    bool finalized = false;
    Functor func;
};

template <typename Functor>
auto makeFinalizer(Functor f) {
    return Finalizer<Functor>(f);
}
