cmake_minimum_required(VERSION 3.19)
project(KR C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS -pthread)
set(CMAKE_C_FLAGS -lrt)

SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
SET(GCC_COVERAGE_LINK_FLAGS "-lgcov")
include_directories(.)

add_executable(KR
        server.c server.h
        serverUtils.c serverUtils.h
        utils.c utils.h
        dataUtils.c dataUtils.h
        pipeUtils.c pipeUtils.h
        player.c player.h
        communicationConstants.h)

find_package(Threads REQUIRED)
target_link_libraries(KR PRIVATE Threads::Threads)
