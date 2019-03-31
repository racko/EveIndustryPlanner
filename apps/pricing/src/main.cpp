#include "json_reader.h"
#include "json_writer.h"
//#include <Eigen/Core>
#include <boost/filesystem.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <random>

namespace {
namespace bf = boost::filesystem;
using boost::filesystem::resize_file;

[[maybe_unused]] void write_orders(const char* source_directory_path, const char* destination_file_name) {
    // create file
    { std::ofstream f{destination_file_name}; }
    // preallocate 8GB. We will truncate later.
    resize_file(destination_file_name, 1ULL << 33U);
    // resize_file(destination_file_name, 8589934592);

    JsonWriter writer(destination_file_name);
    writer.write(source_directory_path);
}

struct TritaniumBuyAnalysis {

    TritaniumBuyAnalysis() {
        new_order_counts.precision(16);
        total_new_order_volumes.precision(16);
        new_order_volumes.precision(16);
        new_order_prices.precision(16);
        changed_order_prices.precision(16);
        sold_volumes.precision(16);
        total_volumes.precision(16);
        min_sells.precision(16);
        max_buys.precision(16);
    }
    void operator()(const std::uint32_t viewtime, const std::vector<Order>& view) {
        std::vector<Order> x;
        x.reserve(view.size());
        auto want_buy = 0;
        // std::copy_if(view.begin(), view.end(), std::back_inserter(x), [](const Order& o) {
        //    return buy(o).data == 1 && volume(o).data > 0 && type(o).data == 34 && stationId(o).data == 60003760;
        //});
        // 1248: Capacitor Flux Coil II
        std::copy_if(view.begin(), view.end(), std::back_inserter(x), [want_buy](const Order& o) {
            return buy(o).data == want_buy && volume(o).data > 0 && type(o).data == 1248 &&
                   stationId(o).data == 60003760;
        });
        std::sort(x.begin(), x.end(), [](const Order& a, const Order& b) { return id(a) < id(b); });
        stepAndUpdate(static_cast<std::int32_t>(viewtime), std::move(x), want_buy);
    }

    void stepAndUpdate(const std::int32_t viewtime, std::vector<Order> view, int want_buy) {
        if (!last_orders.empty())
            step(viewtime, view, want_buy);
        last_viewtime = viewtime;
        last_orders = std::move(view);
    }

    //    void step2(const std::int32_t viewtime, const std::vector<Order>& view) const {
    //        if (std::fabs(double(viewtime - last_viewtime) - 1800.0) > 120.0) {
    //            return;
    //        }
    //        struct Diff {
    //            enum class type { PRICE_CHANGE, SOLD, CLEARED, NEW, UNCHANGED };
    //            Order before;
    //            Order after;
    //            type t;
    //            Diff(type tt, const Order& oo, const Order& nn) : before{oo}, after{nn}, t{tt} {}
    //        };
    //
    //        std::vector<Diff> diffs;
    //        auto a = last_orders.cbegin(), b = view.cbegin();
    //        while (a != last_orders.cend() && b != view.cend()) {
    //            if (id(*a) == id(*b)) {
    //                if (price(*a) != price(*b)) {
    //                    //diffs.emplace_back(Diff::type::PRICE_CHANGE, *a, *b);
    //                    // diffs.emplace_back(Diff::type::NEW, *b);
    //                } else if (volume(*a) != volume(*b)) {
    //                    diffs.emplace_back(Diff::type::SOLD, *a, *b);
    //                    // diffs.emplace_back(Diff::type::NEW, *b);
    //                } else {
    //                    diffs.emplace_back(Diff::type::UNCHANGED, *a, *a);
    //                }
    //                ++a;
    //                ++b;
    //            } else if (id(*a) < id(*b)) { // a has been deleted
    //                diffs.emplace_back(Diff::type::CLEARED, *a, *a);
    //                ++a;
    //            } else { // b is new
    //                //diffs.emplace_back(Diff::type::NEW, *b, *b);
    //                ++b;
    //            }
    //        }
    //        for (; a != last_orders.cend(); ++a) {
    //            // a has been deleted
    //            diffs.emplace_back(Diff::type::CLEARED, *a, *a);
    //        }
    //        //for (; b != view.cend(); ++b) {
    //        //    // b is new
    //        //    diffs.emplace_back(Diff::type::NEW, *b, *b);
    //        //}
    //        std::sort(diffs.begin(), diffs.end(),
    //                  [](const Diff& lhs, const Diff& rhs) { return price(lhs.before) > price(rhs.before); });
    //
    //        for (const auto& d : diffs) {
    //            if (d.t != Diff::type::SOLD && d.t != Diff::type::CLEARED)
    //                break;
    //
    //        }
    //    }

