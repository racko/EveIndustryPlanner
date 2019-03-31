add_executable(price_reader price_reader.cpp)

target_link_libraries(price_reader
    QueryRunner
    profiling
    HTTPSRequestHandler
    jsoncpp::jsoncpp
)
