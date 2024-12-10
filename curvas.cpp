#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <filesystem> 
#include <GLES3/gl3.h>
#include <GLFW/glfw3.h>
#include <emscripten/emscripten.h>

// Estrutura para ponto 2D
struct Pontos {
    float x, y;
    Pontos(float x = 0, float y = 0) : x(x), y(y) {}
};

// Estrutura para transformação
struct Transformacao {
    char tipo;   // 't' para translação, 'r' para rotação, 's' para escala
    float param[3];  // [x, y] para translação e escala, [angulo, px, py] para rotação
};

// Variáveis globais
std::vector<std::vector<Pontos>> CURVAs;
std::vector<Transformacao> transforms;
bool mostrarPoligono = false;
int atualTransform = -1;
std::vector<std::vector<Pontos>> originalCURVAs;

// Cores
const float COR_EIXO_X[] = { 0.0f, 1.0f, 0.0f };
const float COR_EIXO_Y[] = { 0.0f, 0.0f, 1.0f };
const float COR_CURVA[] = { 1.0f, 0.0f, 0.0f };
const float COR_ORIGINAL[] = { 0.5f, 0.5f, 0.5f };
const float COR_PONTOS_CONTROLE[] = { 1.0f, 1.0f, 0.0f };

// Shader de vértice atualizado com suporte a tamanho de ponto
const char* vertexShaderSource = R"(#version 300 es
layout(location = 0) in vec2 aPos;
uniform mat4 uProjection;
uniform vec3 uColor;
out vec3 fragColor;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = 5.0;  // Tamanho fixo do ponto
    fragColor = uColor;
}
)";

const char* fragmentShaderSource = R"(#version 300 es
precision mediump float;
in vec3 fragColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(fragColor, 1.0);
}
)";

// Objetos OpenGL
GLuint shaderProgram;
GLuint VBO;
GLfloat projectionMatrix[16] = {1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1};

// Declarações antecipadas
void aplicarTransformacao(std::vector<std::vector<Pontos>>& CURVAs, const Transformacao& transform);
bool loadObjFile(const std::string& directory, const std::string& filename);
void remodelar(GLFWwindow* window, int w, int h);
void teclado(GLFWwindow* window, int key, int scancode, int action, int mods);
void desenharPontosControle(const std::vector<Pontos>& controlePontos);  // Adicione esta linha

// Função para calcular ponto na curva de Bézier
Pontos calcularPontosBezier(const std::vector<Pontos>& controlePontos, float t) {
    std::vector<Pontos> pontos = controlePontos;
    int n = pontos.size();

    for (int j = 1; j < n; j++) {
        for (int i = 0; i < n - j; i++) {
            pontos[i].x = (1 - t) * pontos[i].x + t * pontos[i + 1].x;
            pontos[i].y = (1 - t) * pontos[i].y + t * pontos[i + 1].y;
        }
    }

    return pontos[0];
}

// Inicialização do shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        printf("Erro de compilação do shader: %s\n", infoLog);
    }
    return shader;
}

void initGL() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGenBuffers(1, &VBO);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// Funções de desenho
void drawLines(const std::vector<Pontos>& points, const float color[3], GLenum mode) {
    if (points.empty()) return;

    glUseProgram(shaderProgram);
    
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    glUniform3fv(colorLoc, 1, color);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);
    
    std::vector<float> vertices;
    for (const auto& p : points) {
        vertices.push_back(p.x);
        vertices.push_back(p.y);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glDrawArrays(mode, 0, points.size());
}

void desenharEixos() {
    std::vector<Pontos> xAxis = {
        Pontos(-1000.0f, 0.0f),
        Pontos(1000.0f, 0.0f)
    };
    std::vector<Pontos> yAxis = {
        Pontos(0.0f, -1000.0f),
        Pontos(0.0f, 1000.0f)
    };
    
    drawLines(xAxis, COR_EIXO_X, GL_LINES);
    drawLines(yAxis, COR_EIXO_Y, GL_LINES);
}

void desenharCurvaBezier(const std::vector<Pontos>& controlePontos, const float COR[3]) {
    std::vector<Pontos> curvePoints;
    for (float t = 0; t <= 1.0f; t += 0.01f) {
        curvePoints.push_back(calcularPontosBezier(controlePontos, t));
    }
    
    drawLines(curvePoints, COR, GL_LINE_STRIP);
    
    if (mostrarPoligono) {
        drawLines(controlePontos, COR_PONTOS_CONTROLE, GL_LINE_STRIP);
        drawLines(controlePontos, COR_PONTOS_CONTROLE, GL_POINTS);
    }
}


void aplicarTransformacao(std::vector<std::vector<Pontos>>& CURVAs, const Transformacao& transform) {
    for (auto& CURVA : CURVAs) {
        for (auto& ponto : CURVA) {
            switch (transform.tipo) {
            case 't':  // Translação
                ponto.x += transform.param[0];
                ponto.y += transform.param[1];
                break;

            case 'r': { // Rotação em torno de um ponto
                float angulo = transform.param[0];
                float px = transform.param[1];
                float py = transform.param[2];

                float rad = angulo * M_PI / 180.0f;
                float dx = ponto.x - px;
                float dy = ponto.y - py;

                ponto.x = px + (dx * cos(rad) - dy * sin(rad));
                ponto.y = py + (dx * sin(rad) + dy * cos(rad));
                break;
            }

            case 's': { // Escala
                float sx = transform.param[0];
                float sy = transform.param[1];

                ponto.x *= sx;
                ponto.y *= sy;
                break;
            }
            }
        }
    }
}

