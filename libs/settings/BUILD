cc_library(
    name = "settings",
    srcs = [
        "blueprint_selection.cpp",
        "facility_costs.cpp",
        "industry_limits.cpp",
        "queries.cpp",
        "settings.cpp",
        "skill_settings.cpp",
        "trade_costs.cpp",
    ],
    hdrs = [
        "blueprint_selection.h",
        "facility_costs.h",
        "industry_limits.h",
        "queries.h",
        "settings.h",
        "skill_settings.h",
        "trade_costs.h",
    ],
    linkopts = ["-lyaml-cpp"],
    visibility = [
        "//libs/industry_resources/resource_manager:__pkg__",
    ],
    deps = [
        "//libs/industry_resources:type_id",
        "//libs/profiling",
        "//libs/yaml_fwd",
    ],
)
