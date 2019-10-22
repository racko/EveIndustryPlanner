cc_library(
    name = "finalizer",
    hdrs = ["Finalizer.h"],
    visibility = ["//apps:__pkg__"],
)

cc_test(
    name = "test",
    srcs = ["test.cpp"],
    deps = [
        ":finalizer",
        "//:test_main",
    ],
)
