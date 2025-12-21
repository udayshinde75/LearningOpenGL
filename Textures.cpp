//OpenGL program with textures

#include "glad/glad.h"
#include "glfw/glfw3.h"
#include "iostream"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

//Vertex shader
const char* vertexShaderSource =
    "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCord;\n"
    "out vec2 TexCord;\n"
    "uniform float offsetX;"
    "void main()\n"
    "{\n"
    "   vec3 pos = aPos;\n"
    "   pos.x += offsetX;"
    "   gl_Position = vec4(pos, 1.0);\n"
    "   TexCord = aTexCord;\n"
    "}\0";

//Fragment shader
const char* fragmentShaderSource =
    "#version 410 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCord;\n"
    "uniform sampler2D Texture1;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(Texture1, TexCord);\n"
    "}\0";
//indices common
unsigned int indices[] = {
    0, 1, 3,
    1, 2, 3
};

//Shader Helper
GLuint CompileShader(const GLenum type, const char* source)
{
    GLuint Shader = glCreateShader(type);
    glShaderSource(Shader, 1, &source, nullptr);
    glCompileShader(Shader);

    int success;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char Log[512];
        glGetShaderInfoLog(Shader, 512, nullptr, Log);
        cout << "Error in compling shader:" << Log << endl;
    }

    return Shader;
}

GLuint CreateShaderProgram() {
    GLuint VertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint FragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);

    glLinkProgram(ShaderProgram);
    int success;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char Log[512];
        glGetProgramInfoLog(ShaderProgram, 512, nullptr, Log);
        cout << "Error in compiling shader program:" << Log << endl;
    }

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return ShaderProgram;
}

//create VAO
GLuint createRectVAO(float* vertices, size_t size)
{
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return VAO;
}

GLuint CreateTexture(const char* path) {
    GLuint texture;
    int width, height, channels;
    GLenum Format;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    unsigned char *data = stbi_load(path, &width, &height, &channels,0);
    if (!data) {
        cout << "Error loading window assets" << endl;
        return -1;
    }
    else {
        cout << "Loaded window assets" << endl;
        cout << "Height: " << height << " Width: " << width << " Channels: " << channels << endl;
        Format = (channels == 4) ? GL_RGBA : GL_RGB;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, Format, width, height, 0, Format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}


//main
int main() {
    //Initialization
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Textures", nullptr, nullptr);

    if (window == nullptr) {
        cout << "Error creating window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        cout << "Error initializing glad" << endl;
        glfwTerminate();
        return -1;
    }

    float BigRecVertices[] = {
        // positions        // tex coords
        1.0f,  1.0f, 0.0f,  1.0f, 1.0f, // top right
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f, // bottom right
       -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, // bottom left
       -1.0f,  1.0f, 0.0f,  0.0f, 1.0f  // top left
    };

    float SmallRecVertices[] = {
        // positions        // tex coords
        0.6f,  0.75f, 0.0f,  1.0f, 1.0f, // top right
        0.56f, -0.77f, 0.0f,  1.0f, 0.0f, // bottom right
       -0.56f, -0.77f, 0.0f,  0.0f, 0.0f, // bottom left
       -0.6f,  0.75f, 0.0f,  0.0f, 1.0f  // top left
    };

    float Cloud1Vertices[] = {
        // positions        // tex coords
        0.2f,  0.6f, 0.0f,  1.0f, 1.0f, // top right
        0.2f, 0.2f, 0.0f,  1.0f, 0.0f, // bottom right
       -0.2f, 0.2f, 0.0f,  0.0f, 0.0f, // bottom left
       -0.2f,  0.6f, 0.0f,  0.0f, 1.0f  // top left
    };

    float Cloud2Vertices[] = {
        // positions        // tex coords
        0.3f,  0.4f, 0.0f,  1.0f, 1.0f, // top right
        0.3f, 0.1f, 0.0f,  1.0f, 0.0f, // bottom right
       -0.1f, 0.1f, 0.0f,  0.0f, 0.0f, // bottom left
       -0.1f,  0.4f, 0.0f,  0.0f, 1.0f  // top left
    };

    GLuint BigVAO = createRectVAO(BigRecVertices, sizeof(BigRecVertices));
    GLuint SmallVAO = createRectVAO(SmallRecVertices, sizeof(SmallRecVertices));
    GLuint Cloud1VAO = createRectVAO(Cloud1Vertices, sizeof(Cloud1Vertices));
    GLuint Cloud2VAO = createRectVAO(Cloud2Vertices, sizeof(Cloud2Vertices));

    //Load textures
    stbi_set_flip_vertically_on_load(true);

    GLuint BigWindowTexture = CreateTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/window.png");
    GLuint SkyTexture = CreateTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/sky.png");
    GLuint CloudTexture = CreateTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/Clouds.png");
    GLuint Cloud2Texture = CreateTexture("/Users/udayshinde/Desktop/OpenGLWindow/Assets/Clouds.png");

    //shader program
    GLuint ShaderProgram = CreateShaderProgram();
    glUseProgram(ShaderProgram);

    glUniform1i(glGetUniformLocation(ShaderProgram, "Texture1"), 0);

    //Render loop
    while (!glfwWindowShouldClose(window)) {
        int offsetLoc = glGetUniformLocation(ShaderProgram, "offsetX");
        glClearColor(0.3f, 0.7f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1f(offsetLoc, 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, BigWindowTexture);
        glBindVertexArray(BigVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SkyTexture);
        glBindVertexArray(SmallVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        float time = glfwGetTime();
        float offsetX = sin(time * 0.5f) * 0.3f;

        glUniform1f(offsetLoc, offsetX);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, CloudTexture);
        glBindVertexArray(Cloud1VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Cloud2Texture);
        glBindVertexArray(Cloud2VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}