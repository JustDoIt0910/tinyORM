cmake_minimum_required(VERSION 2.8)
project(tinyorm_example)

SET(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 17)

set(EXAMPLE example/test.cpp tinyorm/datatype.hpp)
set(SPDLOGD ${CMAKE_SOURCE_DIR}/tinyorm/mysql4cpp/spdlog/lib)

add_subdirectory(tinyorm/mysql4cpp)

link_directories(${SPDLOGD})

add_executable(example ${EXAMPLE})

target_link_libraries(example spdlog mysql4cpp mysqlclient)

target_include_directories(example PRIVATE
	${CMAKE_SOURCE_DIR}
)
