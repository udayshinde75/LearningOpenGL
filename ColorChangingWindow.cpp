#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>

using namespace std;

//Callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);

float R(0.0f), G(0.0f), B(0.0f), A(1.0f);

int main() {
    //Initiating GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Color Changing Window", nullptr, nullptr);

    if (window == nullptr) {
        cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    } else {
        cout << "Window created\n";
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        cout << "Failed to initialise GLAD\n";
        glfwTerminate();
        return -1;
    } else {
        cout << "Loaded GLAD\n";
    }

    glViewport(0, 0 ,800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    while (!glfwWindowShouldClose(window)) {
        ProcessInput(window);

        glClearColor(R, G, B, A);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        cout << "ESC pressed\n";
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (R < 1.0f || G < 1.0f || B < 1.0f) {
            R = R + 0.001f;
            G = G + 0.001f;
            B = B + 0.001f;
        }
        else {
            cout << "Max Limit\n";
        }
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        if (R > 0.1f || G > 0.1f || B > 0.1f) {
            R = R - 0.001f;
            G = G - 0.001f;
            B = B - 0.001f;
        }
        else {
            cout << "Min Limit\n";
        }
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        R = R + 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        G = G + 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        B = B + 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        R = R - 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
        G = G - 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
        B = B - 0.001f;
    }

    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        cout << "Restored colors\n";
        R = 0.0f , G = 0.0f, B = 0.0f, A =1.0f;
    }
}