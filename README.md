<div align="center">

# Projeto 1 CG

</div>

### _Projeto em `OpenGL` sobre Curvas de Bézier na disciplina da Universidade: `Computação Gráfica`_

<img src="assets/image.png" alt="O Inseto" ></img>

<div align="center">

## 👁️Tutorial Para Visualizar

</div>

1. Clique [aqui](https://sal0minh0.github.io/projeto1_CG/) 👈
2. Clique `duas vezes` na tela do site e estará no `FullScreen`
3. Aperte `"p"` e vc visualizará o polígono de controle
4. Aperte `"Espaço"` continuamente e vc visualizará as transformações (Com a ordem Translação, Rotação e Escala)
5. Aperte `"Esc"` e vc sairá do programa ou `"Q"` (Não funciona na versão Web)

<div align="center">

## 🤓O que esse código faz?

</div>

- **_É um visualizador interativo de curvas de Bézier_** 📈 <br> 
- **_Feito em [C ++](https://cplusplus.com/)_** 👨‍💻 <br>
- **_Usando [OpenGL ES 3.0](https://www.khronos.org/opengles/) com [GLFW](https://www.glfw.org/) e [Emscripten](https://emscripten.org) (Para versão Web)._** 🪟

<div align="center">

## 🧐A funcionalidade principal do programa:

</div>

- Inclui carregar curvas de um arquivo .obj;
- Aplicar 3 transformações geométricas (Translação (t), Rotação (r) e Escala (s));
- E exibi-las com suporte a visualização em tempo real.

<div align="center">

## 📄Arquivo .obj

|         `"v"`         |         `"c"`         |              `"t/r/s"`               |
| :-------------------: | :-------------------: | :----------------------------------: |
| Coordenadas de pontos | Inicia uma nova curva | Define as transformações geométricas |

</div>

[Comentário]: <> (Estilos para a Imagem)

<style>
    body {
      display: flex;
      justify-content: center;
      align-items: center;
      height: 190vh;
    }
    img {
      max-width: 80%;
      height: auto;
      margin-left: 80px;
    }
</style>
