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

///
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

///
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

enum Direction {
    forwardDir,
    backwardDir,
    leftDir,
    rightDir
};

void cameraUpdateVectors(Camera* camera) {
    vec3 front;
    front.x = cos(radians(camera->Yaw)) * cos(radians(camera->Pitch));
    front.y = sin(radians(camera->Pitch));
    front.z = sin(radians(camera->Yaw)) * cos(radians(camera->Pitch));
    camera->Front = normalize(front);
    camera->Right = normalize(cross(camera->Front, camera->WorldUp));
    camera->Up = normalize(cross(camera->Front, camera->Right));
}
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
mat4 getCameraViewMatrix(Camera* camera) {
    return lookAt(camera->Position, camera->Position + camera->Front, camera->Up);
}
int main() {

}