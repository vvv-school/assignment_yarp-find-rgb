# requires minimum cmake version
cmake_minimum_required(VERSION 3.0)

# enforce C++11 features
set(CMAKE_CXX_STANDARD 11)

# produce the cmake var PROJECT_NAME
project(assignment_yarp-find-rgb)

# mandatory use of these packages
find_package(YARP REQUIRED)
find_package(ICUB REQUIRED)
find_package(ICUBcontrib REQUIRED)

# extend the current search path used by cmake to load helpers
list(APPEND CMAKE_MODULE_PATH ${YARP_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${ICUB_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})

# helpers defining certain macros (e.g. "yarp_install")
include(YarpInstallationHelpers)
include(ICUBcontribHelpers)
include(ICUBcontribOptions)

# specify to install in $ICUBcontrib_DIR/bin
icubcontrib_set_default_prefix()

# import math symbols from standard cmath
add_definitions(-D_USE_MATH_DEFINES)

include_directories(${YARP_INCLUDE_DIRS})
include_directories(${ctrlLib_INCLUDE_DIRS})

# code for creating a ball in the iCub world
add_executable(${PROJECT_NAME}-ball ${CMAKE_SOURCE_DIR}/src/ball.cpp)
target_link_libraries(${PROJECT_NAME}-ball ${YARP_LIBRARIES} ctrlLib)
install(TARGETS ${PROJECT_NAME}-ball DESTINATION bin)

# code for moving the head looking for the head
add_executable(${PROJECT_NAME}-head ${CMAKE_SOURCE_DIR}/src/head.cpp)
target_link_libraries(${PROJECT_NAME}-head ${YARP_LIBRARIES} ctrlLib)
install(TARGETS ${PROJECT_NAME}-head DESTINATION bin)

# generate ad-hoc project to perform "make uninstall"
icubcontrib_add_uninstall_target()

file(GLOB scripts ${CMAKE_SOURCE_DIR}/app/scripts/*.xml)
yarp_install(FILES ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_INSTALL_DIR})
