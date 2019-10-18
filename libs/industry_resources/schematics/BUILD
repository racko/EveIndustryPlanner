cc_library(
    name = "schematics",
    srcs = [
        "schematics.cpp",
    ],
    hdrs = [
        "schematics.h",
    ],
    linkopts = ["-lyaml-cpp"],
    visibility = [
        "//apps/main:__pkg__",
        "//libs/industry_resources/schematics/test:__pkg__",
    ],
    deps = [
        "//libs/assets",
        "//libs/industry_resources:type_id",
        "//libs/industry_resources/resource_manager",
        "//libs/sqlite-util:sqlite_util",
    ],
)
