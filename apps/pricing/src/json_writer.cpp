#include "json_writer.h"

#include "file_system.h"
#include "json_writer_util.h"
#include <iostream>
#include <numeric>

namespace bi = boost::interprocess;
namespace bf = boost::filesystem;
using bf::file_size;
using bf::resize_file;
using bi::copy_on_write;
using bi::file_mapping;
using bi::mapped_region;
using bi::read_only;
using bi::read_write;

JsonWriter::JsonWriter(const boost::filesystem::path file_name)
    : file_name_{std::move(file_name)},
      file_{file_name_.c_str(), read_write},
      region_{file_, read_write, 0, file_size(file_name_)},
      start_{static_cast<char*>(region_.get_address())},
      current_{start_},
      stop_{static_cast<char*>(start_ + region_.get_size())},
      reader_{{start_, region_.get_size()}} {}
JsonWriter::~JsonWriter() {
    // destroy region? close file?
    resize_file(file_name_, static_cast<std::size_t>(current_ - start_));
}

namespace {
bool equal_ignoring_issued(const Order& a, const Order& b) {
    return buy(a) == buy(b) && price(a) == price(b) && volumeEntered(a) == volumeEntered(b) &&
           stationId(a) == stationId(b) && volume(a) == volume(b) && range(a) == range(b) &&
           minVolume(a) == minVolume(b) && duration(a) == duration(b) && type(a) == type(b) && id(a) == id(b);
}
} // namespace

void JsonWriter::check(const char* const start, const char* const stop, std::int64_t order_count) {
    std::string_view tmpdoc{start, static_cast<std::size_t>(std::distance(start, stop))};
    reader_.read_step(tmpdoc);
    reader_.finalize();

    const auto& read_orders = reader_.orders();
    if (static_cast<std::int64_t>(read_orders.size()) < order_count) {
        throw std::runtime_error("not enough orders");
    }
    assert(order_count <= static_cast<std::int64_t>(active_orders_.size()));
    // if (read_orders.size() != active_orders_.size())
    //    std::cerr << "size mismatch. Actual: " << read_orders.size() << ", Expected: " << active_orders_.size() <<
    //    '\n';
    bool abort = false;

    for (auto i = 0U; i < order_count; ++i) {
        if (!equal_ignoring_issued(read_orders[i], active_orders_[i])) {
            abort = true;
            std::cerr << "order mismatch at index " << i << ". Actual:\n"
                      << read_orders[i] << "\nExpected:\n"
                      << active_orders_[i] << '\n';
        }
    }

    if (abort)
        throw std::runtime_error("order write/read mismatch");
}

