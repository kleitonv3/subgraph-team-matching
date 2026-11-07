# Subgraph Team Matching

> Matchmaking de equipes usando **Isomorfismo de Subgrafos** — projeto da disciplina de Teoria da Computação.

🚧 **Em construção**

## O problema

Na hora de montar um time para um projeto complexo, o RH costuma avaliar cada pessoa isoladamente
(quem sabe Java, quem sabe SQL). Só que projetos reais dependem da **dinâmica do grupo**: quem já
trabalhou bem com quem, quem se comunica bem com quem.

Este projeto modela a empresa como um **grafo social**:

- **Vértices** são os funcionários, cada um com um papel (Líder, Dev, Designer, Arquiteto, ...).
- **Arestas** representam um histórico de boa colaboração entre duas pessoas.

A empresa descreve a **equipe ideal** como um grafo pequeno, o "molde". Por exemplo: um líder que já
trabalhou com dois devs e um designer. O sistema procura no grafo da empresa um subgrupo de
funcionários que se encaixe nesse molde. Isso é o problema de **Isomorfismo de Subgrafos**, que é
NP-Completo.

## Objetivo desta fase

- Resolver o problema por **força bruta**, testando todos os mapeamentos possíveis até achar uma solução.
- Medir **até que tamanho de entrada** a força bruta encontra uma resposta em tempo viável.
- Mostrar numa **interface gráfica** (C++ + raylib) o passo a passo dos subgrafos testados.

## Como compilar e rodar

O projeto precisa só de um compilador C/C++ e do `make`. A raylib (gráficos) e a raygui (botões e
controles) já vêm dentro do repositório, em `third_party/`, e são compiladas junto.

| Comando | O que faz |
|---|---|
| `make` | compila tudo (a primeira vez demora mais, porque compila a raylib) |
| `make run` | compila, se precisar, e abre a interface |
| `make clean` | apaga os arquivos gerados pelo build |

### Windows

O MinGW antigo (MinGW.org, GCC 6.3) **não** compila a raylib. Use o **w64devkit**, um GCC moderno
portátil: não precisa de instalador nem de permissão de administrador.

1. Baixe o `w64devkit-x64-<versão>.7z.exe` em <https://github.com/skeeto/w64devkit/releases>
   (versão de referência: 2.10.1).
2. Execute o arquivo e escolha onde extrair, por exemplo `C:\`. Ele cria a pasta `C:\w64devkit`.
3. Abra o `C:\w64devkit\w64devkit.exe`. É um terminal com o compilador e o `make` já configurados.
4. Vá até a pasta do projeto e rode:
   ```sh
   cd C:/caminho/para/subgraph-team-matching
   make run
   ```

Também dá para usar o terminal de sempre: basta colocar `C:\w64devkit\bin` no `PATH`, **antes** de
qualquer outro MinGW.

### Linux (Debian/Ubuntu)

```sh
sudo apt install build-essential libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev
make run
```

### macOS

Ainda não testado. Deve bastar ter as ferramentas de linha de comando do Xcode (`xcode-select --install`)
e rodar `make run`.

## Bibliotecas e recursos de terceiros

| O quê | Onde | Licença |
|---|---|---|
| [raylib](https://github.com/raysan5/raylib) 5.5 | `third_party/raylib/` | zlib |
| [raygui](https://github.com/raysan5/raygui) 4.0 | `third_party/raygui/` | zlib |
| Fonte [Inter](https://github.com/rsms/inter) 4.1 | `assets/fonts/` | SIL Open Font License 1.1 |

## Licença

[MIT](LICENSE)