    void step(const std::int32_t viewtime, const std::vector<Order>& view, int want_buy) {
        std::cout << "#-" << last_viewtime << '\n';
        std::cout << "#+" << viewtime << '\n';
        if (std::fabs(double(viewtime - last_viewtime) - 1800.0) > 120.0) {
            std::cout << "# views aren't approximately 30 minutes apart\n";
            return;
        }

        assert(!view.empty());

        struct Diff {
            enum class type { NEW, OLD, UNCHANGED };
            Order o;
            type t;
            const char* note;
            Diff(type tt, const Order& oo, const char* n) : o{oo}, t{tt}, note{n} {}

            static char typechar(type t) {
                switch (t) {
                case type::NEW:
                    return '+';
                case type::OLD:
                    return '-';
                case type::UNCHANGED:
                    return ' ';
                default:
                    throw std::runtime_error("invalid type");
                }
            }
        };

        const char* price_increase_and_volume_change = " // price increase and volume change\n";
        const char* price_decrease_and_volume_change = " // price decrease and volume change\n";
        const char* price_increase = " // price increase\n";
        const char* price_decrease = " // price decrease\n";
        const char* volume_change = " // volume change\n";
        const char* new_order = " // new\n";
        const char* cleared_order = " // cleared\n";
        const char* unchanged_order = "\n";
        std::vector<Diff> diffs;
        std::vector<std::tuple<double, double, int>> price_changes;
        std::vector<double> new_prices;
        const auto minmax{std::minmax_element(last_orders.begin(), last_orders.end(),
                                              [](const Order& a, const Order& b) { return price(a) < price(b); })};
        // we don't know if we are looking at sell or buy orders ...
        const auto max_buy = price(*minmax.second).data;
        const auto min_sell = price(*minmax.first).data;
        auto sold_volume{0LL};
        auto new_count{0LL};
        auto new_sum{0LL};
        auto a = last_orders.cbegin(), b = view.cbegin();
        while (a != last_orders.cend() && b != view.cend()) {
            if (id(*a) == id(*b)) {
                sold_volume += volume(*a).data - volume(*b).data;
                price_changes.emplace_back(price(*a).data, price(*b).data, range(*a).data);
                if (price(*b) > price(*a) && volume(*a) != volume(*b)) {
                    diffs.emplace_back(Diff::type::OLD, *a, price_increase_and_volume_change);
                    diffs.emplace_back(Diff::type::NEW, *b, price_increase_and_volume_change);
                } else if (price(*b) < price(*a) && volume(*a) != volume(*b)) {
                    diffs.emplace_back(Diff::type::OLD, *a, price_decrease_and_volume_change);
                    diffs.emplace_back(Diff::type::NEW, *b, price_decrease_and_volume_change);
                } else if (price(*b) > price(*a)) {
                    diffs.emplace_back(Diff::type::OLD, *a, price_increase);
                    diffs.emplace_back(Diff::type::NEW, *b, price_increase);
                } else if (price(*b) < price(*a)) {
                    diffs.emplace_back(Diff::type::OLD, *a, price_decrease);
                    diffs.emplace_back(Diff::type::NEW, *b, price_decrease);
                } else if (volume(*a) != volume(*b)) {
                    diffs.emplace_back(Diff::type::OLD, *a, volume_change);
                    diffs.emplace_back(Diff::type::NEW, *b, volume_change);
                } else {
                    diffs.emplace_back(Diff::type::UNCHANGED, *a, unchanged_order);
                }
                ++a;
                ++b;
            } else if (id(*a) < id(*b)) { // a has been deleted
                diffs.emplace_back(Diff::type::OLD, *a, cleared_order);
                sold_volume += volume(*a).data;
                ++a;
            } else { // b is new
                diffs.emplace_back(Diff::type::NEW, *b, new_order);
                ++new_count;
                new_sum += volume(*b).data;
                new_order_volumes << viewtime << ',' << volume(*b).data << '\n';
                new_prices.push_back(price(*b).data);

                ++b;
            }
        }
        for (; a != last_orders.cend(); ++a) {
            // a has been deleted
            diffs.emplace_back(Diff::type::OLD, *a, cleared_order);
            sold_volume += volume(*a).data;
        }
        for (; b != view.cend(); ++b) {
            // b is new
            diffs.emplace_back(Diff::type::NEW, *b, new_order);
            ++new_count;
            new_sum += volume(*b).data;
            new_order_volumes << viewtime << ',' << volume(*b).data << '\n';
            new_prices.push_back(price(*b).data);
        }
        total_volumes << viewtime << ','
                      << std::accumulate(view.begin(), view.end(), 0LL,
                                         [](std::int64_t acc, const Order& o) { return acc + volume(o).data; })
                      << '\n';
        sold_volumes << viewtime << ',' << sold_volume << '\n';
        total_new_order_volumes << viewtime << ',' << new_sum << '\n';
        new_order_counts << viewtime << ',' << new_count << '\n';
        min_sells << viewtime << ',' << min_sell << '\n';
        max_buys << viewtime << ',' << max_buy << '\n';
        // stable_sort: old before new
        std::stable_sort(diffs.begin(), diffs.end(),
                         [](const Diff& lhs, const Diff& rhs) { return price(lhs.o) < price(rhs.o); });
        for (const auto& d : diffs) {
            const auto& o = d.o;
            std::cout << Diff::typechar(d.t) << ' ' << id(o).data << ' ' << price(o).data << ' ' << volume(o).data
                      << ' ' << stationId(o).data << ' ' << int(range(o).data) << d.note;
        }
        std::cout << '\n';

        for (const auto& p : price_changes) {
            changed_order_prices << viewtime << ',' << (want_buy ? max_buy : min_sell) << ',' << std::get<0>(p) << ','
                                 << std::get<1>(p) << ',' << std::get<2>(p) << '\n';
        }

        for (const auto& p : new_prices) {
            new_order_prices << viewtime << ',' << (want_buy ? max_buy : min_sell) << ',' << p << '\n';
        }
    }

