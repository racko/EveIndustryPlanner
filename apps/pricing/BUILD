cc_library(
    name = "pricing_lib",
    srcs = [
        "src/archive_iterator.cpp",
        "src/archive_iterator.h",
        "src/buffer.cpp",
        "src/buffer.h",
        "src/file_system.cpp",
        "src/file_system.h",
        "src/getLine.cpp",
        "src/json_writer_util.cpp",
        "src/order.cpp",
        "src/parse_datetime.cpp",
        "src/parse_datetime.h",
        "src/rapidjson_errstr.cpp",
        "src/rapidjson_errstr.h",
        "src/read_json_order.cpp",
        "src/read_json_order.h",
    ],
    hdrs = [
        "include/getLine.h",
        "include/json_writer_util.h",
        "include/order.h",
    ],
    linkopts = [
        "-larchive",
        "-lboost_filesystem",
    ],
    strip_include_prefix = "include",
    visibility = [
        "//apps/pricing/test:__pkg__",
    ],
)

cc_binary(
    name = "pricing",
    srcs = [
        "src/json_reader.cpp",
        "src/json_reader.h",
        "src/json_writer.cpp",
        "src/json_writer.h",
        "src/main.cpp",
    ],
    linkopts = ["-lboost_filesystem"],
    deps = [
        ":pricing_lib",
    ],
)
