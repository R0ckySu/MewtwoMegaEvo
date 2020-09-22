project(QSimCoreLib VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)

add_executable(QSimCoreLibTest QSimLibTest.cpp)

add_library(
        QSimCoreLib STATIC
        Hamiltonian.cpp Hamiltonian.h
        VonNeumannSolver.cpp VonNeumannSolver.h
        Gate.cpp Gate.h
        Sequence.cpp Sequence.h
        TimingBasic.cpp TimingBasic.h
        Utils.h Utils.cpp)

target_link_libraries(QSimCoreLibTest PRIVATE QSimCoreLib)
target_link_libraries(QSimCoreLib PRIVATE omp)
target_link_libraries(QSimCoreLib PUBLIC armadillo)
target_link_libraries(QSimCoreLib PRIVATE nlohmann_json::nlohmann_json)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS -g)
set(CMAKE_C_FLAGS  -g)
