add_library(morpion morpion.cpp)
add_library(lpp lpp.cpp)
add_library(morpion_lpp morpion_lpp.cpp)
add_library(potential_lpp potential_lpp.cpp)

add_executable(generator generator.cpp)
target_link_libraries(generator morpion lpp morpion_lpp potential_lpp ${COMMON_LIBRARIES})

add_executable(octagons octagons.cpp)
target_link_libraries(octagons ${COMMON_LIBRARIES})
