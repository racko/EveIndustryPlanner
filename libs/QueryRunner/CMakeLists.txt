add_library(QueryRunner src/QueryRunner.cpp)
target_include_directories(QueryRunner PUBLIC include)
target_link_libraries(QueryRunner PUBLIC
    sqlite-util
)
target_link_libraries(QueryRunner PRIVATE
    Boost::system
)
