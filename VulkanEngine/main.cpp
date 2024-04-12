#include "Renderer/VulkanCore.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

using namespace renderer;

GLFWwindow* init()
{
    glfwInit();

    if (!vulkan::init())
        return nullptr;

    // disabling OpenGL API
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window{ glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr) };

    return window;
}


void shutdown(GLFWwindow* window)
{

    vulkan::shut_down();

    if (!window)
        glfwDestroyWindow(window);

    glfwTerminate();
}

int main() 
{
    GLFWwindow* window = init();
    if (!window)
        shutdown(window);

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    shutdown(window);
    return 0;
}


