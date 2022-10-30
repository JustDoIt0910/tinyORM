cmake_minimum_required(VERSION 2.8)

project(test)


SET(CMAKE_BUILD_TYPE "Debug")
add_definitions("-Wall -g")

set(CMAKE_CXX_STANDARD 17)
set(MYSQL_INCLUDE_DIR /usr/include/mysql)
set(MYSQL4CPP_INCLUDE_DIR ..)
set(SPDLOG_INCLUDE_DIR ../spdlog/include)

set(LIBMYSQL4CPP ../../../build/tinyorm/mysql4cpp)
set(LIBSPDLOG ../spdlog/lib)

aux_source_directory(. DIR_SRC)

link_directories(${LIBMYSQL4CPP} ${LIBSPDLOG})

add_executable(${PROJECT_NAME} ${DIR_SRC})

target_link_libraries(${PROJECT_NAME} mysql4cpp spdlog mysqlclient)

target_include_directories(${PROJECT_NAME} PRIVATE
            ${MYSQL4CPP_INCLUDE_DIR}
            ${MYSQL_INCLUDE_DIR}
            ${SPDLOG_INCLUDE_DIR}
        )