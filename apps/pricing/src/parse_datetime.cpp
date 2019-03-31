#include "parse_datetime.h"
#include <ctime>
#include <stdexcept>

std::uint32_t parse_datetime(const std::string_view x) {
    std::tm timeTm{};
    const auto result = strptime(x.data(), "%FT%T", &timeTm);
    if (result == nullptr)
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + ": strptime failed");
    // safe until
    // $ date --date='@4294967295'
    // Sun Feb  7 07:28:15 CET 2106
    return static_cast<std::uint32_t>(timegm(&timeTm));
}
