cmake_minimum_required(VERSION 3.11)
project(MewtwoMegaEvo)

set(CMAKE_CXX_STANDARD 11)

include_directories(./3rd_party_libs/include)
link_directories(./3rd_party_libs/lib)

add_executable(MewtwoMegaEvo
                main.cpp)


add_subdirectory(QSimTask)

target_link_libraries(MewtwoMegaEvo PRIVATE QSimTask)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)
set(CMAKE_CXX_FLAGS -O3)
set(CMAKE_C_FLAGS -O3)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)


