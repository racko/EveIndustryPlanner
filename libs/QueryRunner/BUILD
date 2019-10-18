cc_library(
    name = "query_runner",
    srcs = [
        "src/QueryRunner.cpp",
    ],
    hdrs = [
        "include/QueryRunner.h",
    ],
    linkopts = ["-pthread"],
    strip_include_prefix = "include",
    visibility = [
        "//apps:__pkg__",
        "//apps/price_reader:__pkg__",
    ],
    deps = [
        "//libs/sqlite-util:sqlite_util",
    ],
)
