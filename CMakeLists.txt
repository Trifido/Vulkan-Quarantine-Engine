cmake_minimum_required(VERSION 3.16)
project(QuarantineEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

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
# Submodules dependencies
# ------------------------------

# Bullet
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BULLET_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/extern/bullet3/src)
add_subdirectory(extern/bullet3)

# Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/assimp)

# SPIRV-Reflect
file(GLOB SPIRV_REFLECT_SRC
  extern/SPIRV-Reflect/spirv_reflect.c
)

add_library(SPIRV-Reflect STATIC ${SPIRV_REFLECT_SRC})

target_include_directories(SPIRV-Reflect PUBLIC
  extern/SPIRV-Reflect
)

# meshoptimizer
set(MESHOPT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/meshoptimizer)

# ImGui como submódulo (ya lo tienes en extern/imgui)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)

add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${glfw_SOURCE_DIR}/include  # Para evitar errores con imgui_impl_glfw
)

# ------------------------------
# Main Executable
# ------------------------------

add_executable(QuarantineEngine
  src/main.cpp
)

target_link_libraries(QuarantineEngine
  glfw
  imgui
  assimp
  BulletDynamics
  BulletCollision
  LinearMath
  SPIRV-Reflect
  meshoptimizer
)

target_include_directories(QuarantineEngine PRIVATE
  ${glfw_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${assimp_SOURCE_DIR}/include
  ${BULLET_INCLUDE_DIR}
  ${spirv_reflect_SOURCE_DIR}
)
