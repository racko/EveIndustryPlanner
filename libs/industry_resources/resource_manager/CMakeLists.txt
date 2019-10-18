add_library(resource_manager
  src/stuff.cpp
)

add_library(resource_manager::resource_manager ALIAS resource_manager)

target_include_directories(resource_manager
  PUBLIC
    include
)

target_link_libraries(resource_manager
  PUBLIC
    type_id::type_id
  PRIVATE
    assets::assets
    settings::settings
    names::names
    types::types
)
