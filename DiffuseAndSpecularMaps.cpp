// Program with diffuse maps and specular maps

#include <iostream>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

// Shader helpers ----- (start)
int success;
char infoLog[512];

/// Summary compileShader()
/// @param type ShaderType
/// @param source ShaderSource
/// @return ShaderID
GLuint compileShader(const GLenum type, const GLchar* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "Error in compiling shader : " << std::to_string(type) << " : " << infoLog << std::endl;
    }
    return shader;
}

/// summary createProgram()
/// @param vertexSource vertex shader program source
/// @param fragmentSource fragment shader program source
/// @return shader program ID
GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    const GLuint program = glCreateProgram();
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "Error in linking shader : " << infoLog << std::endl;
    }
    return program;
}
// Shader helpers ----- (end)

// Camera functions ------- (start)
struct CameraState {
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
};

enum Direction {
    forward,
    backward,
    left,
    right
};
void cameraInit(CameraState* camera,glm::vec3 position,glm::vec3 up,float yaw,float pitch);
glm::mat4 cameraGetViewMatrix(CameraState* camera);
void cameraUpdateVectors(CameraState* camera);
void cameraProcessKeyboard(CameraState* camera, Direction direction, float deltaTime);
void cameraProcessMouseMovement(CameraState* camera, float xoffset, float yoffset);
void cameraProcessMouseScroll(CameraState* camera, float yoffset);

void cameraInit(CameraState* camera,
                glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
                float yaw = -90.0f,
                float pitch = 0.0f) {
    camera->Position = position;
    camera->WorldUp = up;
    camera->Yaw = yaw;
    camera->Pitch = pitch;
    camera->Front = glm::vec3(0.0f, 0.0f, -1.0f);
    camera->MovementSpeed = 2.0f;
    camera->MouseSensitivity = 0.2f;
    camera->Zoom = 45.0f;

    cameraUpdateVectors(camera);
}

glm::mat4 cameraGetViewMatrix(CameraState* camera) {
    return glm::lookAt(camera->Position, camera->Position + camera->Front, camera->Up);
}

void cameraUpdateVectors(CameraState* camera) {
    glm::vec3 front;
    front.x = cos(glm::radians(camera->Yaw)) * cos(glm::radians(camera->Pitch));
    front.y = sin(glm::radians(camera->Pitch));
    front.z = sin(glm::radians(camera->Yaw)) * cos(glm::radians(camera->Pitch));
    camera->Front = glm::normalize(front);
    camera->Right = glm::normalize(glm::cross(camera->Front, camera->WorldUp));
    camera->Up    = glm::normalize(glm::cross(camera->Right, camera->Front));
}

void cameraProcessKeyboard(CameraState* camera, Direction direction, float deltaTime) {
    float Speed =  camera->MovementSpeed * deltaTime;
    if (direction == forward) {
        camera->Position += camera->Front * Speed;
    }

    if (direction == backward) {
        camera->Position -= camera->Front * Speed;
    }

    if (direction == left) {
        camera->Position -= camera->Right * Speed;
    }

    if(direction == right) {
        camera->Position += camera->Right * Speed;
    }
}

void cameraProcessMouseMovement(CameraState* camera, float xoffset, float yoffset) {
    xoffset *= camera->MouseSensitivity;
    yoffset *= camera->MouseSensitivity;
    camera->Yaw  += xoffset;
    camera->Pitch += yoffset;
    camera->Pitch = glm::clamp(camera->Pitch, -89.0f, 89.0f);
    cameraUpdateVectors(camera);
}

void cameraProcessMouseScroll(CameraState* camera, float yoffset) {
    camera->Zoom -= yoffset;
    if (camera->Zoom < 1.0f) camera->Zoom = 1.0f;
    if (camera->Zoom > 45.0f) camera->Zoom = 45.0f;
}
// Camera functions ------- (end)

// lighting shader ------ (start)
const char* lightingVertexShaderSource = R"(
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

const char* lightingFragmentShaderSource = R"(
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
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)";
// lighting shader ------ (end)

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

// lighting cube shader ------ (start)
const char* lightCubeVertexShaderSource = R"(
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

const char* lightCubeFragmentShaderSource = R"(
#version 410 core
out vec4 FragColor;
uniform vec3 lightColor;
void main()
{
    FragColor = vec4(lightColor, 1.0);
}
)";

// Texture loaders ---- (start)
GLuint loadTexture(const char* path) {
    GLuint texture;
    glGenTextures(1, &texture);
    int width, height, channels;
    unsigned char *data = stbi_load(path, &width, &height, &channels, 0);
    if (data) {
        GLenum format = GL_RGBA;
        if (channels == 1) { format = GL_RED; }
        else if (channels == 3) { format = GL_RGB; }
        else if (channels == 4) { format = GL_RGBA; }

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "Failed to load " << path << std::endl;
    }
    stbi_image_free(data);
    return texture;
}

GLuint loadCubemap(std::vector<std::string> faces)
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
// Texture loaders ---- (end)

// App Global --- (start)
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

CameraState camera;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
// App Global --- (end)

// Callbacks ---- (start)
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, GLdouble xPos, GLdouble yPos);
void scroll_callback(GLFWwindow* window, GLdouble xOffset, GLdouble yOffset);
void ProcessInput(GLFWwindow* window);
// Callbacks ---- (end)

