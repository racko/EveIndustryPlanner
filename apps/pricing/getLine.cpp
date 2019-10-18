#include "apps/pricing/getLine.h"

#include <boost/numeric/conversion/cast.hpp>
#include <stdexcept>

// FIXME: the loop is inefficient due to the unpredictable conditions (it alternates between decreasing start and
// increasing stop) ... duplication to the rescue ...
//
// Also we need to deal with s[start] "usually" being '\n', but simply doing ++start forces us to check again if it is
// even valid ...
// std::string_view getLine(std::string_view s, std::size_t offset) {
//    // s.empty() => s[offset] yields undefined behavior
//    // s[offset] == '\n' => ... what would we like to happen? Print the two lines surrounding the '\n'? Print the one
//    // starting or the one ending with '\n'?
//    // Also we don't actually consider these use cases. I.e. we will not parse empty inputs and rapidjson should not
//    // return an offset pointing to '\n'.
//    if (s.empty() || s[offset] == '\n')
//        throw std::runtime_error("");
//
//    const auto p = s.begin() + offset;
//    auto start = p;
//    auto stop = p;
//    // search simultaneously in both directions for '\n' or start/end or a total of 120 characters
//    while (true) {
//        if (stop - start == 120 || ((start == s.begin() || *start == '\n') && (stop == s.end() || *stop == '\n')))
//            break;
//        if (start != s.begin() && *start != '\n' && p - start < stop - p) {
//            --start;
//            continue;
//        }
//        if (stop != s.end() && *stop != '\n') {
//            ++stop;
//            continue;
//        }
//    }
//
//    const auto length = static_cast<std::size_t>(stop - start);
//    return std::string_view(start, length);
//}

// std::string_view getLine(const std::string_view s, const std::size_t offset) {
//    // s.empty() => s[offset] yields undefined behavior
//    // s[offset] == '\n' => ... what would we like to happen? Print the two lines surrounding the '\n'? Print the one
//    // starting or the one ending with '\n'?
//    // Also we don't actually consider these use cases. I.e. we will not parse empty inputs and rapidjson should not
//    // return an offset pointing to '\n'.
//    if (s.empty() || s[offset] == '\n')
//        throw std::runtime_error("");
//
//    auto start = s.rfind('\n', offset);
//    if (start == std::string_view::npos)
//        start = 0;
//    else
//        ++start; // start != std::string_view::npos) => s[start] == '\n' ... thus std::string_view(s.begin() + start,
//                 // ...) would start with '\n'
//    auto stop = s.find('\n', offset);
//    if (stop == std::string_view::npos)
//        stop = s.size();
//
//    // s[offset] == '\n' => start == offset + 1 && stop == offset => stop < start
//    if (stop < start)
//        return std::string_view();
//
//    const auto length = stop - start;
//    return std::string_view(s.begin() + start, length);
//}

std::string_view getLine_no_newline(const std::string_view s, const std::int64_t offset) {
    const auto size = boost::numeric_cast<std::int64_t>(s.size());
    if (size < 120)
        return s;
    const auto a = 60 <= offset;
    const auto b = offset <= size - 60;
    if (a && b)
        return s.substr(static_cast<std::size_t>(offset - 60), 120);
    if (!a)
        return s.substr(0, 120);
    return s.substr(s.size() - 120, 120);
}
