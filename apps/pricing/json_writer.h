#pragma once

#include "apps/pricing/json_reader.h"
#include "apps/pricing/order.h"
#include <boost/filesystem/path.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <deque>
#include <vector>

class archive_iterator;

class JsonWriter {
  public:
    JsonWriter(boost::filesystem::path file_name);
    JsonWriter(const JsonWriter&) = delete;
    JsonWriter& operator=(const JsonWriter&) = delete;
    JsonWriter(JsonWriter&&) = delete;
    JsonWriter& operator=(JsonWriter&&) = delete;
    ~JsonWriter();
    void write(const char* source_directory_path);

  private:
    void extract2(const char* file_name);

    std::vector<Order>::iterator insert(std::vector<Order>::iterator hint, const Order& o);

    void writeOrders(const std::vector<Order>& last_orders);

    std::vector<Order>::const_iterator
    writeOrder(const Order& o, const std::vector<Order>& last_orders, std::vector<Order>::const_iterator hint);

    void updateActiveOrders(const std::vector<std::vector<Order>>& docs);

    void updateActiveOrders(const std::vector<Order>& arr);

    std::vector<Order>::const_iterator writeOrdersIfRemoved(const Order& existing_entry,
                                                            std::vector<Order>::const_iterator hint);
    void writeRemovedOrders(const std::vector<Order>& last_orders);

    void check(const char* start, const char* stop, std::int64_t order_count);

    std::vector<Order> active_orders_;
    std::vector<Order> active_orders2_;
    std::unordered_map<std::uint64_t, std::int32_t> index_;
    // std::vector<std::int32_t> free_list_;
    std::deque<std::int32_t> free_list_;

    boost::filesystem::path file_name_{};
    boost::interprocess::file_mapping file_;
    boost::interprocess::mapped_region region_;
    char* start_{};
    char* current_{};
    char* stop_{};
    OrderReader reader_;
    OrderReader::OrderIterator reader_it_;
};
