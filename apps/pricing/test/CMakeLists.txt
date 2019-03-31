add_executable(pricing_test
  getLine_test.cpp
  pricing_test.cpp
  test_main.cpp
)

target_link_libraries(pricing_test
  PRIVATE
    pricing_lib::pricing_lib
)

add_test(pricing_test pricing_test)
