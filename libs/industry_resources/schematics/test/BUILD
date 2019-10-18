cc_test(
    name = "test",
    srcs = ["schematics_test.cpp"],
    flaky = True,  # depends on file on network drive. First access usually fails
    deps = [
        "//:test_main",
        "//libs/industry_resources/schematics",
    ],
)
