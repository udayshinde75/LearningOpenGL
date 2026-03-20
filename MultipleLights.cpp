#include <iostream>
#include <ostream>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/detail/setup.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


/// Defining Globals variable ---- (start)
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
bool bIsCameraLightOn = true;
/// Defining Globals variable ---- (end)

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
public:
    // OpenGL texture handle — 0 = invalid/uninitialized
    GLuint TextureID = 0;
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



/// Light structs --------- (Start)
struct PointLight {
    glm::vec3 position, color;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    PointLight(glm::vec3 p, glm::vec3 c) : position(p), color(c) {}
};

struct SpotLight {
    glm::vec3 position, dir, color;
    float innerCutoff, outerCutoff;
    float constant = 1.0f;
    float linear = 0.07f;
    float quadratic = 0.017f;

    SpotLight(glm::vec3 p, glm::vec3 d, glm::vec3 c, float inner, float outer)
        : position(p), dir(glm::normalize(d)), color(c), innerCutoff(cos(glm::radians(inner))), outerCutoff(cos(glm::radians(outer))) {}
};
/// Light structs --------- (end)


/// Material ------ (start)
struct Material { glm::vec3 specular; float shininess; };

Material materials[6] = {
    {{1.0f,1.0f,1.0f}, 32.0f},
    {{1.0f,1.0f,1.0f}, 64.0f},
    {{1.0f,1.0f,1.0f}, 128.0f},
    {{1.0f,1.0f,1.0f}, 256.0f},
    {{0.9f,0.9f,0.9f}, 512.0f},
    {{0.4f,0.9f,0.4f}, 1028.0f}
};

/// Material ------ (end)


/// Cube vertex data ------------- (start)
unsigned int cubeVAO = 0;

float cubeData[288] = {
    -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,   0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,   0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
     0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,  -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,  -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,
    -0.5f,-0.5f, 0.5f, 0,0, 1, 0,0,   0.5f,-0.5f, 0.5f, 0,0, 1, 1,0,   0.5f, 0.5f, 0.5f, 0,0, 1, 1,1,
     0.5f, 0.5f, 0.5f, 0,0, 1, 1,1,  -0.5f, 0.5f, 0.5f, 0,0, 1, 0,1,  -0.5f,-0.5f, 0.5f, 0,0, 1, 0,0,
    -0.5f, 0.5f, 0.5f,-1,0, 0, 1,0,  -0.5f,-0.5f,-0.5f,-1,0, 0, 0,1,  -0.5f, 0.5f,-0.5f,-1,0, 0, 1,1,
    -0.5f,-0.5f,-0.5f,-1,0, 0, 0,1,  -0.5f, 0.5f, 0.5f,-1,0, 0, 1,0,  -0.5f,-0.5f, 0.5f,-1,0, 0, 0,0,
     0.5f, 0.5f, 0.5f, 1,0, 0, 1,0,   0.5f,-0.5f,-0.5f, 1,0, 0, 0,1,   0.5f, 0.5f,-0.5f, 1,0, 0, 1,1,
     0.5f,-0.5f,-0.5f, 1,0, 0, 0,1,   0.5f, 0.5f, 0.5f, 1,0, 0, 1,0,   0.5f,-0.5f, 0.5f, 1,0, 0, 0,0,
    -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1,   0.5f,-0.5f,-0.5f, 0,-1,0, 1,1,   0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,
     0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,  -0.5f,-0.5f, 0.5f, 0,-1,0, 0,0,  -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1,
    -0.5f, 0.5f,-0.5f, 0, 1,0, 0,1,   0.5f, 0.5f, 0.5f, 0, 1,0, 1,0,   0.5f, 0.5f,-0.5f, 0, 1,0, 1,1,
     0.5f, 0.5f, 0.5f, 0, 1,0, 1,0,  -0.5f, 0.5f,-0.5f, 0, 1,0, 0,1,  -0.5f, 0.5f, 0.5f, 0, 1,0, 0,0
};

