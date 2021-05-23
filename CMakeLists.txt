cmake_minimum_required(VERSION 3.19)
project(KR C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS -pthread)
set(CMAKE_C_FLAGS -lrt)

SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
SET(GCC_COVERAGE_LINK_FLAGS "-lgcov")
include_directories(.)

add_executable(server
        server/server.c server/server.h
        tcpUtils.c tcpUtils.h
        server/utils.c server/utils.h
        server/dataUtils.c server/dataUtils.h
        server/pipeUtils.c server/pipeUtils.h
        server/player.c server/player.h
        communicationConstants.h)

add_executable(client
        client/client.c client/client.h
        tcpUtils.c tcpUtils.h
        inputUtils.c inputUtils.h
        server/dataUtils.c server/dataUtils.h
        gameResultEvaluation.c gameResultEvaluation.h
        server/player.c server/player.h
        gameResultEvaluation.c gameResultEvaluation.h
        server/pipeUtils.c server/pipeUtils.h
        server/utils.c server/utils.h
        communicationConstants.h)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)