    std::ofstream new_order_counts{"/tmp/new_order_counts.csv"};
    std::ofstream new_order_volumes{"/tmp/new_order_volumes.csv"};
    std::ofstream total_new_order_volumes{"/tmp/total_new_order_volumes.csv"};
    std::ofstream new_order_prices{"/tmp/new_order_prices.csv"};
    std::ofstream changed_order_prices{"/tmp/changed_order_prices.csv"};
    std::ofstream sold_volumes{"/tmp/sold_volumes.csv"};
    std::ofstream total_volumes{"/tmp/total_volumes.csv"};
    std::ofstream min_sells{"/tmp/min_sell.csv"};
    std::ofstream max_buys{"/tmp/max_buy.csv"};
    std::int32_t last_viewtime;
    std::vector<Order> last_orders;
};

template <typename Key, typename Value>
class map {
  public:
    std::vector<std::pair<Key, Value>> values;
    std::vector<std::int32_t> slots;
};

using MinimumPrices = map<std::int32_t, double>;

[[maybe_unused]] MinimumPrices collect_minimum_prices(const std::vector<Order>& view) {
    MinimumPrices x;
    // x.values.reserve(20000);
    // x.slots.resize(50000, -1);
    x.values.reserve(1);
    x.slots.resize(35, -1);
    for (const Order& o : view) {
        if (buy(o).data == 1 || volume(o).data == 0 || type(o).data != 34) {
            continue;
        }
        const auto t = type(o).data;
        if (t >= x.slots.size()) {
            x.slots.resize(t + 1, -1);
        }
        auto& s = x.slots[t];
        if (s == -1) {
            s = static_cast<std::int32_t>(x.values.size());
            x.values.emplace_back(t, price(o).data);
        } else {
            auto& current_price = x.values[static_cast<std::size_t>(s)].second;
            current_price = std::min(price(o).data, current_price);
        }
    }
    x.values.shrink_to_fit();
    // std::cerr << "final count: " << x.values.size() << '\n';
    // std::cerr << "slots.size(): " << x.slots.size() << '\n';
    return x;
}

[[maybe_unused]] void analyze(std::vector<MinimumPrices> minimum_prices) {
    std::vector<double> samples;
    samples.reserve(minimum_prices.size() - 1);
    if (minimum_prices.size() < 2) {
        std::cerr << "Nothing to predict\n";
        return;
    }
    std::ofstream tritanium_prices{"/tmp/tritanium.csv"};
    for (auto a = minimum_prices.begin(), b = std::next(minimum_prices.begin()); b != minimum_prices.end(); ++a, ++b) {
        const auto a_slot = a->slots[34];
        const auto b_slot = b->slots[34];
        if (a_slot == -1 && b_slot == -1) {
            std::cerr << "Missing tritanium price\n";
            continue;
        }
        const auto a_price = a->values[static_cast<std::size_t>(a_slot)].second;
        const auto b_price = b->values[static_cast<std::size_t>(b_slot)].second;
        tritanium_prices << a_price << '\n';
        // std::cout << "a_price = " << a_price << ", b_price = " << b_price << ", diff = " << (b_price - a_price) <<
        // '\n';
        samples.push_back(b_price - a_price);
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(samples.begin(), samples.end(), gen);
    constexpr auto k = 2;
    const auto len = static_cast<std::int64_t>(samples.size()) / k;
    for (auto i = 0; i < k; ++i) {
        const auto train_begin = samples.begin() + i * len;
        const auto train_end = samples.begin() + (i + 1) * len;

        double var{};
        std::int32_t zero_count{};
        std::int32_t nonzero_count{};

        for (auto p = train_begin; p != train_end; ++p) {
            if (*p == 0) {
                ++zero_count;
                continue;
            }
            var += (*p * *p - var) / ++nonzero_count;
        }
        const auto log_zero_pct = std::log(double(zero_count) / double(zero_count + nonzero_count));
        const auto log_nonzero_pct = std::log(double(nonzero_count) / double(zero_count + nonzero_count));

        std::cerr << "zeros: " << zero_count << ", non-zeros: " << nonzero_count
                  << ", zero_pct = " << (double(zero_count) / double(zero_count + nonzero_count)) << ", var = " << var
                  << '\n';

        const auto log_normalization = std::log(2 * M_PI * var);
        const auto c = 0.5 / var;
        auto accum_likelihood = [log_zero_pct, log_nonzero_pct, log_normalization, c](const double acc,
                                                                                      const double price_diff) {
            return acc + (price_diff == 0 ? -log_zero_pct
                                          : -log_nonzero_pct + log_normalization + c * price_diff * price_diff);
        };
        const auto L = std::accumulate(samples.begin(), train_begin, 0.0, accum_likelihood) +
                       std::accumulate(train_end, samples.end(), 0.0, accum_likelihood);

        std::cerr << "L(" << i << ") = " << L << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3)
        return 1;

    // write_orders(argv[1], argv[2]);

    // read_orders_benchmark(argv[2]);

    // read_orders(argv[2], [](const std::vector<Order>&) {});

    // std::vector<MinimumPrices> minimum_prices;
    // minimum_prices.reserve(48 * 2 * 365);
    // read_orders(argv[2], [&minimum_prices](const std::vector<Order>& view) {
    //    minimum_prices.push_back(collect_minimum_prices(view));
    //});
    // analyze(std::move(minimum_prices));

    TritaniumBuyAnalysis analysis;
    read_orders(argv[2], [&analysis](const std::uint32_t viewtime, const std::vector<Order>& view) {
        analysis(viewtime, view);
    });
    return 0;
}