void remodelar(GLFWwindow* window, int w, int h) {
    glViewport(0, 0, w, h);
    
    float proporcao = (float)w / h;
    float left, right, bottom, top;
    
    if (w <= h) {
        left = -100.0f;
        right = 100.0f;
        bottom = -100.0f / proporcao;
        top = 100.0f / proporcao;
    } else {
        left = -100.0f * proporcao;
        right = 100.0f * proporcao;
        bottom = -100.0f;
        top = 100.0f;
    }
    
    // Atualizar matriz de projeção
    projectionMatrix[0] = 2.0f / (right - left);
    projectionMatrix[5] = 2.0f / (top - bottom);
    projectionMatrix[10] = -1.0f;
    projectionMatrix[12] = -(right + left) / (right - left);
    projectionMatrix[13] = -(top + bottom) / (top - bottom);
}

void teclado(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        case GLFW_KEY_P:
            mostrarPoligono = !mostrarPoligono;
            break;

        case GLFW_KEY_SPACE:
            if (atualTransform < (int)transforms.size() - 1) {
                atualTransform++;
                CURVAs = originalCURVAs;

                for (int i = 0; i <= atualTransform; i++) {
                    aplicarTransformacao(CURVAs, transforms[i]);
                }
            } else {
                atualTransform = -1;
                CURVAs = originalCURVAs;
            }
            break;

        case GLFW_KEY_Q:
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        }
    }
}

bool loadObjFile(const std::string& directory, const std::string& filename) {
    std::filesystem::path filePath = std::filesystem::path(directory) / filename;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    CURVAs.clear();
    transforms.clear();
    std::vector<Pontos> atualCURVA;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        char tipo;
        iss >> tipo;

        if (tipo == 'v') {  // Vértice
            float x, y;
            if (iss >> x >> y) {
                atualCURVA.push_back(Pontos(x, y));
            }
        }
        else if (tipo == 'c') {  // Nova curva
            if (!atualCURVA.empty()) {
                CURVAs.push_back(atualCURVA);
                atualCURVA.clear();
            }
        }
        else if (tipo == 't' || tipo == 'r' || tipo == 's') {  // Transformação
            Transformacao transform;
            transform.tipo = tipo;

            if (tipo == 't') {
                float x, y;
                if (iss >> x >> y) {
                    transform.param[0] = x;
                    transform.param[1] = y;
                    transform.param[2] = 0;
                    transforms.push_back(transform);
                }
            }
            else if (tipo == 'r') {
                float angulo, px, py;
                if (iss >> angulo >> px >> py) {
                    transform.param[0] = angulo;
                    transform.param[1] = px;
                    transform.param[2] = py;
                    transforms.push_back(transform);
                }
            }
            else if (tipo == 's') {
                float sx, sy;
                if (iss >> sx >> sy) {
                    transform.param[0] = sx;
                    transform.param[1] = sy;
                    transform.param[2] = 0;
                    transforms.push_back(transform);
                }
            }
        }
    }

    // Adicionar última curva se houver
    if (!atualCURVA.empty()) {
        CURVAs.push_back(atualCURVA);
    }

    // Fazer cópia das curvas originais
    originalCURVAs = CURVAs;

    file.close();
    return true;
}

// Função para pontos de controle
void desenharPontosControle(const std::vector<Pontos>& controlePontos) {
    std::vector<float> vertices;
    for (const auto& p : controlePontos) {
        vertices.push_back(p.x);
        vertices.push_back(p.y);
    }

    glUseProgram(shaderProgram);
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
    glUniform3fv(colorLoc, 1, COR_PONTOS_CONTROLE);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    glDrawArrays(GL_POINTS, 0, controlePontos.size());
}

void display(GLFWwindow* window) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    desenharEixos();
    
    if (atualTransform >= 0) {
        for (const auto& CURVA : originalCURVAs) {
            desenharCurvaBezier(CURVA, COR_ORIGINAL);
            if (mostrarPoligono) {
                desenharPontosControle(CURVA);
            }
        }
    }
    
    for (const auto& CURVA : CURVAs) {
        desenharCurvaBezier(CURVA, COR_CURVA);
        if (mostrarPoligono) {
            desenharPontosControle(CURVA);
        }
    }
    
    glfwSwapBuffers(window);
}

// Função de loop principal
void main_loop(void* arg) {
    GLFWwindow* window = static_cast<GLFWwindow*>(arg);
    display(window);
}

int main(int argc, char** argv) {
    if (!glfwInit()) {
        fprintf(stderr, "Falha ao inicializar GLFW\n");
        return -1;
    }
    
    // Configurar GLFW para OpenGL ES 3.0
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    // Obter monitor primário
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    // Criar janela em fullscreen
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, 
        "Visualizador de Curvas de Bézier", monitor, nullptr);
    
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Inicializar OpenGL
    initGL();
    
    // Configurar OpenGL
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Carregar arquivo .obj
    if (!loadObjFile(".", "arquivo.obj")) {
        printf("Erro ao carregar arquivo.obj\n");
        return 1;
    }
    
    // Definir callbacks
    glfwSetFramebufferSizeCallback(window, remodelar);
    glfwSetKeyCallback(window, teclado);
    
    // Redimensionamento inicial
    remodelar(window, 640, 480);
    
    // Iniciar loop principal
    emscripten_set_main_loop_arg(main_loop, window, 0, 1);
    
    return 0;
}