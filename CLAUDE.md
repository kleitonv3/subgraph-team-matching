# Subgraph Team Matching

Escopo, decisões e fases do projeto: ver [SPEC.md](SPEC.md).

## Idioma

- **Código em inglês:** nomes de variáveis, funções, classes, enums, arquivos, pastas, alvos do Makefile,
  palavras-chave do formato dos arquivos de dados e colunas do CSV.
- **Texto para pessoas em português:** comentários, documentação (README, SPEC), textos da interface
  gráfica, mensagens da CLI e mensagens de erro. Usar acentuação correta (a fonte da interface suporta).

```cpp
// Retorna false se a busca deve ser interrompida (tempo limite).
bool step(const int* map, int k, StepResult r);
DrawText("Equipe formada", x, y, 20, DARKGRAY);
```

## Regras do projeto

- O algoritmo de solução (`src/solvers/brute_force.cpp` e futuros solvers) é escrito pelo Kleiton.
  O Claude nunca escreve código de solução no repositório.
- Commits seguem o gitmoji com shortcode e mensagem em inglês (`:sparkles: Add preset loader`).
