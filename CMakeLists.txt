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
        utils/IO/tcpUtils.c utils/IO/tcpUtils.h
        utils/utils.c utils/utils.h
        utils/dataUtils.c utils/dataUtils.h
        utils/IO/pipeUtils.c utils/IO/pipeUtils.h
        server/player.c server/player.h
        communicationConstants.h)

add_executable(client
        client/client.c client/client.h
        utils/IO/tcpUtils.c utils/IO/tcpUtils.h
        utils/IO/inputUtils.c utils/IO/inputUtils.h
        utils/dataUtils.c utils/dataUtils.h
        client/gameResultEvaluation.c client/gameResultEvaluation.h
        server/player.c server/player.h
        client/gameResultEvaluation.c client/gameResultEvaluation.h
        utils/IO/pipeUtils.c utils/IO/pipeUtils.h
        utils/utils.c utils/utils.h
        communicationConstants.h utils/IO/outputUtils.c utils/IO/outputUtils.h)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)
target_link_libraries(client PRIVATE Threads::Threads)
