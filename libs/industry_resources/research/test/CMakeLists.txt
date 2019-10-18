add_executable(research_test
  research_test.cpp
  test_main.cpp
)

target_link_libraries(research_test
  PRIVATE
    research::research
)

add_test(research_test research_test)
