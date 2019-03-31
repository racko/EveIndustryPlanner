add_executable(infinity_test
  infinity_test.cpp
  main_test.cpp
)

target_link_libraries(infinity_test
  PRIVATE
    infinity::infinity
    libsoplex
)

add_test(infinity_test infinity_test)
