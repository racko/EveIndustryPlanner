add_library(types
  src/types.cpp
)

add_library(types::types ALIAS types)

target_include_directories(types PUBLIC include)

target_link_libraries(types
  PUBLIC
    type_id::type_id
    yaml_fwd::yaml_fwd
  PRIVATE
    groups::groups
    yaml-cpp
)
