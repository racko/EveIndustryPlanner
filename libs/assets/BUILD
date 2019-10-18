cc_library(
    name = "assets",
    srcs = [
        "src/assets.cpp",
    ],
    hdrs = [
        "include/assets.h",
    ],
    linkopts = ["-lpugixml"],
    strip_include_prefix = "include",
    visibility = [
        "//apps/main:__pkg__",
        "//libs/industry_resources/resource_manager:__pkg__",
        "//libs/industry_resources/schematics:__pkg__",
    ],
    deps = [
        "//libs/industry_resources:type_id",
        "//libs/names",
    ],
)
