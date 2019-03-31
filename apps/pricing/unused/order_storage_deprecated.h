#include <unordered_set>

struct hash_order {
    std::uint64_t operator()(const Order& order) const { return hash_(id(order).data); }
    std::hash<std::uint64_t> hash_;
};

struct equal_order {
    bool operator()(const Order& lhs, const Order& rhs) const { return id(lhs).data == id(rhs).data; }
};

struct OrderData {
    Id id;
    Type type;
    Buy buy;
    constexpr OrderData(const Id i, const Type t, const Buy b) : id{i}, type{t}, buy{b} {}
};
[[maybe_unused]] constexpr bool operator==(const OrderData lhs, const OrderData rhs) { return lhs.id == rhs.id; }
} // namespace

namespace std {
template <>
struct hash<OrderData> {
    std::size_t operator()(const OrderData o) const { return hash_(o.id.data); }
    std::hash<std::uint64_t> hash_;
};
} // namespace std

namespace {

using OrderSetBase = std::unordered_set<Order, hash_order, equal_order>;
class OrderSet : private OrderSetBase {
  public:
    using OrderSetBase::insert;
    bool contains(const Order& o) const { return OrderSetBase::find(o) != end(); }

    const Order* find(const Order& o) const {
        auto it = OrderSetBase::find(o);
        return it != end() ? &*it : nullptr;
    }

    using OrderSetBase::begin;
    using OrderSetBase::end;
};

using OrderSetVectorBase = std::vector<OrderSet>;
class OrderSetVector : private OrderSetVectorBase {
  public:
    OrderSet& operator[](const Type t) {
        const auto i = t.data;
        ensureSize(i);
        return OrderSetVectorBase::operator[](i);
    }

    const OrderSet& operator[](const Type t) const {
        const auto i = t.data;
        if (size() <= i) {
            throw std::range_error{std::to_string(size()) + " <= " + std::to_string(i)};
        }
        return OrderSetVectorBase::operator[](i);
    }

    void insert(const Order& o) { [[maybe_unused]] const auto [it, new_order] = operator[](type(o)).insert(o); }

    bool contains(const Order& o) const { return operator[](type(o)).contains(o); }

    const Order* find(const Order& o) const { return type(o).data >= size() ? nullptr : operator[](type(o)).find(o); }

    template <typename Functor>
    void for_each(Functor f) const {
        for (const auto& x : *this) {
            for (const Order& o : x) {
                f(o);
            }
        }
    }

    using OrderSetVectorBase::begin;
    using OrderSetVectorBase::clear;
    using OrderSetVectorBase::end;

  private:
    void ensureSize(const Type::type i) {
        if (size() <= i) {
            reserve(2 * i);
            resize(i + 1);
        }
    }
};

using BuyAndSellOrdersBase = std::array<OrderSetVector, 2>;
class BuyAndSellOrders : private BuyAndSellOrdersBase {
  public:
    OrderSetVector& operator[](const Buy t) {
        return BuyAndSellOrdersBase::operator[](static_cast<std::size_t>(t.data));
    }

    const OrderSetVector& operator[](const Buy t) const {
        return BuyAndSellOrdersBase::operator[](static_cast<std::size_t>(t.data));
    }

    void insert(const Order& o) { operator[](buy(o)).insert(o); }
    bool contains(const Order& o) const { return operator[](buy(o)).contains(o); }

    void clear() {
        BuyAndSellOrdersBase::operator[](0).clear();
        BuyAndSellOrdersBase::operator[](1).clear();
    }

    const Order* find(const Order& o) const { return operator[](buy(o)).find(o); }

    template <typename Functor>
    void for_each(Functor f) const {
        BuyAndSellOrdersBase::operator[](0).for_each(f);
        BuyAndSellOrdersBase::operator[](1).for_each(f);
    }

    using BuyAndSellOrdersBase::begin;
    using BuyAndSellOrdersBase::end;
};
