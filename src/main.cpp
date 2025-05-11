#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

int main() {
    // Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW" << std::endl;
        return -1;
    }

    // Crear ventana
    GLFWwindow* window = glfwCreateWindow(800, 600, "Quarantine Engine", nullptr, nullptr);
    if (!window) {
        std::cerr << "No se pudo crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Usar GLM para operaciones matemáticas
    glm::vec3 vector(1.0f, 2.0f, 3.0f);
    std::cout << "Vector GLM: " << vector.x << ", " << vector.y << ", " << vector.z << std::endl;

    // Loop de renderizado
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
