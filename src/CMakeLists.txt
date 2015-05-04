cmake_minimum_required (VERSION 2.8)
project(morpion_lpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

# build configurations (picked when running cmake-gui)
# default: Debug
# OptimizedWithAssert = optimized with asserts
# Release = Release without asserts (-DNDEBUG)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS_OPTIMIZEDWITHASSERT "${CMAKE_CXX_FLAGS} -O3")

#########
#  BOOST
#########

  set(Boost_USE_STATIC_LIBS       OFF)
  set(Boost_USE_MULTITHREADED     ON)
  set(Boost_USE_STATIC_RUNTIME    OFF)

  find_package(Boost REQUIRED COMPONENTS regex date_time unit_test_framework program_options serialization)
  include_directories(${Boost_INCLUDE_DIR})
  #add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})

  message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

#########
# /BOOST
#########

###########
# GraphViz
###########

link_directories(/opt/local/lib)

############
# /GraphViz
############

#######
# GLPK
#######


########
# /GLPK
########

#########
# Gurobi
#########

##########
# /Gurobi
##########

add_library(morpion morpion.cpp)
add_library(lpp lpp.cpp)
add_library(morpion_lpp morpion_lpp.cpp)
add_library(potential_lpp potential_lpp.cpp)

set(COMMON_LIBRARIES ${Boost_LIBRARIES} glpk)

add_executable(generator generator.cpp)
target_link_libraries(generator morpion lpp morpion_lpp potential_lpp ${COMMON_LIBRARIES})

add_executable(octagons octagons.cpp)
target_link_libraries(octagons ${COMMON_LIBRARIES})
