cc_library(
    name = "research",
    srcs = [
        "src/inventions.cpp",
    ],
    hdrs = [
        "include/inventions.h",
    ],
    linkopts = ["-lyaml-cpp"],
    strip_include_prefix = "include",
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
