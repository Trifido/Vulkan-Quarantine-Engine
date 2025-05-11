#include <iostream>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ImGui
#include <imgui.h>

// stb
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Bullet
#include <btBulletDynamicsCommon.h>

// meshoptimizer
#include <meshoptimizer.h>

// SPIRV-Reflect
#include <spirv_reflect.h>

int main() {
    std::cout << "Quarantine Engine - Dependency Check\n";

    // GLFW: initialize and terminate immediately
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwTerminate();

    // GLM: use a matrix
    glm::mat4 projection = glm::perspective(45.0f, 1.6f, 0.1f, 100.0f);
    std::cout << "GLM matrix[0][0]: " << projection[0][0] << "\n";

    // ImGui: init context and shutdown
    ImGui::CreateContext();
    ImGui::DestroyContext();

    // stb_image: dummy load (invalid path, just to link symbols)
    int width, height, channels;
    stbi_uc* data = stbi_load("dummy.jpg", &width, &height, &channels, 0);
    stbi_image_free(data); // even if null

    // Assimp: just create importer
    Assimp::Importer importer;

    // Bullet: create empty world
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher(&config);
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world(&dispatcher, &broadphase, &solver, &config);
    world.setGravity(btVector3(0, -9.8, 0));

    // meshoptimizer: dummy optimization
    unsigned int indices[] = {0, 1, 2, 2, 3, 0};
    meshopt_optimizeVertexCache(indices, indices, 6, 4);

    // SPIRV-Reflect: dummy usage
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(0, nullptr, &module);
    if (result == SPV_REFLECT_RESULT_SUCCESS) {
        spvReflectDestroyShaderModule(&module);
    }

    std::cout << "All dependencies included and minimal usage succeeded.\n";
    return 0;
}
