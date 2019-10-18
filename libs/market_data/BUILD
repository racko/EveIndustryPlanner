cc_library(
    name = "market_data",
    srcs = [
        "market_data.cpp",
        "order.cpp",
    ],
    hdrs = [
        "buy.h",
        "market_data.h",
        "order.h",
        "order_serialization.h",
        "sell.h",
    ],
    linkopts = ["-ljsoncpp"],
    visibility = [
        "//libs/industry_resources/copying:__pkg__",
        "//libs/industry_resources/production:__pkg__",
    ],
    deps = [
        "//libs/industry_resources:type_id",
        "//libs/industry_resources/resource_manager",
        "//libs/names",
        "//libs/sqlite-util:sqlite_util",
    ],
)
