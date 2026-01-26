#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// Vertex and Fragment shaders for flag ------------------------ (start)
const char* VertexShaderSourceFlag = R"(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCord;

    out vec2 TexCord;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    uniform float time;

void main()
{
    vec3 newPos = aPos;

    float x = aPos.x;          // horizontal distance from pole
    float y = aPos.y;          // vertical for flutter
    float t = time;

    // base travel
    float mainWave =
        sin(x * 6.0 - t * 3.0) * 0.5 +
        sin(x * 12.0 - t * 6.0) * 0.25;

    // diagonal shear wave
    float diagonal =
        sin((x + y * 0.8) * 10.0 - t * 4.5) * 0.2;

    // flutter noise
    float flutter =
        sin((x * 25.0 + y * 15.0) - t * 10.0) * 0.08;

    // lag + stiffness (near pole no motion)
    float damp = pow(clamp(x, 0.0, 1.0), 1.7);

    float wave = (mainWave + diagonal + flutter) * damp;

    // depth displacement
    newPos.z += wave * 0.35;

    // slight vertical tension
    newPos.y += sin(x * 5.0 - t * 2.5) * 0.03 * damp;

    gl_Position = projection * view * model * vec4(newPos, 1.0);
    TexCord = aTexCord;
}
)";

const char* FragmentShaderSourceFlag = R"(
#version 410 core
out vec4 FragColor;
in vec2 TexCord;
uniform sampler2D Texture;

void main()
{
    FragColor = texture(Texture, TexCord);
}
)";
// Vertex and Fragment shaders for flag ------------------------ (end)

// Vertex and Fragment shaders for Base ------------------------ (Start)
const char* VertexShaderSourceBase = R"(
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

const char* FragmentShaderSourceBase = R"(
#version 410 core
out vec4 FragColor;
in vec3 vertex_Color;

void main()
{
    FragColor = vec4(vertex_Color, 1.0);
}
)";
// Vertex and Fragment shaders for Base ------------------------ (End)

// Vertex and Fragment shaders for sky ------------------------ (Start)
const char* VertexShaderSourceSky = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
out vec3 TexCoord;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoord = aPos;
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

const char* FragmentShaderSourceSky = R"(
#version 410 core
in vec3 TexCoord;
out vec4 FragColor;
uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoord);
}
)";
// Vertex and Fragment shaders for sky ------------------------ (End)

// Global variables ------------------ (start)
float AspectRatio(0.0f);
float camAngle = 0.0f;
float camRadius = 3.0f;
float camHeight = -50.0f;
float skyboxVertices[] = {
    -1,  1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1,  1,  1, -1, -1,  1, -1,
    -1, -1,  1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1,  1,  1, -1, -1,  1,
     1, -1, -1,  1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,  1, -1, -1,
    -1, -1,  1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,  1, -1, -1,  1,
    -1,  1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1,  1, -1,
    -1, -1, -1, -1, -1,  1,  1, -1, -1,  1, -1, -1, -1, -1,  1,  1, -1,  1
};
// Global variables ------------------ (end)

// Callbacks --------------------------- (start)
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
// Callbacks --------------------------- (end)

// structures ------------------ (start)
struct StructVertexFlag {
    glm::vec3 pos;
    glm::vec2 TextureCord;
};
struct StructVertexBox {
    glm::vec3 pos;
    glm::vec3 color;
};
// structures ------------------ (end)

// Function Declarations ---------------------- (start)
void CreateFlagVertices(
    std::vector<StructVertexFlag>& FlagVertices,
    glm::vec3 UpperLeft,
    glm::vec3 UpperRight,
    glm::vec3 BottomLeft,
    float Density
);
void AddStrip(
    std::vector<StructVertexFlag>& FlagVertices,
    glm::vec3 UpperLeft,
    glm::vec3 BottomRight,
    float Density,
    float StripNumber
);
GLuint CreateShaderProgram(const char* VertexShaderSource, const char* FragmentShaderSource);
void addBox(
    std::vector<StructVertexBox>& verts,
    glm::vec3 min,
    glm::vec3 max,
    glm::vec3 boxColor
);
GLuint loadCubemap(std::vector<std::string> faces);
// Function Declarations ---------------------- (end)

