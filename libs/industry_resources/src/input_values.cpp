#include "input_values.h"

#include "production.h"

#include <cmath>

namespace {
// TODO: this function still depends on the Production class (via the typedef)
template <typename Cont>
void inputValue(const Production::materials_t& materials,
                const std::unordered_map<std::size_t, double>& adjPrices,
                Cont c) {
    auto iv = 0.0;
    for (const auto& m : materials) {
        if (const auto it = adjPrices.find(m.first); it != adjPrices.end()) {
            iv += double(m.second) * it->second;
        } else {
            return;
        }
    }
    c(iv);
}
} // namespace

std::unordered_map<std::size_t, double> inputValues(const std::vector<Production>& products,
                                                    const std::unordered_map<std::size_t, double>& adjPrices) {
    std::unordered_map<std::size_t, double> iv(products.size());
    for (const auto& p : products) {
        inputValue(p.getMaterials(), adjPrices, [&](double v) { iv[p.getProduct()] = v; });
    }
    return iv;
}

//#include <unordered_map>
//
// class InputValues;
//
// InputValues inputValues(const std::vector<Production>& products, const std::unordered_map<std::size_t, double>&
// adjPrices);
//
// std::unordered_map<std::size_t,double>::const_iterator find(const std::unordered_map<std::size_t,double>& adjPrices,
// std::size_t material_id);
//
// class InputValues {
// public:
//    InputValues(std::size_t product_count);
//    ~InputValues();
//    void insert(std::size_t product_id, double input_value);
// private:
//    std::unordered_map<std::size_t,double> input_values_;
//};
