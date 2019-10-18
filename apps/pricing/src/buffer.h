#pragma once

#include <cstdint>

template <typename T>
class Buffer {
  public:
    using value_type = T;

    Buffer() = default;

    Buffer(Buffer&& rhs) noexcept : data_{rhs.data_}, end_{rhs.end_} {
        rhs.data_ = nullptr;
        rhs.end_ = nullptr;
    }

    Buffer& operator=(Buffer&& rhs) noexcept {
        data_ = rhs.data_;
        end_ = rhs.end_;
        rhs.data_ = nullptr;
        rhs.end_ = nullptr;
        return *this;
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    ~Buffer() noexcept { delete[] data_; }

    const value_type* data() const& { return data_; }
    value_type* data() & { return data_; }

    std::int64_t size() const { return end_ - data_; }

    value_type* begin() & { return data_; }
    value_type* end() & { return end_; }

    const value_type* begin() const& { return data_; }
    const value_type* end() const& { return end_; }

    const value_type* cbegin() const { return data_; }
    const value_type* cend() const { return end_; }

    value_type& operator[](const std::int64_t i) & { return data_[i]; }
    const value_type& operator[](const std::int64_t i) const& { return data_[i]; }

    value_type& front() & { return *data_; }
    const value_type& front() const& { return *data_; }

    value_type& back() & { return *(end_ - 1); }
    const value_type& back() const& { return *(end_ - 1); }

    // provides strong exception safety guarantee
    // FIXME: a.resize(n) suggests that afterwards a.back() == a[n-1], which isn't the case when n < a.size()
    // Also, it doesn't keep existing data.
    void resize(const std::int64_t n) {
        if (n > size()) {
            auto tmp = new value_type[static_cast<std::size_t>(n)];
            delete[] data_;
            data_ = tmp;
            end_ = data_ + n;
        }
    }

  private:
    value_type* data_{};
    value_type* end_{};
};
