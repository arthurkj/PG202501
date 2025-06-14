#include "gl_utils.h"
#include <glad/glad.h> // Carregamento dos ponteiros para funções OpenGL
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
#define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include "TileMap.h"
// #include "DiamondView.h"
#include "SlideView.h"
#include "ltMath.h"
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;

int g_gl_width = 800;
int g_gl_height = 800;
float xi = -1.0f, xf = 1.0f;
float yi = -1.0f, yf = 1.0f;
float w, h;
float tw, th; // Largura e altura do tile no mundo

// Posição atual do "personagem" no mapa (coluna, linha)
// Começa no centro do mapa 3x3
int cx = 1, cy = 1;

TilemapView* tview = new SlideView();
TileMap* tmap = NULL;
GLFWwindow* g_window = NULL;

// --- Funções Auxiliares ---

// Carrega uma textura a partir de um arquivo
int loadTexture(unsigned int &texture, const char *filename) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (data) {
        if (nrChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Falha ao carregar a textura: " << filename << std::endl;
        stbi_image_free(data);
        return 0;
    }
    stbi_image_free(data);
    return 1; 
}

int main() {
    restart_gl_log();
    start_gl();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const int mapWidth = 3;
    const int mapHeight = 3;
    int mapData[mapHeight][mapWidth] = {
        {1, 1, 4},
        {4, 1, 4},
        {4, 4, 1}
    };

    tmap = new TileMap(mapWidth, mapHeight, 0);
    for (int r = 0; r < mapHeight; r++) {
        for (int c = 0; c < mapWidth; c++) {
            // Inverte 'r' para que a matriz no código corresponda à visualização
            tmap->setTile(c, (mapHeight - 1) - r, mapData[r][c]);
        }
    }
    
    w = xf - xi;
    h = yf - yi;
    tw = w / (float)tmap->getWidth();
    th = tw / 2.0f; // Proporção 2:1 para tiles isométricos

    int tileSetCols = 9, tileSetRows = 9;
    float tileW_tex = 1.0f / (float)tileSetCols;
    float tileH_tex = 1.0f / (float)tileSetRows;

    GLuint tileset_tid;
    loadTexture(tileset_tid, "terrain.png");
    tmap->setTid(tileset_tid);

    char vertex_shader[1024 * 256];
    char fragment_shader[1024 * 256];
    parse_file_into_str("_geral_vs.glsl", vertex_shader, 1024 * 256);
    parse_file_into_str("_geral_fs.glsl", fragment_shader, 1024 * 256);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, (const GLchar**)&vertex_shader, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, (const GLchar**)&fragment_shader, NULL);
    glCompileShader(fs);

    GLuint shader_programme = glCreateProgram();
    glAttachShader(shader_programme, fs);
    glAttachShader(shader_programme, vs);
    glLinkProgram(shader_programme);

    float vertices[] = {
        xi,        yi + th / 2.0f,   0.0f,         tileH_tex,
        xi + tw/2.0f,  yi,             tileW_tex/2.0f, tileH_tex/2.0f,
        xi + tw,     yi + th/2.0f,   tileW_tex,      tileH_tex,
        xi + tw/2.0f,  yi + th,        tileW_tex/2.0f, tileH_tex*1.5f
    };
    unsigned int indices[] = {0, 1, 3, 1, 2, 3};

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    double last_key_time = 0.0;
    while (!glfwWindowShouldClose(g_window)) {
        _update_fps_counter(g_window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, g_gl_width, g_gl_height);

        glUseProgram(shader_programme);
        glBindVertexArray(VAO);

        //Desenho do Mapa
        for (int r = 0; r < tmap->getHeight(); r++) {
            for (int c = 0; c < tmap->getWidth(); c++) {
                int t_id = (int)tmap->getTile(c, r);
                int u = t_id % tileSetCols;
                int v = t_id / tileSetCols;
                
                float draw_x, draw_y;
                tview->computeDrawPosition(c, r, tw, th, draw_x, draw_y);

                glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), u * tileW_tex);
                glUniform1f(glGetUniformLocation(shader_programme, "offsety"), v * tileH_tex);
                glUniform1f(glGetUniformLocation(shader_programme, "tx"), draw_x);
                glUniform1f(glGetUniformLocation(shader_programme, "ty"), draw_y + 1.0f); // Ajuste de posição global
                glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), tmap->getZ());
                glUniform1f(glGetUniformLocation(shader_programme, "weight"), (c == cx) && (r == cy) ? 0.5f : 0.0f);

                glBindTexture(GL_TEXTURE_2D, tmap->getTileSet());
                glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }

        //  Navegação por Teclado 
        if (glfwGetTime() > last_key_time + 0.15) { // Delay de 150ms
            bool key_pressed = false;
            int next_c = cx, next_r = cy;

            // Lógica de movimento baseada na Tabela 4-2 (Slide Maps) 
            if (glfwGetKey(g_window, GLFW_KEY_UP) == GLFW_PRESS) { // Norte
                next_c = cx + 1; next_r = cy - 2; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_DOWN) == GLFW_PRESS) { // Sul
                next_c = cx - 1; next_r = cy + 2; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) { // Leste
                next_c = cx + 1; next_r = cy + 0; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) { // Oeste
                next_c = cx - 1; next_r = cy + 0; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS) { // Nordeste
                next_c = cx + 1; next_r = cy - 1; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS) { // Noroeste
                next_c = cx + 0; next_r = cy - 1; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_C) == GLFW_PRESS) { // Sudeste
                next_c = cx + 0; next_r = cy + 1; key_pressed = true;
            } else if (glfwGetKey(g_window, GLFW_KEY_Z) == GLFW_PRESS) { // Sudoeste
                next_c = cx - 1; next_r = cy + 1; key_pressed = true;
            }

            if (key_pressed) {
                // Checa se o movimento é válido (dentro dos limites do mapa)
                if (next_c >= 0 && next_c < mapWidth && next_r >= 0 && next_r < mapHeight) {
                    cx = next_c;
                    cy = next_r;
                }
                last_key_time = glfwGetTime();
            }
        }

        if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(g_window, 1);
        }

        glfwPollEvents();
        glfwSwapBuffers(g_window);
    }

    glfwTerminate();
    delete tmap;
    delete tview;
    return 0;
}