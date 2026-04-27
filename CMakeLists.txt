cmake_minimum_required(VERSION 3.16)
project(QuarantineEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")
endif()

set(BUILD_TESTING OFF CACHE BOOL "Disable third-party test/dashboard targets" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

include(FetchContent)
include(GNUInstallDirs)

# ------------------------------
# Vulkan SDK
# ------------------------------
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
  message(FATAL_ERROR "Vulkan SDK not found. Please install it and ensure VULKAN_SDK is set.")
else()
  message(STATUS "Found Vulkan: ${Vulkan_LIBRARY} (version ${Vulkan_VERSION})")
endif()

# ------------------------------
# FetchContent dependencies
# ------------------------------

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
set_target_properties(glfw PROPERTIES DEBUG_POSTFIX "d")

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

FetchContent_Declare(
  ktx
  GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
  GIT_TAG v4.4.2
  GIT_SHALLOW TRUE
)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_VK_UPLOAD ON CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ktx)

# ------------------------------
# Submodules as libraries
# ------------------------------

set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_SHARED_LIBRARY ON CACHE BOOL "" FORCE)
set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(JPH_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
add_subdirectory(extern/jolt/Build EXCLUDE_FROM_ALL)

set(ASSIMP_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/assimp EXCLUDE_FROM_ALL)

file(GLOB SPIRV_REFLECT_SRC
  extern/SPIRV-Reflect/spirv_reflect.c
  extern/SPIRV-Reflect/spirv_reflect.h
)
add_library(SPIRV-Reflect STATIC ${SPIRV_REFLECT_SRC})
target_include_directories(SPIRV-Reflect PUBLIC extern/SPIRV-Reflect)

set(MESHOPT_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/meshoptimizer EXCLUDE_FROM_ALL)

# ------------------------------
# ImGui
# ------------------------------

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)
add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${glfw_SOURCE_DIR}/include
  ${Vulkan_INCLUDE_DIR}
)

# ------------------------------
# ImGuizmo
# ------------------------------

set(IMGUIZMO_DIR ${CMAKE_SOURCE_DIR}/extern/ImGuizmo)
add_library(imguizmo STATIC
  ${IMGUIZMO_DIR}/ImGuizmo.cpp
  ${IMGUIZMO_DIR}/GraphEditor.cpp
)
target_include_directories(imguizmo PUBLIC
  ${IMGUIZMO_DIR}
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)
target_link_libraries(imguizmo PUBLIC imgui)

# ------------------------------
# Source collection helpers
# ------------------------------

