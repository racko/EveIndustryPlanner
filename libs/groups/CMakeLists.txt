add_library(groups
  src/groups.cpp
)

add_library(groups::groups ALIAS groups)

target_include_directories(groups PUBLIC include)

target_link_libraries(groups
  PUBLIC
    yaml_fwd::yaml_fwd
  PRIVATE
    Boost::serialization
    profiling::profiling
    yaml-cpp
)
