add_library(assets
  src/assets.cpp
)

add_library(assets::assets ALIAS assets)

target_include_directories(assets PUBLIC include)

target_link_libraries(assets
  PRIVATE
    names::names
    pugixml
    type_id::type_id
)
