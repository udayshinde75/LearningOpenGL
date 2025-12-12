#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;
const char* VertexShaderSource =
    "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform float time;\n"
    "void main()\n"
    "{\n"
    "float offset = sin(time) * 0.2;"
    "vec3 newPos = aPos;"
    "newPos.y += offset;"
    "gl_Position = vec4(newPos, 1.0);\n"
    "}\0";

const char* FragmentShaderSource =
    "#version 410 core\n"
    "uniform vec4 updatedColor;"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = updatedColor;\n"
    "}\n";

GLFWwindow* GetWindow();

int main() {
    cout << "Initiating GLFW" << endl;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", nullptr, nullptr);

    cout << "Window Created" << endl;
    if (window == nullptr) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        cout << "Failed to initialize glad" << endl;
        glfwTerminate();
        return -1;
    }

    // --- TRIANGLE DATA ----
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    int success;
    char infoLog[512];

    const unsigned int VertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(VertexShader, 1, &VertexShaderSource, nullptr);
    glCompileShader(VertexShader);

    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(VertexShader, 512, nullptr, infoLog);
        cout << "Vertex shader compilation error" << endl;
        cout << infoLog << endl;
    }

    cout << "Created Vertex Shader" << endl;

    const unsigned int FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(FragmentShader, 1, &FragmentShaderSource, nullptr);
    glCompileShader(FragmentShader);

    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(FragmentShader, 512, nullptr, infoLog);
        cout << "Fragment shader compilation error" << endl;
        cout << infoLog << endl;
    }

    cout << "Created Fragment Shader" << endl;

    const unsigned int ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);

    cout << "Linked program" << endl;

    cout << "Checking error step 1" << endl;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    cout << "Checking error step 2" << endl;
    if (success == GL_FALSE) {
        glGetProgramInfoLog(ShaderProgram, 512, nullptr, infoLog);
        cout << "Shader program compilation error" << endl;
        cout << infoLog << endl;
    }

    cout << "Created Shader Program" << endl;

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    unsigned int VAO, VBO_Positions;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_Positions);

    cout << "Created Vertex Arrays" << endl;

    glBindVertexArray(VAO);

    //positions VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Positions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);

    cout << "Created Vertex buffer positions" << endl;

    glBindVertexArray(0);

    cout << "Starting render loop" << endl;

    int Loc = glGetUniformLocation(ShaderProgram, "updatedColor");
    int timeLoc = glGetUniformLocation(ShaderProgram, "time");

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(ShaderProgram);

        float t = glfwGetTime();
        float r = sin(t * 0.8f) * 0.5f + 0.5f;
        float g = sin(t * 1.2f) * 0.5f + 0.5f;
        float b = sin(t * 1.6f) * 0.5f + 0.5f;

        glUniform1f(timeLoc, t);
        glUniform4f(Loc, r, g, b, 1.0f);

        glBindVertexArray(VAO);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO_Positions);
    glDeleteProgram(ShaderProgram);
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    glfwTerminate();
    return 0;

}
