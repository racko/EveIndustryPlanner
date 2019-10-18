add_executable(production_test
  production_test.cpp
  test_main.cpp
)

target_link_libraries(production_test
  PRIVATE
    production::production
)

add_test(production_test production_test)