void setupCubeVAO() {
    unsigned int VBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeData), cubeData, GL_STATIC_DRAW);
    // position
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),static_cast<void *>(nullptr));
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void *>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // uv
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),reinterpret_cast<void *>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}
/// Cube vertex data ------------- (end)

/// separated functionality for retrieving model matrix with rotation for object in scene ---- (start)
class SceneObject {
public:
    glm::mat4 model{};
    int matId;
    float rotSpeed;
    Texture texture;

    SceneObject(glm::vec3 pos, float scale, int mat, float rot) : matId(mat), rotSpeed(rot) {
        model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(scale));
    }

    void update() {
        model = glm::rotate(model, glm::radians(rotSpeed), glm::vec3(0.0f, 1.0f, 0.0f));
    }
};
/// separated functionality for retrieving model matrix with rotation for object in scene ---- (start)


/// shaders --------- (Start)
const char* sceneVertexShaderSource = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model, view, projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}

)";

const char* sceneFragmentShaderSource = R"(
#version 410 core
struct Material {
    vec3 specular;
    sampler2D diffuseTex;
    float shininess;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float innerCutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};

uniform SpotLight cameraLight;
uniform PointLight pointLight[3];
uniform vec3 viewPos;
uniform Material material;
uniform int isCameraLightOn;
uniform float time;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
out vec4 FragColor;

float attenuate(float distance, float constant, float linear, float quadratic) {
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

vec3 calPointLightEffect(PointLight light, vec3 norm, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - FragPos);
    float distance = length(light.position - FragPos);
    float attenuation = attenuate(distance, light.constant, light.linear, light.quadratic);

    vec3 ambient = 0.05 * light.color;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color;

    vec3 halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfway), 0.0), material.shininess);
    vec3 specular = spec * light.color * material.specular;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calSpotLightEffect(SpotLight light, vec3 norm, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - FragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon , 0.0, 1.0);

    if(intensity <= 0) return vec3(0.0);

    float distance = length(light.position - FragPos);
    float attenuation = attenuate(distance, light.constant, light.linear, light.quadratic);

    vec3 ambient = 0.05 * light.color;

    float diff    = max(dot(norm, lightDir), 0.0);
    vec3  diffuse = diff * light.color;

    vec3 halfway = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfway), 0.0), material.shininess);
    vec3 specular = spec * light.color * material.specular;

    return (ambient + (diffuse + specular) * intensity) * attenuation;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec4 texColor = texture(material.diffuseTex, TexCoord);

    vec3 result = 0.1 * texColor.rgb;
    if (isCameraLightOn == 1) {
        result += calSpotLightEffect(cameraLight, norm, viewDir) * texColor.rgb;
    }

    for (int i = 0; i < 3; i++) {
        result += calPointLightEffect(pointLight[i], norm, viewDir) * texColor.rgb;
    }

    FragColor = vec4(result, 1.0);
}
)";

const char* lightCubeVS = R"(
#version 410 core
layout(location=0) in vec3 aPos;
uniform mat4 model, view, projection;
void main(){
    gl_Position = projection * view * model * vec4(aPos, 1.0);
})";

const char* lightCubeFS = R"(
#version 410 core
uniform vec3 emissiveColor;
out vec4 FragColor;
void main(){
    FragColor = vec4(emissiveColor, 1.0);
})";
/// shaders --------- (End)


/// Global Objects ------ (Start)
Camera* camera = nullptr;
Shader* sceneShader = nullptr;
Shader* lightingShader = nullptr;
Texture* defaultTexture = nullptr;

std::vector<SceneObject> sceneObjects;
SpotLight cameraLight(glm::vec3(0.0f), glm::vec3(0.0f,0.0f,-1.0f), glm::vec3(1.0f, 1.0f, 1.0f), SPOT_LIGHT_INNER, SPOT_LIGHT_OUTER);

