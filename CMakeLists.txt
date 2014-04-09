cmake_minimum_required(VERSION 2.8)
project(obs_recording_notifier)

# http://www.cmake.org/pipermail/cmake/2008-November/025575.html
add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_FLAGS "/Wall /EHsc")

find_package(Boost REQUIRED)

include_external_msproject(libsamplerate "${OBS}/libsamplerate/libsamplerate.vcxproj")
include_external_msproject(OBSApi "${OBS}/OBSApi/OBSApi.vcxproj")
add_dependencies(OBSApi libsamplerate)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(WINSDK_LIBRARIES_DIR "${WINSDK}/Lib/x64")
else()
    set(WINSDK_LIBRARIES_DIR "${WINSDK}/Lib")
endif()
find_library(WINSDK_SAPI_LIBRARY "sapi" "${WINSDK_LIBRARIES_DIR}")
set(WINSDK_LIBRARIES ${WINSDK_SAPI_LIBRARY})

add_subdirectory(src)
