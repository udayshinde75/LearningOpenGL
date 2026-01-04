#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Vertex shader source
const char* VertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertex_Color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertex_Color = aColor;
}
)";

const char* FragmentShaderSource = R"(
#version 410 core
out vec4 FragColor;
in vec3 vertex_Color;

void main()
{
    FragColor = vec4(vertex_Color, 1.0);
}
)";

//Variables
bool DrawU = true;
bool DrawD = true;
bool DrawA = true;
bool DrawY = true;
float UtranslateZ = -50.0f;
float UtranslateX = -1.5f;
float DtranslateZ = -50.0f;
float DtranslateX = -0.5f;
float AtranslateZ = -50.0f;
float AtranslateX = 0.5f;
float YtranslateZ = -50.0f;
float YtranslateX = 1.5f;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
};

void addBox(
    std::vector<Vertex>& vertices,
    glm::vec3 min,
    glm::vec3 max
);

glm::vec3 depthGradient(float z, float zMin, float zMax);

//AspectRatio
float aspectRatio(0.0f);

//Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Main Function
int main() {
    //Initializing GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1400, 900, "My Name", nullptr, nullptr);

    if (window == nullptr) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "Created window successfully" << std::endl;

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to Initialize GLAD";
        glfwTerminate();
        return -1;
    }

    std::cout << "Initialize GLAD successfully";

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    aspectRatio = fbWidth / fbHeight;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //VertexShader and FragmentShader
    GLuint VertexShader, FragmentShader;
    int success;
    char infoLog[512];
    VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertexShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(VertexShader);
    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(VertexShader, 512, nullptr, infoLog);
        std::cout << "Vertex Shader Error : " << infoLog << std::endl;
    }

    FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragmentShader, 1, &FragmentShaderSource, nullptr);
    glCompileShader(FragmentShader);
    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(FragmentShader, 512, nullptr, infoLog);
        std::cout << "Fragment Shader Error : " << infoLog << std::endl;
    }

    //Shader Program
    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ShaderProgram, 512, nullptr, infoLog);
        std::cout << "Shader Program Error : " << infoLog << std::endl;
    }

    //VAO, VBO for letter U
    GLuint LetterU_VAO, LetterU_VBO;
    glGenVertexArrays(1, &LetterU_VAO);
    glGenBuffers(1, &LetterU_VBO);
    glBindVertexArray(LetterU_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, LetterU_VBO);
    std::vector<Vertex> U_vertices;

    float depth = 0.6f;

    // Left vertical bar
    addBox(U_vertices,{-0.3f, -0.5f, -depth},{-0.2f,  0.5f,  0.0f});

    // Right vertical bar
    addBox(U_vertices,{ 0.2f, -0.5f, -depth},{ 0.3f,  0.5f,  0.0f});

    // Bottom bar
    addBox(U_vertices,{-0.2f, -0.5f, -depth},{ 0.2f, -0.4f,  0.0f});

    glBufferData(GL_ARRAY_BUFFER,U_vertices.size() * sizeof(Vertex),U_vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    //VAO, VBO for letter D
    GLuint LetterD_VAO, LetterD_VBO;
    glGenVertexArrays(1, &LetterD_VAO);
    glGenBuffers(1, &LetterD_VBO);
    glBindVertexArray(LetterD_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, LetterD_VBO);
    std::vector<Vertex> D_vertices;

    // Left vertical bar
    addBox(D_vertices,{-0.3f, -0.5f, -depth},{-0.2f,  0.5f,  0.0f});

    // Right vertical bar
    addBox(D_vertices,{ 0.2f, -0.45f, -depth},{ 0.3f,  0.45f,  0.0f});

    // Bottom bar
    addBox(D_vertices,{-0.2f, -0.45f, -depth},{ 0.2f, -0.35f,  0.0f});

    // Top bar
    addBox(D_vertices,{-0.2f,  0.35f, -depth},{ 0.2f,  0.45f,  0.0f});

    glBufferData(GL_ARRAY_BUFFER,D_vertices.size() * sizeof(Vertex),D_vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    //VAO, VBO for letter A
    GLuint LetterA_VAO, LetterA_VBO;
    glGenVertexArrays(1, &LetterA_VAO);
    glGenBuffers(1, &LetterA_VBO);
    glBindVertexArray(LetterA_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, LetterA_VBO);
    std::vector<Vertex> A_vertices;

    // Left vertical bar
    addBox(A_vertices,{-0.3f, -0.5f, -depth},{-0.2f,  0.5f,  0.0f});

    // Right vertical bar
    addBox(A_vertices,{ 0.2f, -0.5f, -depth},{ 0.3f,  0.5f,  0.0f});

    // Middle bar
    addBox(A_vertices,{-0.2f, -0.05f, -depth},{ 0.2f, 0.05f,  0.0f});

    // Top bar
    addBox(A_vertices,{-0.2f,  0.4f, -depth},{ 0.2f,  0.5f,  0.0f});

    glBufferData(GL_ARRAY_BUFFER,A_vertices.size() * sizeof(Vertex),A_vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);


    //VAO, VBO for letter Y
    GLuint LetterY_VAO, LetterY_VBO;
    glGenVertexArrays(1, &LetterY_VAO);
    glGenBuffers(1, &LetterY_VBO);
    glBindVertexArray(LetterY_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, LetterY_VBO);
    std::vector<Vertex> Y_vertices;

    // Left vertical bar
    addBox(Y_vertices,{-0.3f, -0.0f, -depth},{-0.2f,  0.5f,  0.0f});

    // Right vertical bar
    addBox(Y_vertices,{ 0.2f, -0.0f, -depth},{ 0.3f,  0.5f,  0.0f});

    // Middle bar
    addBox(Y_vertices,{-0.2f, -0.05f, -depth},{ 0.2f, 0.05f,  0.0f});

    // central bar
    addBox(Y_vertices,{-0.05f, 0.0f, -depth},{0.05f,  -0.5f,  0.0f});

    glBufferData(GL_ARRAY_BUFFER,Y_vertices.size() * sizeof(Vertex),Y_vertices.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);


    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.9f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(ShaderProgram);

        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),aspectRatio,0.1f,100.0f);

        glUniformMatrix4fv(glGetUniformLocation(ShaderProgram,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(ShaderProgram,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        if (DrawU) {
            if (UtranslateZ <= 0.0f) {
                UtranslateZ += 0.05f;
            }

            glBindVertexArray(LetterU_VAO);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model,{UtranslateX, 0.0f, UtranslateZ});
            model = glm::rotate(model,(float)glfwGetTime()*glm::radians(UtranslateZ),glm::vec3(1,0.3,0.5));
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(U_vertices.size()));
        }


        if (DrawD && (float)glfwGetTime() > 2.0f) {
            if (DtranslateZ <= 0.0f) {
                DtranslateZ += 0.05f;
            }

            glBindVertexArray(LetterD_VAO);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model,{DtranslateX, 0.0f, DtranslateZ});
            model = glm::rotate(model,(float)glfwGetTime()*glm::radians(DtranslateZ),glm::vec3(1,0.3,0.5));
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(D_vertices.size()));
        }

        if (DrawA && (float)glfwGetTime() > 4.0f) {
            if (AtranslateZ <= 0.0f) {
                AtranslateZ += 0.05f;
            }

            glBindVertexArray(LetterA_VAO);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model,{AtranslateX, 0.0f, AtranslateZ});
            model = glm::rotate(model,(float)glfwGetTime()*glm::radians(AtranslateZ),glm::vec3(1,0.3,0.5));
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(A_vertices.size()));
        }

        if (DrawY && (float)glfwGetTime() > 6.0f) {
            if (YtranslateZ <= 0.0f) {
                YtranslateZ += 0.05f;
            }

            glBindVertexArray(LetterY_VAO);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model,{YtranslateX, 0.0f, YtranslateZ});
            model = glm::rotate(model,(float)glfwGetTime()*glm::radians(YtranslateZ),glm::vec3(1,0.3,0.5));
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgram,"model"),1,GL_FALSE,glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(Y_vertices.size()));
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

