#include <GL/glut.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#define _USE_MATH_DEFINES 
#include <math.h>

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
std::vector<std::vector<Pontos>> CURVAs;  // Lista de curvas (cada curva tem seus pontos de controle)
std::vector<Transformacao> transforms;  // Lista de transformações
bool mostrarPoligono = false;  // Controle para exibir/ocultar polígono de controle
int atualTransform = -1;  // Índice da transformação atual
std::vector<std::vector<Pontos>> originalCURVAs;  // Cópia das curvas originais

// Cores
const float COR_EIXO_X[] = { 0.0f, 1.0f, 0.0f };  // Verde
const float COR_EIXO_Y[] = { 0.0f, 0.0f, 1.0f };  // Azul
const float COR_CURVA[] = { 1.0f, 0.0f, 0.0f };   // Vermelho
const float COR_ORIGINAL[] = { 0.5f, 0.5f, 0.5f }; // Cinza
const float COR_PONTOS_CONTROLE[] = { 1.0f, 1.0f, 0.0f };  // Amarelo

// Função para calcular ponto na curva de Bézier usando o algoritmo de De Casteljau
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

// Aplicar transformação aos pontos
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
                float px = transform.param[1];  // Ponto de rotação x
                float py = transform.param[2];  // Ponto de rotação y

                // Converter ângulo para radianos
                float rad = angulo * M_PI / 180.0f;

                // Translação para origem do ponto de rotação
                float dx = ponto.x - px;
                float dy = ponto.y - py;

                // Aplicar rotação
                ponto.x = px + (dx * cos(rad) - dy * sin(rad));
                ponto.y = py + (dx * sin(rad) + dy * cos(rad));
                break;
            }

            case 's': { // Escala
                float sx = transform.param[0];  // Fator de escala x
                float sy = transform.param[1];  // Fator de escala y

                // Aplicar escala em relação à origem
                ponto.x *= sx;
                ponto.y *= sy;
                break;
            }
            }
        }
    }
}

// Função para desenhar o plano cartesiano
void desenharEixos() {
    glBegin(GL_LINES);

    // Eixo das abscissas (Verde)
    glColor3fv(COR_EIXO_X);
    glVertex2f(-1000.0f, 0.0f);
    glVertex2f(1000.0f, 0.0f);

    // Eixo das Ordenadas (Azul)
    glColor3fv(COR_EIXO_Y);
    glVertex2f(0.0f, -1000.0f);
    glVertex2f(0.0f, 1000.0f);

    glEnd();
}

// Função para desenhar uma curva de Bézier
void desenharCurvaBezier(const std::vector<Pontos>& controlePontos, const float COR[3]) {
    glColor3fv(COR);
    glBegin(GL_LINE_STRIP);

    for (float t = 0; t <= 1.0f; t += 0.01f) {
        Pontos p = calcularPontosBezier(controlePontos, t);
        glVertex2f(p.x, p.y);
    }

    glEnd();

    // Desenhar polígono de controle se necessário
    if (mostrarPoligono) {
        glColor3fv(COR_PONTOS_CONTROLE);
        glBegin(GL_LINE_STRIP);
        for (const auto& p : controlePontos) {
            glVertex2f(p.x, p.y);
        }
        glEnd();

        // Desenhar pontos de controle
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        for (const auto& p : controlePontos) {
            glVertex2f(p.x, p.y);
        }
        glEnd();
    }
}

// Função do display
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Desenhar eixos
    desenharEixos();

    // Desenhar curva original em cinza se houver transformação aplicada
    if (atualTransform >= 0) {
        for (const auto& CURVA : originalCURVAs) {
            desenharCurvaBezier(CURVA, COR_ORIGINAL);
        }
    }

    // Desenhar curvas atuais
    for (const auto& CURVA : CURVAs) {
        desenharCurvaBezier(CURVA, COR_CURVA);
    }

    glutSwapBuffers();
}

// Função de reshape
void remodelar(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Manter proporção 
    float proporcao = (float)w / h;
    if (w <= h) {
        glOrtho(-100.0, 100.0, -100.0 / proporcao, 100.0 / proporcao, -1.0, 1.0);
    }
    else {
        glOrtho(-100.0 * proporcao, 100.0 * proporcao, -100.0, 100.0, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);
}

// Função de teclado
void teclado(unsigned char chave, int x, int y) {
    switch (chave) {
    case 'p':
    case 'P':
        mostrarPoligono = !mostrarPoligono;
        break;

    case ' ':  // Barra de espaço
        if (atualTransform < (int)transforms.size() - 1) {
            atualTransform++;
            CURVAs = originalCURVAs;  // Resetar para posição original

            // Aplicar todas as transformações até a atual
            for (int i = 0; i <= atualTransform; i++) {
                aplicarTransformacao(CURVAs, transforms[i]);
            }
        }
        else {
            atualTransform = -1;
            CURVAs = originalCURVAs;  // Voltar para posição original
        }
        break;

    case 'q': // Botão "q" ou "Q" para sair 
    case 'Q':
    case 27:  // Botão ESC para sair também
        exit(0);
        break;
    }

    glutPostRedisplay();
}

// Função para carregar arquivo .obj
bool loadObjFile(const char* filename) {
    std::ifstream file(filename);
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

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s arquivo.obj\n", argv[0]);
        return 1;
    }

    // Inicializar GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Visualizador de Curvas de Bézier");

    // Configurar cor de fundo
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Fundo preto

    // Carregar arquivo .obj
    if (!loadObjFile(argv[1])) {
        printf("Erro ao carregar esse arquivo %s\n", argv[1]);
        return 1;
    }

    // Registrar callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(remodelar);
    glutKeyboardFunc(teclado);

    // Iniciar loop principal
    glutMainLoop();

    return 0;
}
