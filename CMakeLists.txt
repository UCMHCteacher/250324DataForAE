cmake_minimum_required(VERSION 3.10)

project(AEChunkProgram_Data VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)









include(ImportPaths.cmake)

find_library(MySQL_LIB  mysqlcppconn    PATHS ${MySQL_LIB_DIR})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND
        ALL_LIB_FILES
        ws2_32.lib
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(pthread_LIB pthread)

    list(APPEND
        ALL_LIB_FILES
        ${pthread_LIB}
    )
endif()





list(APPEND
    ALL_INCLUDE_DIRS
    ${MySQL_INCLUDE_DIR}
)
include_directories(${ALL_INCLUDE_DIRS})








# 添加可执行目标
add_executable(RecordDataGen ./src/RecordDataGen.cpp)


add_executable(CompareDataGen ./src/CompareDataGen.cpp)
target_link_libraries(CompareDataGen PRIVATE ${ALL_LIB_FILES})
target_link_libraries(CompareDataGen PRIVATE ${MySQL_LIB})
target_compile_options(CompareDataGen PRIVATE "-pthread")


add_executable(Receiver ./src/Receiver.cpp)
target_link_libraries(Receiver PRIVATE ${ALL_LIB_FILES})
target_compile_options(Receiver PRIVATE "-pthread")

