cmake_minimum_required(VERSION 3.11)
project(MewtwoMegaEvo)

set(CMAKE_CXX_STANDARD 11)
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Playground/Precompiled_exe/${CMAKE_SYSTEM_NAME}/)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Playground/)

include_directories(./3rd_party_libs/include)
link_directories(./3rd_party_libs/lib)

add_executable(MewtwoMegaEvo
                main.cpp command_arg_parser.h)

add_subdirectory(QSimTask)
add_subdirectory(Noise)

target_link_libraries(MewtwoMegaEvo PRIVATE QSimTask)

set(CMAKE_CXX_FLAGS -fopenmp)
set(CMAKE_C_FLAGS -fopenmp)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)

include(FetchContent)
FetchContent_Declare(json GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent GIT_TAG v3.7.3)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()