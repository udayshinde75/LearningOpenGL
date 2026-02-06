#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

const unsigned int SCR_WIDTH = 900;
const unsigned int SCR_HEIGHT = 600;

const char* LightingVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* LightingFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

// Specular
float specularStrength = 0.5;
vec3 viewDir = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
vec3 specular = specularStrength * spec * lightColor;


    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}

)";

const char* LightCubeVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

const char* LightCubeFragmentShader = R"(
#version 330 core
out vec4 FragColor;
void main() { FragColor = vec4(1.0, 1.0, 1.0, 1.0); }

)";

float LightX(0.0f);
float LightY(0.0f);
float LightZ(25.0f);
void ProcessInput(GLFWwindow* window);

unsigned int loadShader(const char* vsPath, const char* fsPath)
{
    const char* vShaderCode = vsPath;
    const char* fShaderCode = fsPath;

    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Real Lighting",NULL,NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    float vertices[] = {
        // positions         // normals
        -0.5,-0.5,-0.5,  0,0,-1,  0.5,-0.5,-0.5,  0,0,-1,  0.5,0.5,-0.5,  0,0,-1,
         0.5,0.5,-0.5,  0,0,-1, -0.5,0.5,-0.5,  0,0,-1, -0.5,-0.5,-0.5,  0,0,-1,

        -0.5,-0.5,0.5,  0,0,1,   0.5,-0.5,0.5,  0,0,1,   0.5,0.5,0.5,  0,0,1,
         0.5,0.5,0.5,  0,0,1,  -0.5,0.5,0.5,  0,0,1,  -0.5,-0.5,0.5,  0,0,1,

        -0.5,0.5,0.5, -1,0,0,  -0.5,0.5,-0.5,-1,0,0,  -0.5,-0.5,-0.5,-1,0,0,
        -0.5,-0.5,-0.5,-1,0,0, -0.5,-0.5,0.5,-1,0,0,  -0.5,0.5,0.5,-1,0,0,

         0.5,0.5,0.5, 1,0,0,   0.5,0.5,-0.5,1,0,0,   0.5,-0.5,-0.5,1,0,0,
         0.5,-0.5,-0.5,1,0,0,  0.5,-0.5,0.5,1,0,0,   0.5,0.5,0.5,1,0,0,

        -0.5,-0.5,-0.5,0,-1,0,  0.5,-0.5,-0.5,0,-1,0,  0.5,-0.5,0.5,0,-1,0,
         0.5,-0.5,0.5,0,-1,0, -0.5,-0.5,0.5,0,-1,0, -0.5,-0.5,-0.5,0,-1,0,

        -0.5,0.5,-0.5,0,1,0,   0.5,0.5,-0.5,0,1,0,   0.5,0.5,0.5,0,1,0,
         0.5,0.5,0.5,0,1,0,  -0.5,0.5,0.5,0,1,0,  -0.5,0.5,-0.5,0,1,0
    };

    unsigned int VBO, cubeVAO, lightVAO;
    glGenVertexArrays(1,&cubeVAO);
    glGenVertexArrays(1,&lightVAO);
    glGenBuffers(1,&VBO);

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(lightVAO);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    auto lightingShader = loadShader(LightingVertexShader,LightingFragmentShader);
    auto lightCubeShader = loadShader(LightCubeVertexShader,LightCubeFragmentShader);

    glm::vec3 lightPos;

    while(!glfwWindowShouldClose(window))
    {
        float time = glfwGetTime();
        ProcessInput(window);
        LightX = 25*sin(time*2.0f);
        LightZ = 25*cos(time*2.0f);
        lightPos = glm::vec3(LightX,LightY,LightZ);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(60.0f),(float)SCR_WIDTH/SCR_HEIGHT,0.1f,100.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f),glm::vec3(0,0,-15));

        glUseProgram(lightingShader);
        glUniform3f(glGetUniformLocation(lightingShader,"lightPos"),lightPos.x,lightPos.y,lightPos.z);
        glUniform3f(glGetUniformLocation(lightingShader,"lightColor"),1,1,1);
        glUniform3f(glGetUniformLocation(lightingShader,"objectColor"),0.1f,0.5f,0.5f);
        glUniform3f(glGetUniformLocation(lightingShader,"viewPos"),0.0f, 0.0f, 15.0f);

        time = time * 0.1;
        glm::mat4 model(1.0f);
        model = glm::rotate(model, (time), glm::vec3(1, 1, 1));
        model = glm::scale(model,glm::vec3(10.05f));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES,0,36);

        glUseProgram(lightCubeShader);
        model = glm::translate(glm::mat4(1.0f),lightPos);
        model = glm::scale(model,glm::vec3(0.5f));
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES,0,36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window,GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window,GLFW_TRUE);
    }

    if (glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS) {
        LightX = LightX + 0.01f;
    }

    if (glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS) {
        LightX = LightX - 0.01f;
    }

    if (glfwGetKey(window,GLFW_KEY_2) == GLFW_PRESS) {
        LightY = LightY + 0.01f;
    }

    if (glfwGetKey(window,GLFW_KEY_3) == GLFW_PRESS) {
        LightY = LightY - 0.01f;
    }

    if (glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS) {
        LightZ = LightZ + 0.01f;
    }

    if (glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS) {
        LightZ = LightZ - 0.01f;
    }
}
