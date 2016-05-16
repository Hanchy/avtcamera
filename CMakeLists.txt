cmake_minimum_required(VERSION 2.4)
project(AVTCam)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ggdb3 -march=native")
add_definitions(-DUSER_SHARED_POINTER)

set(VIMBA "/home/fans/Vimba_2_0")
set(VMB_LIBS
  ${VIMBA}/VimbaCPP/DynamicLib/x86_64bit/libVimbaC.so
  ${VIMBA}/VimbaCPP/DynamicLib/x86_64bit/libVimbaCPP.so
  )
set(VMB_INCS
  "${VIMBA}"
  #"${VIMBA}/VimbaC/Include "
  )

#add_executable(ListCams "${PROJECT_SOURCE_DIR}/src/ListCameras.cpp")
#target_link_libraries(ListCams ${VMB_LIBS})

set(VMB_COMMONS "${VIMBA}/VimbaCPP/Examples")
list(APPEND VMB_INCS ${VMB_COMMONS})
include_directories(${VMB_INCS})

find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
  include_directories(${OpenCV_INCLUDES})
endif()


# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#   PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "dir='${dir}'")
# endforeach()

set(SRC
  ${PROJECT_SOURCE_DIR}/src/ApiController.cpp
  ${PROJECT_SOURCE_DIR}/src/CameraObserver.cpp
  ${PROJECT_SOURCE_DIR}/src/FrameObserver.cpp
  ${PROJECT_SOURCE_DIR}/src/FindCorners.cpp
  ${PROJECT_SOURCE_DIR}/src/ReadAVT.cpp
  )

#add_library(DiscoverGigECameras OBJECT ${VMB_COMMONS}/DiscoverGigECameras.c)
#add_library(ErrorCodeToMessage OBJECT ${VMB_COMMONS}/ErrorCodeToMessage.c)
# add_library(Common STATIC ${VMB_COMMONS}/ErrorCodeToMessage.c)

add_executable(ReadAVT ${SRC})

target_link_libraries(ReadAVT ${VMB_LIBS} ${OpenCV_LIBS})
