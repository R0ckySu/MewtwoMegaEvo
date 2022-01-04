project(QSimTask VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Playground/)

add_executable(NoiseGen
        noise_main.cpp
        NoiseBasic.h NoiseBasic.cpp
        )

target_link_libraries(NoiseGen PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(NoiseGen PUBLIC omp)
target_link_libraries(NoiseGen PUBLIC armadillo)
target_link_libraries(NoiseGen PUBLIC muparserx)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)