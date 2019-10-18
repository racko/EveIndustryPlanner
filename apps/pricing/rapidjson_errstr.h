#pragma once

#include <ostream>
#include <rapidjson/error/error.h>
#include <string_view>

void errstr(std::ostream& s, rapidjson::ParseResult status, const std::string_view text);