// main function
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Light With Camera", monitor, NULL);
    glfwMakeContextCurrent(window);
    lastX = mode->width / 2.0f;
    lastY = mode->height / 2.0f;
    camera = CameraState();
    cameraInit(&camera);

    // load GL funcs
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    // Compile shader
    std::cout << "Compiling Lighting Program --- (start)" << std::endl;
    GLuint lightingProgram = createProgram(lightingVertexShaderSource, lightingFragmentShaderSource);
    std::cout << "Compiling Lighting Program --- (end)" << std::endl;

    std::cout << "Compiling lighting cube program -- (start)" << std::endl;
    GLuint lightCubeProgram = createProgram(lightCubeVertexShaderSource, lightCubeFragmentShaderSource);
    std::cout << "Compiling lighting cube program -- (end)" << std::endl;

    std::cout << "Generating ShaderProgramSky --- (start)" << std::endl;
    GLuint ShaderProgramSky = createProgram(VertexShaderSourceSky, FragmentShaderSourceSky);
    std::cout << "Generating ShaderProgramSky --- (end)" << std::endl;


    // cube vertex data
    float vertices[] = {
        // positions          // normals           // texcoords
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
    GLuint cubemapTex = loadCubemap(faces);

    stbi_set_flip_vertically_on_load(true);
    GLuint diffuseMap = loadTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/container2-2.png");
    GLuint speculatMap = loadTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/container2_specular-2.png");

    // will implement later
    // GLuint diffuseMapGround = loadTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/GroundDiffuse.png");
    // GLuint speculatMapGround = loadTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/GroundSpecular.png");

    glUseProgram(lightingProgram);

    GLint LightPosition = glGetUniformLocation(lightingProgram, "light.position");
    std::cout << "LightPosition : " << LightPosition << std::endl;
    GLint ViewPosition = glGetUniformLocation(lightingProgram, "viewPos");
    std::cout << "ViewPosition : " << ViewPosition << std::endl;
    GLint LightAmbient = glGetUniformLocation(lightingProgram, "light.ambient");
    std::cout << "LightAmbient : " << LightAmbient << std::endl;
    GLint LightDiffuse = glGetUniformLocation(lightingProgram, "light.diffuse");
    std::cout << "LightDiffuse : " << LightDiffuse << std::endl;
    GLint LightSpecular = glGetUniformLocation(lightingProgram, "light.specular");
    std::cout << "LightSpecular : " << LightSpecular << std::endl;
    GLint MaterialShininess = glGetUniformLocation(lightingProgram, "material.shininess");
    std::cout << "MaterialShininess : " << MaterialShininess << std::endl;
    GLint MaterialDiffuse = glGetUniformLocation(lightingProgram, "material.diffuse");
    std::cout << "MaterialDiffuse : " << MaterialDiffuse << std::endl;
    glUniform1i(glGetUniformLocation(lightingProgram, "material.diffuse"), 0);
    GLint MaterialSpecular = glGetUniformLocation(lightingProgram, "material.specular");
    std::cout << "MaterialSpecular : " << MaterialSpecular << std::endl;
    glUniform1i(glGetUniformLocation(lightingProgram, "material.specular"), 1);
    GLint LightShaderProjection = glGetUniformLocation(lightingProgram, "projection");
    std::cout << "LightShaderProjection : " << LightShaderProjection << std::endl;
    GLint LightShaderView = glGetUniformLocation(lightingProgram, "view");
    std::cout << "LightShaderView : " << LightShaderView << std::endl;
    GLint LightShaderModel = glGetUniformLocation(lightingProgram, "model");
    std::cout << "LightShaderModel : " << LightShaderModel << std::endl;


    float currentFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(lightingProgram);
        glUniform3fv(LightPosition,1, glm::value_ptr(lightPos));
        glUniform3fv(ViewPosition,1,  glm::value_ptr(camera.Position));

        glUniform3fv(LightAmbient, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.2f)));
        glUniform3fv(LightDiffuse, 1, glm::value_ptr(glm::vec3(0.5f, 0.5f, 0.5f)));
        glUniform3fv(LightSpecular, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));

        glUniform1f(MaterialShininess, 512.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)fbWidth / (float)fbHeight, 0.1f, 100.0f);
        glm::mat4 view = cameraGetViewMatrix(&camera);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(20 * currentFrame), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(LightShaderProjection, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(LightShaderView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(LightShaderModel, 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, speculatMap);

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

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
    glDeleteProgram(lightingProgram);
    glDeleteProgram(lightCubeProgram);

    glfwTerminate();
    return 0;
}

void ProcessInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, forward, deltaTime); // FORWARD
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, backward, deltaTime); // BACKWARD
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, left, deltaTime); // LEFT
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraProcessKeyboard(&camera, right, deltaTime); // RIGHT
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed
    lastX = xpos;
    lastY = ypos;
    cameraProcessMouseMovement(&camera, xoffset, yoffset);
}

void scroll_callback(GLFWwindow*, double, double yoffset)
{
    cameraProcessMouseScroll(&camera, static_cast<float>(yoffset));
}