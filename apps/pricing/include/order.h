#pragma once

#include <cstdint>
#include <functional>
#include <ostream>

template <typename T>
struct Data {
    constexpr Data(T x) : data(std::move(x)) {}
    constexpr Data() = default;
    T data;
};

#define DefData(Name, Type)                                                                                            \
    struct Name : private Data<Type> {                                                                                 \
        using type = Type;                                                                                             \
        using Data<Type>::Data;                                                                                        \
        using Data<Type>::operator=;                                                                                   \
        using Data<Type>::data;                                                                                        \
    };                                                                                                                 \
    constexpr bool operator<(const Name lhs, const Name rhs) { return lhs.data < rhs.data; }                           \
    constexpr bool operator>(const Name lhs, const Name rhs) { return lhs.data > rhs.data; }                           \
    constexpr bool operator<=(const Name lhs, const Name rhs) { return lhs.data <= rhs.data; }                         \
    constexpr bool operator>=(const Name lhs, const Name rhs) { return lhs.data >= rhs.data; }                         \
    constexpr bool operator==(const Name lhs, const Name rhs) { return lhs.data == rhs.data; }                         \
    constexpr bool operator!=(const Name lhs, const Name rhs) { return !(lhs == rhs); }                                \
    namespace std {                                                                                                    \
    template <>                                                                                                        \
    struct hash<Name> {                                                                                                \
        std::size_t operator()(const Name x) const { return hash_(x.data); }                                           \
        std::hash<Name::type> hash_;                                                                                   \
    };                                                                                                                 \
    }                                                                                                                  \
    struct Name // swallow semicolon

DefData(Buy, std::int8_t);        // 1 (int8, 0 or 1)
DefData(Issued, std::uint32_t);   // 4 (uint32 timestamp)
DefData(Price, double);           // 8
DefData(VolumeEntered, std::uint32_t); // 4
DefData(StationId, std::uint64_t);     // 8: not written (Jita only)
DefData(Volume, std::uint32_t);        // 4
DefData(Range, std::int8_t);      // 1: see _range_to_int comment
DefData(MinVolume, std::uint32_t);     // 4
DefData(Duration, std::uint32_t);      // 4
DefData(Type, std::uint32_t);          // 4
DefData(Id, std::uint64_t);            // 8
//DefData(Index, std::uint32_t);
// 50 Bytes

// Static Data
//
// DefData(Id, std::uint64_t);             // 8
// DefData(Buy, bool);                // 1 (int8, 0 or 1)
// DefData(VolumeEntered, std::uint32_t);  // 4
// DefData(StationId, std::uint64_t);      // 0: not written (Jita only)
// DefData(Range, std::string_view);  // 1: see _range_to_int comment
// DefData(MinVolume, std::uint32_t);      // 4
// DefData(Duration, std::uint32_t);       // 4
// DefData(Type, std::uint32_t);           // 4
// 26 Bytes

// Dynamic Data
//
// DefData(Id, std::uint64_t);             // 8
// DefData(Issued, std::string_view); // 4 (uint32 timestamp) ... shouldn't write it ... don't care
// DefData(Price, double);            // 8
// DefData(Volume, std::uint32_t);         // 4
// 24 Bytes

DefData(ViewTime, std::uint32_t);
#undef DefData

using OrderImpl =
    std::tuple<Buy, Issued, Price, VolumeEntered, StationId, Volume, Range, MinVolume, Duration, Type, Id>;

struct Order : public OrderImpl {
    using Base = OrderImpl;
    using Base::Base;
    Order(const Base& t) : Base{t} {}
};

#define DefOrderAccessors(Name, accessor)                                                                              \
    inline Name& accessor(Order& o) { return std::get<Name>(o); }                                                      \
    inline const Name& accessor(const Order& o) { return std::get<Name>(o); }                                          \
    struct Name // only used to swallow the semicolons below. And I need to add semicolons so clang-format doesn't freak
                // out

DefOrderAccessors(Buy, buy);
DefOrderAccessors(Issued, issued);
DefOrderAccessors(Price, price);
DefOrderAccessors(VolumeEntered, volumeEntered);
DefOrderAccessors(StationId, stationId);
DefOrderAccessors(Volume, volume);
DefOrderAccessors(Range, range);
DefOrderAccessors(MinVolume, minVolume);
DefOrderAccessors(Duration, duration);
DefOrderAccessors(Type, type);
DefOrderAccessors(Id, id);
#undef DefOrderAccessors

// we use "Buy" as disambiguation between "full" and "diff": Buy == 0 => Sell, Buy == 1 => Buy, Buy = 2 => Diff
//using DiffImpl = std::tuple<Buy, Price, Volume, Id>;
using DiffImpl = std::tuple<Buy, Price, Volume>;

struct Diff : public DiffImpl {
    using Base = DiffImpl;
    using Base::Base;
    Diff(const Base& t) : Base{t} {}
};

#define DefDiffAccessors(Name, accessor)                                                                               \
    inline Name& accessor(Diff& o) { return std::get<Name>(o); }                                                       \
    inline const Name& accessor(const Diff& o) { return std::get<Name>(o); }                                           \
    struct Name // only used to swallow the semicolons below. And I need to add semicolons so clang-format doesn't freak
                // out

DefDiffAccessors(Buy, buy);
DefDiffAccessors(Price, price);
DefDiffAccessors(Volume, volume);
//DefDiffAccessors(Id, id);
//DefDiffAccessors(Index, index);
#undef DefDiffAccessors

std::ostream& operator<<(std::ostream& s, const Order& o);
