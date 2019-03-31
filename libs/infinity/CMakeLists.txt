add_library(infinity INTERFACE)
add_library(infinity::infinity ALIAS infinity)
target_include_directories(infinity
  INTERFACE
    include
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
