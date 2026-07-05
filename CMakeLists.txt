cmake_minimum_required(VERSION 3.11)
project(MewtwoMegaEvo)

set(CMAKE_CXX_STANDARD 17)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/Playground/)

include_directories(./3rd_party_libs/include)
link_directories(./3rd_party_libs/lib)

find_package(HDF5)
find_package(OpenMP)
find_package(muparserx)
find_package(exprtk)
find_package(Armadillo)

include_directories(${muparserx_INCLUDE_DIRS})
include_directories(${HDF5_INCLUDE_DIRS})
include_directories(${exprtk_SOURCE_DIR})
include_directories(${Armadillo_INCLUDE_DIR})

add_executable(MewtwoMegaEvo
                main.cpp command_arg_parser.h)

add_subdirectory(QSimTask)
add_subdirectory(Noise)

target_link_libraries(MewtwoMegaEvo ${CONAN_LIBS})
target_link_libraries(MewtwoMegaEvo PRIVATE QSimTask)

set(CMAKE_CXX_FLAGS -O3)
set(CMAKE_C_FLAGS -O3)

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_CXX_FLAGS -g)
#set(CMAKE_C_FLAGS  -g)

include(FetchContent)

FetchContent_Declare(rttr GIT_REPOSITORY https://github.com/rttrorg/rttr.git GIT_TAG v0.9.6)
option(BUILD_RTTR_DYNAMIC "Build the dynamic/shared version of RTTR library" ON)
option(BUILD_UNIT_TESTS "Build the unit tests of RTTR" OFF)
option(BUILD_STATIC "Build RTTR as static library" ON)
option(BUILD_WITH_STATIC_RUNTIME_LIBS "Link against the static runtime libraries" OFF)
option(BUILD_WITH_RTTI "Enable build with C++ runtime type information for compilation" ON)
option(BUILD_BENCHMARKS "Enable this to build the benchmarks" OFF)
option(BUILD_EXAMPLES "Enable this to build the examples" OFF)
option(BUILD_DOCUMENTATION "Enable this to build the documentation" OFF)
option(BUILD_INSTALLER "Enable this to build the installer" ON)
option(BUILD_PACKAGE "Enable this to build the installer" ON)
option(USE_PCH "Use precompiled header files for compilation" ON)
option(CUSTOM_DOXYGEN_STYLE "Enable this option to use a custom doxygen style for HTML documentation; Otherwise the default will be used" OFF)
option(BUILD_WEBSITE_DOCU "Enable this option to create the special docu for the website" OFF)
FetchContent_MakeAvailable(rttr)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent GIT_TAG v3.7.3)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


FetchContent_Declare(
  rttr
  GIT_REPOSITORY https://github.com/rttrorg/rttr.git
  GIT_TAG        v0.9.6
)

FetchContent_MakeAvailable(rttr)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if (TARGET rttr_core)
        target_compile_options(rttr_core PRIVATE
            -Wno-implicit-const-int-float-conversion
        )
    endif()
    if (TARGET rttr_core_lib)
        target_compile_options(rttr_core_lib PRIVATE
            -Wno-implicit-const-int-float-conversion
        )
    endif()
endif()

FetchContent_Declare(exprtk GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git GIT_TAG cmake)
FetchContent_MakeAvailable(exprtk)

#FetchContent_Declare(armadillo GIT_REPOSITORY https://gitlab.com/conradsnicta/armadillo-code.git GIT_TAG 11.4.x)
#option(DETECT_HDF5 ON)
#FetchContent_MakeAvailable(armadillo)