cmake_minimum_required(VERSION 3.3)

project(eve_industry_optimization)

include(CTest)

#find_package( PkgConfig )
#pkg_check_modules( EIGEN3 REQUIRED eigen3 )

set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS system regex filesystem REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(pugixml REQUIRED)
find_package(Poco REQUIRED COMPONENTS Net NetSSL)

find_library(SQLITE3 sqlite3)
if(NOT SQLITE3)
    message(FATAL_ERROR "sqlite3 library not found")
endif()
add_library(sqlite::sqlite UNKNOWN IMPORTED)
set_target_properties(sqlite::sqlite PROPERTIES
    INTERFACE_COMPILE_OPTIONS -pthread
    IMPORTED_LOCATION ${SQLITE3}
)

find_library(JSONCPP jsoncpp)

if(NOT JSONCPP)
    message(FATAL_ERROR "jsoncpp library not found")
endif()

add_library(jsoncpp::jsoncpp UNKNOWN IMPORTED)
set_target_properties(jsoncpp::jsoncpp PROPERTIES
    IMPORTED_LOCATION ${JSONCPP}
)

find_package(soplex REQUIRED)
if (TARGET libsoplex)
    message("libsoplex is a target")
    get_target_property(libsoplex_include_dirs libsoplex INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(libsoplex_link_libraries libsoplex INTERFACE_LINK_LIBRARIES)
    set_target_properties(libsoplex PROPERTIES IMPORTED_LOCATION /usr/lib/libsoplex.a)
    get_target_property(libsoplex_location libsoplex IMPORTED_LOCATION)
    message("libsoplex_include_dirs: ${libsoplex_include_dirs}")
    message("libsoplex_link_libraries: ${libsoplex_link_libraries}")
    message("libsoplex_location: ${libsoplex_location}")
else()
    message(FATAL_ERROR "libsoplex is not a target")
endif()

find_library(ARCHIVE archive)

if(NOT ARCHIVE)
    message(FATAL_ERROR "archive library not found")
endif()

add_library(archive::archive UNKNOWN IMPORTED)
set_target_properties(archive::archive PROPERTIES
    IMPORTED_LOCATION ${ARCHIVE}
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -Werror
    -Wall
    -Wextra
    -pedantic
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wold-style-cast
    -Woverloaded-virtual
    -Wunused
    -march=native
    -fdiagnostics-color=always
    #$<$<CONFIG:Debug>:-Og>
)

add_definitions(
    -DRAPIDJSON_HAS_CXX11_RVALUE_REFS
)

add_subdirectory(apps)
add_subdirectory(libs)
