#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#define _USE_MATH_DEFINES 
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <emscripten/emscripten.h>

// Estrutura para ponto 2D
struct Pontos {
    float x, y;
    Pontos(float x = 0, float y = 0) : x(x), y(y) {}
};

// Estrutura para transforma��o
struct Transformacao {
    char tipo;   // 't' para transla��o, 'r' para rota��o, 's' para escala
    float param[3];  // [x, y] para transla��o e escala, [angulo, px, py] para rota��o
};

// Vari�veis globais
std::vector<std::vector<Pontos>> CURVAs;  // Lista de curvas (cada curva tem seus pontos de controle)
std::vector<Transformacao> transforms;  // Lista de transforma��es
bool mostrarPoligono = false;  // Controle para exibir/ocultar pol�gono de controle
int atualTransform = -1;  // �ndice da transforma��o atual
std::vector<std::vector<Pontos>> originalCURVAs;  // C�pia das curvas originais

// Cores
const float COR_EIXO_X[] = { 0.0f, 1.0f, 0.0f };  // Verde
const float COR_EIXO_Y[] = { 0.0f, 0.0f, 1.0f };  // Azul
const float COR_CURVA[] = { 1.0f, 0.0f, 0.0f };   // Vermelho
const float COR_ORIGINAL[] = { 0.5f, 0.5f, 0.5f }; // Cinza
const float COR_PONTOS_CONTROLE[] = { 1.0f, 1.0f, 0.0f };  // Amarelo

// Fun��o para calcular ponto na curva de B�zier usando o algoritmo de De Casteljau
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

// Aplicar transforma��o aos pontos
void aplicarTransformacao(std::vector<std::vector<Pontos>>& CURVAs, const Transformacao& transform) {
    for (auto& CURVA : CURVAs) {
        for (auto& ponto : CURVA) {
            switch (transform.tipo) {
            case 't':  // Transla��o
                ponto.x += transform.param[0];
                ponto.y += transform.param[1];
                break;

            case 'r': { // Rota��o em torno de um ponto
                float angulo = transform.param[0];
                float px = transform.param[1];  // Ponto de rota��o x
                float py = transform.param[2];  // Ponto de rota��o y

                // Converter �ngulo para radianos
                float rad = angulo * M_PI / 180.0f;

                // Transla��o para origem do ponto de rota��o
                float dx = ponto.x - px;
                float dy = ponto.y - py;

                // Aplicar rota��o
                ponto.x = px + (dx * cos(rad) - dy * sin(rad));
                ponto.y = py + (dx * sin(rad) + dy * cos(rad));
                break;
            }

            case 's': { // Escala
                float sx = transform.param[0];  // Fator de escala x
                float sy = transform.param[1];  // Fator de escala y

                // Aplicar escala em rela��o � origem
                ponto.x *= sx;
                ponto.y *= sy;
                break;
            }
            }
        }
    }
}

// Fun��o para desenhar o plano cartesiano
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

// Fun��o para desenhar uma curva de B�zier
void desenharCurvaBezier(const std::vector<Pontos>& controlePontos, const float COR[3]) {
    glColor3fv(COR);
    glBegin(GL_LINE_STRIP);

    for (float t = 0; t <= 1.0f; t += 0.01f) {
        Pontos p = calcularPontosBezier(controlePontos, t);
        glVertex2f(p.x, p.y);
    }

    glEnd();

    // Desenhar pol�gono de controle se necess�rio
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

// Fun��o do display
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Desenhar eixos
    desenharEixos();

    // Desenhar curva original em cinza se houver transforma��o aplicada
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

// Fun��o de reshape
void remodelar(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Manter propor��o 
    float proporcao = (float)w / h;
    if (w <= h) {
        glOrtho(-100.0, 100.0, -100.0 / proporcao, 100.0 / proporcao, -1.0, 1.0);
    }
    else {
        glOrtho(-100.0 * proporcao, 100.0 * proporcao, -100.0, 100.0, -1.0, 1.0);
    }

    glMatrixMode(GL_MODELVIEW);
}

// Fun��o de teclado
void teclado(unsigned char chave, int x, int y) {
    switch (chave) {
    case 'p':
    case 'P':
        mostrarPoligono = !mostrarPoligono;
        break;

    case ' ':  // Barra de espa�o
        if (atualTransform < (int)transforms.size() - 1) {
            atualTransform++;
            CURVAs = originalCURVAs;  // Resetar para posi��o original

            // Aplicar todas as transforma��es at� a atual
            for (int i = 0; i <= atualTransform; i++) {
                aplicarTransformacao(CURVAs, transforms[i]);
            }
        }
        else {
            atualTransform = -1;
            CURVAs = originalCURVAs;  // Voltar para posi��o original
        }
        break;

    case 'q': // Bot�o "q" ou "Q" para sair 
    case 'Q':
    case 27:  // Bot�o ESC para sair tamb�m
        exit(0);
        break;
    }

    glutPostRedisplay();
}

// Fun��o para carregar arquivo .obj
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

        if (tipo == 'v') {  // V�rtice
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
        else if (tipo == 't' || tipo == 'r' || tipo == 's') {  // Transforma��o
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

    // Adicionar �ltima curva se houver
    if (!atualCURVA.empty()) {
        CURVAs.push_back(atualCURVA);
    }

    // Fazer c�pia das curvas originais
    originalCURVAs = CURVAs;

    file.close();
    return true;
}

int main(int argc, char** argv) {
    // Inicializar GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Visualizador de Curvas de Bézier");

    // Configurar cor de fundo
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Fundo preto

    // Carregar arquivo .obj
    if (!loadObjFile("arquivo.obj")) {
        printf("Erro ao carregar esse arquivo arquivo.obj\n");
        return 1;
    }

    // Registrar callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(remodelar);
    glutKeyboardFunc(teclado);

    // Iniciar loop principal usando Emscripten
    emscripten_set_main_loop(glutMainLoop, 0, 1);

    return 0;
}