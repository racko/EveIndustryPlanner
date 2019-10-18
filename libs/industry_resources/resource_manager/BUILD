cc_library(
    name = "resource_manager",
    srcs = [
        "src/stuff.cpp",
    ],
    hdrs = [
        "include/stuff.h",
    ],
    strip_include_prefix = "include",
    visibility = [
        "//libs/industry_resources/research:__pkg__",
        "//libs/industry_resources/schematics:__pkg__",
        "//libs/market_data:__pkg__",
    ],
    deps = [
        "//libs/assets",
        "//libs/industry_resources:type_id",
        "//libs/industry_resources/types",
        "//libs/settings",
    ],
)
