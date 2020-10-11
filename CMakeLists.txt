# requires minimum cmake version
cmake_minimum_required(VERSION 3.5)

# enforce C++11 features
set(CMAKE_CXX_STANDARD 11)

# produce the cmake var PROJECT_NAME
project(assignment_yarp-find-rgb)

# mandatory use of these packages
find_package(YARP 3.1.101 REQUIRED)
find_package(ICUB REQUIRED)
find_package(ICUBcontrib REQUIRED)

# extend the current search path used by cmake to load helpers
list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})

# helpers defining certain macros
include(ICUBcontribHelpers)
include(ICUBcontribOptions)

# specify to install in $ICUBcontrib_DIR/bin
icubcontrib_set_default_prefix()

add_subdirectory(gazebo)

# code for creating a ball in the iCub world
add_executable(${PROJECT_NAME}-ball ${CMAKE_SOURCE_DIR}/src/ball.cpp)
target_compile_definitions(${PROJECT_NAME}-ball PRIVATE _USE_MATH_DEFINES)
target_link_libraries(${PROJECT_NAME}-ball ${YARP_LIBRARIES} ctrlLib)
install(TARGETS ${PROJECT_NAME}-ball DESTINATION bin)

# code for moving the head looking for the head
add_executable(${PROJECT_NAME}-head ${CMAKE_SOURCE_DIR}/src/head.cpp)
target_compile_definitions(${PROJECT_NAME}-head PRIVATE _USE_MATH_DEFINES)
target_link_libraries(${PROJECT_NAME}-head ${YARP_LIBRARIES} ctrlLib)
install(TARGETS ${PROJECT_NAME}-head DESTINATION bin)

# generate ad-hoc project to perform "make uninstall"
icubcontrib_add_uninstall_target()

file(GLOB scripts ${CMAKE_SOURCE_DIR}/app/scripts/*.xml)
yarp_install(FILES ${scripts} DESTINATION ${ICUBCONTRIB_APPLICATIONS_INSTALL_DIR})
