// Adding attenuation (dimming light)

#include <iostream>
#include <glm/detail/setup.hpp>
#include <glm/glm.hpp>
#include "glad/glad.h"
#include "glm/gtc/matrix_transform.hpp"
using namespace std;
using namespace glm;
// Shader helpers : ----
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
    camera->Up = normalize(cross(camera->Front, camera->Right));
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
    FragPos = vec3(Model * vec4(aPos, 1.0));
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
}

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
}

in vec3 FragPos;
in vec3 Normal;
in vec3 TexCoords;

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
    float diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
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
/// Shader to render objects in environment with attenuation ---- (start)


int main() {

}