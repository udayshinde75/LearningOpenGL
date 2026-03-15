#include <iostream>
#include <ostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/detail/setup.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


/// Defining Globals ---- (start)
/// Rendering
constexpr int NUM_CUBES = 64;

/// Camera
constexpr float CAMERA_SPEED = 4.0f;
constexpr float MOUSE_SENSITIVITY = 0.1f;
constexpr float FOV = 45.0f;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 1000.0f;

/// SpotLight
constexpr float SPOT_LIGHT_INNER = 15.0f;
constexpr float SPOT_LIGHT_OUTER = 30.0f;

/// Turn off or on spotlight
bool bIsSpotLightOn = true;
/// Defining Globals ---- (end)

/// Implementing camera ----------- (Start)
/// Camera Class
class Camera {
    public:
    // public variables
    // Camera axes : rebuilt everytime yaw and pitch changes
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f); // position of the camera
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f); // direction where camera is looking
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // perpendicular vector to front and right
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f); // perpendicular to front, directs towards the right of camera

    // Euler angles in degrees.
    // yaw : rotation left/right around world Y axis (-90 = looking down -Z)
    // pitch : rotation up/down, clamped to (-89, 89) to avoid gimbal lock
    float yaw = -90.0f;
    float pitch = 0.0f;

    // Default constructor — members use in-class initializers above.
    Camera() = default;

    // Recomputes front/right/up from current yaw and pitch.
    void updateVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);

        // right = front × worldUp
        right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

        // up = right × front
        up = glm::normalize(glm::cross(right, front));
    }

    // Called every frame with raw mouse delta (pixels moved since last frame).
    // deltaX → yaw  (left/right look)
    // deltaY → pitch (up/down look)
    void processMouse(const float deltaX,const float deltaY) {
        yaw += deltaX * MOUSE_SENSITIVITY;
        pitch = glm::clamp(pitch + deltaY * MOUSE_SENSITIVITY, -89.0f, 89.0f);
        updateVectors();
    }

    // Moves camera position based on input direction and elapsed time.
    // dir.x : strafe  (-1 = left,    +1 = right)
    // dir.z : forward (-1 = back,    +1 = forward)
    // dir.y : vertical(-1 = down,    +1 = up)
    // dir.y uses world Y (0,1,0) not camera up — so Space always goes straight up in world space regardless of where camera is looking.
    void processKeyboard(const glm::vec3 dir, float deltaTime) {
        position += glm::normalize(dir.x * right + dir.y * glm::vec3(0.0f, 1.0f, 0.0f) + dir.z * front) * deltaTime * CAMERA_SPEED;
    }

    // Returns the view matrix for this frame.
    // Transforms world-space coordinates into camera-space.
    // (position + front) is the target point — direction matters, not distance.
    // [[nodiscard]] — ignoring this return value is always a bug.
    [[nodiscard]] glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    // Returns the perspective projection matrix.
    // aspectRatio = window width / height — pass every frame in case of resize.
    // FOV, NEAR_PLANE, FAR_PLANE are global constexpr constants, Zoom is not implemented here.
    // [[nodiscard]] — ignoring this return value is always a bug.
    [[nodiscard]] glm::mat4 getProjectionMatrix(const float aspectRatio) const {
        return glm::perspective(FOV, aspectRatio, NEAR_PLANE, FAR_PLANE);
    }
};
/// Implementing camera ----------- (End)

/// Implementing shader class --------------------- (start)
// Shader class wraps an OpenGL shader program (vertex + fragment).
class Shader {
    // OpenGL handle to the linked shader program.
    // 0 = invalid/uninitialized
    GLuint ProgramID = 0;

    public:
    // Constructor — compiles vertex and fragment shader from raw GLSL source strings, links them into a program
    Shader(const char* vertexSource, const char* fragmentSource) {
        // Local lambda — compiles a single shader stage and reports errors. Defined here because it is only needed during construction.
        auto compileShader = [](const GLuint s, const char* source) {
            glShaderSource(s, 1, &source, nullptr);
            glCompileShader(s);
            int success; char info[1024];
            glGetShaderiv(s, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(s, 1024, nullptr, info);
                std::cout << "Shader Error : " << info << std::endl;
            }
        };

        ProgramID = glCreateProgram();
        const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        compileShader(vertexShader, vertexSource);
        compileShader(fragmentShader, fragmentSource);
        glAttachShader(ProgramID, vertexShader);
        glAttachShader(ProgramID, fragmentShader);
        glLinkProgram(ProgramID);
        int success; char info[1024];
        glGetProgramiv(ProgramID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ProgramID, 1024, nullptr, info);
            std::cout << "Shader Error : " << info << std::endl;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    // Binds this shader program for all subsequent draw calls.
    void use() const {
        glUseProgram(ProgramID);
    }

    // Sets a 4x4 matrix uniform (model, view, projection matrices).
    void setMat4 (const char* uniform, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ProgramID, uniform), 1, GL_FALSE, glm::value_ptr(mat));
    }

    // Sets a vec3 uniform (positions, colors, directions).
    void setVec3(const char* uniform, const glm::vec3& vec) const {
        glUniform3fv(glGetUniformLocation(ProgramID, uniform), 1, glm::value_ptr(vec));
    }

    // Sets a float uniform (shininess, attenuation values, time, etc).
    void setFloat(const char* uniform, const float val) const {
        glUniform1f(glGetUniformLocation(ProgramID, uniform), val);
    }

    // Sets an int uniform (texture unit slots, boolean flags, counts).
    void setInt(const char* uniform, const int val) const {
        glUniform1i(glGetUniformLocation(ProgramID, uniform), val);
    }

    // Destructor — releases the OpenGL program
    ~Shader() {
        if (ProgramID != 0) {
            glDeleteProgram(ProgramID);
        }
        ProgramID = 0;
    }
};

/// Implementing shader class --------------------- (end)

