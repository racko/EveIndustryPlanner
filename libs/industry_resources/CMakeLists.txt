add_library(type_id
  src/blueprint_efficiency.cpp
  src/blueprint_with_efficiency.cpp
  src/input_values.cpp
  src/job.cpp
  src/job_values.cpp
  src/production.cpp
  src/resource_base.cpp
)

add_library(type_id::type_id ALIAS type_id)

target_include_directories(type_id
  PUBLIC
    include
)

target_link_libraries(type_id
  PRIVATE
    infinity::infinity
)

add_subdirectory(copying)
add_subdirectory(production)
add_subdirectory(research)
add_subdirectory(resource_manager)
add_subdirectory(schematics)
add_subdirectory(types)
