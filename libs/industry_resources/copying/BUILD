cc_library(
    name = "copying",
    srcs = [
        "src/copies.cpp",
    ],
    hdrs = [
        "include/copies.h",
    ],
    linkopts = ["-lyaml-cpp"],
    strip_include_prefix = "include",
    visibility = [
        "//apps/main:__pkg__",
    ],
    deps = [
        "//libs/industry_resources:type_id",
        "//libs/market_data",
        "//libs/skills",
        "//libs/yaml_fwd",
    ],
)
