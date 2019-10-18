add_library(profiling INTERFACE)

target_include_directories(profiling INTERFACE include)

add_library(profiling::profiling ALIAS profiling)
