add_library(HTTPSRequestHandler src/HTTPSRequestHandler.cpp)
target_include_directories(HTTPSRequestHandler PUBLIC include)
target_link_libraries(HTTPSRequestHandler PRIVATE
    Boost::system
    Poco::NetSSL
)
