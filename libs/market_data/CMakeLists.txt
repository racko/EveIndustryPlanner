add_library(market_data
    src/market_data.cpp
    src/order.cpp
)

add_library(market_data::market_data ALIAS market_data)

target_include_directories(market_data
  PUBLIC
    include
)

target_link_libraries(market_data
  PUBLIC
    sqlite-util::sqlite-util
    type_id::type_id
  PRIVATE
    Boost::serialization
    names::names
    profiling::profiling
    resource_manager::resource_manager
    types::types
)
