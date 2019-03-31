#add_compile_options(-Og)
add_library(pricing_lib
  src/getLine.cpp
  src/json_writer_util.cpp
  src/order.cpp
)

target_include_directories(pricing_lib
  PUBLIC
    include
)

add_library(pricing_lib::pricing_lib ALIAS pricing_lib)

add_executable(pricing
  src/archive_iterator.cpp
  src/buffer.cpp
  src/file_system.cpp
  src/json_writer.cpp
  src/json_reader.cpp
  src/main.cpp
  src/parse_datetime.cpp
  src/rapidjson_errstr.cpp
  src/read_json_order.cpp
)

target_link_libraries(pricing
  PRIVATE
    pricing_lib::pricing_lib
    Boost::filesystem
    archive::archive
)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
