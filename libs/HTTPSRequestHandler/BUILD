cc_library(
    name = "https_request_handler",
    srcs = ["HTTPSRequestHandler.cpp"],
    hdrs = ["HTTPSRequestHandler.h"],
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