/*
void JsonWriter::extract(const char* const file_name) {
    logExtracting(file_name);
    const std::vector<std::vector<Order>> docs = readOrders(file_name);

    // example: path/to/2016_12_01_00_25_13.tgz
    const auto [dirname, basename] = split_directory_and_basename(file_name);
    std::tm timeTm{};
    const auto result = strptime(basename.data(), "%Y_%m_%d_%H_%M_%S", &timeTm);
    if (result == nullptr)
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": strptime failed on '" + basename.data() + '\'');
    // safe until
    // $ date --date='@4294967295'
    // Sun Feb  7 07:28:15 CET 2106
    const auto viewtime = static_cast<std::uint32_t>(timegm(&timeTm));
    std::memcpy(current_, &viewtime, sizeof(viewtime));
    current_ += sizeof(viewtime);
    const auto doc_count = static_cast<std::int8_t>(docs.size());
    std::memcpy(current_, &doc_count, sizeof(doc_count));
    current_ += sizeof(doc_count);

    for (const auto& doc : docs) {
        if (doc.empty())
            continue;
        const auto first_id = id(doc.front());
        const auto last_id = id(doc.back());
        std::memcpy(current_, &first_id, sizeof(first_id));
        current_ += sizeof(first_id);
        std::memcpy(current_, &last_id, sizeof(last_id));
        current_ += sizeof(last_id);
        const auto header = current_;
        current_ += 4; // 4 bytes for size of doc in bytes as written to the file ... should be variably sized
        const auto snapshot = std::move(active_orders_);
        const auto r1 = std::lower_bound(snapshot.begin(), snapshot.end(), doc.front(),
                                         [](const Order& a, const Order& b) { return id(a).data < id(b).data; });
        const auto r2 = std::upper_bound(r1, snapshot.end(), doc.back(),
                                         [](const Order& a, const Order& b) { return id(a).data < id(b).data; });

        active_orders_.clear();
        active_orders_.reserve(static_cast<std::size_t>((r1 - snapshot.begin()) +
                                                        static_cast<std::int64_t>(doc.size()) + (snapshot.end() - r2)));
        active_orders_.insert(active_orders_.end(), snapshot.begin(), r1);
        auto current = active_orders_.insert(active_orders_.end(), doc.begin(), doc.end());
        const auto current_stop = active_orders_.insert(active_orders_.end(), r2, snapshot.end());

        auto last = r1;
        if (current != current_stop && last != r2) {
            while (true) {
                // const auto tmp = current_;
                if (id(*current).data < id(*last).data) {
                    // const auto index = static_cast<std::uint32_t>(current - active_orders_.begin());
                    // std::memcpy(current_, &index, 4);
                    // current_ += 4;

                    // std::cout << "new  " << id(*current).data << " (" << index << ")\n";
                    // std::cout << "new  " << id(*current).data << '\n';

                    write_order(current_, *current);
                    ++current;
                    if (current == current_stop)
                        break;
                } else if (id(*current).data == id(*last).data) {
                    if (price(*current) != price(*last) || volume(*current) != volume(*last)) {
                        // write_diff(current_, Diff{{2}, price(*current), volume(*current), id(*current)});

                        // const auto index = static_cast<std::uint32_t>(current - active_orders_.begin());
                        // std::memcpy(current_, &index, 4);
                        // current_ += 4;

                        // std::cout << "diff " << id(*current).data << '\n';

                        write_diff(current_, Diff{{2},
                                                  price(*current),
                                                  volume(*current),
                                                  id(*current),
                                                  static_cast<std::uint32_t>(current - active_orders_.begin())});
                    } else {
                        // order did not change since last view.
                    }
                    ++current;
                    ++last;
                    if (current == current_stop || last == r2)
                        break;
                } else {
                    // const auto index = static_cast<std::uint32_t>(current - active_orders_.begin());
                    // std::memcpy(current_, &index, 4);
                    // current_ += 4;

                    // std::cout << "del  " << id(*last).data << '\n';

                    write_diff(current_, Diff{{2},
                                              price(*last),
                                              {0},
                                              id(*last),
                                              static_cast<std::uint32_t>(current - active_orders_.begin())});
                    ++last;
                    if (last == r2)
                        break;
                }
                // check(tmp, current_, current - active_orders_.begin());
            }
        }

        for (; current != current_stop; ++current) {
            // const auto tmp = current_;
            // const auto index = static_cast<std::uint32_t>(current - active_orders_.begin());
            // std::memcpy(current_, &index, 4);
            // current_ += 4;

            // std::cout << "new  " << id(*current).data << '\n';

            write_order(current_, *current);
            // check(tmp, current_, current - active_orders_.begin());
        }

        for (; last != r2; ++last) {
            // const auto tmp = current_;
            // const auto index =
            //    static_cast<std::uint32_t>(current - active_orders_.begin()); // FIXME: constant throughout the loop
            // std::memcpy(current_, &index, 4);
            // current_ += 4;

            // std::cout << "del  " << id(*last).data << '\n';

            write_diff(
                current_,
                Diff{{2}, price(*last), {0}, id(*last), static_cast<std::uint32_t>(current - active_orders_.begin())});
            // check(tmp, current_, current - active_orders_.begin());
        }
        const auto size = static_cast<std::int32_t>(current_ - header - 4);
        std::memcpy(header, &size, sizeof(size));
    }
    if (reader_it_.reader_ == nullptr) {
        reader_it_ = reader_.begin();
    } else {
        ++reader_it_;
    }
    // here we sort orders and remove deleted ones (volume == 0) so that the reader has more freedom in it's
    // implementation. All clients can do this if they need to.
    const auto& read_orders = [this] {
        auto orders = reader_.orders();
        orders.erase(std::remove_if(orders.begin(), orders.end(), [](const Order& o) { return volume(o).data > 0; }),
                     orders.end());
        std::sort(orders.begin(), orders.end(), [](const Order& a, const Order& b) { return id(a) < id(b); });
        return orders;
    }();

    const auto order_count = std::min(read_orders.size(), active_orders_.size());
    if (read_orders.size() != active_orders_.size())
        std::cerr << "size mismatch. Actual: " << read_orders.size() << ", Expected: " << active_orders_.size() << '\n';
    bool abort = false;

    for (auto i = 0U; i < order_count; ++i) {
        if (!equal_ignoring_issued(read_orders[i], active_orders_[i])) {
            abort = true;
            std::cerr << "order mismatch at index " << i << ". Actual:\n"
                      << read_orders[i] << "\nExpected:\n"
                      << active_orders_[i] << '\n';
        }
    }

    if (abort)
        throw std::runtime_error("order write/read mismatch");
}
*/

