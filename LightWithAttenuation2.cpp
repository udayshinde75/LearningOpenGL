// Adding attenuation (dimming light)

#include <iostream>
#include <glm/detail/setup.hpp>
#include <glm/glm.hpp>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "glm/gtc/matrix_transform.hpp"
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;
using namespace glm;
/// Shader helpers : ---- (start)
int success;
char infoLog[512];

/// This function will create shaders
/// @param type Shader type
/// @param source shader source
/// @return shader ID
GLuint compileShader(GLenum type, const GLchar* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        cout << "Error compiling shader: " << infoLog << endl;
        return 0;
    }
    return shader;
}
/// This function will create shader program
/// @param vertexSource vertex shader source
/// @param fragmentSource fragment shader source
/// @return program ID
GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint program = glCreateProgram();
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        cout << "Error linking program: " << infoLog << endl;
    }
    return  program;
}
/// Shader helpers : ---- (end)

/// Camera Structure consisting imp camera properties
struct Camera {
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
};

/// Directions camera will move in
enum Direction {
    forwardDir,
    backwardDir,
    leftDir,
    rightDir
};

/// This function will updates camera vectors
/// @param camera Camera object.
void cameraUpdateVectors(Camera* camera) {
    vec3 front;
    front.x = cos(radians(camera->Yaw)) * cos(radians(camera->Pitch));
    front.y = sin(radians(camera->Pitch));
    front.z = sin(radians(camera->Yaw)) * cos(radians(camera->Pitch));
    camera->Front = normalize(front);
    camera->Right = normalize(cross(camera->Front, camera->WorldUp));
    camera->Up = normalize(cross(camera->Right, camera->Front));
}

/// This function will initiate camera
/// @param camera Camera Object
/// @param position Camera Position
/// @param up Camera Up vector
/// @param yaw Along X axis
/// @param pitch Along Y axis
/// @param movementSpeed Movement speed of camera
/// @param mouseSensitivity
/// @param zoom Initial zoom
void cameraInit(Camera* camera, glm::vec3 position, glm::vec3 up, float yaw, float pitch, float movementSpeed, float mouseSensitivity, float zoom) {
    camera->Position = position;
    camera->WorldUp = up;
    camera->Yaw = yaw;
    camera->Pitch = pitch;
    camera->MovementSpeed = movementSpeed;
    camera->MouseSensitivity = mouseSensitivity;
    camera->Zoom = zoom;
    cameraUpdateVectors(camera);
}

/// Returns view matrix according to current camera vectors
/// @param camera Camera object
/// @return View matrix
mat4 getCameraViewMatrix(Camera* camera) {
    return lookAt(camera->Position, camera->Position + camera->Front, camera->Up);
}

/// Function to process camera inputs
/// @param camera Camera object
/// @param direction Direction to move
/// @param deltaTime Time between two consecutive frames
void cameraProcessKeyboard(Camera* camera, Direction direction, float deltaTime) {
    float Speed = camera->MovementSpeed * deltaTime;
    if (direction == forwardDir) {
        camera->Position += camera->Front * Speed;
    }
    else if (direction == backwardDir) {
        camera->Position -= camera->Front * Speed;
    }
    else if (direction == leftDir) {
        camera->Position -= camera->Right * Speed;
    }
    else if (direction == rightDir) {
        camera->Position += camera->Right * Speed;
    }
}

/// Function to process mouse movement
/// @param camera camera object
/// @param xoffset Yaw movement
/// @param yoffset Pitch movement
void cameraProcessMouseMovement(Camera* camera, float xoffset, float yoffset) {
    xoffset *= camera->MouseSensitivity;
    yoffset *= camera->MouseSensitivity;
    camera->Yaw += xoffset;
    camera->Pitch += yoffset;
    camera->Pitch = glm::clamp(camera->Pitch, -89.f, 89.f);
    cameraUpdateVectors(camera);
}

/// Function to process scroll movements
/// @param camera Camera object
/// @param Zoom Zoom level
void cameraProcessMouseScroll(Camera* camera, float Zoom) {
    camera->Zoom -= Zoom;
    if (camera->Zoom <= 1.0f) {camera->Zoom = 1.0f;}
    else if (camera->Zoom >= 45.0f) {camera->Zoom = 45.0f;}
}


/// Shader to render objects in environment with attenuation ---- (start)
const char* cubeObjectVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* cubeObjectFragmentShader = R"(
#version 410 core
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
    vec3 specularMap = vec3(texture(material.specular, TexCoords));
    vec3 specular = light.specular * spec * specularMap;

    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (
        light.constant + (light.linear * distance) + (light.quadratic * distance * distance)
    );

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";
/// Shader to render objects in environment with attenuation ---- (end)

