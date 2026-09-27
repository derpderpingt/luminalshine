if(NOT SUNSHINE_ENABLE_PYROWAVE)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    granite
    GIT_REPOSITORY https://github.com/Themaister/Granite.git
    GIT_TAG 1b2d1801d2910fb09ebcded2f0bb3a3a781103b5
    GIT_SUBMODULES
        third_party/volk
        third_party/khronos/vulkan-headers
)

set(GRANITE_TOOLS OFF CACHE BOOL "" FORCE)
set(GRANITE_RENDERER OFF CACHE BOOL "" FORCE)
set(GRANITE_VULKAN_SPIRV_CROSS OFF CACHE BOOL "" FORCE)
set(GRANITE_VULKAN_SHADER_MANAGER_RUNTIME_COMPILER OFF CACHE BOOL "" FORCE)
set(GRANITE_VULKAN_FOSSILIZE OFF CACHE BOOL "" FORCE)
set(GRANITE_VULKAN_SYSTEM_HANDLES OFF CACHE BOOL "" FORCE)
set(GRANITE_VULKAN_DXGI_INTEROP ON CACHE BOOL "" FORCE)
set(GRANITE_RENDERDOC_CAPTURE OFF CACHE BOOL "" FORCE)
set(GRANITE_INSTALL_TARGETS OFF CACHE BOOL "" FORCE)
set(GRANITE_INSTALL_EXE_TARGETS OFF CACHE BOOL "" FORCE)
set(GRANITE_SHIPPING ON CACHE BOOL "" FORCE)
set(GRANITE_PLATFORM null CACHE STRING "" FORCE)
set(GRANITE_FFMPEG OFF CACHE BOOL "" FORCE)
set(GRANITE_FFMPEG_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(granite)

FetchContent_Declare(
    pyrowave
    GIT_REPOSITORY https://github.com/Themaister/pyrowave.git
    GIT_TAG 89f7e47d4abbf650c91fae766728af866c5e32a0
)

set(PYROWAVE_DEVEL OFF CACHE BOOL "" FORCE)
set(PYROWAVE_UTILS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pyrowave)

add_library(pyrowave-c-api STATIC
    "${pyrowave_SOURCE_DIR}/pyrowave_c.cpp"
    "${granite_SOURCE_DIR}/video/scaler.cpp"
)
target_include_directories(pyrowave-c-api
    PUBLIC "${pyrowave_SOURCE_DIR}"
    PRIVATE
        "${pyrowave_SOURCE_DIR}/shaders"
        "${granite_SOURCE_DIR}/video"
)
target_link_libraries(pyrowave-c-api
    PUBLIC pyrowave granite-vulkan granite-math granite-volk-headers
)
target_compile_definitions(pyrowave-c-api PUBLIC VK_USE_PLATFORM_WIN32_KHR)
target_compile_options(pyrowave-c-api PRIVATE ${SUNSHINE_COMPILE_OPTIONS})
set_target_properties(pyrowave-c-api PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

install(FILES "${pyrowave_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
    RENAME "PyroWave.txt"
)
install(FILES "${granite_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
    RENAME "Granite.txt"
)

list(APPEND SUNSHINE_EXTERNAL_LIBRARIES pyrowave-c-api)
list(APPEND SUNSHINE_DEFINITIONS SUNSHINE_ENABLE_PYROWAVE=1)