// Main function
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_OPENGL_CORE_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Flag", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        cout << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return 1;
    }

    int fbWidth, fbHeight;

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    AspectRatio = (float)fbWidth / (float)fbHeight;

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    cout << "Generating ShaderProgramFlag --- (start)" << endl;
    GLuint ShaderProgramFlag = CreateShaderProgram(VertexShaderSourceFlag, FragmentShaderSourceFlag);
    cout << "Generating ShaderProgramFlag --- (End)" << endl;
    // Flag vertex Data -------------- (start)
    GLuint VAOFlag, VBOFlag;
    glGenVertexArrays(1, &VAOFlag);
    glGenBuffers(1, &VBOFlag);
    glBindVertexArray(VAOFlag);
    glBindBuffer(GL_ARRAY_BUFFER, VBOFlag);

    std::vector<StructVertexFlag> FlagVertices;

    CreateFlagVertices(FlagVertices, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.5f, 0.5f, 0.0f), glm::vec3(0.0f, -0.5f, 0.0f), 100.0f);

    glBufferData(GL_ARRAY_BUFFER, FlagVertices.size()*sizeof(StructVertexFlag), FlagVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StructVertexFlag), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(StructVertexFlag), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    // Flag vertex Data -------------- (end)

    cout << "Generating ShaderProgramBase --- (start)" << endl;
    GLuint ShaderProgramBase = CreateShaderProgram(VertexShaderSourceBase, FragmentShaderSourceBase);
    cout << "Generating ShaderProgramBase --- (End)" << endl;

    // Pole vertex Data -------------- (start)
    GLuint VAOPole, VBOPole;
    glGenVertexArrays(1, &VAOPole);
    glGenBuffers(1, &VBOPole);
    glBindVertexArray(VAOPole);
    glBindBuffer(GL_ARRAY_BUFFER, VBOPole);

    std::vector<StructVertexBox> PoleVertices;

    addBox(PoleVertices,
        glm::vec3(-0.05f, -5.5f, -0.05f),
        glm::vec3( 0.05f,  0.5f,  0.05f),
        glm::vec3(0.7f, 0.7f, 0.7f)
    );

    addBox(PoleVertices,
        glm::vec3(-0.5f, -5.3f, -0.5f),
        glm::vec3( 0.5f,  -5.5f,  0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f)
    );

    glBufferData(GL_ARRAY_BUFFER, PoleVertices.size()*sizeof(StructVertexBox), PoleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StructVertexBox), static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StructVertexBox), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    // Pole vertex Data -------------- (end)

    cout << "Generating ShaderProgramSky --- (start)" << endl;
    GLuint ShaderProgramSky = CreateShaderProgram(VertexShaderSourceSky, FragmentShaderSourceSky);
    cout << "Generating ShaderProgramSky --- (end)" << endl;

    // Skybox VAO
    GLuint skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);

    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), nullptr);
    glBindVertexArray(0);

    std::vector<std::string> faces = {
        "/Users/udayshinde/Desktop/OpenGLWindow/Assets/right.jpg",
        "/Users/udayshinde/Desktop/OpenGLWindow/Assets/left.jpg",
        "/Users/udayshinde/Desktop/OpenGLWindow/Assets/top.jpg",
        "/Users/udayshinde/Desktop/OpenGLWindow/Assets/bottom.jpg",
        "/Users/udayshinde/Desktop/OpenGLWindow/Assets/front.jpg",
        "/Users/udayshinde/Desktop/OpenGLWindow/Assets/back.jpg"
        };
    GLuint cubemapTex = loadCubemap(faces);

    GLuint FlagTexture;
    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    glGenTextures(1, &FlagTexture);
    glBindTexture(GL_TEXTURE_2D, FlagTexture);

    unsigned char *data = stbi_load("/Users/udayshinde/Desktop/OpenGLWindow/Assets/Flag_of_India.png", &width, &height, &channels, 0);

    if (!data) {
        cout << "Error loading assets" << endl;
        return -1;
    }

    GLenum Format = (channels == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, Format, width, height, 0, Format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_DEPTH_TEST);

    int timeLoc = glGetUniformLocation(ShaderProgramFlag, "time");
    int ViewmatrixLocation = glGetUniformLocation(ShaderProgramFlag,"view");
    int ProjectionMatrixLocation = glGetUniformLocation(ShaderProgramFlag,"projection");
    int ModelMatrixLocation = glGetUniformLocation(ShaderProgramFlag,"model");

    int ViewMatrixLocationPole = glGetUniformLocation(ShaderProgramBase,"view");
    int ProjectionMatrixLocationPole = glGetUniformLocation(ShaderProgramBase,"projection");
    int ModelMatrixLocationPole = glGetUniformLocation(ShaderProgramBase,"model");

    float camX = 0.0f, camZ = 10.0f;
    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        float t = glfwGetTime();
        glUniform1f(timeLoc, t);

        if (camHeight < 1.0f) {
            camHeight = camHeight + 0.005f;
        }
        else {
            camAngle += 0.05f * 0.016f; // orbit speed
            camX = sin(camAngle) * 10.0f;
            camZ = cos(camAngle) * 10.0f;
        }


        glm::vec3 camPos   = glm::vec3(camX, camHeight, camZ);
        glm::vec3 target   = glm::vec3(-0.7f, 0.0f, 0.0f); // center of flag
        glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(camPos, target, up);
        glm::mat4 projection = glm::perspective(glm::radians(30.0f),AspectRatio,0.1f,100.0f);
        glm::mat4 model = glm::mat4(1.0f);

        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glUseProgram(ShaderProgramSky);

        glm::mat4 skyView = glm::mat4(glm::mat3(view)); // remove translation
        glUniformMatrix4fv(glGetUniformLocation(ShaderProgramSky,"view"),1,GL_FALSE,glm::value_ptr(skyView));
        glUniformMatrix4fv(glGetUniformLocation(ShaderProgramSky,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        glBindVertexArray(skyVAO);
        glUniform1i(glGetUniformLocation(ShaderProgramSky, "skybox"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        // Drawing Flag
        glUseProgram(ShaderProgramFlag);
        glUniform1f(timeLoc, t);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, FlagTexture);
        glUniform1i(glGetUniformLocation(ShaderProgramFlag, "Texture"), 0);
        glBindVertexArray(VAOFlag);
        model = glm::translate(model,{-0.7f, 0.0f, 0.0f});
        glUniformMatrix4fv(ViewmatrixLocation,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(ProjectionMatrixLocation,1,GL_FALSE,glm::value_ptr(projection));
        glUniformMatrix4fv(ModelMatrixLocation,1,GL_FALSE,glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(FlagVertices.size()));

        // Drawing Pole
        glUseProgram(ShaderProgramBase);
        glBindVertexArray(VAOPole);
        //glm::mat4 viewPole = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, CameraDepth));
        glm::mat4 projectionPole = glm::perspective(glm::radians(30.0f),AspectRatio,0.1f,100.0f);
        glm::mat4 modelPole = glm::mat4(1.0f);
        modelPole = glm::translate(modelPole,{-0.7f, 0.0f, 0.0f});
        glUniformMatrix4fv(ViewMatrixLocationPole,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(ProjectionMatrixLocationPole,1,GL_FALSE,glm::value_ptr(projectionPole));
        glUniformMatrix4fv(ModelMatrixLocationPole,1,GL_FALSE,glm::value_ptr(modelPole));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(PoleVertices.size()));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAOFlag);
    glDeleteBuffers(1, &VBOFlag);
    glDeleteProgram(ShaderProgramFlag);
    glfwTerminate();
    return 0;
}

// Callback Definitions --------------- (start)
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    AspectRatio = (float)width / (float)height;
}
// Callback Definitions --------------- (start)

// Function definitions ------------------------------------------------------------ (start)
// Single Function to Create Shader program from shaders
GLuint CreateShaderProgram(const char* VertexShaderSource, const char* FragmentShaderSource) {
    int success;
    char infoLog[512];
    const GLuint VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertexShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(VertexShader);
    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(VertexShader, 512, nullptr, infoLog);
        cout << "Vertex Shader Error : " << infoLog << endl;
    }

    const GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragmentShader, 1, &FragmentShaderSource, nullptr);
    glCompileShader(FragmentShader);
    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(FragmentShader, 512, nullptr, infoLog);
        cout << "Fragment Shader Error : " << infoLog << endl;
    }

    const GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ShaderProgram, 512, nullptr, infoLog);
        cout << "Shader Program Error : " << infoLog << endl;
    }

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return ShaderProgram;
}


