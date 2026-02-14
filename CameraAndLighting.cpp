#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

//Camera
glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

/*
Yaw is initiated to -90 degress because the direction formula assumes 0 degree points along +X, but in openGL the default forward is -Z, so we roatate by -90 degree to align them.
*/
float yaw = -90.0f;
/*
Pitch is initialized to 0 degree so the camera starts looking straight ahead horizontally. +/- pitch will then move the view up or down from thus neutral position.
*/
float pitch = 0.0f;

float lastX = SCR_WIDTH/2, lastY = SCR_HEIGHT/2;
bool firstMouse = true;

bool autoMoveCamera = false;

float deltaTime = 0.0f, lastFrame = 0.0f;

//Mouse Callback
void mouse_callback(GLFWwindow*, double xpos, double ypos)
{
	/*
	On the very first mouse event, just store the position. This prevents a huge jump in camera rotation on the first frame.
	*/
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	/*
	Calculate how much the mouse moved since last frame. We use deltas (offsets), not absolute positions.
	*/
	float xoffset = xpos - lastX; //+ve if mouse moved right
	float yoffset = lastY - ypos;

	// Update last positions for next frame
	lastX = xpos;
	lastY = ypos;

	// Mouse sensitivity : contorls how fast should camera move
	float sens = 0.1f;
	xoffset *= sens;
	yoffset *= sens;

	// Update yaw (left/right) and pitch(up/down) angles
	yaw += xoffset;
	pitch += yoffset;

	/*
	clamp pitch to avoid flipping the ccamera upside down(gimbal lock). Looking straight up/down(+/- 90 degree) breaks the direction calculation
	*/
	pitch = glm::clamp(pitch, -89.0f, 89.0f);

	/*
	Convert yaw and pitch angles into a 3D direction vector. this uses spherical coordinate formulas
	*/
	glm::vec3 dir;
	dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	dir.y = sin(glm::radians(pitch));
	dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	// Normalize to ensure the direction vector has lenght of 1
	// this keeps camera movement speed consistent
	cameraFront = glm::normalize(dir);
}

void processInput(GLFWwindow* window)
{
	// Movement speed scaled by deltaTime so motion is frame-rate independent
	// same speedof 30FPS and 300 FPS
	float speed = 3.0f* deltaTime;

	// Move forward : go in the direction the camera is currently facing
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraPos += speed * cameraFront;
	}

	// Move Backward : opposite of the facing direction
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraPos -= speed * cameraFront;
	}

	// Compute the right direction using cross product
	// cross (Front, up) gives a vectore pointing to the camera's right
	glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

	// Move left : go opposite to the right vector
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraPos -= right * speed;
	}

	// Move right : go along the right vector
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraPos += right * speed;
	}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		autoMoveCamera = true;
	}

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		autoMoveCamera = false;
	}
}

GLuint compileShader(const char* vs, const char* fs)
{
	int success;
	char infoLog[1024];

	// vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vs, nullptr);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	std::cout << "VertexShader Compile success : " << success << std::endl;
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 1024, nullptr, infoLog);
		std::cout << "Error in vertex shader " << infoLog << std::endl;
	}

	// fragment shader;
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fs, nullptr);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	std::cout << "Fragment shader success : " << success << std::endl;
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 1024, nullptr, infoLog);
		std::cout << "Error in fragment shader " << infoLog << std::endl;
	}

	// Program
	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 1024, nullptr, infoLog);
		std::cout << "Errors in generating program " << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

const char* objectVS = R"(
#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	FragPos = vec3(model*vec4(aPos, 1.0));
	Normal = mat3(transpose(inverse(model))) * aNormal;
	gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* objectFS = R"(
#version 410 core
struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

