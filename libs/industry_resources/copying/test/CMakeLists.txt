add_executable(copying_test
  copying_test.cpp
  test_main.cpp
)

target_link_libraries(copying_test
  PRIVATE
    copying::copying
)

add_test(copying_test copying_test)
