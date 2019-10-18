#include "apps/pricing/json_writer_util.h"

#include "apps/pricing/archive_iterator.h"
#include "apps/pricing/buffer.h"
#include "apps/pricing/file_system.h"
#include "apps/pricing/order.h"
#include "apps/pricing/rapidjson_errstr.h"
#include "apps/pricing/read_json_order.h"
#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <charconv>
#include <cstring>
#include <iostream>
#include <optional>
#include <rapidjson/document.h>

#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)

using JsonDoc = rapidjson::GenericDocument<rapidjson::ASCII<>>;
using JsonObject = JsonDoc::Object;
using ConstJsonObject = JsonDoc::ConstObject;
using JsonArray = JsonDoc::Array;
using ConstJsonArray = JsonDoc::ConstArray;
using JsonValue = JsonDoc::ValueType;

std::vector<Order>::const_iterator findOrderWithEqualOrGreaterId(std::vector<Order>::const_iterator begin,
                                                                 std::vector<Order>::const_iterator end,
                                                                 const Id existing_entry) {
    return std::find_if(begin, end, [&existing_entry](const Order& o) { return id(o).data >= existing_entry.data; });
}

std::vector<Order>::reverse_iterator findOrderWithSmallerId(const std::vector<Order>::reverse_iterator begin,
                                                            const std::vector<Order>::reverse_iterator end,
                                                            const Id o) {
    return std::find_if(begin, end, [&o](const Order& existing) { return id(existing).data < o.data; });
}

template <typename T>
struct write_impl {
    static void write(char*& s, const T x) {
        constexpr auto size = sizeof(T);
        std::memcpy(s, &x, size);
        s += size;
    }
};

template <typename... T>
void write_tuple(char*& s, const std::tuple<T...>& x) {
    (write_impl<T>::write(s, std::get<T>(x)), ...);
}

void write_order(char*& s, const Order& o) { write_tuple(s, o); }
void write_diff(char*& s, const Diff& d) { write_tuple(s, d); }

// template <typename T>
// struct write_impl {
//    static void write(std::ostream& f, const T x) {
//        constexpr auto size = sizeof(T);
//        std::array<char, size> buffer;
//        std::memcpy(buffer.data(), &x, size);
//        f.write(buffer.data(), size);
//    }
//};
//
// template <typename... T>
// void write_tuple(std::ostream& s, const std::tuple<T...>& x) {
//    (write_impl<T>::write(s, std::get<T>(x)), ...);
//}
//
//// Problem: tuple<T...> is padded. So for example the 50 Byte Order takes up 64 Byte. Could reorder members to improve
//// on this, but, for example, the initial Byte indicating Buy, Sell or Diff makes this problematic ...
//// template <typename... T>
//// void write_tuple(std::ostream& s, const std::tuple<T...>& x) {
////    constexpr auto size = sizeof(x);
////    std::array<char, size> buffer;
////    std::memcpy(buffer.data(), &x, size);
////    s.write(buffer.data(), size);
////}
//
// void write_order(std::ostream& s, const Order& o) { write_tuple(s, o); }
// void write_diff(std::ostream& s, const Diff& d) { write_tuple(s, d); }

void logExtracting(const char* file_name) { std::cout << "extracting " << file_name << '\n'; }

void logSkipping(const char* file_name) { std::cout << "  skipping  " << file_name << '\n'; }

void logParsing(const char* file_name) { std::cout << "  parsing " << file_name << '\n'; }

void logInvalidPage(const char* file_name) { std::cout << "page " << file_name << " is invalid\n"; }

namespace {
Buffer<char> readIntoBuffer(archive_iterator& ar) {
    Buffer<char> b;
    b.resize(ar.size() + 1);
    ar.read(b.data());
    b.back() = '\0';
    return b;
}

struct ParseResult {
    Buffer<char> b;
    rapidjson::GenericDocument<rapidjson::ASCII<>> d;
};

ParseResult parseOrders(archive_iterator& ar) {
    logParsing(ar.path());

    auto b = readIntoBuffer(ar);

    JsonDoc d;
    rapidjson::ParseResult status = d.ParseInsitu<rapidjson::kParseStopWhenDoneFlag>(b.data());

    if (!status) {
        // TODO: ParseInsitu changes the buffer, so printing the text producing the error is not very helpful :(
        errstr(std::cout, status, std::string_view{b.data(), boost::numeric_cast<std::size_t>(ar.size())});
        logInvalidPage(ar.path());
        return {};
    }

    return {std::move(b), std::move(d)};
}

std::vector<Order> convertOrders(const ParseResult& parse_result) {
    if (parse_result.d.IsNull()) {
        return {};
    }
    const auto& arr = parse_result.d["items"].GetArray();
    std::vector<Order> orders;
    orders.reserve(arr.Size());
    std::transform(arr.begin(), arr.end(), std::back_inserter(orders), [](const JsonValue& order) {
        if (!order.IsObject())
            throw std::runtime_error("not an object");
        return convert(order.GetObject());
    });
    return orders;
}

std::size_t parsePageNumber(const std::string_view f) {
    std::size_t page_number;
    const auto [p, ec] = std::from_chars(f.begin(), f.end(), page_number);
    if (ec != std::errc{}) {
        throw std::runtime_error(std::string{"invalid page number: '"} + std::string{f} + '\'');
    }
    // std::size_t characters_processed;
    // const auto possibly_signed_page_number = std::stoi(std::string{f}, &characters_processed);
    // if (characters_processed == 0) {
    //    throw std::runtime_error(std::string{"invalid file name: '"} + std::string{f} + '\'');
    //}
    // if (possibly_signed_page_number < 0) {
    //    throw std::runtime_error("negative page number: " + std::to_string(possibly_signed_page_number));
    //}
    // return static_cast<std::size_t>(possibly_signed_page_number);
    return page_number;
}

// 2016_12_04_14_55_06/8.json, order 4698198329 has volume 0, causing problems ... ... ...
// And don't ask about the rvalue ref ...
std::vector<Order> filterOrders(std::vector<Order>&& docs) {
    auto tmp{std::move(docs)};
    tmp.erase(std::remove_if(tmp.begin(), tmp.end(), [](const Order& o) { return volume(o).data == 0; }), tmp.end());
    return tmp;
}

std::vector<Order> readOrders(archive_iterator& ar) { return filterOrders(convertOrders(parseOrders(ar))); }

std::optional<std::size_t> parseFileName(const char* path) {
    // implicit conversion to string_view
    const auto [dirname, basename] = split_directory_and_basename(path);
    const auto [f, extension] = split_extension(basename);
    using namespace std::literals;

    return extension == "json"sv ? parsePageNumber(f) : std::optional<std::size_t>{};
}
} // namespace

std::vector<std::vector<Order>> readOrders(const char* file_name) {
    std::vector<std::vector<Order>> docs;
    try {
        for (auto& ar : a::archive(file_name)) {
            const char* path = ar.path();
            const auto maybe_page = parseFileName(path);

            if (UNLIKELY(!maybe_page)) {
                logSkipping(path);
                continue;
            }
            const auto page = *maybe_page;

            if (static_cast<std::int64_t>(docs.size()) < static_cast<std::int64_t>(page) + 1) {
                docs.resize(page + 1);
            }
            docs[page] = readOrders(ar);
        }
    } catch (const std::exception& e) {
        std::cout << "Exception in readOrders: " << e.what() << '\n';
    }
    docs.erase(std::remove_if(docs.begin(), docs.end(), [](const std::vector<Order>& doc) { return doc.empty(); }),
               docs.end());
    return docs;
}
