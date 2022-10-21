cmake_minimum_required(VERSION 2.8)
project(mysql4cpp)

set(CMAKE_CXX_STANDARD 17)


aux_source_directory(. DIR_SRC)
add_library(${PROJECT_NAME} ${DIR_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/mysql/include
	${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
)
