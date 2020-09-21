cmake_minimum_required(VERSION 3.11)
project(MewtwoMegaEvo)

set(CMAKE_CXX_STANDARD 11)

add_executable(MewtwoMegaEvo
                main.cpp)


add_subdirectory(QSimTask)

target_link_libraries(MewtwoMegaEvo PRIVATE QSimTask)
