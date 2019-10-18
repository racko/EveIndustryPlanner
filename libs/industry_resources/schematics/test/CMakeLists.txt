add_executable(schematics_test
  schematics_test.cpp
  test_main.cpp
)

target_link_libraries(schematics_test
  PRIVATE
    schematics::schematics
)

add_test(schematics_test schematics_test)
