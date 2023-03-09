cmake_minimum_required(VERSION 2.8)

add_subdirectory(mysql4cpp)
add_subdirectory(json-3.11.2)
file(COPY rules.json DESTINATION ${CMAKE_BINARY_DIR})
