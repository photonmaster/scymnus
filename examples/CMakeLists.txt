cmake_minimum_required(VERSION 3.0)
project (examples)



add_executable(points points/main.cpp)
add_executable(metadata metadata/main.cpp)
add_executable(aspects aspects/main.cpp)
add_executable(minimal minimal/main.cpp)
add_executable(simple simple/main.cpp)
add_executable(models models/main.cpp)
add_executable(echo_aspects echo_aspects/main.cpp)
add_executable(settings settings/main.cpp)
add_executable(calculator calculator/main.cpp)



add_link_options(-nostartfiles)




target_link_libraries(points scymnus)
target_link_libraries(points ${Boost_LIBRARIES})

target_link_libraries(metadata scymnus)
target_link_libraries(metadata ${Boost_LIBRARIES})

target_link_libraries(aspects scymnus)
target_link_libraries(aspects ${Boost_LIBRARIES})


target_link_libraries(minimal scymnus)
target_link_libraries(minimal ${Boost_LIBRARIES})

target_link_libraries(simple scymnus)
target_link_libraries(simple ${Boost_LIBRARIES})


target_link_libraries(models scymnus)
target_link_libraries(models ${Boost_LIBRARIES})

target_link_libraries(echo_aspects scymnus)
target_link_libraries(echo_aspects ${Boost_LIBRARIES})

target_link_libraries(settings scymnus)
target_link_libraries(settings ${Boost_LIBRARIES})

target_link_libraries(calculator scymnus)
target_link_libraries(calculator ${Boost_LIBRARIES})
