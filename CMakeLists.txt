cmake_minimum_required(VERSION 2.8)
project(tinyorm_example)

set(CMAKE_CXX_STANDARD 17)

set(EXAMPLE example/test.cpp)
set(LIBMYSQL ${CMAKE_SOURCE_DIR}/tinyorm/mysql4cpp/mysql/lib)
set(SPDLOGD ${CMAKE_SOURCE_DIR}/tinyorm/mysql4cpp/spdlog/lib)

add_subdirectory(tinyorm/mysql4cpp)

link_directories(${LIBMYSQL} ${SPDLOGD})

add_executable(example ${EXAMPLE})

target_link_libraries(example libmysql.lib spdlogd.lib mysql4cpp)

target_include_directories(example PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/tinyorm/mysql4cpp/mysql/include
	${CMAKE_SOURCE_DIR}/tinyorm/mysql4cpp/spdlog/include
)