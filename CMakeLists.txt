cmake_minimum_required(VERSION 2.8)
project(obs_recording_notifier)
option(OBS "OBS" "")

set(CMAKE_CXX_FLAGS "/Wall")
include_external_msproject(libsamplerate "${OBS_ROOT}/libsamplerate/libsamplerate.vcxproj")
include_external_msproject(OBSApi "${OBS_ROOT}/OBSApi/OBSApi.vcxproj")
add_dependencies(OBSApi libsamplerate)
add_subdirectory(src)
