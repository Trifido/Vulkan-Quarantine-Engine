# Asegúrate de poner esto al principio del archivo
cmake_minimum_required(VERSION 3.5)

project(QuarantineEngine)

# Configuración básica del proyecto
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Incluir FetchContent para manejar dependencias
include(FetchContent)

# GLFW: Declaramos la dependencia
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4  # Versión 3.4 de GLFW
)
FetchContent_MakeAvailable(glfw)

# GLM: Declaramos la dependencia
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1  # Versión 1.0.1 de GLM
)
FetchContent_MakeAvailable(glm)

# Agregar el código fuente
add_executable(QuarantineEngine
  src/main.cpp  # Tu archivo principal
)

# Vincular las librerías al ejecutable
target_link_libraries(QuarantineEngine PRIVATE glfw)

# Incluir directorios de cabeceras
target_include_directories(QuarantineEngine PRIVATE ${glm_SOURCE_DIR})
