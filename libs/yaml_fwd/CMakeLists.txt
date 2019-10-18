add_library(yaml_fwd INTERFACE)

target_include_directories(yaml_fwd INTERFACE include)

add_library(yaml_fwd::yaml_fwd ALIAS yaml_fwd)
