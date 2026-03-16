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

    // Sets a float uniform (shininess, attenuation values, time, etc.).
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


/// Creating procedural texture ------------------ (start)
class Texture {
    // OpenGL texture handle — 0 = invalid/uninitialized
    GLuint TextureID = 0;
    public:
    Texture() {
        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);

        // Texture dimensions — 256x256 pixels.
        constexpr int size = 256;

        // Allocate CPU-side pixel buffer.
        // sz*sz = total pixels, *4 = RGBA (red, green, blue, alpha channels).
        // Each channel is 1 byte (0-255), so total = 256*256*4 = 262144 bytes.
        auto* data = new unsigned char[size * size * 4];

        // Fill every pixel with a procedural pattern.
        // No image file needed — the pattern is computed mathematically.
        for (int y = 0 ; y < size ; y++) {
            for (int x = 0 ; x < size ; x++) {
                // Normalize pixel coordinates to 0.0 - 1.0 range.
                // fx=0.0 at left edge, fx=1.0 at right edge (same for fy vertically)
                const float fx = static_cast<float>(x) / size;
                const float fy = static_cast<float>(y) / size;

                // Noise value — drives the mix between R and G channels.
                // sin() oscillates between -1 and +1, *0.5+0.5 shifts to 0.0-1.0.
                // High frequency (20, 10) creates a fine diagonal stripe pattern.
                float n = sin(fx*20 + fy*10) * 0.5f + 0.5f;

                // Calculate flat array index for this pixel.
                // Each pixel takes 4 consecutive bytes: [R, G, B, A]
                // Row y starts at y*sz, pixel x is at offset x, times 4 bytes each.
                const int i = (y * size + x) * 4;

                // RED channel — sin wave scaled to 0-255, fades with noise n.
                // When n=1.0 → full red contribution
                // When n=0.0 → red is zero
                data[i+0] = static_cast<unsigned char>((sin(fx * 12 + fy * 8) * 127 + 128) * n);

                // GREEN channel — cos wave scaled to 0-255, fades opposite to red.
                // (1-n) means green is bright where red is dark and vice versa.
                // Creates a complementary color shift across the texture.
                data[i+1] = static_cast<unsigned char>((cos(fx * 10 + fy * 12) * 127 + 128) * (1 - n));

                // BLUE channel — independent sin wave, not affected by noise n.
                // Adds a third color variation layer across the texture.
                data[i+2] = static_cast<unsigned char>(sin(fx * 8 + fy * 15) * 127 + 128);

                // ALPHA channel — fully opaque, no transparency.
                data[i+3] = 255;
            }
        }

        // Upload the pixel data from CPU memory to GPU texture memory.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        // Auto-generate all mipmap levels from the base image just uploaded
        glGenerateMipmap(GL_TEXTURE_2D);

        // Delete CPU side array
        delete[] data;

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    }
};

/// Creating procedural texture ------------------ (end)


