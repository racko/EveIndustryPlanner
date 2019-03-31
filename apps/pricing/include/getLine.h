#include <string_view>
#include <cstdint>

//std::string_view getLine(std::string_view s, std::size_t offset);

// simply returns a number of characters around the offset, ignoring newlines
std::string_view getLine_no_newline(std::string_view s, std::int64_t offset);
