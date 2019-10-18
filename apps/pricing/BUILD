cc_library(
    name = "pricing_lib",
    srcs = [
        "archive_iterator.cpp",
        "archive_iterator.h",
        "buffer.cpp",
        "buffer.h",
        "file_system.cpp",
        "file_system.h",
        "getLine.cpp",
        "json_writer_util.cpp",
        "order.cpp",
        "parse_datetime.cpp",
        "parse_datetime.h",
        "rapidjson_errstr.cpp",
        "rapidjson_errstr.h",
        "read_json_order.cpp",
        "read_json_order.h",
    ],
    hdrs = [
        "getLine.h",
        "json_writer_util.h",
        "order.h",
    ],
    linkopts = [
        "-larchive",
        "-lboost_filesystem",
    ],
    visibility = [
        "//apps/pricing/test:__pkg__",
    ],
)

cc_binary(
    name = "pricing",
    srcs = [
        "json_reader.cpp",
        "json_reader.h",
        "json_writer.cpp",
        "json_writer.h",
        "main.cpp",
    ],
    linkopts = ["-lboost_filesystem"],
    deps = [
        ":pricing_lib",
    ],
)
