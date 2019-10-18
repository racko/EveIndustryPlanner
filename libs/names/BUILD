cc_library(
    name = "names",
    srcs = [
        "src/names.cpp",
    ],
    hdrs = [
        "include/names.h",
    ],
    linkopts = [
        "-lboost_serialization",
        "-lyaml-cpp",
    ],
    strip_include_prefix = "include",
    visibility = [
        "//libs/assets:__pkg__",
        "//libs/industry_resources/research:__pkg__",
        "//libs/market_data:__pkg__",
        "//libs/skills:__pkg__",
    ],
    deps = [
        "//libs/profiling",
        "//libs/yaml_fwd",
    ],
)
