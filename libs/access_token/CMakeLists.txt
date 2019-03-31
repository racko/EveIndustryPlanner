add_library(access_token src/access_token.cpp)
target_include_directories(access_token PUBLIC include)
target_link_libraries(access_token PRIVATE
    HTTPSRequestHandler
    Boost::regex
)
