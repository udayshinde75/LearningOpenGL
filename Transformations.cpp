//Open GL Program with transformation matrices

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* VertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCord;
uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(aPos, 1.0);
    TexCord = aTex;
}
)";

const char* FragmentShaderSource = R"(
#version 410 core
out vec4 FragColor;
in vec2 TexCord;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
    vec4 Tex1 = texture(texture1, TexCord);
    vec4 Tex2 = texture(texture2, TexCord);

    FragColor = mix(Tex1, Tex2, 0.3);
}
)";
float translateX(0.0f), translateY(0.0f);
float scaleX(1.0f), scaleY(1.0f);
float rotateX(0.0f), rotateY(0.0f), rotateZ(0.0f);
float angle(0.0f);


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
//Shader loader helpers
GLuint CompileShader(GLenum type, const char* source) {
    GLuint Shader = glCreateShader(type);
    glShaderSource(Shader, 1, &source, nullptr);
    glCompileShader(Shader);

    int success;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char info[1024];
        glGetShaderInfoLog(Shader, 1024, nullptr, info);
        std::cout << "Error in Compiling shader :" << info << std::endl;
    }

    return Shader;
}

GLuint CreateShaderProgram(const char* VertexShader, const char* FragmentShader) {
    GLuint VShader = CompileShader(GL_VERTEX_SHADER, VertexShader);
    GLuint FShader = CompileShader(GL_FRAGMENT_SHADER, FragmentShader);

    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VShader);
    glAttachShader(ShaderProgram, FShader);

    glLinkProgram(ShaderProgram);

    int success;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetProgramInfoLog(ShaderProgram, 1024, nullptr, info);
        std::cout << "Error in Shader Program :" << info << std::endl;
    }

    glDeleteShader(VShader);
    glDeleteShader(FShader);

    return ShaderProgram;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Ben10", nullptr, nullptr);

    if (window == nullptr) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize glad" << std::endl;
        glfwTerminate();
        return -1;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    float vertices[] = {
        // positions          // tex coords
        0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
       -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
       -0.5f,  0.5f, 0.0f,   0.0f, 1.0f
   };

    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);

    GLuint texture1, texture2;
    int w, h, c;
    GLenum Format;

    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load("/Users/udayshinde/Desktop/OpenGLWindow/Assets/Ben10Watch.jpg", &w, &h, &c, 0);

    if (!data) {
        std::cout << "Failed to load ben10watch" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "Loaded Ben10Watch.jpg assets" << std::endl;
    std::cout << "Height: " << h << " Width: " << w << " Channels: " << c << std::endl;
    Format = (c == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, Format, w, h, 0, Format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    data = stbi_load("/Users/udayshinde/Desktop/OpenGLWindow/Assets/Ben.jpg", &w, &h, &c, 0);
    if (!data) {
        std::cout << "Failed to load ben10watch" << std::endl;
        glfwTerminate();
        return -1;
    }
    std::cout << "Loaded Ben.jpg assets" << std::endl;
    std::cout << "Height: " << h << " Width: " << w << " Channels: " << c << std::endl;
    Format = (c == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, Format, w, h, 0, Format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint ShaderProgram = CreateShaderProgram(VertexShaderSource, FragmentShaderSource);
    glUseProgram(ShaderProgram);

    glUniform1i(glGetUniformLocation(ShaderProgram, "texture1"), 0);
    glUniform1i(glGetUniformLocation(ShaderProgram, "texture2"), 1);

    while (!glfwWindowShouldClose(window)) {
        ProcessInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        float time = glfwGetTime();
        float Scale = sin(time * 0.5f) * 0.3f;
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(translateX, translateY, 0.0f));

        if (rotateX != 0.0f || rotateY != 0.0f || rotateZ != 0.0f) {
            transform = glm::rotate(transform, angle,glm::vec3(rotateX, rotateY, rotateZ));
        }

        transform = glm::scale(transform, glm::vec3(scaleX, scaleY, 1.0f));

        glUseProgram(ShaderProgram);
        glUniformMatrix4fv(
            glGetUniformLocation(ShaderProgram, "transform"),
            1,
            GL_FALSE,
            glm::value_ptr(transform)
        );

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

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
        translateX = 0.0f;
        translateY = 0.0f;

        rotateX = 0.0f;
        rotateY = 0.0f;
        rotateZ = 0.0f;

        scaleX = 1.0f;
        scaleY = 1.0f;

        angle = 0.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (translateX < 1.0f) {
            translateX += 0.05f;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        if (translateX > -1.0f) {
            translateX -= 0.05f;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        if (translateY < 1.0f) {
            translateY += 0.05f;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        if (translateY > -1.0f) {
            translateY -= 0.05f;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        rotateX = 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        rotateY = 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        rotateZ = 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        scaleX += 0.05f;
        scaleY += 0.05f;
    }

    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        scaleX -= 0.05f;
        scaleY -= 0.05f;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        angle += 0.05f;
    }

}