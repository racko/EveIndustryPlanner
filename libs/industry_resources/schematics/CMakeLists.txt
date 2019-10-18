add_library(schematics
  src/schematics.cpp
)

add_library(schematics::schematics ALIAS schematics)

target_include_directories(schematics PUBLIC include)

target_link_libraries(schematics
  PUBLIC
    type_id::type_id
  PRIVATE
    assets::assets
    names::names
    sqlite-util
    resource_manager::resource_manager
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
