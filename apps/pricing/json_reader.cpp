#include "apps/pricing/json_reader.h"

#include <boost/filesystem.hpp>
#include <iostream>

namespace bi = boost::interprocess;
namespace bf = boost::filesystem;
using bf::file_size;
using bf::resize_file;
using bi::copy_on_write;
using bi::file_mapping;
using bi::mapped_region;
using bi::read_only;
using bi::read_write;

namespace {
template <typename T>
struct skip_impl {
    static void skip(std::string_view& data) { data.remove_prefix(sizeof(T)); }
};

template <typename... T>
struct skip_impl<std::tuple<T...>> {
    static void skip(std::string_view& data) { (skip_impl<T>::skip(data), ...); }
};

template <typename T>
void skip(std::string_view& data) {
    return skip_impl<T>::skip(data);
}

[[maybe_unused]] void skip_order(std::string_view& data) { return skip<Order::Base>(data); }

[[maybe_unused]] void skip_diff(std::string_view& data) { return skip<Diff::Base>(data); }

template <typename T>
struct read_impl {
    static T read(std::string_view& data) {
        constexpr auto size = sizeof(T);
        T x;
        std::memcpy(&x, data.data(), size);
        data.remove_prefix(size);
        return x;
    }
};

template <typename... T>
struct read_impl<std::tuple<T...>> {
    static std::tuple<T...> read(std::string_view& data) { return {read_impl<T>::read(data)...}; }
};

template <typename T>
T read(std::string_view& data) {
    return read_impl<T>::read(data);
}

// without "always_inline", this function would not get inlined in read_step. RVO only stores to the stack, in read_step
// the order would then get copied to it's destination. 235 vs 300ms
__attribute__((always_inline)) inline Order read_order(std::string_view& data) { return read<Order::Base>(data); }

Diff read_diff(std::string_view& data) { return read<Diff::Base>(data); }

template <typename T>
T peek(const std::string_view& data) {
    auto tmp{data};
    return read<T>(tmp);
}

[[maybe_unused]] Order peek_order(const std::string_view& data) { return peek<Order::Base>(data); }

[[maybe_unused]] Diff peek_diff(const std::string_view& data) { return peek<Diff::Base>(data); }

} // namespace

mapped_file::mapped_file(const char* file_name)
    : file{file_name, read_only}, region{file, read_only, 0, file_size(file_name)} {}

mapped_file::~mapped_file() = default;

OrderReader::OrderReader(const std::string_view s)
    : text{s}, active_orders(600000) { /*active_orders.reserve(600'000);*/
}
OrderReader::~OrderReader() = default;

/*
void OrderReader::read_step(std::string_view& doc, std::vector<std::uint64_t>::iterator& delete_it,
                            std::vector<Order>::iterator& insert_it) {
    // const auto read_index = read<std::int32_t>(doc);
    if (doc[0] == 2) {
        const Diff d = read_diff(doc);
        delete_it = std::find_if(delete_it, deleted.end(), [i = id(d).data](const std::uint64_t j) { return j >= i; });
        insert_it = std::find_if(insert_it, inserted.end(), [i = id(d)](const Order& o) { return id(o) >= i; });
        const auto offset = std::distance(deleted.begin(), delete_it) - std::distance(inserted.begin(), insert_it);

        const auto fixed_index = index(d).data + offset;
        if (fixed_index >= static_cast<std::int64_t>(active_orders.size()))
            throw std::runtime_error("fixed_index out of bounds");

        if (insert_it != inserted.end() && id(*insert_it) == id(d)) {
            std::cout << id(d).data << " is in the 'inserted' vector\n";
        }
        auto& o = insert_it != inserted.end() && id(*insert_it) == id(d)
                      ? *insert_it
                      : active_orders[static_cast<std::size_t>(fixed_index)];
        volume(o) = volume(d);
        price(o) = price(d);
        if (volume(d).data == 0) {
            // std::cout << "delete " << id(o).data << " (" << index(d).data << ")\n";
            if (id(o) != id(d))
                throw std::runtime_error("id mismatch in delete");
            const auto k = std::distance(deleted.begin(), delete_it);
            deleted.push_back(id(o).data);
            delete_it = deleted.begin() + k;
            //++offset;
        } else {
            // std::cout << "change " << id(o).data << " (" << index(d).data << ")\n";
            if (id(o) != id(d))
                throw std::runtime_error("id mismatch in diff");
        }
    } else {
        const auto o = read_order(doc);
        // active_orders.insert(active_orders.begin() + index, read_order(doc));

        // delete_it = std::find_if(delete_it, deleted.end(), [i = id(o).data](const std::uint64_t j) { return j >= i;
        // });

        // insert_it = std::find_if(insert_it, inserted.end(), [i = id(o)](const Order& x) { return id(x) >= i; });

        // const auto offset = std::distance(deleted.begin(), delete_it) - std::distance(inserted.begin(), insert_it);

        // const auto fixed_index = index(o).data + offset;

        // if (fixed_index > static_cast<std::int64_t>(active_orders.size()))
        //    throw std::runtime_error("fixed_index out of bounds");

        if (active_orders.empty() || id(o) > id(active_orders.back())) {
            // std::cout << "push_back " << o << '\n';
            if (!active_orders.empty() && id(o) <= id(active_orders.back()))
                throw std::runtime_error("id mismatch in push_back");
            active_orders.push_back(o);
        } else {
            // std::cout << "insert " << o << '\n';
            // if (id(o) >= id(active_orders[static_cast<std::size_t>(index)]) ||
            //    (index > 0 && id(o) <= id(active_orders[static_cast<std::size_t>(index - 1)])))
            //    throw std::runtime_error("id mismatch in insert");
            const auto k = std::distance(inserted.begin(), insert_it);
            inserted.push_back(o);
            insert_it = inserted.begin() + k;
            //--offset;
        }
    }
}
*/

