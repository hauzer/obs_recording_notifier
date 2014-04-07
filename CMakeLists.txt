cmake_minimum_required(VERSION 2.8)
project(obs_recording_notifier)

# http://www.cmake.org/pipermail/cmake/2008-November/025575.html
add_definitions(-DUNICODE -D_UNICODE)
set(CMAKE_CXX_FLAGS "/Wall")

include_external_msproject(libsamplerate "${OBS}/libsamplerate/libsamplerate.vcxproj")
include_external_msproject(OBSApi "${OBS}/OBSApi/OBSApi.vcxproj")
add_dependencies(OBSApi libsamplerate)

find_library(SAPI_LIBRARIES "sapi" "${SAPI}/Lib/i386")

add_subdirectory(src)
