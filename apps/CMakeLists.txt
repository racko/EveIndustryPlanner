add_subdirectory(main)
add_subdirectory(pricing)
add_subdirectory(price_reader)

add_executable(history_reader history_reader.cpp)
target_link_libraries(history_reader
    eve_stuff
    QueryRunner
    HTTPSRequestHandler
    Semaphore
    access_token
    Finalizer
    jsoncpp::jsoncpp
)

add_executable(order_reader order_reader.cpp)
target_link_libraries(order_reader
    eve_stuff
    Finalizer
    HTTPSRequestHandler
    QueryRunner
    Semaphore
    jsoncpp::jsoncpp
    profiling::profiling
)

add_executable(server server.cpp)
target_link_libraries(server
    Semaphore
    QueryRunner
    Poco::Net
    jsoncpp::jsoncpp
)

add_executable(station_reader station_reader.cpp)
target_link_libraries(station_reader
    QueryRunner
    profiling::profiling
    HTTPSRequestHandler
    yaml-cpp
)
