#include "file_system.h"

#include <algorithm>
#include <cassert>
#include <iterator>

template <size_t N>
bool endswith(const std::string_view s, const char (&ext)[N]) {
    constexpr auto len = N - 1; // N includes terminating '\0'
    return s.size() >= len && std::strncmp(s.end() - len, static_cast<const char*>(ext), len) == 0;
}

bool isTgz(const char* path) {
    return endswith(path, ".tgz");
}

std::pair<std::string_view, std::string_view> split_directory_and_basename(const std::string_view s) {
    const auto it = std::find(s.rbegin(), s.rend(), '/');
    const auto dist = std::distance(s.rbegin(), it);
    assert(dist >= 0);
    const auto len = static_cast<std::string_view::size_type>(dist);
    return std::pair{std::string_view{s.begin(), s.size() - len - 1}, std::string_view{it.base(), len}};
}

std::pair<std::string_view, std::string_view> split_extension(const std::string_view s) {
    const auto it = std::find(s.begin(), s.end(), '.');
    if (it == s.end()) {
        return {s, {}};
    }
    const auto dist = std::distance(s.begin(), it);
    assert(dist >= 0);
    const auto len = static_cast<std::string_view::size_type>(dist);
    return std::pair{s.substr(0,len), s.substr(len + 1,  s.size() - len - 1)};
}

void create_directories(boost::filesystem::path& path, std::initializer_list<const char*> dirs) {
    boost::filesystem::create_directories(path);
    for (auto dir : dirs) {
        path /= dir;
        boost::filesystem::create_directory(path);
        path.remove_filename();
    }
}

directory_iterator::directory_iterator(const char* path) : it{path} {}
directory_iterator::~directory_iterator() = default;

bool directory_iterator::operator!=(const directory_iterator& rhs) const { return it != rhs.it; }

directory_iterator& directory_iterator::operator++() {
    ++it;
    return *this;
}

recursive_directory_iterator::recursive_directory_iterator(const char* path) : it{path} {}
recursive_directory_iterator::~recursive_directory_iterator() = default;

bool recursive_directory_iterator::operator!=(const recursive_directory_iterator& rhs) const { return it != rhs.it; }

recursive_directory_iterator& recursive_directory_iterator::operator++() {
    ++it;
    return *this;
}
