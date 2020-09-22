project(QSimTask VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)

#add_executable(QSimCoreLibTest QSimLibTest.cpp)

add_library(
        QSimTask STATIC
        QuantumSimulationTask.h QuantumSimulationTask.cpp)

add_subdirectory(QSimCoreLib)

target_link_libraries(QSimTask PRIVATE QSimCoreLib)
target_link_libraries(QSimTask PRIVATE omp)
target_link_libraries(QSimTask PUBLIC armadillo)

#Json Integration#
include(FetchContent)
FetchContent_Declare(json GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent GIT_TAG v3.7.3)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
target_link_libraries(QSimTask PUBLIC nlohmann_json::nlohmann_json)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS -g)
set(CMAKE_C_FLAGS  -g)