/// Shader to render a cube map -------- (start)
const char* cubeMapVertexShader = R"(
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

const char* cubeMapFragmentShader = R"(
#version 410 core
in vec3 TexCoord;
out vec4 FragColor;
uniform samplerCube skybox;

void main()
{
    FragColor = texture(skybox, TexCoord);
}
)";
/// Shader to render a cube map -------- (end)

/// Shader to render a light cube ------ (start)
const char* lightCubeVertexShader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* lightCubeFragmentShader = R"(
#version 410 core
out vec4 FragColor;
uniform vec3 lightColor;
void main()
{
    FragColor = vec4(lightColor, 1.0);
}
)";
/// Shader to render a light cube ------ (end)

/// Texture helpers ---------- (start)
GLuint loadTexture(const char* path) {
    GLuint texture;
    glGenTextures(1, &texture);
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format = GL_RGBA;
        if (channels == 1) { format = GL_RED; }
        else if (channels == 3) { format = GL_RGB; }
        else if (channels == 4) { format = GL_RGBA; }

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(data);
    return texture;
}

GLuint loadCubeMap(std::vector<std::string> faces)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    int w, h, ch;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &ch, 0);
        if (!data) {
            std::cout << "Failed to load cubemap: " << faces[i] << std::endl;
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
/// Texture helpers ---------- (end)



/// App Global --- (start)
int SCR_WIDTH;
int SCR_HEIGHT;

Camera camera;
float lastX;
float lastY;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
/// App Global --- (end)

/// Callbacks ---- (start)
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, GLdouble xPos, GLdouble yPos);
void scroll_callback(GLFWwindow* window, GLdouble xOffset, GLdouble yOffset);
void ProcessInput(GLFWwindow* window);
/// Callbacks ---- (end)

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Point light with decreasing brightness", monitor, nullptr);
    glfwMakeContextCurrent(window);

    lastX = mode->width / 2.0f;
    lastY = mode->height / 2.0f;

    camera = Camera();
    cameraInit(&camera, vec3(0.0f, 0.0f, 3.0f), vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 2.0f, 0.2f, 45.0f);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialise GLAD" << std::endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    // Compile shader
    std::cout << "Compiling cube objects Program --- (start)" << std::endl;
    GLuint cubeObjectProgram = createProgram(cubeObjectVertexShader, cubeObjectFragmentShader);
    std::cout << "Compiling cube objetcs Program --- (end)" << std::endl;

    std::cout << "Compiling lighting cube program -- (start)" << std::endl;
    GLuint lightCubeProgram = createProgram(lightCubeVertexShader, lightCubeFragmentShader);
    std::cout << "Compiling lighting cube program -- (end)" << std::endl;

    std::cout << "Generating ShaderProgramSky --- (start)" << std::endl;
    GLuint ShaderProgramSky = createProgram(cubeMapVertexShader, cubeMapFragmentShader);
    std::cout << "Generating ShaderProgramSky --- (end)" << std::endl;

        // cube vertex data
    float vertices[] = {
        // positions          // normals           // tex coords
        -0.5f,-0.5f,-0.5f,   0.0f,0.0f,-1.0f,     0.0f,0.0f,
         0.5f,-0.5f,-0.5f,   0.0f,0.0f,-1.0f,     1.0f,0.0f,
         0.5f, 0.5f,-0.5f,   0.0f,0.0f,-1.0f,     1.0f,1.0f,
         0.5f, 0.5f,-0.5f,   0.0f,0.0f,-1.0f,     1.0f,1.0f,
        -0.5f, 0.5f,-0.5f,   0.0f,0.0f,-1.0f,     0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,   0.0f,0.0f,-1.0f,     0.0f,0.0f,

        -0.5f,-0.5f, 0.5f,   0.0f,0.0f,1.0f,      0.0f,0.0f,
         0.5f,-0.5f, 0.5f,   0.0f,0.0f,1.0f,      1.0f,0.0f,
         0.5f, 0.5f, 0.5f,   0.0f,0.0f,1.0f,      1.0f,1.0f,
         0.5f, 0.5f, 0.5f,   0.0f,0.0f,1.0f,      1.0f,1.0f,
        -0.5f, 0.5f, 0.5f,   0.0f,0.0f,1.0f,      0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,   0.0f,0.0f,1.0f,      0.0f,0.0f,

        -0.5f, 0.5f, 0.5f,  -1.0f,0.0f,0.0f,      1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,  -1.0f,0.0f,0.0f,      1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f,0.0f,0.0f,      0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  -1.0f,0.0f,0.0f,      0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,  -1.0f,0.0f,0.0f,      0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,  -1.0f,0.0f,0.0f,      1.0f,0.0f,

         0.5f, 0.5f, 0.5f,   1.0f,0.0f,0.0f,      1.0f,0.0f,
         0.5f, 0.5f,-0.5f,   1.0f,0.0f,0.0f,      1.0f,1.0f,
         0.5f,-0.5f,-0.5f,   1.0f,0.0f,0.0f,      0.0f,1.0f,
         0.5f,-0.5f,-0.5f,   1.0f,0.0f,0.0f,      0.0f,1.0f,
         0.5f,-0.5f, 0.5f,   1.0f,0.0f,0.0f,      0.0f,0.0f,
         0.5f, 0.5f, 0.5f,   1.0f,0.0f,0.0f,      1.0f,0.0f,

        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f,0.0f,     0.0f,1.0f,
         0.5f,-0.5f,-0.5f,   0.0f,-1.0f,0.0f,     1.0f,1.0f,
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f,0.0f,     1.0f,0.0f,
         0.5f,-0.5f, 0.5f,   0.0f,-1.0f,0.0f,     1.0f,0.0f,
        -0.5f,-0.5f, 0.5f,   0.0f,-1.0f,0.0f,     0.0f,0.0f,
        -0.5f,-0.5f,-0.5f,   0.0f,-1.0f,0.0f,     0.0f,1.0f,

        -0.5f, 0.5f,-0.5f,   0.0f,1.0f,0.0f,      0.0f,1.0f,
         0.5f, 0.5f,-0.5f,   0.0f,1.0f,0.0f,      1.0f,1.0f,
         0.5f, 0.5f, 0.5f,   0.0f,1.0f,0.0f,      1.0f,0.0f,
         0.5f, 0.5f, 0.5f,   0.0f,1.0f,0.0f,      1.0f,0.0f,
        -0.5f, 0.5f, 0.5f,   0.0f,1.0f,0.0f,      0.0f,0.0f,
        -0.5f, 0.5f,-0.5f,   0.0f,1.0f,0.0f,      0.0f,1.0f
    };

    float skyboxVertices[] = {
        -1,  1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1,  1,  1, -1, -1,  1, -1,
        -1, -1,  1, -1, -1, -1, -1,  1, -1, -1,  1, -1, -1,  1,  1, -1, -1,  1,
         1, -1, -1,  1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,  1, -1, -1,
        -1, -1,  1, -1,  1,  1,  1,  1,  1,  1,  1,  1,  1, -1,  1, -1, -1,  1,
        -1,  1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1,  1, -1,
        -1, -1, -1, -1, -1,  1,  1, -1, -1,  1, -1, -1, -1, -1,  1,  1, -1,  1
    };

    glm::vec3 cubePositions[20] = {
        glm::vec3( 5.0f,  0.0f,  0.0f),
        glm::vec3( 0.0f,  5.0f,  0.0f),
        glm::vec3( 0.0f,  0.0f,  5.0f),
        glm::vec3(-5.0f, 0.0f,  0.0f),
        glm::vec3( 0.0f, -5.0f,  0.0f),
        glm::vec3(0.0f,  0.0f,  -5.0f)
    };

    GLuint VBO, cubeVAO, lightCubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(0));
    glEnableVertexAttribArray(0);

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
    GLuint cubeMapTex = loadCubeMap(faces);

    stbi_set_flip_vertically_on_load(true);
    GLuint diffuseMap = loadTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/container2-2.png");
    GLuint specularMap = loadTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/container2_specular-2.png");

    glUseProgram(cubeObjectProgram);

    GLint LightPosition = glGetUniformLocation(cubeObjectProgram, "light.position");
    std::cout << "LightPosition : " << LightPosition << std::endl;
    GLint ViewPosition = glGetUniformLocation(cubeObjectProgram, "viewPos");
    std::cout << "ViewPosition : " << ViewPosition << std::endl;
    GLint LightAmbient = glGetUniformLocation(cubeObjectProgram, "light.ambient");
    std::cout << "LightAmbient : " << LightAmbient << std::endl;
    GLint LightDiffuse = glGetUniformLocation(cubeObjectProgram, "light.diffuse");
    std::cout << "LightDiffuse : " << LightDiffuse << std::endl;
    GLint LightSpecular = glGetUniformLocation(cubeObjectProgram, "light.specular");
    std::cout << "LightSpecular : " << LightSpecular << std::endl;
    GLint LightConstant = glGetUniformLocation(cubeObjectProgram, "light.constant");
    std::cout << "LightConstant : " << LightSpecular << std::endl;
    GLint LightLinear = glGetUniformLocation(cubeObjectProgram, "light.linear");
    std::cout << "LightLinear : " << LightSpecular << std::endl;
    GLint LightQuadratic = glGetUniformLocation(cubeObjectProgram, "light.quadratic");
    std::cout << "LightQuadratic : " << LightSpecular << std::endl;
    GLint MaterialShininess = glGetUniformLocation(cubeObjectProgram, "material.shininess");
    std::cout << "MaterialShininess : " << MaterialShininess << std::endl;
    GLint MaterialDiffuse = glGetUniformLocation(cubeObjectProgram, "material.diffuse");
    std::cout << "MaterialDiffuse : " << MaterialDiffuse << std::endl;
    glUniform1i(glGetUniformLocation(cubeObjectProgram, "material.diffuse"), 0);
    GLint MaterialSpecular = glGetUniformLocation(cubeObjectProgram, "material.specular");
    std::cout << "MaterialSpecular : " << MaterialSpecular << std::endl;
    glUniform1i(glGetUniformLocation(cubeObjectProgram, "material.specular"), 1);
    GLint LightShaderProjection = glGetUniformLocation(cubeObjectProgram, "projection");
    std::cout << "LightShaderProjection : " << LightShaderProjection << std::endl;
    GLint LightShaderView = glGetUniformLocation(cubeObjectProgram, "view");
    std::cout << "LightShaderView : " << LightShaderView << std::endl;
    GLint LightShaderModel = glGetUniformLocation(cubeObjectProgram, "model");
    std::cout << "LightShaderModel : " << LightShaderModel << std::endl;

    float currentFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(cubeObjectProgram);
        glUniform3fv(LightPosition,1, glm::value_ptr(lightPos));
        glUniform3fv(ViewPosition,1,  glm::value_ptr(camera.Position));

        glUniform3fv(LightAmbient, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.2f)));
        glUniform3fv(LightDiffuse, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
        glUniform3fv(LightSpecular, 1, glm::value_ptr(glm::vec3(2.0f, 2.0f, 2.0f)));
        glUniform1f(LightConstant, 1.0f);
        glUniform1f(LightLinear, 0.045f);
        glUniform1f(LightQuadratic, 0.0075f);

        glUniform1f(MaterialShininess, 512.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 1000.0f);
        glm::mat4 view = getCameraViewMatrix(&camera);
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(LightShaderProjection, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(LightShaderView, 1, GL_FALSE, glm::value_ptr(view));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        glBindVertexArray(cubeVAO);

        for (unsigned int i = 0; i < 6; i++) {

            model = glm::mat4(1.0f);
            const float angle = 5.0f * (i+1) * glfwGetTime();
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 1.0f, 1.0f));

            model = glm::translate(model, cubePositions[i]);


            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

            glUniformMatrix4fv(LightShaderModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glUseProgram(lightCubeProgram);
        glUniformMatrix4fv(glGetUniformLocation(lightCubeProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lightCubeProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        glUniformMatrix4fv(glGetUniformLocation(lightCubeProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(lightCubeProgram, "lightColor"), 1, glm::value_ptr(glm::vec3(1.0f)));

        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        if (true) {
            glDepthMask(GL_FALSE);
            glDepthFunc(GL_LEQUAL);
            glUseProgram(ShaderProgramSky);

            glm::mat4 skyView = glm::mat4(glm::mat3(view)); // remove translation
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgramSky,"view"),1,GL_FALSE,glm::value_ptr(skyView));
            glUniformMatrix4fv(glGetUniformLocation(ShaderProgramSky,"projection"),1,GL_FALSE,glm::value_ptr(projection));

            glBindVertexArray(skyVAO);
            glUniform1i(glGetUniformLocation(ShaderProgramSky, "skybox"), 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTex);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindTexture(GL_TEXTURE_2D, 0);

            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error: 0x" << std::hex << err << std::dec << std::endl;
        }
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(cubeObjectProgram);
    glDeleteProgram(lightCubeProgram);

    glfwTerminate();
    return 0;
}

void ProcessInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, forwardDir, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, backwardDir, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, leftDir, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, rightDir, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int, int)
{
    glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void mouse_callback(GLFWwindow* window, const double xPos, const double yPos)
{
    const auto xpos = static_cast<float>(xPos);
    const auto ypos = static_cast<float>(yPos);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; // reversed
    lastX = xpos;
    lastY = ypos;
    cameraProcessMouseMovement(&camera, xOffset, yOffset);
}

void scroll_callback(GLFWwindow*, double, const double yOffset)
{
    cameraProcessMouseScroll(&camera, static_cast<float>(yOffset));
}