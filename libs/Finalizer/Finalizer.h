#pragma once

template <typename Functor>
class Finalizer {
  public:
    Finalizer(Functor f) : func(f) {}
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
