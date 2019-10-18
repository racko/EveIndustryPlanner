cc_library(
    name = "query_runner",
    srcs = [
        "QueryRunner.cpp",
    ],
    hdrs = [
        "QueryRunner.h",
    ],
    linkopts = ["-pthread"],
    visibility = [
        "//apps:__pkg__",
        "//apps/price_reader:__pkg__",
    ],
    deps = [
        "//libs/sqlite-util:sqlite_util",
    ],
)
