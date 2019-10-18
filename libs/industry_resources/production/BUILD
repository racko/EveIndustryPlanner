cc_library(
    name = "production",
    srcs = [
        "products.cpp",
    ],
    hdrs = [
        "products.h",
    ],
    linkopts = ["-lyaml-cpp"],
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