uniform Material material;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main()
{
	vec3 ambient = lightColor * material.ambient;

	vec3 norm = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * material.diffuse * lightColor;

	vec3 viewDir = normalize(viewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = spec * material.specular * lightColor;

	vec3 result = ambient + diffuse + specular;
	FragColor = vec4(result, 1.0);

}
)";

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Light With Camera", monitor, NULL);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	int fbWidth, fbHeight;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);

	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glEnable(GL_DEPTH_TEST);

	float vertices[] = {
        // positions         // normals
        -0.5,-0.5,-0.5,  0,0,-1,  0.5,-0.5,-0.5,  0,0,-1,  0.5,0.5,-0.5,  0,0,-1,
         0.5,0.5,-0.5,  0,0,-1, -0.5,0.5,-0.5,  0,0,-1, -0.5,-0.5,-0.5,  0,0,-1,

        -0.5,-0.5,0.5,  0,0,1,   0.5,-0.5,0.5,  0,0,1,   0.5,0.5,0.5,  0,0,1,
         0.5,0.5,0.5,  0,0,1,  -0.5,0.5,0.5,  0,0,1,  -0.5,-0.5,0.5,  0,0,1,

        -0.5,0.5,0.5, -1,0,0,  -0.5,0.5,-0.5,-1,0,0,  -0.5,-0.5,-0.5,-1,0,0,
        -0.5,-0.5,-0.5,-1,0,0, -0.5,-0.5,0.5,-1,0,0,  -0.5,0.5,0.5,-1,0,0,

         0.5,0.5,0.5, 1,0,0,   0.5,0.5,-0.5,1,0,0,   0.5,-0.5,-0.5,1,0,0,
         0.5,-0.5,-0.5,1,0,0,  0.5,-0.5,0.5,1,0,0,   0.5,0.5,0.5,1,0,0,

        -0.5,-0.5,-0.5,0,-1,0,  0.5,-0.5,-0.5,0,-1,0,  0.5,-0.5,0.5,0,-1,0,
         0.5,-0.5,0.5,0,-1,0, -0.5,-0.5,0.5,0,-1,0, -0.5,-0.5,-0.5,0,-1,0,

        -0.5,0.5,-0.5,0,1,0,   0.5,0.5,-0.5,0,1,0,   0.5,0.5,0.5,0,1,0,
         0.5,0.5,0.5,0,1,0,  -0.5,0.5,0.5,0,1,0,  -0.5,0.5,-0.5,0,1,0
     };

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);

	// angles in degrees
	float angleX = 30.0f;
	float angleY = 45.0f;
	float angleZ = 60.0f;

	GLuint objectShader = compileShader(objectVS, objectFS);

	while(!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.5, 0.5, 0.5, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::vec3 lightPos;
		lightPos.x = sin(glfwGetTime()) * 5.0f;
		lightPos.z = cos(glfwGetTime()) * 5.0f;
		lightPos.y = 1;

		glm::vec3 center;
		if (autoMoveCamera)
		{
			center = glm::vec3(0, 0, 0);
			// cameraPos.x = cos(currentFrame) * 6.0f;
			// cameraPos.z = sin(currentFrame) * 6.0f;
			// cameraPos.y = sin(currentFrame * 0.5f) * 1.5f + 2.0f;

			cameraPos.x = cos(currentFrame * 1.5f + 30.0f) * 6.0f;
			cameraPos.z = sin(currentFrame * 2.0f + 60.0f) * 9.0f;
			cameraPos.y = sin(currentFrame * 0.5f + 90.0f) * 12.0f;
		}
		else
		{
			center = cameraPos + cameraFront;
		}

		glm::mat4 view = glm::lookAt(cameraPos, center, cameraUp);
		glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 100.0f);

		glUseProgram(objectShader);
        glUniform3f(glGetUniformLocation(objectShader,"lightPos"),lightPos.x,lightPos.y,lightPos.z);
        glUniform3f(glGetUniformLocation(objectShader,"viewPos"),cameraPos.x,cameraPos.y,cameraPos.z);
        glUniform3f(glGetUniformLocation(objectShader,"lightColor"),1,1,1);

        glUniform3f(glGetUniformLocation(objectShader,"material.ambient"),0.5,0.0,0.0);
        glUniform3f(glGetUniformLocation(objectShader,"material.diffuse"),1,0.0,0.0);
        glUniform3f(glGetUniformLocation(objectShader,"material.specular"),0.5,0.5,0.5);
        glUniform1f(glGetUniformLocation(objectShader,"material.shininess"),128);

        glm::mat4 model = glm::mat4(1);

		model = glm::rotate(model, glm::radians(angleX * currentFrame), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(angleY * currentFrame), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(angleZ * currentFrame), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(objectShader,"model"),1,GL_FALSE,glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(objectShader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(objectShader,"projection"),1,GL_FALSE,glm::value_ptr(projection));
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES,0,36);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
		glUniformMatrix4fv(glGetUniformLocation(objectShader,"model"),1,GL_FALSE,glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES,0,36);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
}

