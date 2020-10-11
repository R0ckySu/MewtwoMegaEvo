cmake_minimum_required(VERSION 3.11)
project(MewtwoMegaEvo)

set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Playground/Precompiled_exe/${CMAKE_SYSTEM_NAME}/)

include_directories(./3rd_party_libs/include)
link_directories(./3rd_party_libs/lib)

add_executable(MewtwoMegaEvo
                main.cpp command_arg_parser.h)


add_subdirectory(QSimTask)

target_link_libraries(MewtwoMegaEvo PRIVATE QSimTask)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)
set(CMAKE_CXX_FLAGS -O3)
set(CMAKE_C_FLAGS -O3)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)