cc_library(
    name = "groups",
    srcs = [
        "src/groups.cpp",
    ],
    hdrs = [
        "include/groups.h",
    ],
    linkopts = [
        "-lboost_serialization",
        "-lyaml-cpp",
    ],
    strip_include_prefix = "include",
    visibility = [
        "//libs/industry_resources/types:__pkg__",
    ],
    deps = [
        "//libs/profiling",
        "//libs/yaml_fwd",
    ],
)
