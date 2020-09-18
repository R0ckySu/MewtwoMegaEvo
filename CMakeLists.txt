cmake_minimum_required(VERSION 3.11)
project(MewtwoMegaEvo)

set(CMAKE_CXX_STANDARD 11)

add_executable(MewtwoMegaEvo
        main.cpp QSimCoreLib/Hamiltonian.cpp QSimCoreLib/Hamiltonian.h QSimCoreLib/QSimLibTest.cpp)

add_subdirectory(QSimCoreLib)

target_link_libraries(MewtwoMegaEvo PRIVATE QSimCoreLib)
