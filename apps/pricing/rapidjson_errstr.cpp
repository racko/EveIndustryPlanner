#include "apps/pricing/rapidjson_errstr.h"

#include "apps/pricing/getLine.h"
#include <algorithm>
#include <boost/numeric/conversion/cast.hpp>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <rapidjson/error/en.h>

void errstr(std::ostream& s, rapidjson::ParseResult status, const std::string_view text) {
    s << "Parse Error: " << rapidjson::GetParseError_En(status.Code()) << " (offset " << status.Offset() << ')' << '\n';
    auto zero_based_index = boost::numeric_cast<std::int64_t>(status.Offset()) - 1;
    if (zero_based_index < 0 || zero_based_index >= boost::numeric_cast<std::int64_t>(text.size())) {
        s << "Offset is invalid. Full text: \n";
        s << text << '\n';
        return;
    }
    const auto line = getLine_no_newline(text, zero_based_index);
    s << line << '\n';
    std::fill_n(std::ostream_iterator<char>(s), zero_based_index - (line.data() - text.data()), ' ');
    s << '^' << '\n';
}
