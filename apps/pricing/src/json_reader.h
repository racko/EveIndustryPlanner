#pragma once

#include "order.h"
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <vector>

class mapped_file {
  public:
    mapped_file(const char* file_name);
    ~mapped_file();
    std::string_view text() { return {static_cast<const char*>(region.get_address()), region.get_size()}; }

  private:
    boost::interprocess::file_mapping file;
    boost::interprocess::mapped_region region;
};

class OrderReader {
  public:
    struct OrderIterator {
        OrderIterator() = default;
        OrderIterator(OrderReader* reader) : reader_{reader} { step(); }
        void step() { reader_->step(); }
        OrderIterator& operator++() {
            step();
            return *this;
        }
        //const std::vector<Order>& operator*() const { return reader_->active_orders; }
        auto operator*() const { return std::tie(reader_->viewtime, reader_->active_orders); }
        // hack: return true when we are done
        bool operator!=(const OrderIterator&) { return !reader_->text.empty(); }
        OrderReader* reader_{};
    };

    OrderReader(std::string_view text);
    OrderReader();
    ~OrderReader();
    OrderIterator begin() { return {this}; }
    OrderIterator end() { return {}; }
    // apply changes in deleted and inserted to active_orders
    void finalize();
    // read single order/diff
    //void read_step(std::string_view& doc, std::vector<std::uint64_t>::iterator& delete_it,
    //               std::vector<Order>::iterator& insert_it);
    std::string_view read_step(std::string_view doc);
    // read_step with delete_it = deleted.begin() a,d insert_it = inserted.begin()
    //void read_step(std::string_view doc);
    // read_doc + finalize
    void step();
    // read_step in a loop until doc.empty()
    void read_doc(std::string_view doc);

    const std::vector<Order>& orders() const { return active_orders; }

  private:
    std::string_view text;
    std::uint32_t viewtime;
    std::vector<Order> active_orders;
    std::vector<std::uint64_t> deleted;
    std::vector<Order> inserted;
};

template <typename Functor>
void read_orders(const char* file_name, Functor f) {
    mapped_file file{file_name};
    OrderReader order_reader{file.text()};
    for (const auto& orders : order_reader) {
        f(std::get<0>(orders), std::get<1>(orders));
    }
}

void read_orders_benchmark(const char* file_name);
