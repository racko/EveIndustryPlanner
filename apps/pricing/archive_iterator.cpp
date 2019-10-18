#include "apps/pricing/archive_iterator.h"

#include <archive.h>
#include <archive_entry.h>
#include <stdexcept>

archive_iterator::archive_iterator(const char* filename) : p_{archive_read_new()} {
    if (p_ == nullptr) {
        throw std::runtime_error("archive_iterator: archive_read_new failed");
    }
    init();
    open(filename);
    step();
}

la_int64_t archive_iterator::size() const {
    // remove this if it is to expensive (unlikely ...)
    if (archive_entry_size_is_set(ae_) == 0) {
        throw std::runtime_error("no size");
    }
    return archive_entry_size(ae_);
}

archive_iterator::~archive_iterator() noexcept { archive_read_free(p_); }

const char* archive_iterator::path() const { return archive_entry_pathname(ae_); }

void archive_iterator::read(char* b) {
    const auto n = size();
    const auto actually_read = archive_read_data(p_, b, static_cast<std::size_t>(n));
    if (actually_read != n) {
        throw std::runtime_error("archive_iterator::read: archive_read_data failed");
    }
}

void archive_iterator::open(const char* filename) { check(archive_read_open_filename(p_, filename, 16384)); }

void archive_iterator::step() {
    const auto status = archive_read_next_header(p_, &ae_);
    switch (status) {
    case ARCHIVE_OK:
        return;
    case ARCHIVE_EOF:
        ae_ = nullptr;
        return;
    default:
        throw std::runtime_error(archive_error_string(p_));
    }
}

void archive_iterator::check(int archive_result_code) const {
    if (archive_result_code != ARCHIVE_OK) {
        throw std::runtime_error(archive_error_string(p_));
    }
}

void archive_iterator::support_gzip() { check(archive_read_support_filter_gzip(p_)); }

void archive_iterator::support_tar() { check(archive_read_support_format_tar(p_)); }

void archive_iterator::init() {
    support_gzip();
    support_tar();
}
