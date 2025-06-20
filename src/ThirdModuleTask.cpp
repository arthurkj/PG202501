/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para a disciplina de Processamento Gráfico - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 13/08/2024
 *
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

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

#include <cmath>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

// Protótipos das funções
GLuint createSquare();
int setupShader();
void eliminatesSimilar();
void createGame();
void restartGame();

// Cálculos
float calculateDistance(vec3 keyColor, vec3 otherColor);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;
const GLuint ROWS = 6, COLS = 8;
const GLuint SQUARE_WIDTH = 100, SQUARE_HEIGHT = 100;
const float MAX_DISTANCE = 1.73;
const float TOLERANCE = 0.2;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec3 position;
 uniform mat4 projection;
 uniform mat4 model;
 void main()	
 {
     //...pode ter mais linhas de código aqui!
     gl_Position = projection * model * vec4(position.x, position.y, position.z, 1.0);
 }
 )";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
 #version 400
 uniform vec4 inputColor;
 out vec4 color;
 void main()
 {
     color = inputColor;
 }
 )";

struct Square
{
    vec3 position;
    vec3 dimensions;
    vec3 color;
    bool isClicked;
};

Square grid[ROWS][COLS];
int rowSelected = -1;
int colSelected = -1;

int numberOfEliminated = 0;
int points = 0;

// Função MAIN
int main()
{
    srand(time(0));

    // Inicialização da GLFW
    glfwInit();

    // Muita atenção aqui: alguns ambientes não aceitam essas configurações
    // Você deve adaptar para a versão do OpenGL suportada por sua placa
    // Sugestão: comente essas linhas de código para desobrir a versão e
    // depois atualize (por exemplo: 4.5 com 4 e 5)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Essencial para computadores da Apple
    // #ifdef __APPLE__
    //	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // #endif

    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das Cores! M3 - Arthur", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Fazendo o registro da função de callback para a janela GLFW
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // GLAD: carrega todos os ponteiros d funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    // Obtendo as informações de versão
    const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
    const GLubyte *version = glGetString(GL_VERSION);   /* version as a string */
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    GLuint VAO = createSquare();

    createGame();

    glUseProgram(shaderID);

    // Enviando a cor desejada (vec4) para o fragment shader
    // Utilizamos a variáveis do tipo uniform em GLSL para armazenar esse tipo de info
    // que não está nos buffers
    GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");

    // Matriz de projeção paralela ortográfica
    // mat4 projection = ortho(-10.0, 10.0, -10.0, 10.0, -1.0, 1.0);
    mat4 projection = ortho(0.0, 800.0, 600.0, 0.0, -1.0, 1.0);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // Loop da aplicação - "game loop"
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // cor de fundo
        glClear(GL_COLOR_BUFFER_BIT);

        glLineWidth(10);
        glPointSize(20);

        glBindVertexArray(VAO); // Conectando ao buffer de geometria

        if (numberOfEliminated >= ROWS * COLS)
        {
            restartGame();
        }

        for (int i = 0; i < ROWS; i++)
        {
            for (int j = 0; j < COLS; j++)
            {
                Square square = grid[i][j];

                if (!square.isClicked)
                {
                    // Matriz de modelo: transformações na geometria (objeto)
                    mat4 model = mat4(1); // matriz identidade
                    // Translação
                    model = translate(model, square.position);

                    // Escala
                    model = scale(model, square.dimensions);
                    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

                    glUniform4f(colorLoc, square.color.r, square.color.b, square.color.g, 1.0f); // enviando cor para variável uniform inputColor
                    // Chamada de desenho - drawcall
                    // Poligono Preenchido - GL_TRIANGLE_STRIP
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
                }
            }
        }

        glBindVertexArray(0); // Desconectando o buffer de geometria

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }
    // Pede pra OpenGL desalocar os buffers
    // glDeleteVertexArrays(1, &VAO);
    // Finaliza a execução da GLFW, limpando os recursos alocados por ela
    glfwTerminate();
    return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        restartGame();
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
//  shader simples e único neste exemplo de código
//  O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
//  fragmentShader source no iniçio deste arquivo
//  A função retorna o identificador do programa de shader
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

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int rowClicked = ypos / SQUARE_HEIGHT;
        int colClicked = xpos / SQUARE_WIDTH;

        rowSelected = rowClicked;
        colSelected = colClicked;

        if (!grid[rowClicked][colClicked].isClicked)
        {
            eliminatesSimilar();
            points--;
        }
    }
}

float calculateDistance(vec3 keyColor, vec3 otherColor)
{
    float rDistance = keyColor.r - otherColor.r;
    float gDistance = keyColor.g - otherColor.g;
    float bDistance = keyColor.b - otherColor.b;

    float sum = (rDistance * rDistance) + (gDistance * gDistance) + (bDistance * bDistance);

    float sqrtValue = sqrt(sum);

    return sqrtValue;
}

GLuint createSquare()
{
    GLuint VAO;

    GLfloat vertices[] = {
        // x    y    z
        // T0
        -0.5, 0.5, 0.0,  // v0
        -0.5, -0.5, 0.0, // v1
        0.5, 0.5, 0.0,   // v2
        0.5, -0.5, 0.0,  // v3
    };

    GLuint VBO;
    // Geração do identificador do VBO
    glGenBuffers(1, &VBO);
    // Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Envia os dados do array de floats para o buffer da OpenGl
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Geração do identificador do VAO (Vertex Array Object)
    glGenVertexArrays(1, &VAO);
    // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);
    // Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando:
    //  Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
    //  Numero de valores que o atributo tem (por ex, 3 coordenadas xyz)
    //  Tipo do dado
    //  Se está normalizado (entre zero e um)
    //  Tamanho em bytes
    //  Deslocamento a partir do byte zero
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
    // atualmente vinculado - para que depois possamos desvincular com segurança
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
    glBindVertexArray(0);

    return VAO;
}

void eliminatesSimilar()
{
    Square selectedSquare = grid[rowSelected][colSelected];

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            Square otherSquare = grid[i][j];

            if (!otherSquare.isClicked)
            {
                float distance = calculateDistance(selectedSquare.color, otherSquare.color);

                float percentualDistance = distance / MAX_DISTANCE;

                if (percentualDistance <= TOLERANCE)
                {
                    grid[i][j].isClicked = true;
                    points++;
                    numberOfEliminated++;
                }
            }
        }
    }

    rowSelected = -1;
    colSelected = -1;
}

void createGame() {
    cout << "Iniciando novo jogo..." << endl;
    numberOfEliminated = 0;
    points = 0;

    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            Square square;

            vec iniPos = vec2(SQUARE_WIDTH / 2, SQUARE_HEIGHT / 2);

            square.position = vec3(iniPos.x + j * SQUARE_WIDTH, iniPos.y + i * SQUARE_HEIGHT, 0.0);
            square.dimensions = vec3(SQUARE_WIDTH, SQUARE_HEIGHT, 1.0);

            float r, g, b;
            r = rand() % 256 / 255.0;
            g = rand() % 256 / 255.0;
            b = rand() % 256 / 255.0;
            square.color = vec3(r, g, b);

            square.isClicked = false;

            grid[i][j] = square;
        }
    }
}

void restartGame() {
    cout << "Jogo encerrado!" << endl;
    cout << "Pontuação: " << points << endl;
    createGame();
}