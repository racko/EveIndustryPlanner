cc_binary(
    name = "main",
    srcs = [
        "src/jobs.cpp",
        "src/jobs.h",
        "src/main.cpp",
        "src/planner.cpp",
        "src/planner.h",
        "src/workspace.cpp",
        "src/workspace.h",
    ],
    linkopts = [
        "-lsoplex",
        "-lz",
        "-lgmp",
        "-lgmpxx",
        "-lboost_filesystem",
    ],
    deps = [
        "//libs/assets",
        "//libs/industry_resources/copying",
        "//libs/industry_resources/production",
        "//libs/industry_resources/research",
        "//libs/industry_resources/schematics",
    ],
)
