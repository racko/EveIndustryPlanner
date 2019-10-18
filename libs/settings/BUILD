cc_library(
    name = "settings",
    srcs = [
        "src/blueprint_selection.cpp",
        "src/facility_costs.cpp",
        "src/industry_limits.cpp",
        "src/queries.cpp",
        "src/settings.cpp",
        "src/skill_settings.cpp",
        "src/trade_costs.cpp",
    ],
    hdrs = [
        "include/blueprint_selection.h",
        "include/facility_costs.h",
        "include/industry_limits.h",
        "include/queries.h",
        "include/settings.h",
        "include/skill_settings.h",
        "include/trade_costs.h",
    ],
    linkopts = ["-lyaml-cpp"],
    strip_include_prefix = "include",
    visibility = [
        "//libs/industry_resources/resource_manager:__pkg__",
    ],
    deps = [
        "//libs/industry_resources:type_id",
        "//libs/profiling",
        "//libs/yaml_fwd",
    ],
)
