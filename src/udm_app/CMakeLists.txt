cmake_minimum_required (VERSION 3.2)

project(udm)

if (CMAKE_OPENXGUDM_BUILD_TYPE STREQUAL Debug)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DDEBUG_IS_ON=1 -DINFO_IS_ON=1 -DTRACE_IS_ON=1")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(SRC_DIR ${CMAKE_OPENXGUDM_DIR}/src)
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_OPENXGUDM_DIR}/build/ext)
link_directories(${EXTERNAL_INSTALL_LOCATION}/pistache/lib)

include_directories(${EXTERNAL_INSTALL_LOCATION}/pistache/include)
include_directories(${EXTERNAL_INSTALL_LOCATION}/json/include)
include_directories(${SRC_DIR}/model)
include_directories(${SRC_DIR}/api)
include_directories(${SRC_DIR}/impl)
include_directories(${SRC_DIR}/5gaka)
include_directories(${SRC_DIR}/common)
include_directories(${SRC_DIR}/utils)
include_directories(${SRC_DIR}/asnlib)
#include_directories(${SRC_DIR}/udm_app)

##############################################
find_package(PkgConfig REQUIRED)

pkg_search_module(CONFIG REQUIRED libconfig++)
include_directories(${CONFIG_INCLUDE_DIRS})

pkg_search_module(NETTLE nettle)
if(NOT ${NETTLE_FOUND})
  message("PACKAGE nettle not found: some targets will fail")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

pkg_search_module(NETTLE nettle)
if(NOT ${NETTLE_FOUND})
  message( FATAL_ERROR "PACKAGE nettle not found: some targets will fail. Run SCRIPTS/build_mme -i or SCRIPTS/build_hss -i  again!")
else()
  include_directories(${NETTLE_INCLUDE_DIRS})
endif()

message ("NETTLE VERSION_INSTALLED  = ${NETTLE_VERSION}")

string(REGEX REPLACE "([0-9]+).*" "\\1" NETTLE_VERSION_MAJOR ${NETTLE_VERSION})
string(REGEX REPLACE "[0-9]+\\.([0-9]+).*" "\\1" NETTLE_VERSION_MINOR ${NETTLE_VERSION})
message ("NETTLE_VERSION_MAJOR = ${NETTLE_VERSION_MAJOR}")
message ("NETTLE_VERSION_MINOR = ${NETTLE_VERSION_MINOR}")

if ("${NETTLE_VERSION_MAJOR}" STREQUAL "" OR "${NETTLE_VERSION_MINOR}" STREQUAL "")
  message( FATAL_ERROR "The nettle version not detected properly. Try to run SCRIPTS/build_mme -i or SCRIPTS/build_hss -i again" )
endif()

add_definitions("-DNETTLE_VERSION_MAJOR=${NETTLE_VERSION_MAJOR}")
add_definitions("-DNETTLE_VERSION_MINOR=${NETTLE_VERSION_MINOR}")
##############################################

###  for log 
add_library( LOG STATIC
  ${SRC_DIR}/common/logger.cpp)
add_library( OPTIONS STATIC
  ${SRC_DIR}/common/options.cpp)
###end for log


file(GLOB SRCS
    ${SRC_DIR}/api/*.cpp
    ${SRC_DIR}/impl/*.cpp
    ${SRC_DIR}/model/*.cpp
    ${SRC_DIR}/5gaka/*.cpp
    ${SRC_DIR}/common/*.cpp
    ${SRC_DIR}/utils/*.cpp
    ${SRC_DIR}/utils/*.c
    ${SRC_DIR}/asnlib/*.c
    ${SRC_DIR}/udm_app/*.cpp
)

add_executable(${PROJECT_NAME} ${SRCS} )
target_link_libraries(${PROJECT_NAME} LOG OPTIONS pistache pthread gmp config++ ${NETTLE_LIBRARIES} curl)
