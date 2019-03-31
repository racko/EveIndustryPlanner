#pragma once

#include "tech_level_fwd.h"

#include <string>

enum class TechLevel : unsigned {
    T1 = 1,
    T2,
    T3
};

inline const char* to_c_string(TechLevel l) {
    constexpr const char* s[] = {"T1", "T2", "T3"};
    return s[unsigned(l)-1u];
}

inline std::string to_string(TechLevel l) {
    return std::string{to_c_string(l)};
}
