add_library(research
  src/inventions.cpp
)

add_library(research::research ALIAS research)

target_include_directories(research PUBLIC include)

target_link_libraries(research
  PUBLIC
    type_id::type_id
    yaml_fwd::yaml_fwd
  PRIVATE
    skills::skills
    yaml-cpp
    resource_manager::resource_manager
    names::names
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
