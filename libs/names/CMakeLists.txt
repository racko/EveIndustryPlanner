add_library(names
  src/names.cpp
)

add_library(names::names ALIAS names)

target_include_directories(names PUBLIC include)

target_link_libraries(names
  PUBLIC
    yaml_fwd::yaml_fwd
  PRIVATE
    Boost::serialization
    profiling::profiling
    yaml-cpp
)
