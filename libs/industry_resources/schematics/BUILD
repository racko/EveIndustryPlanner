cc_library(
    name = "schematics",
    srcs = [
        "src/schematics.cpp",
    ],
    hdrs = [
        "include/schematics.h",
    ],
    linkopts = ["-lyaml-cpp"],
    strip_include_prefix = "include",
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
