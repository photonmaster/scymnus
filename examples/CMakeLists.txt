cmake_minimum_required(VERSION 3.0)
project (examples)

add_executable(points points/main.cpp)
add_executable(metadata metadata/main.cpp)
add_executable(aspects aspects/main.cpp)
add_executable(minimal minimal/main.cpp)
add_executable(simple simple/main.cpp)


add_link_options(-nostartfiles)


#target_include_directories(simple PUBLIC ..)
#target_include_directories(points PUBLIC ../..)


target_link_libraries(points scymnous)
target_link_libraries(points ${Boost_LIBRARIES})

target_link_libraries(metadata scymnous)
target_link_libraries(metadata ${Boost_LIBRARIES})

target_link_libraries(aspects scymnous)
target_link_libraries(aspects ${Boost_LIBRARIES})


target_link_libraries(minimal scymnous)
target_link_libraries(minimal ${Boost_LIBRARIES})

target_link_libraries(simple scymnous)
target_link_libraries(simple ${Boost_LIBRARIES})

