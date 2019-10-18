add_library(copying
  src/copies.cpp
)

add_library(copying::copying ALIAS copying)

target_include_directories(copying PUBLIC include)

target_link_libraries(copying
  PUBLIC
    type_id::type_id
    yaml_fwd::yaml_fwd
  PRIVATE
    market_data::market_data
    names::names
    resource_manager::resource_manager
    skills::skills
    yaml-cpp
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
