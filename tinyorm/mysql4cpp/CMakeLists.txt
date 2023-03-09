cmake_minimum_required(VERSION 2.8)
project(mysql4cpp)

set(CMAKE_CXX_STANDARD 17)


 
if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
	message("current platform: Windows")
	set(MYSQL_INCLUDE_DIR mysql/include)

else()
	message("current platform: Linux")
	set(MYSQL_INCLUDE_DIR /usr/include/mysql)

endif()


aux_source_directory(. DIR_SRC)
add_library(${PROJECT_NAME} ${DIR_SRC})

target_include_directories(${PROJECT_NAME} PUBLIC
        .
	${MYSQL_INCLUDE_DIR}
        spdlog/include
)
