cc_binary(
    name = "price_reader",
    srcs = [
        "price_reader.cpp",
        "price_reader.h",
    ],
    linkopts = [
        "-ljsoncpp",
    ],
    deps = [
        "//libs/HTTPSRequestHandler:https_request_handler",
        "//libs/QueryRunner:query_runner",
        "//libs/profiling",
    ],
)
