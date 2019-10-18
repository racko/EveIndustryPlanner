cc_library(
    name = "test_main",
    testonly = True,
    srcs = [
        "test_main.cpp",
    ],
    linkstatic = True,
    visibility = ["//:__subpackages__"],
)

load("@bazel_compilation_database//:aspects.bzl", "compilation_database")

compilation_database(
    name = "compdb",
    # ideally should be the same as `bazel info execution_root`.
    exec_root = "/home/racko/.cache/bazel/_bazel_racko/7f0b4c7c4acf6796a46181f3473bdf00/execroot/__main__",
    targets = [
    ],
)
