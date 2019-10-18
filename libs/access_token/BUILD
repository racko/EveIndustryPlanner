cc_library(
    name = "access_token",
    srcs = [
        "src/access_token.cpp",
    ],
    hdrs = [
        "include/access_token.h",
    ],
    linkopts = ["-lboost_regex"],
    strip_include_prefix = "include",
    deps = [
        "//libs/HTTPSRequestHandler:https_request_handler",
    ],
)
