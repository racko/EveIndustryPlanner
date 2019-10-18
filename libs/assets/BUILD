cc_library(
    name = "assets",
    srcs = [
        "assets.cpp",
    ],
    hdrs = [
        "assets.h",
    ],
    linkopts = ["-lpugixml"],
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
