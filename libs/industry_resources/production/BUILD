cc_library(
    name = "production",
    srcs = [
        "src/products.cpp",
    ],
    hdrs = [
        "include/products.h",
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
