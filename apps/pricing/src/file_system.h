#pragma once

#include <boost/filesystem.hpp>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <tuple>

bool isTgz(const char* path);

std::pair<std::string_view, std::string_view> split_directory_and_basename(const std::string_view s);

std::pair<std::string_view, std::string_view> split_extension(const std::string_view s);

// void create_directories(boost::filesystem::path& path, std::initializer_list<const char*> dirs);

class directory_iterator {
  public:
    using value_type = const char*;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = const char**;
    using reference = const char*;

    directory_iterator() = default;
    directory_iterator(const char* path);
    ~directory_iterator();

    // "must return reference, convertible to value_type"
    const char* operator*() const { return it->path().c_str(); }

    bool operator!=(const directory_iterator& rhs) const;

    directory_iterator& operator++() &;

  private:
    boost::filesystem::directory_iterator it;
};

class recursive_directory_iterator {
  public:
    using value_type = const char*;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = const char**;
    using reference = const char*;

    recursive_directory_iterator() = default;
    recursive_directory_iterator(const char* path);
    ~recursive_directory_iterator();

    // "must return reference, convertible to value_type"
    const char* operator*() const { return it->path().c_str(); }

    bool operator!=(const recursive_directory_iterator& rhs) const;

    recursive_directory_iterator& operator++() &;

  private:
    boost::filesystem::recursive_directory_iterator it;
};

struct directory {
    using iterator = directory_iterator;
    directory(const char* path) : path_(path) {}
    iterator begin() { return iterator(path_); }
    iterator end() { return iterator(); }
    const char* path_;
};

struct directory_tree {
    using iterator = recursive_directory_iterator;
    directory_tree(const char* path) : path_(path) {}
    iterator begin() { return iterator(path_); }
    iterator end() { return iterator(); }
    const char* path_;
};
