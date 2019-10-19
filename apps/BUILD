cc_binary(
    name = "history_reader",
    srcs = [
        "history_reader.cpp",
    ],
    linkopts = [
        "-ljsoncpp",
    ],
    deps = [
        "//libs/HTTPSRequestHandler:https_request_handler",
        "//libs/QueryRunner:query_runner",
        "//libs/Semaphore:semaphore",
        "//libs/eve_stuff",
    ],
)

cc_binary(
    name = "order_reader",
    srcs = [
        "order_reader.cpp",
    ],
    linkopts = [
        "-ljsoncpp",
    ],
    deps = [
        "//libs/HTTPSRequestHandler:https_request_handler",
        "//libs/QueryRunner:query_runner",
        "//libs/Semaphore:semaphore",
        "//libs/eve_stuff",
    ],
)

cc_binary(
    name = "server",
    srcs = [
        "server.cpp",
    ],
    linkopts = [
        "-lPocoFoundation",
        "-lPocoNet",
        "-ljsoncpp",
    ],
    deps = [
        "//libs/QueryRunner:query_runner",
        "//libs/Semaphore:semaphore",
    ],
)

cc_binary(
    name = "station_reader",
    srcs = [
        "station_reader.cpp",
    ],
    linkopts = [
        "-lyaml-cpp",
    ],
    deps = [
        "//libs/HTTPSRequestHandler:https_request_handler",
        "//libs/QueryRunner:query_runner",
        "//libs/profiling",
    ],
)
