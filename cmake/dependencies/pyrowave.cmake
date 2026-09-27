# PyroWave codec (https://github.com/Themaister/pyrowave)
#
# Links libpyrowave-shared as installed by PyroWave's own `cmake --install`:
#   <prefix>/include/pyrowave/pyrowave.h
#   <prefix>/lib/libpyrowave-shared.dll.a   (import library; .so on Linux)
#   <prefix>/bin/libpyrowave-shared-0.dll   (runtime, Windows)
# Point PYROWAVE_ROOT at <prefix>. pyrowave.h also needs the Vulkan headers
# (vulkan/vulkan.h); only the headers are used; PyroWave loads the Vulkan
# loader itself at runtime, so there is nothing extra to link.

if(NOT SUNSHINE_ENABLE_PYROWAVE)
    return()
endif()

set(_pyrowave_hints "")
if(PYROWAVE_ROOT)
    list(APPEND _pyrowave_hints "${PYROWAVE_ROOT}")
endif()
if(DEFINED ENV{PYROWAVE_ROOT})
    list(APPEND _pyrowave_hints "$ENV{PYROWAVE_ROOT}")
endif()

find_path(PYROWAVE_INCLUDE_DIR pyrowave/pyrowave.h
        HINTS ${_pyrowave_hints}
        PATH_SUFFIXES include)
find_library(PYROWAVE_LIBRARY
        NAMES pyrowave-shared libpyrowave-shared
        HINTS ${_pyrowave_hints}
        PATH_SUFFIXES lib lib64)
find_path(PYROWAVE_VULKAN_INCLUDE_DIR vulkan/vulkan.h
        HINTS "$ENV{VULKAN_SDK}" ${_pyrowave_hints}
        PATH_SUFFIXES include Include)

if(NOT PYROWAVE_INCLUDE_DIR OR NOT PYROWAVE_LIBRARY)
    message(FATAL_ERROR
            "SUNSHINE_ENABLE_PYROWAVE=ON but libpyrowave-shared was not found.\n"
            "  Build and install https://github.com/Themaister/pyrowave, then pass\n"
            "  -DPYROWAVE_ROOT=<install prefix>, or configure with -DSUNSHINE_ENABLE_PYROWAVE=OFF.")
endif()
if(NOT PYROWAVE_VULKAN_INCLUDE_DIR)
    message(FATAL_ERROR
            "SUNSHINE_ENABLE_PYROWAVE=ON but vulkan/vulkan.h was not found.\n"
            "  Install the Vulkan headers (msys2: mingw-w64-ucrt-x86_64-vulkan-headers) or set VULKAN_SDK.")
endif()

if(WIN32)
    get_filename_component(_pyrowave_prefix "${PYROWAVE_INCLUDE_DIR}" DIRECTORY)
    find_file(PYROWAVE_RUNTIME_DLL
            NAMES libpyrowave-shared-0.dll libpyrowave-shared.dll
            HINTS ${_pyrowave_hints} "${_pyrowave_prefix}"
            PATH_SUFFIXES bin
            NO_DEFAULT_PATH)
    if(NOT PYROWAVE_RUNTIME_DLL)
        message(FATAL_ERROR "Found the PyroWave import library but not libpyrowave-shared-0.dll under <prefix>/bin.")
    endif()
    message(STATUS "PyroWave runtime: ${PYROWAVE_RUNTIME_DLL}")
endif()

message(STATUS "PyroWave: ${PYROWAVE_LIBRARY} (headers ${PYROWAVE_INCLUDE_DIR})")

list(APPEND PYROWAVE_INCLUDE_DIRS "${PYROWAVE_INCLUDE_DIR}" "${PYROWAVE_VULKAN_INCLUDE_DIR}")
list(APPEND SUNSHINE_EXTERNAL_LIBRARIES "${PYROWAVE_LIBRARY}")
list(APPEND SUNSHINE_DEFINITIONS SUNSHINE_ENABLE_PYROWAVE=1)

unset(_pyrowave_hints)
unset(_pyrowave_prefix)
