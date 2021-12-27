project(QSimCoreLib VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

#add_executable(QSimCoreLibTest QSimLibTest.cpp)

add_library(
        QSimCoreLib STATIC
        QSimCore.h
        Hamiltonian.cpp
        VonNeumannSolver.cpp
        Gate.cpp
        Sequence.cpp
        TimingBasic.cpp TimingBasic.h
        Utils.cpp
        Measurement.cpp)

#target_link_libraries(QSimCoreLibTest PRIVATE QSimCoreLib)
#target_link_libraries(QSimCoreLib PRIVATE omp)
target_link_libraries(QSimCoreLib PUBLIC armadillo)
target_link_libraries(QSimCoreLib PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(QSimCoreLib PUBLIC librttr_core.a)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)

#set(CMAKE_CXX_FLAGS -O3)
#set(CMAKE_C_FLAGS -O3)

