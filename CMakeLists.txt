cmake_minimum_required(VERSION 3.16)
project(QuarantineEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Usar /MDd y /MD en MSVC
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

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

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# stb
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# ------------------------------
# Submodules as libraries
# ------------------------------

set(BUILD_SHARED_LIBS OFF)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/bullet3 EXCLUDE_FROM_ALL)

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
# Source Files
# ------------------------------

file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE ENGINE_HDR CONFIGURE_DEPENDS src/*.h)
add_executable(QuarantineEngine ${ENGINE_SRC} ${ENGINE_HDR})

if (MSVC)
  target_compile_options(QuarantineEngine PRIVATE /Zc:__cplusplus)
  add_compile_options(/MP)
endif()

# + /MP para que cl.exe compile en paralelo
if(MSVC)
  target_compile_options(QuarantineEngine PRIVATE /MP)
  # Fuerza en el .vcxproj el checkbox de multiprocesador
  set_target_properties(QuarantineEngine
    PROPERTIES
      VS_GLOBAL_MultiProcessorCompilation "true"
  )
endif()

# ------------------------------
# Visual Studio Folder Grouping
# ------------------------------

foreach(FILE ${ENGINE_SRC} ${ENGINE_HDR})
  file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}/src" "${FILE}")
  get_filename_component(DIR "${REL_PATH}" PATH)
  string(REPLACE "/" "\\" GROUP "${DIR}")
  source_group("${GROUP}" FILES "${FILE}")
endforeach()

# ------------------------------
# Include Paths
# ------------------------------

target_include_directories(QuarantineEngine PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/App
  ${CMAKE_SOURCE_DIR}/src/Data
  ${CMAKE_SOURCE_DIR}/src/Data/Dtos
  ${CMAKE_SOURCE_DIR}/src/Draw
  ${CMAKE_SOURCE_DIR}/src/Editor
  ${CMAKE_SOURCE_DIR}/src/GraphicsPipeline
  ${CMAKE_SOURCE_DIR}/src/GUI
  ${CMAKE_SOURCE_DIR}/src/Input
  ${CMAKE_SOURCE_DIR}/src/Memory
  ${CMAKE_SOURCE_DIR}/src/Presentation
  ${CMAKE_SOURCE_DIR}/src/Presentation/Grid
  ${CMAKE_SOURCE_DIR}/src/RayTracing
  ${CMAKE_SOURCE_DIR}/src/SetUp
  ${CMAKE_SOURCE_DIR}/src/Templates
  ${CMAKE_SOURCE_DIR}/src/Utilities
  ${CMAKE_SOURCE_DIR}/src/Utilities/Animation
  ${CMAKE_SOURCE_DIR}/src/Utilities/Atmosphere
  ${CMAKE_SOURCE_DIR}/src/Utilities/Camera
  ${CMAKE_SOURCE_DIR}/src/Utilities/Compute
  ${CMAKE_SOURCE_DIR}/src/Utilities/Controller
  ${CMAKE_SOURCE_DIR}/src/Utilities/General
  ${CMAKE_SOURCE_DIR}/src/Utilities/Geometry
  ${CMAKE_SOURCE_DIR}/src/Utilities/Light
  ${CMAKE_SOURCE_DIR}/src/Utilities/Material
  ${CMAKE_SOURCE_DIR}/src/Utilities/Particles
  ${CMAKE_SOURCE_DIR}/src/Utilities/Physics

  ${glfw_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${CMAKE_SOURCE_DIR}/extern/assimp/include
  ${CMAKE_SOURCE_DIR}/extern/bullet3/src
  ${CMAKE_SOURCE_DIR}/extern/SPIRV-Reflect
  ${CMAKE_SOURCE_DIR}/extern
  ${Vulkan_INCLUDE_DIR}
)

# ------------------------------
# Linking
# ------------------------------

target_link_libraries(QuarantineEngine
  glfw
  imgui
  Vulkan::Vulkan
  assimp
  BulletDynamics
  BulletCollision
  LinearMath
  SPIRV-Reflect
  meshoptimizer
)

# ------------------------------
# Habilitar extensiones GLM
# ------------------------------

target_compile_definitions(QuarantineEngine PRIVATE GLM_ENABLE_EXPERIMENTAL)

# ------------------------------
# Ocultar dependencias en VS
# ------------------------------

function(hide_from_vs TARGET_NAME)
  if (MSVC AND TARGET ${TARGET_NAME})
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Dependencies")
  endif()
endfunction()

foreach(TARGET_NAME
  glfw imgui assimp BulletDynamics BulletCollision LinearMath
  SPIRV-Reflect meshoptimizer glm zlibstatic
)
  hide_from_vs(${TARGET_NAME})
endforeach()

# ------------------------------
# Post-build: copiar DLLs (solo necesarias)
# ------------------------------

if(WIN32)
  add_custom_command(TARGET QuarantineEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:assimp>
      $<TARGET_FILE:meshoptimizer>
      $<TARGET_FILE_DIR:QuarantineEngine>
  )
endif()

if (MSVC)
  # Función para excluir un target del build por defecto en VS
  function(exclude_from_vs_build tgt)
    if (TARGET ${tgt})
      set_target_properties(${tgt}
        PROPERTIES
          EXCLUDE_FROM_DEFAULT_BUILD TRUE
      )
    endif()
  endfunction()

  # Lista de tus dependencias que quieres compilar sólo una vez
  foreach(dep
    glfw
    glm
    stb
    assimp
    BulletDynamics
    BulletCollision
    LinearMath
    SPIRV-Reflect
    meshoptimizer
    imgui
    zlibstatic
  )
    exclude_from_vs_build(${dep})
  endforeach()
endif()