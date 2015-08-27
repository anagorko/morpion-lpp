add_library(morpiongame morpiongame.cpp)

add_executable(nrpa nrpa.cpp)
target_link_libraries(nrpa morpiongame ${COMMON_LIBRARIES})
