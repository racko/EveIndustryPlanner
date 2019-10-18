add_library(production
  src/products.cpp
)

add_library(production::production ALIAS production)

target_include_directories(production PUBLIC include)

target_link_libraries(production
  PUBLIC
    type_id::type_id
    yaml_fwd::yaml_fwd
  PRIVATE
    names::names
    market_data::market_data
    resource_manager::resource_manager
    settings::settings
    skills::skills
    types::types
    yaml-cpp
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