// For Flag vertex data ------- (start)
/*
 *std::vector<Vertex> &FlagVertices :: Actual vector
 * glm::vec3 UpperLeft :: respective corners
 * glm::vec3 UpperRight :: respective corners
 * glm::vec3 BottomLeft :: respective corners
 * glm::vec3 BottomRight :: respective corners
 * Density : decides total strips in flag, higher strips == more detailed flag, strips = (density - 1)
 * Keep density minimum at 5
 */
void CreateFlagVertices(
    std::vector<StructVertexFlag>& FlagVertices,
    glm::vec3 UpperLeft,
    glm::vec3 UpperRight,
    glm::vec3 BottomLeft,
    float Density
) {
    //we need to create strips.
    float flagLength = UpperRight.x - UpperLeft.x;
    float StripLength = flagLength / Density;

    for (float i = 1.0f; i < Density; i++) {
        glm::vec3 StripUpperLeft = UpperLeft + glm::vec3((i - 1.0f)*StripLength, 0.0f, 0.0f);
        glm::vec3 StripBottomRight = BottomLeft + glm::vec3((i)*StripLength, 0.0f, 0.0f);

        AddStrip(FlagVertices, StripUpperLeft, StripBottomRight, Density, i);
    }
}

void AddStrip(
    std::vector<StructVertexFlag>& FlagVertices,
    glm::vec3 UpperLeft,
    glm::vec3 BottomRight,
    float Density,
    float StripNumber
) {
    auto push = [&](glm::vec3 p, float u, float v) {
        FlagVertices.push_back({ p, {u, v} });
    };

    float u0 = (StripNumber - 1.0f) / Density;
    float u1 = StripNumber / Density;

    push(UpperLeft,            u0, 1.0f);
    push({BottomRight.x, UpperLeft.y, 0}, u1, 1.0f);
    push({UpperLeft.x, BottomRight.y, 0}, u0, 0.0f);

    push({UpperLeft.x, BottomRight.y, 0}, u0, 0.0f);
    push({BottomRight.x, UpperLeft.y, 0}, u1, 1.0f);
    push(BottomRight,          u1, 0.0f);
}
// For Flag vertex data ------- (end)

// For box vertex data ------- (start)
void addBox(
    std::vector<StructVertexBox>& verts,
    glm::vec3 min,
    glm::vec3 max,
    glm::vec3 boxColor
) {
    auto push = [&](glm::vec3 p) {
        glm::vec3 color = boxColor;
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
// For box vertex data ------- (end)

GLuint loadCubemap(std::vector<std::string> faces)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int w, h, ch;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 0);
        if (!data) {
            cout << "Failed to load cubemap: " << faces[i] << endl;
            continue;
        }
        GLenum format = (ch == 4) ? GL_RGBA : GL_RGB;

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                     0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return texID;
}

// Function definitions ------------------------------------------------------------ (End)