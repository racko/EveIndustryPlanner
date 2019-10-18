cc_library(
    name = "sqlite_util",
    srcs = [
        "sqlite.cpp",
    ],
    hdrs = [
        "sqlite.h",
    ],
    linkopts = ["-lsqlite3"],
    visibility = [
        "//libs/QueryRunner:__pkg__",
        "//libs/eve_stuff:__pkg__",
        "//libs/industry_resources/schematics:__pkg__",
        "//libs/market_data:__pkg__",
    ],
)
