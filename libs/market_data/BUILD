cc_library(
    name = "market_data",
    srcs = [
        "src/market_data.cpp",
        "src/order.cpp",
    ],
    hdrs = [
        "include/buy.h",
        "include/market_data.h",
        "include/order.h",
        "include/order_serialization.h",
        "include/sell.h",
    ],
    linkopts = ["-ljsoncpp"],
    strip_include_prefix = "include",
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
