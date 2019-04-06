#include "read_json_order.h"

#include "parse_datetime.h"
#include <cstdint>
#include <stdexcept>

namespace {
//_range_to_int = {
//            '1': 1,
//            '2': 2,
//            '3': 3,
//            '4': 4,
//            '5': 5,
//            '10': 10,
//            '20': 20,
//            '30': 30,
//            '40': 40,
//            'region': -1,
//            'solarsystem': -2,
//            'station': -3
//    }

std::int8_t range_to_int8(const std::string_view x) {
    std::int8_t a;
    switch (x.size()) {
    case 1:
        a = static_cast<std::int8_t>(x[0] - '0');
        break;
    case 2:
        a = static_cast<std::int8_t>(10 * (x[0] - '0'));
        break;
    case 6:
        a = -1;
        break;
    case 7:
        a = -3;
        break;
    default:
        a = -2;
    }
    return a;
}
} // namespace

Order convert(const rapidjson::GenericDocument<rapidjson::ASCII<>>::ConstObject& v) {
    // 2
    //"id": 4020240384
    // 3
    //"buy": false,
    // 4
    //"type": 366699,
    // 5
    //"price": 3000,
    //"range": "region",
    // 6
    //"issued": "2016-04-02T01:40:43",
    //"volume": 833,
    // 8
    //"duration": 365,
    // 9
    //"stationID": 60006484,
    //"minVolume": 1,
    // 13
    //"volumeEntered": 833,
    Order order;
    std::uint64_t members_read = 0;
    for (const auto& member : v) {
        const auto name = std::string_view(member.name.GetString(), member.name.GetStringLength());
        switch (name.size()) {
        case 2:
            if (!member.value.IsUint64())
                throw std::runtime_error("!member.value.IsUint64()");
            id(order).data = member.value.GetUint64();
            members_read |= 1U << 0U;
            break;
        case 3:
            if (!member.value.IsBool())
                throw std::runtime_error("!member.value.IsBool()");
            buy(order).data = static_cast<std::int8_t>(member.value.GetBool());
            members_read |= 1U << 1U;
            break;
        case 4:
            if (!member.value.IsUint())
                throw std::runtime_error("!member.value.IsUint()");
            type(order).data = member.value.GetUint();
            members_read |= 1U << 2U;
            break;
        case 5:
            if (name[0] == 'p') {
                if (!member.value.IsDouble())
                    throw std::runtime_error("!member.value.IsDouble()");
                price(order).data = member.value.GetDouble();
                members_read |= 1U << 3U;
            } else {
                if (!member.value.IsString())
                    throw std::runtime_error("!member.value.IsString()");
                range(order).data =
                    range_to_int8(std::string_view(member.value.GetString(), member.value.GetStringLength()));
                members_read |= 1U << 4U;
            }
            break;
        case 6:
            if (name[0] == 'i') {
                if (!member.value.IsString())
                    throw std::runtime_error("!member.value.IsString()");
                issued(order).data =
                    parse_datetime(std::string_view(member.value.GetString(), member.value.GetStringLength()));
                members_read |= 1U << 5U;
            } else {
                if (!member.value.IsUint())
                    throw std::runtime_error("!member.value.IsUint()");
                volume(order).data = member.value.GetUint();
                members_read |= 1U << 6U;
            }
            break;
        case 8:
            if (!member.value.IsUint())
                throw std::runtime_error("!member.value.IsUint()");
            duration(order).data = member.value.GetUint();
            members_read |= 1U << 7U;
            break;
        case 9:
            if (name[0] == 's') {
                if (!member.value.IsUint64())
                    throw std::runtime_error("!member.value.IsUint64()");
                stationId(order).data = member.value.GetUint64();
                members_read |= 1U << 8U;
            } else {
                if (!member.value.IsUint())
                    throw std::runtime_error("!member.value.IsUint()");
                minVolume(order).data = member.value.GetUint();
                members_read |= 1U << 9U;
            }
            break;
        default:
            if (!member.value.IsUint())
                throw std::runtime_error("!member.value.IsUint()");
            volumeEntered(order).data = member.value.GetUint();
            members_read |= 1U << 10U;
        }
    }
    if (members_read != (1U << 11U) - 1) {
        throw std::runtime_error(std::string("missing some member: ") + std::to_string(members_read));
    }
    return order;
}