void JsonWriter::extract2(const char* const file_name) {
    logExtracting(file_name);
    const std::vector<std::vector<Order>> docs = readOrders(file_name);

    // example: path/to/2016_12_01_00_25_13.tgz
    const auto [dirname, basename] = split_directory_and_basename(file_name);
    std::tm timeTm{};
    const auto result = strptime(basename.data(), "%Y_%m_%d_%H_%M_%S", &timeTm);
    if (result == nullptr)
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": strptime failed on '" + basename.data() + '\'');
    // safe until
    // $ date --date='@4294967295'
    // Sun Feb  7 07:28:15 CET 2106
    const auto viewtime = static_cast<std::uint32_t>(timegm(&timeTm));
    std::memcpy(current_, &viewtime, sizeof(viewtime));
    current_ += sizeof(viewtime);
    const auto doc_count = static_cast<std::int8_t>(docs.size());
    std::memcpy(current_, &doc_count, sizeof(doc_count));
    current_ += sizeof(doc_count);

    for (const auto& doc : docs) {
        if (doc.empty())
            continue;
        const auto first_id = id(doc.front());
        const auto last_id = id(doc.back());
        std::memcpy(current_, &first_id, sizeof(first_id));
        current_ += sizeof(first_id);
        std::memcpy(current_, &last_id, sizeof(last_id));
        current_ += sizeof(last_id);
        const auto header = current_;
        current_ += 4; // 4 bytes for size of doc in bytes as written to the file ... should be variably sized
        const auto snapshot = std::move(active_orders_);
        const auto r1 =
            std::lower_bound(snapshot.begin(), snapshot.end(), doc.front(), [](const Order& a, const Order& b) {
                return id(a).data < id(b).data;
            });
        const auto r2 = std::upper_bound(
            r1, snapshot.end(), doc.back(), [](const Order& a, const Order& b) { return id(a).data < id(b).data; });

        active_orders_.clear();
        active_orders_.reserve(static_cast<std::size_t>((r1 - snapshot.begin()) +
                                                        static_cast<std::int64_t>(doc.size()) + (snapshot.end() - r2)));
        active_orders_.insert(active_orders_.end(), snapshot.begin(), r1);
        auto current = active_orders_.insert(active_orders_.end(), doc.begin(), doc.end());
        const auto current_stop = active_orders_.insert(active_orders_.end(), r2, snapshot.end());

        auto last = r1;
        if (current != current_stop && last != r2) {
            while (true) {
                // const auto tmp = current_;
                if (id(*current).data < id(*last).data) {
                    // std::cout << "new  " << id(*current).data << " (" << index << ")\n";
                    // std::cout << "new  " << id(*current).data << '\n';

                    const auto it = index_.find(id(*current).data);
                    if (it != index_.end())
                        throw std::runtime_error("inconsistency");
                    const auto new_index = [this, current] {
                        if (free_list_.empty()) {
                            active_orders2_.push_back(*current);
                            return static_cast<std::int32_t>(active_orders2_.size());
                        } else {
                            // const auto i = free_list_.back();
                            // free_list_.pop_back();
                            const auto i = free_list_.front();
                            free_list_.pop_front();
                            active_orders2_[static_cast<std::size_t>(i)] = *current;
                            return i;
                        }
                    }();
                    index_.emplace(id(*current).data, new_index);

                    std::memcpy(current_, &new_index, 4);
                    current_ += 4;

                    write_order(current_, *current);
                    ++current;
                    if (current == current_stop)
                        break;
                } else if (id(*current).data == id(*last).data) {
                    if (price(*current) != price(*last) || volume(*current) != volume(*last)) {
                        // write_diff(current_, Diff{{2}, price(*current), volume(*current), id(*current)});

                        // std::cout << "diff " << id(*current).data << '\n';

                        const auto it = index_.find(id(*current).data);
                        if (it == index_.end())
                            throw std::runtime_error("inconsistency");
                        const auto existing_index = static_cast<std::size_t>(it->second);
                        active_orders2_[existing_index] = *current;

                        std::memcpy(current_, &existing_index, 4);
                        current_ += 4;

                        // write_diff(current_, Diff{{2}, price(*current), volume(*current), id(*current)});
                        write_diff(current_, Diff{{2}, price(*current), volume(*current)});
                    } else {
                        // order did not change since last view.
                    }
                    ++current;
                    ++last;
                    if (current == current_stop || last == r2)
                        break;
                } else {
                    // std::cout << "del  " << id(*last).data << '\n';

                    const auto it = index_.find(id(*last).data);
                    if (it == index_.end())
                        throw std::runtime_error("inconsistency");
                    const auto existing_index = static_cast<std::size_t>(it->second);
                    free_list_.push_back(it->second);
                    index_.erase(it);
                    volume(active_orders2_[existing_index]).data = 0;

                    std::memcpy(current_, &existing_index, 4);
                    current_ += 4;

                    write_diff(current_, Diff{{2}, price(*last), {0}});
                    // write_diff(current_, Diff{{2}, price(*last), {0}, id(*last)});
                    ++last;
                    if (last == r2)
                        break;
                }
                // check(tmp, current_, current - active_orders_.begin());
            }
        }

        for (; current != current_stop; ++current) {
            // const auto tmp = current_;

            // std::cout << "new  " << id(*current).data << '\n';

            const auto it = index_.find(id(*current).data);
            if (it != index_.end())
                throw std::runtime_error("inconsistency");
            const auto new_index = [this, current] {
                if (free_list_.empty()) {
                    active_orders2_.push_back(*current);
                    return static_cast<std::int32_t>(active_orders2_.size());
                } else {
                    // const auto i = free_list_.back();
                    // free_list_.pop_back();
                    const auto i = free_list_.front();
                    free_list_.pop_front();
                    active_orders2_[static_cast<std::size_t>(i)] = *current;
                    return i;
                }
            }();
            index_.emplace(id(*current).data, new_index);

            std::memcpy(current_, &new_index, 4);
            current_ += 4;

            write_order(current_, *current);
            // check(tmp, current_, current - active_orders_.begin());
        }

        for (; last != r2; ++last) {
            // const auto tmp = current_;

            // std::cout << "del  " << id(*last).data << '\n';

            const auto it = index_.find(id(*last).data);
            if (it == index_.end())
                throw std::runtime_error("inconsistency");
            const auto existing_index = static_cast<std::size_t>(it->second);
            free_list_.push_back(it->second);
            index_.erase(it);
            volume(active_orders2_[existing_index]).data = 0;

            std::memcpy(current_, &existing_index, 4);
            current_ += 4;

            write_diff(current_, Diff{{2}, price(*last), {0}});
            // write_diff(current_, Diff{{2}, price(*last), {0}, id(*last)});
            // check(tmp, current_, current - active_orders_.begin());
        }
        const auto size = static_cast<std::int32_t>(current_ - header - 4);
        std::memcpy(header, &size, sizeof(size));
    }
    if (reader_it_.reader_ == nullptr) {
        reader_it_ = reader_.begin();
    } else {
        ++reader_it_;
    }
    // here we sort orders and remove deleted ones (volume == 0) so that the reader has more freedom in it's
    // implementation. All clients can do this if they need to.
    const auto& read_orders = [this] {
        auto orders = reader_.orders();
        orders.erase(std::remove_if(orders.begin(), orders.end(), [](const Order& o) { return volume(o).data == 0; }),
                     orders.end());
        std::sort(orders.begin(), orders.end(), [](const Order& a, const Order& b) { return id(a) < id(b); });
        return orders;
    }();

    const auto order_count = std::min(read_orders.size(), active_orders_.size());
    if (read_orders.size() != active_orders_.size())
        std::cerr << "size mismatch. Actual: " << read_orders.size() << ", Expected: " << active_orders_.size() << '\n';
    bool abort = false;

    for (auto i = 0U; i < order_count; ++i) {
        if (!equal_ignoring_issued(read_orders[i], active_orders_[i])) {
            abort = true;
            std::cerr << "order mismatch at index " << i << ". Actual:\n"
                      << read_orders[i] << "\nExpected:\n"
                      << active_orders_[i] << '\n';
        }
    }

    if (abort)
        throw std::runtime_error("order write/read mismatch");
}

void JsonWriter::write(const char* const source_directory_path) {
    // TODO: Fix boost::path -> const char* -> std:string -> const char* ...
    std::vector<std::string> paths;
    paths.reserve(30000);
    directory_tree dir(source_directory_path);
    std::copy_if(dir.begin(), dir.end(), std::back_inserter(paths), [](const char* file_name) {
        // char* -> std::string_view
        return isTgz(file_name);
    });
    std::sort(paths.begin(), paths.end());
    for (const auto& file_name : paths) {
        extract2(file_name.c_str());
    }
}
