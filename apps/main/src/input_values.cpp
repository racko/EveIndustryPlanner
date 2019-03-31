#include "input_values.h"

#include "production.h"

#include <cmath>

namespace {
// TODO: this function still depends on the Production class (via the typedef)
template<typename Cont>
void inputValue(const Production::materials_t& materials, const std::unordered_map<std::size_t, double>& adjPrices, Cont c) {
    auto inputValue = 0.0;
    for (const auto& m : materials) {
        if (auto it = adjPrices.find(m.first); it != adjPrices.end()) {
            inputValue += double(m.second) * it->second;
        } else {
            return;
        }
    }
    c(inputValue);
}
}

std::unordered_map<std::size_t,double> inputValues(const std::vector<Production>& products, const std::unordered_map<std::size_t, double>& adjPrices) {
    std::unordered_map<std::size_t,double> inputValues(products.size());
    for (const auto& p : products) {
        inputValue(p.getMaterials(), adjPrices, [&]  (double v) { inputValues[p.getProduct()] = v; });
    }
    return inputValues;
}

//#include <unordered_map>
//
//class InputValues;
//
//InputValues inputValues(const std::vector<Production>& products, const std::unordered_map<std::size_t, double>& adjPrices);
//
//std::unordered_map<std::size_t,double>::const_iterator find(const std::unordered_map<std::size_t,double>& adjPrices, std::size_t material_id);
//
//class InputValues {
//public:
//    InputValues(std::size_t product_count);
//    ~InputValues();
//    void insert(std::size_t product_id, double input_value);
//private:
//    std::unordered_map<std::size_t,double> input_values_;
//};
//
//namespace {
//
//// TODO: this function still depends on the Production class (via the typedef)
//template<typename Cont>
//void inputValue(const Production::materials_t& materials, const std::unordered_map<std::size_t, double>& adjPrices, Cont c) {
//    auto inputValue = 0.0;
//    for (const auto& m : materials) {
//        auto mId = m.first;
//        auto quantity = m.second;
//        auto it = find(adjPrices, mId);
//        if (it == adjPrices.end()) {
//            return;
//        }
//        inputValue += double(quantity) * it->second;
//    }
//    c(inputValue);
//}
//}
//
//InputValues inputValues(const std::vector<Production>& products, const std::unordered_map<std::size_t, double>& adjPrices) {
//    InputValues inputValues(products.size());
//    for (const auto& p : products) {
//        inputValue(p.getMaterials(), adjPrices, [&](double v) {inputValues.insert(p.getProduct(), v); });
//    }
//    return inputValues;
//}
