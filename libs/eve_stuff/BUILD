cc_library(
    name = "eve_stuff",
    srcs = [
        "src/outdatedHistories.cpp",
        "src/outdatedOrders.cpp",
    ],
    hdrs = [
        "include/outdatedHistories.h",
        "include/outdatedOrders.h",
    ],
    strip_include_prefix = "include",
    visibility = ["//apps:__pkg__"],
    deps = [
        "//libs/profiling",
        "//libs/sqlite-util:sqlite_util",
    ],
)
