cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 20)

project(scymnus VERSION 0.0.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -msse4.2")


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

#Minimum boost version required
SET(BOOST_MIN_VERSION "1.70")

SET(Boost_NO_BOOST_CMAKE ON)

#build options
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_DEBUG_LOGS "Enable debug logs" OFF)

if(ENABLE_DEBUG_LOGS)
    add_definitions(-DTEST_DEBUG)
endif()


set(dir .)
file(GLOB main "${dir}/*.hpp" "${dir}/*.h")


set(dir utilities)
file(GLOB utilities "${dir}/*.hpp" "${dir}/*.h")

set(dir meta)
file(GLOB meta "${dir}/*.hpp" "${dir}/*.h")

set(dir external)
file(GLOB external "${dir}/*.hpp" "${dir}/*.h")

set(dir external/http_parser)
file(GLOB http_parser  "${dir}/*.h" "${dir}/*.c")

set(dir server)
file(GLOB server "${dir}/*.hpp")

set(dir properties)
file(GLOB properties "${dir}/*.hpp")

set(dir core)
file(GLOB core "${dir}/*.hpp")

set(dir http)
file(GLOB http "${dir}/*.hpp")


set(dir url)
file(GLOB url "${dir}/*.hpp" "${dir}/*.h")



set(dir controllers)
file(GLOB controllers "${dir}/*.hpp")

set(dir mime)
file(GLOB mime "${dir}/*.hpp" "${dir}/*.cpp")



set(files ${main}
    ${meta}
    ${utilities}
    ${external}
    ${http_parser}
    ${server}
    ${core}
    ${http}
    ${url}
    ${controllers}
    ${mime}
     ${properties}
    )




FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system thread)

if (NOT Boost_FOUND)
      message(FATAL_ERROR "Fatal error: Boost version >= 1.70) required.\n")
endif (NOT Boost_FOUND)


include_directories( ${Boost_INCLUDE_DIR} )


set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR})

include_directories("${PROJECT_INCLUDE_DIR}")

add_library(${CMAKE_PROJECT_NAME} SHARED ${files})
target_link_libraries(${CMAKE_PROJECT_NAME})
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HTTPDCORE_LIBRARY)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

