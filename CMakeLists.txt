cmake_minimum_required(VERSION 3.0.0)
project(autotag VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
include(CTest)
enable_testing()

add_subdirectory(libs/glog-0.4.0)
add_subdirectory(libs/restclient-cpp-0.5.2)
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(libs/json-3.9.1)

configure_file(${CMAKE_SOURCE_DIR}/config.ini ${CMAKE_BINARY_DIR} COPYONLY)

add_executable(autotag src/main.cpp src/Watch.cpp src/Watch.h src/movie.cpp src/movie.h src/TMDB.cpp src/TMDB.h)
target_include_directories(autotag PUBLIC ${CMAKE_SOURCE_DIR}/libs/simpleini-4.17)
target_link_libraries(autotag PRIVATE glog::glog restclient-cpp nlohmann_json::nlohmann_json)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