function(qe_collect_source_tree ROOT_DIR OUT_SRC OUT_HDR OUT_INC)
  file(GLOB_RECURSE TREE_SRC CONFIGURE_DEPENDS
    ${ROOT_DIR}/*.cpp
  )
  file(GLOB_RECURSE TREE_HDR CONFIGURE_DEPENDS
    ${ROOT_DIR}/*.h
    ${ROOT_DIR}/*.hpp
  )

  set(TREE_INC ${ROOT_DIR})
  foreach(FILE ${TREE_SRC} ${TREE_HDR})
    get_filename_component(DIR "${FILE}" DIRECTORY)
    list(APPEND TREE_INC "${DIR}")
  endforeach()

  list(REMOVE_DUPLICATES TREE_INC)

  set(${OUT_SRC} ${TREE_SRC} PARENT_SCOPE)
  set(${OUT_HDR} ${TREE_HDR} PARENT_SCOPE)
  set(${OUT_INC} ${TREE_INC} PARENT_SCOPE)
endfunction()

function(qe_assign_source_groups GROUP_ROOT ROOT_DIR)
  foreach(FILE ${ARGN})
    file(RELATIVE_PATH REL_PATH "${ROOT_DIR}" "${FILE}")
    get_filename_component(DIR "${REL_PATH}" PATH)
    if (DIR STREQUAL "")
      set(GROUP "${GROUP_ROOT}")
    else()
      string(REPLACE "/" "\\" GROUP "${DIR}")
      set(GROUP "${GROUP_ROOT}\\${GROUP}")
    endif()
    source_group("${GROUP}" FILES "${FILE}")
  endforeach()
endfunction()

function(qe_configure_msvc TARGET_NAME)
  if (MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /Zc:__cplusplus /MP)
    set_target_properties(${TARGET_NAME}
      PROPERTIES
        VS_GLOBAL_MultiProcessorCompilation "true"
    )
  endif()
endfunction()

qe_collect_source_tree("${CMAKE_SOURCE_DIR}/src/QuarantineEngine" QE_SRC QE_HDR QE_INC)
qe_collect_source_tree("${CMAKE_SOURCE_DIR}/src/QuarantineEditor" QE_EDITOR_SRC QE_EDITOR_HDR QE_EDITOR_INC)

# ------------------------------
# QuarantineEngine target
# ------------------------------

add_library(QuarantineEngine SHARED ${QE_SRC} ${QE_HDR})
qe_configure_msvc(QuarantineEngine)
qe_assign_source_groups("QuarantineEngine" "${CMAKE_SOURCE_DIR}/src/QuarantineEngine" ${QE_SRC} ${QE_HDR})

if (WIN32)
  set_target_properties(QuarantineEngine PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_include_directories(QuarantineEngine
  PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/QuarantineEngine/Utilities
    ${QE_INC}
    ${glfw_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${yaml_cpp_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/extern/assimp/include
    ${CMAKE_SOURCE_DIR}/extern/SPIRV-Reflect
    ${CMAKE_SOURCE_DIR}/extern
    ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(QuarantineEngine
  PUBLIC
    glfw
    imgui
    Vulkan::Vulkan
    assimp
    Jolt
    SPIRV-Reflect
    meshoptimizer
    yaml-cpp
    ktx
)

target_compile_definitions(QuarantineEngine PUBLIC GLM_ENABLE_EXPERIMENTAL)

# ------------------------------
# QuarantineEditor target
# ------------------------------

add_executable(QuarantineEditor ${QE_EDITOR_SRC} ${QE_EDITOR_HDR})
qe_configure_msvc(QuarantineEditor)
qe_assign_source_groups("QuarantineEditor" "${CMAKE_SOURCE_DIR}/src/QuarantineEditor" ${QE_EDITOR_SRC} ${QE_EDITOR_HDR})

target_include_directories(QuarantineEditor
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${QE_EDITOR_INC}
)

target_link_directories(QuarantineEditor PRIVATE
  $<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/lib/Debug>
  $<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/lib/Release>
)

target_link_libraries(QuarantineEditor
  PRIVATE
    QuarantineEngine
    imguizmo
)

target_compile_definitions(QuarantineEditor PRIVATE GLM_ENABLE_EXPERIMENTAL)

# ------------------------------
# Visual Studio folders
# ------------------------------

function(assign_vs_folder FOLDER_NAME)
  if (NOT MSVC)
    return()
  endif()

  foreach(TARGET_NAME IN LISTS ARGN)
    if (TARGET ${TARGET_NAME})
      set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "${FOLDER_NAME}")
    endif()
  endforeach()
endfunction()

assign_vs_folder("Engine" QuarantineEngine)
assign_vs_folder("Editor" QuarantineEditor)
assign_vs_folder("Dependencies"
  Jolt
  SPIRV-Reflect
  assimp
  astcenc-avx2-static
  glfw
  glm
  imgui
  imguizmo
  ktx
  ktx_read
  ktx_version
  meshoptimizer
  objUtil
  obj_basisu_cbind
  yaml-cpp
  zlibstatic
)
assign_vs_folder("Dependencies/GLFW" update_mappings)

# ------------------------------
# Post-build: copy runtime DLLs
# ------------------------------

if (WIN32)
  add_custom_command(TARGET QuarantineEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:QuarantineEngine>
      $<TARGET_FILE:assimp>
      $<TARGET_FILE:Jolt>
      $<TARGET_FILE:meshoptimizer>
      $<TARGET_FILE:yaml-cpp>
      $<TARGET_FILE:glfw>
      $<TARGET_FILE_DIR:QuarantineEditor>
  )
endif()

if (MSVC)
  function(exclude_from_vs_build tgt)
    if (TARGET ${tgt})
      set_target_properties(${tgt}
        PROPERTIES
          EXCLUDE_FROM_DEFAULT_BUILD TRUE
      )
    endif()
  endfunction()

  foreach(dep
    glfw
    glm
    stb
    assimp
    SPIRV-Reflect
    meshoptimizer
    imgui
    imguizmo
    zlibstatic
  )
    exclude_from_vs_build(${dep})
  endforeach()

  set_property(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT QuarantineEditor
  )
endif()