void addBox(
    std::vector<Vertex>& verts,
    glm::vec3 min,
    glm::vec3 max
) {
    auto push = [&](glm::vec3 p) {
        glm::vec3 color = depthGradient(p.z, min.z, max.z);
        verts.push_back({ p, color });
    };

    // Front (+Z)
    push({min.x, min.y, max.z});
    push({max.x, min.y, max.z});
    push({max.x, max.y, max.z});

    push({min.x, min.y, max.z});
    push({max.x, max.y, max.z});
    push({min.x, max.y, max.z});

    // Back (-Z)
    push({max.x, min.y, min.z});
    push({min.x, min.y, min.z});
    push({min.x, max.y, min.z});

    push({max.x, min.y, min.z});
    push({min.x, max.y, min.z});
    push({max.x, max.y, min.z});

    // Left (-X)
    push({min.x, min.y, min.z});
    push({min.x, min.y, max.z});
    push({min.x, max.y, max.z});

    push({min.x, min.y, min.z});
    push({min.x, max.y, max.z});
    push({min.x, max.y, min.z});

    // Right (+X)
    push({max.x, min.y, max.z});
    push({max.x, min.y, min.z});
    push({max.x, max.y, min.z});

    push({max.x, min.y, max.z});
    push({max.x, max.y, min.z});
    push({max.x, max.y, max.z});

    // Top (+Y)
    push({min.x, max.y, max.z});
    push({max.x, max.y, max.z});
    push({max.x, max.y, min.z});

    push({min.x, max.y, max.z});
    push({max.x, max.y, min.z});
    push({min.x, max.y, min.z});

    // Bottom (-Y)
    push({min.x, min.y, min.z});
    push({max.x, min.y, min.z});
    push({max.x, min.y, max.z});

    push({min.x, min.y, min.z});
    push({max.x, min.y, max.z});
    push({min.x, min.y, max.z});
}

glm::vec3 depthGradient(float z, float zMin, float zMax) {
    float t = (z - zMin) / (zMax - zMin);
    t = glm::clamp(t, 0.0f, 1.0f);

    // Front = bright, back = dark
    glm::vec3 frontColor = {1.0f, 1.0f, 1.0f};
    glm::vec3 backColor  = {0.2f, 0.2f, 0.6f};

    return glm::mix(frontColor, backColor, t);
}

