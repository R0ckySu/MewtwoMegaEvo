project(QSimCoreLib VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_library(
        QSimCoreLib STATIC
        QSimCore.h
        Hamiltonian.cpp
        VonNeumannSolver.cpp
        Gate.cpp
        Sequence.cpp
        TimingBasic.cpp TimingBasic.h
        Utils.cpp
        Measurement.cpp
        ExtSigCache.cpp
)

add_definitions(-D_TASK_PROGRESS_)

#target_include_directories(QSimCoreLib PUBLIC ${exprtk_SOURCE_DIR}/include)

target_link_libraries(QSimCoreLib ${CONAN_LIBS})
#target_link_libraries(QSimCoreLibTest PRIVATE QSimCoreLib)
target_link_libraries(QSimCoreLib PRIVATE muparserx::muparserx)
target_link_libraries(QSimCoreLib PUBLIC armadillo::armadillo)
target_link_libraries(QSimCoreLib PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(QSimCoreLib PUBLIC rttr_core)
target_link_libraries(QSimCoreLib PUBLIC exprtk::exprtk)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)

#set(CMAKE_CXX_FLAGS -O3)
#set(CMAKE_C_FLAGS -O3)