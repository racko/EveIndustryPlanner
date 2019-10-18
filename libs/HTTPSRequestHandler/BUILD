cc_library(
    name = "https_request_handler",
    srcs = ["src/HTTPSRequestHandler.cpp"],
    hdrs = ["include/HTTPSRequestHandler.h"],
    includes = ["include"],
    linkopts = [
        "-pthread",
        "-lPocoFoundation",
        "-lPocoNetSSL",
        "-lPocoNet",
    ],
    visibility = [
        "//apps:__pkg__",
        "//apps/price_reader:__pkg__",
        "//libs/access_token:__pkg__",
    ],
)
