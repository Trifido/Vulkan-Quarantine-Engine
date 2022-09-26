#pragma once

#include "glm/glm.hpp"

//GEOMETRY
float cubevertices[] = {
    // positions          // normals           // texture coords
    0.0f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    0.0f, 0.0f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
    0.0f,  0.5f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

    //BIEN
    0.0f, 0.0f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, 0.0f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    0.0f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    0.0f, 0.0f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    //BIEN
    0.0f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    0.0f,  0.5f, 0.0f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    0.0f, 0.0f, 0.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    0.0f, 0.0f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    0.0f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    //BIEN
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f, 0.0f, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, 0.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, 0.0f, 0.0f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f, 0.0f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     //SUPERIOR
    0.0f, 0.0f, 0.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, 0.0f, 0.0f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, 0.0f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, 0.0f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    0.0f, 0.0f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
    //INFERIOR
    0.0f,  0.5f, 0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    0.0f,  0.5f, 0.0f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
    0.0f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
};

// positions all containers
glm::vec3 cubePositions[] = {
    glm::vec3(0.0f,  0.0f,  0.0f),
    glm::vec3(2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),
    glm::vec3(1.5f,  2.0f, -2.5f),
    glm::vec3(1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
};

//glm::vec3 pointLightPositions[] = {
//    glm::vec3(-3.0f, 0.0f, 0.0f),
//    glm::vec3(-1.0f, 0.0f, 0.0f),
//    glm::vec3(1.0f, 0.0f, 0.0f),
//    glm::vec3(3.0f, 0.0f, 0.0f),
//    glm::vec3(0.0f,  0.0f, 0.0f)
//};

glm::vec3 pointLightPositions[] = {
    glm::vec3(0.7f,  0.2f,  2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f,  2.0f, -2.0f),
    glm::vec3(0.0f,  0.0f, -3.0f)
};

float planeVertices[] = {
    // positions          // texture Coords 
     5.0f, -0.5f,  5.0f,  0.0f,  0.0f, 0.0f,  2.0f, 0.0f,
    -5.0f, -0.5f,  5.0f,  0.0f,  0.0f, 0.0f,  0.0f, 0.0f,
    -5.0f, -0.5f, -5.0f,  0.0f,  0.0f, 0.0f,  0.0f, 2.0f,

     5.0f, -0.5f,  5.0f,  0.0f,  0.0f, 0.0f,  2.0f, 0.0f,
    -5.0f, -0.5f, -5.0f,  0.0f,  0.0f, 0.0f,  0.0f, 2.0f,
     5.0f, -0.5f, -5.0f,  0.0f,  0.0f, 0.0f,  2.0f, 2.0f
};

//float floorVertices[] = {
//    // positions            // normals         // texcoords
//     25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
//    -25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
//    -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
//
//     25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
//    -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
//     25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 10.0f
//};


float floorVertices[] = {
    // positions            // normals         // texcoords
     10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  5.0f,  0.0f,
    -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
    -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,

     10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  5.0f,  0.0f,
     10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  5.0f, 5.0f,
    -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f
};

float transparentVertices[] = {
    // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
    0.0f,  0.5f,  0.0f,  0.0f,  0.0f, 0.0f,  0.0f,  0.0f,
    0.0f, -0.5f,  0.0f,  0.0f,  0.0f, 0.0f,  0.0f,  1.0f,
    1.0f, -0.5f,  0.0f,  0.0f,  0.0f, 0.0f,  1.0f,  1.0f,

    0.0f,  0.5f,  0.0f,  0.0f,  0.0f, 0.0f,  0.0f,  0.0f,
    1.0f, -0.5f,  0.0f,  0.0f,  0.0f, 0.0f,  1.0f,  1.0f,
    1.0f,  0.5f,  0.0f,  0.0f,  0.0f, 0.0f,  1.0f,  0.0f
};

float points[] = {
    -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // top-left
     0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // top-right
     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // bottom-right
    -0.5f, -0.5f, 1.0f, 1.0f, 0.0f  // bottom-left
};

float quadVertices[] = {
    // positions     // colors
    -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
     0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
    -0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

    -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
     0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
     0.05f,  0.05f,  0.0f, 1.0f, 1.0f
};


