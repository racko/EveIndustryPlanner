cc_library(
    name = "research",
    srcs = [
        "inventions.cpp",
    ],
    hdrs = [
        "inventions.h",
    ],
    linkopts = ["-lyaml-cpp"],
    visibility = [
        "//apps/main:__pkg__",
    ],
    deps = [
        "//libs/industry_resources:type_id",
        "//libs/industry_resources/resource_manager",
        "//libs/names",
        "//libs/skills",
        "//libs/yaml_fwd",
    ],
)
