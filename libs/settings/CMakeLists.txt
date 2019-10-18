add_library(settings
  src/blueprint_selection.cpp
  src/facility_costs.cpp
  src/industry_limits.cpp
  src/queries.cpp
  src/settings.cpp
  src/skill_settings.cpp
  src/trade_costs.cpp
)

add_library(settings::settings ALIAS settings)

target_include_directories(settings PUBLIC include)

target_link_libraries(settings
  PUBLIC
    yaml_fwd::yaml_fwd
    type_id::type_id
  PRIVATE
    profiling::profiling
    yaml-cpp
)
