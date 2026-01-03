#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --------------------
// Window
// --------------------
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// --------------------
// Vertex Shader
// --------------------
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// --------------------
// Fragment Shader
// --------------------
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
}
)";

// --------------------
// Cube vertices (36)
// --------------------
float vertices[] = {
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,
     0.5f,-0.5f,-0.5f, 1.0f,0.0f,
     0.5f, 0.5f,-0.5f, 1.0f,1.0f,
     0.5f, 0.5f,-0.5f, 1.0f,1.0f,
    -0.5f, 0.5f,-0.5f, 0.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,0.0f,

    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,
     0.5f,-0.5f, 0.5f, 1.0f,0.0f,
     0.5f, 0.5f, 0.5f, 1.0f,1.0f,
     0.5f, 0.5f, 0.5f, 1.0f,1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,1.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,

    -0.5f, 0.5f, 0.5f, 1.0f,0.0f,
    -0.5f, 0.5f,-0.5f, 1.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,1.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,1.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f,0.0f,

     0.5f, 0.5f, 0.5f, 1.0f,0.0f,
     0.5f, 0.5f,-0.5f, 1.0f,1.0f,
     0.5f,-0.5f,-0.5f, 0.0f,1.0f,
     0.5f,-0.5f,-0.5f, 0.0f,1.0f,
     0.5f,-0.5f, 0.5f, 0.0f,0.0f,
     0.5f, 0.5f, 0.5f, 1.0f,0.0f,

    -0.5f,-0.5f,-0.5f, 0.0f,1.0f,
     0.5f,-0.5f,-0.5f, 1.0f,1.0f,
     0.5f,-0.5f, 0.5f, 1.0f,0.0f,
     0.5f,-0.5f, 0.5f, 1.0f,0.0f,
    -0.5f,-0.5f, 0.5f, 0.0f,0.0f,
    -0.5f,-0.5f,-0.5f, 0.0f,1.0f,

    -0.5f, 0.5f,-0.5f, 0.0f,1.0f,
     0.5f, 0.5f,-0.5f, 1.0f,1.0f,
     0.5f, 0.5f, 0.5f, 1.0f,0.0f,
     0.5f, 0.5f, 0.5f, 1.0f,0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f,0.0f,
    -0.5f, 0.5f,-0.5f, 0.0f,1.0f
};

// --------------------
// Cube positions
// --------------------
glm::vec3 cubePositions[] = {
    {0,0,0}, {30,30,-50}, {-1, -1, 0}, {1, 1, 0}, {1, -1, 0}, {-1, 1, 0}
};

float aspectRatio(0.0f);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"10 Cubes",nullptr,nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    aspectRatio = fbWidth / fbHeight;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glEnable(GL_DEPTH_TEST);

    // Shader compilation
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSource,nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSource,nullptr);
    glCompileShader(fs);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram,vs);
    glAttachShader(shaderProgram,fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // VAO / VBO
    unsigned int VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture
    unsigned int texture;
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    int w,h,c;
    unsigned char* data = stbi_load("/Users/udayshinde/Desktop/OpenGLWindow/Assets/Ben10Watch.jpg",&w,&h,&c,0);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram,"texture1"),0);

    while(!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0,0,-3));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),aspectRatio,0.1f,100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        for(int i=0;i<6;i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model,cubePositions[i]);
            model = glm::rotate(model,(float)glfwGetTime()*glm::radians(20.0f*(i+1)),glm::vec3(1,0.3,0.5));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"model"),
                               1,GL_FALSE,glm::value_ptr(model));

            glDrawArrays(GL_TRIANGLES,0,36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}