std::string_view OrderReader::read_step(std::string_view doc) {
    // std::cerr << "doc.size(): " << doc.size() << '\n';
    const auto read_index = read<std::int32_t>(doc);
    // std::cerr << "index: " << read_index << '\n';
    if (doc[0] == 2) {
        // if (read_index >= static_cast<std::int64_t>(active_orders.size()))
        //   throw std::runtime_error("inconsistency");
        const Diff d = read_diff(doc);

        auto& o = active_orders[static_cast<std::size_t>(read_index)];
        volume(o) = volume(d);
        price(o) = price(d);
        // if (id(o) != id(d))
        //   throw std::runtime_error("id mismatch");
    } else
    // if (doc[0] == 0 || doc[0] == 1)
    {
        // if (read_index >= static_cast<std::int64_t>(active_orders.size()))
        //   throw std::runtime_error("oob: " + std::to_string(read_index));
        // if (read_index < static_cast<std::int64_t>(active_orders.size()) &&
        // volume(active_orders[static_cast<std::size_t>(read_index)]) > 0)
        //   throw std::runtime_error("inconsistency");

        active_orders[static_cast<std::size_t>(read_index)] = read_order(doc);
    }
    // else
    //   throw std::runtime_error("invalid record type + " + std::to_string(int(doc[0])));
    return doc;
}

/*
void OrderReader::read_step(std::string_view doc) {
    auto delete_it = deleted.begin();
    auto insert_it = inserted.begin();

    read_step(doc, delete_it, insert_it);
}
*/

void OrderReader::read_doc(std::string_view doc) {
    // auto delete_it = deleted.begin();
    // auto insert_it = inserted.begin();

    while (!doc.empty()) {
        // read_step(doc, delete_it, insert_it);
        doc = read_step(doc);
    }
}

void OrderReader::step() {
    viewtime = read<std::uint32_t>(text);
    const auto doc_count = static_cast<std::int32_t>(read<std::int8_t>(text));

    // std::cerr << "view: " << viewtime << " (" << doc_count << " pages)\n";
    for (auto i = 0; i < doc_count; ++i) {
        [[maybe_unused]] const auto first_id = read<std::int64_t>(text);
        [[maybe_unused]] const auto last_id = read<std::int64_t>(text);
        // std::cerr << "  " << i << ": " << first_id << " - " << last_id << '\n';
        const auto doc_size = read<std::int32_t>(text);
        read_doc(text.substr(0, static_cast<std::size_t>(doc_size)));
        text.remove_prefix(static_cast<std::size_t>(doc_size));
    }
    // std::cout << "deleted.size(): " << deleted.size() << '\n';
    // std::cout << "inserted.size(): " << inserted.size() << '\n';
    // std::cout << "active_orders.size(): " << active_orders.size() << '\n';
    // finalize();
}

void OrderReader::finalize() {
    if (!std::is_sorted(
            active_orders.begin(), active_orders.end(), [](const Order& a, const Order& b) { return id(a) < id(b); }))
        throw std::runtime_error("active_orders is not sorted");
    if (!std::is_sorted(deleted.begin(), deleted.end()))
        throw std::runtime_error("deleted is not sorted");
    if (!std::is_sorted(inserted.begin(), inserted.end(), [](const auto& a, const auto& b) { return id(a) < id(b); }))
        throw std::runtime_error("inserted is not sorted");
    deleted.clear();
    const auto snapshot = std::move(active_orders);
    active_orders.clear();
    active_orders.reserve(600'000);
    auto insert_it = inserted.cbegin();
    for (const auto& o : snapshot) {
        const auto new_insert_it =
            std::find_if(insert_it, inserted.cend(), [i = id(o)](const auto& ins) { return id(ins) > i; });
        active_orders.insert(active_orders.end(), insert_it, new_insert_it);
        insert_it = new_insert_it;
        if (volume(o) == 0) {
            // std::cout << "deleting " << id(*it).data << '\n';
            continue;
        }
        // std::cout << "pushing back " << id(*it).data << '\n';
        active_orders.push_back(o);
    }
    if (insert_it != inserted.end())
        throw std::runtime_error("Did not insert all orders");
    inserted.clear();
    if (!std::is_sorted(
            active_orders.begin(), active_orders.end(), [](const Order& a, const Order& b) { return id(a) < id(b); }))
        throw std::runtime_error("active_orders is not sorted");
}

void read_orders_benchmark(const char* file_name) {
    const auto size = file_size(file_name);
    file_mapping file(file_name, read_only);
    mapped_region region(file, read_only, 0, size);
    region.advise(mapped_region::advice_sequential);
    std::string_view text{static_cast<const char*>(region.get_address()), size};
    while (!text.empty()) {
        [[maybe_unused]] const auto viewtime = read<std::int32_t>(text);
        const auto doc_count = static_cast<std::int32_t>(read<std::int8_t>(text));

        for (auto i = 0; i < doc_count; ++i) {
            [[maybe_unused]] const auto first_id = read<std::int64_t>(text);
            [[maybe_unused]] const auto last_id = read<std::int64_t>(text);
            const auto doc_size = read<std::int32_t>(text);
            auto doc = text.substr(0, static_cast<std::size_t>(doc_size));
            text.remove_prefix(static_cast<std::size_t>(doc_size));

            while (!doc.empty()) {
                if (doc[0] == 2) {
                    read_diff(doc);
                } else {
                    read_order(doc);
                }
            }
        }
    }
}
