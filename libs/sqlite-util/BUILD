cc_library(
    name = "sqlite_util",
    srcs = [
        "src/sqlite.cpp",
    ],
    hdrs = [
        "include/sqlite.h",
    ],
    linkopts = ["-lsqlite3"],
    strip_include_prefix = "include",
    visibility = [
        "//libs/QueryRunner:__pkg__",
        "//libs/eve_stuff:__pkg__",
        "//libs/industry_resources/schematics:__pkg__",
        "//libs/market_data:__pkg__",
    ],
)