PointLight pointLights[3] = {
    { glm::vec3( 0.0f, 8.0f,  0.0f), glm::vec3(1.0f, 0.55f, 0.15f) }, // warm orange
    { glm::vec3(12.0f, 5.0f, -6.0f), glm::vec3(0.2f, 0.75f, 1.0f ) }, // cool cyan
    { glm::vec3(-10.0f,7.0f, 10.0f), glm::vec3(1.0f, 0.3f,  0.5f ) }  // pink-red
};

int WindowWidth, WindowHeight;
/// Global Objects ------ (End)


/// Callbacks ----- (Start)
bool firstMouse = true;
double lastX, lastY;

void mouseCallBack(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
        return;
    }

    camera->processMouse(static_cast<float>(xPos - lastX), static_cast<float>(lastY - yPos));
    lastX = xPos;
    lastY = yPos;
}

void frameBufferCallBack(GLFWwindow* window, int, int) {
    glfwGetFramebufferSize(window, &WindowWidth, &WindowHeight);
    glViewport(0, 0, WindowWidth, WindowHeight);
}

bool roamFirstLight = false;
bool roamSecondLight = false;
bool roamThirdLight = false;
void processInput(GLFWwindow* win, float dt){
    glm::vec3 dir(0);
    if(glfwGetKey(win,GLFW_KEY_W))          dir.z =  1;
    if(glfwGetKey(win,GLFW_KEY_S))          dir.z = -1;
    if(glfwGetKey(win,GLFW_KEY_A))          dir.x = -1;
    if(glfwGetKey(win,GLFW_KEY_D))          dir.x =  1;
    if(glfwGetKey(win,GLFW_KEY_SPACE))      dir.y =  1;
    if(glfwGetKey(win,GLFW_KEY_LEFT_SHIFT)) dir.y = -1;
    if(glm::length(dir) > 0.0f) camera->processKeyboard(dir, dt);
    if(glfwGetKey(win,GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(win, true);
    if(glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS) bIsCameraLightOn = true;
    if(glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS) bIsCameraLightOn = false;

    if(glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) {
        roamFirstLight = true;
    } else if(glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) {
        roamSecondLight = true;
    } else if(glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS) {
        roamThirdLight = true;
    } else {
        roamFirstLight = false;
        roamSecondLight = false;
        roamThirdLight = false;
    }
}
/// Callbacks ----- (End)


int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Multiple Lights", monitor, nullptr);
    if(!window) {
        std::cout<<"Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    WindowWidth = mode->width;
    WindowHeight = mode->height;

    lastX = static_cast<float>(WindowWidth) / 2.0f;
    lastY = static_cast<float>(WindowHeight) / 2.0f;

    glfwMakeContextCurrent(window);

    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))){
        std::cout<<"Failed to init GLAD\n"; return -1;
    }

    glfwSetFramebufferSizeCallback(window, frameBufferCallBack);
    glfwSetCursorPosCallback(window, mouseCallBack);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    setupCubeVAO();
    defaultTexture = new Texture();
    sceneShader = new Shader(sceneVertexShaderSource, sceneFragmentShaderSource);
    lightingShader = new Shader(lightCubeVS, lightCubeFS);
    camera = new Camera();

    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> rPos(-15.0f, 15.0f), rRot(-1.5f,1.5f);
    std::uniform_int_distribution<int> rMat(0,4);

    for(int i = 0; i < NUM_CUBES; i++){
        float scale = 0.6f + (i%5)*0.1f;
        sceneObjects.emplace_back(glm::vec3(rPos(gen), rPos(gen)*0.3f+1.0f, rPos(gen)),scale, rMat(gen), rRot(gen));
    }

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        const float dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - lastTime).count();
        lastTime = std::chrono::high_resolution_clock::now();

        float time = static_cast<float>(glfwGetTime());
        processInput(window, dt);

        // --- Animate point lights ---
        // [0] orbits horizontally around scene centre
        pointLights[0].position.x = 8.0f * sin(time * 0.3f);
        pointLights[0].position.z = 8.0f * cos(time * 0.3f);
        pointLights[0].position.y = 7.0f + sin(time * 0.5f) * 1.5f;

        // [1] bobs up and down on the right side
        pointLights[1].position.y = 5.0f + sin(time * 0.7f) * 3.0f;
        pointLights[1].position.x = 10.0f * cos(time * 0.2f);

        // [2] sweeps front-back on the left side
        pointLights[2].position.z = 8.0f * sin(time * 0.4f);
        pointLights[2].position.y = 6.0f + cos(time * 0.6f) * 2.0f;

        if (roamFirstLight) {
            camera->position = pointLights[0].position + glm::vec3(2.0f, 2.0f, 0.0f);
        } else if(roamSecondLight) {
            camera->position = pointLights[1].position + glm::vec3(2.0f, 2.0f, 0.0f);
        } else if(roamThirdLight) {
            camera->position = pointLights[2].position + glm::vec3(2.0f, 2.0f, 0.0f);
        }

        // --- Animate camera lights ---
        cameraLight.position = camera->position;
        cameraLight.dir = camera->front;

        // --- Clear ---
        glClearColor(0.04f, 0.04f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = camera->getProjectionMatrix(static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight));
        glm::mat4 view = camera->getViewMatrix();

        sceneShader->use();
        sceneShader->setMat4("projection", proj);
        sceneShader->setMat4("view", view);
        sceneShader->setVec3("viewPos", camera->position);

        sceneShader->setInt("isCameraLightOn", bIsCameraLightOn ? 1 : 0);
        sceneShader->setVec3("cameraLight.position", cameraLight.position);
        sceneShader->setVec3("cameraLight.direction", cameraLight.dir);
        sceneShader->setVec3 ("cameraLight.color", cameraLight.color);
        sceneShader->setFloat("cameraLight.innerCutOff", cameraLight.innerCutoff);
        sceneShader->setFloat("cameraLight.outerCutOff", cameraLight.outerCutoff);
        sceneShader->setFloat("cameraLight.constant", cameraLight.constant);
        sceneShader->setFloat("cameraLight.linear", cameraLight.linear);
        sceneShader->setFloat("cameraLight.quadratic", cameraLight.quadratic);

        for(int i = 0; i < 3; i++){
            std::string b = "pointLight[" + std::to_string(i) + "].";
            sceneShader->setVec3 ((b+"position").c_str(), pointLights[i].position);
            sceneShader->setVec3 ((b+"color").c_str(), pointLights[i].color);
            sceneShader->setFloat((b+"constant").c_str(), pointLights[i].constant);
            sceneShader->setFloat((b+"linear").c_str(), pointLights[i].linear);
            sceneShader->setFloat((b+"quadratic").c_str(), pointLights[i].quadratic);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, defaultTexture->TextureID);
        sceneShader->setInt("material.diffuseTex", 0);

        glBindVertexArray(cubeVAO);
        for(auto& obj : sceneObjects){
            obj.update();
            sceneShader->setMat4 ("model", obj.model);
            sceneShader->setVec3 ("material.specular", materials[obj.matId].specular);
            sceneShader->setFloat("material.shininess", materials[obj.matId].shininess);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        lightingShader->use();
        lightingShader->setMat4("projection", proj);
        lightingShader->setMat4("view", view);

        glBindVertexArray(cubeVAO);
        for(auto & pointLight : pointLights){
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pointLight.position);
            model = glm::scale(model, glm::vec3(0.3f));
            lightingShader->setMat4("model", model);
            lightingShader->setVec3("emissiveColor", pointLight.color);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    delete sceneShader;
    delete lightingShader;
    delete defaultTexture;
    delete camera;
    glfwTerminate();
    return 0;
}