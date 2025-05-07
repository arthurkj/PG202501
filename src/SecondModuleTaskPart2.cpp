#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <random>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;

struct Triangle {
	float x;
    float y;
	float r;
	float g;
	float b;
};


int setupShader();
GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
const GLuint WIDTH = 800, HEIGHT = 600;

const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main()	
{
	gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
}
)";

const GLchar *fragmentShaderSource = R"(
	#version 400
	uniform vec4 inputColor;
	out vec4 color;
	void main()
	{
		color = inputColor;
	}
	)";

vector<Triangle> triangles;

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);


	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Exec Triângulos! -- Arthur", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Falha ao inicializar GLAD" << std::endl;
		return -1;
	}

	const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	
	GLuint shaderID = setupShader();
    
	GLuint VAO = createTriangle(-0.5, -0.5, 0.5, -0.5, 0.0, 0.5);

	Triangle mainTriangle;
	mainTriangle.x = 400.0;
	mainTriangle.y = 300.0;
	mainTriangle.r = 0.75;
	mainTriangle.g = 0.01;
	mainTriangle.b = 0.4; 
	triangles.push_back(mainTriangle);

	glUseProgram(shaderID);

	GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");

	mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glfwSetMouseButtonCallback(window, mouse_button_callback);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        
		glLineWidth(10);
		glPointSize(20);

		glBindVertexArray(VAO);

		for (const auto& triangle : triangles) {
			mat4 model = mat4(1); // matriz identidade
			// Translação
			model = translate(model, vec3(triangle.x, triangle.y, 0.0));

			model = rotate(model, radians(180.0f), vec3(0.0, 0.0, 1.0));
			// Escala
			model = scale(model, vec3(100.0, 100.0, 100.0));
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

			glUniform4f(colorLoc, triangle.r, triangle.g, triangle.b, 1.0f);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}

		glBindVertexArray(0); // Desconectando o buffer de geometria
		glfwSwapBuffers(window);
	}
	
	glDeleteVertexArrays(1, &VAO);

	glfwTerminate();
	return 0;
}

int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		Triangle newTriangle;

		newTriangle.x = xpos;
		newTriangle.y = ypos;

		newTriangle.r = (rand() / (float)RAND_MAX);
		newTriangle.g = (rand() / (float)RAND_MAX);
		newTriangle.b = (rand() / (float)RAND_MAX);

		triangles.push_back(newTriangle);
		
	}
}

GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2)
{
	GLfloat vertices[] = {
		//  x    y    z     
		x0, 	y0, 0.0f,
		x1, 	y1, 0.0f,
		x2, 	y2, 0.0f,
	};

	GLuint VAO, VBO;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	return VAO;
}