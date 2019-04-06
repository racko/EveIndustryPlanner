#pragma once

#include <cstdint>
#include <iterator>

struct archive;
struct archive_entry;

class archive_iterator {
  public:
    using iterator_category = std::input_iterator_tag;

    archive_iterator() = default;

    archive_iterator(const char* filename);

    // need to set pointers to nullptr on move, otherwise we double-archive_read_free
    archive_iterator(archive_iterator&&) = delete;
    archive_iterator& operator=(archive_iterator&&) = delete;

    archive_iterator(const archive_iterator&) = delete;
    archive_iterator& operator=(const archive_iterator&) = delete;

    ~archive_iterator() noexcept;

    // FIXME? may return nullptr on error
    const char* path() const;

    std::int64_t size() const;

    archive_iterator& operator*() { return *this; }

    const archive_iterator& operator*() const { return *this; }

    const archive_iterator* operator->() const { return this; }

    archive_iterator* operator->() { return this; }

    bool operator!=(const archive_iterator& rhs) const { return ae_ != rhs.ae_; }

    archive_iterator& operator++() {
        step();
        return *this;
    }

    // assert(b.size() >= n);
    void read(char* b);

  private:
    // calling open on an open archive yields undefined behavior
    void open(const char* filename);

    void step();

    void check(int archive_result_code) const;

    void support_gzip();

    void support_tar();

    void init();

    ::archive* p_{};
    archive_entry* ae_{};
};

// disambiguation: libarchive also has 'struct archive' ...
namespace a {
struct archive {
    using iterator = archive_iterator;
    archive(const char* path) : path_(path) {}
    iterator begin() { return iterator(path_); }
    iterator end() { return iterator(); }
    const char* path_;
};
} // namespace a
