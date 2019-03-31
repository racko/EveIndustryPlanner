add_library(eve_stuff
    src/outdatedHistories.cpp
    src/outdatedOrders.cpp
)
target_include_directories(eve_stuff PUBLIC include)
target_link_libraries(eve_stuff PRIVATE
    Boost::boost
    profiling
)
target_link_libraries(eve_stuff PUBLIC
    sqlite-util
)
