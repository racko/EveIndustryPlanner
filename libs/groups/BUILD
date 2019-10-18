cc_library(
    name = "groups",
    srcs = [
        "groups.cpp",
    ],
    hdrs = [
        "groups.h",
    ],
    linkopts = [
        "-lboost_serialization",
        "-lyaml-cpp",
    ],
    visibility = [
        "//libs/industry_resources/types:__pkg__",
    ],
    deps = [
        "//libs/profiling",
        "//libs/yaml_fwd",
    ],
)
