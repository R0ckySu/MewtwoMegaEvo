project(QSimTask VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

#add_executable(QSimCoreLibTest QSimLibTest.cpp)

add_library(
        QSimTask STATIC
        QuantumSimulationTask.h QuantumSimulationTask.cpp
        ParamScheduler.h ParamScheduler.cpp
        SimPrototypes.h SimPrototypes.cpp
)

add_subdirectory(QSimCoreLib)

add_definitions(-D_TASK_PROGRESS_)

target_link_libraries(QSimTask PRIVATE QSimCoreLib)
target_link_libraries(QSimTask PRIVATE omp)
target_link_libraries(QSimTask PRIVATE hdf5_cpp)
target_link_libraries(QSimTask PRIVATE hdf5)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)
#set(CMAKE_CXX_FLAGS -O3)
#set(CMAKE_C_FLAGS -O3)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)

#Json Integration#

target_link_libraries(QSimTask PUBLIC nlohmann_json::nlohmann_json)