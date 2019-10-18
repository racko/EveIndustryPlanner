cc_library(
    name = "names",
    srcs = [
        "names.cpp",
    ],
    hdrs = [
        "names.h",
    ],
    linkopts = [
        "-lboost_serialization",
        "-lyaml-cpp",
    ],
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
