add_library(sqlite-util
  src/sqlite.cpp
)

add_library(sqlite-util::sqlite-util ALIAS sqlite-util)

target_include_directories(sqlite-util
  PUBLIC
    include
)

target_link_libraries(sqlite-util
  PRIVATE
    sqlite::sqlite
)
