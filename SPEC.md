# Especificação — Subgraph Team Matching

Documento vivo com o escopo, as decisões e as fases de implementação do projeto.
Cada fase termina num **checkpoint**: algo que dá para rodar e revisar antes de seguir para a próxima.

---

## 1. Visão geral

A empresa é modelada como um **grafo social** e a equipe desejada como um **grafo molde**. O sistema
procura um subgrupo de funcionários que se encaixe no molde, o que é uma instância de
**Isomorfismo de Subgrafos** (NP-Completo).

Nesta etapa da disciplina:
1. A solução é por **força bruta**: testar as possibilidades até encontrar uma que sirva.
2. É preciso descobrir **até que tamanho de entrada** a força bruta responde em tempo viável.
3. Uma **interface gráfica** mostra o passo a passo dos candidatos testados.

### Quem faz o quê

| Parte | Responsável |
|---|---|
| Algoritmo de solução (`src/solvers/brute_force.cpp`) | **Kleiton** |
| Todo o resto: estruturas, leitura de arquivos, presets, gerador, tracer, interface, benchmark, Makefile | Claude |

O repositório nunca recebe código de solução escrito pelo Claude. Para testar a interface antes de o
algoritmo ficar pronto, o Claude usa um solver descartável que fica fora do repositório.

---

## 2. Modelo do problema

### Entradas
- **Empresa** `G = (V, E)`: cada vértice é um funcionário com **nome** e **papel**. Cada aresta não
  direcionada `{a, b}` significa que **a** e **b** têm histórico de boa colaboração.
- **Molde** `P = (V', E')`: grafo pequeno (k vértices) em que cada vértice tem um **papel**, e as arestas
  são as colaborações exigidas.

### O que é uma solução
Um mapeamento `map: V' → V` tal que:
1. **Injetivo:** duas posições do molde nunca recebem o mesmo funcionário.
2. **Papel compatível:** `papel(map[i]) == papel(i)` para todo vértice `i` do molde.
3. **Arestas preservadas:** para toda aresta `{u, v}` do molde, `{map[u], map[v]}` é aresta da empresa.

> **Decisão:** usamos a versão **não induzida**, ou seja, colaborações *extras* entre membros do time
> escolhido são permitidas. Faz sentido no domínio, porque o fato de duas pessoas do time já terem
> trabalhado juntas nunca é motivo para descartar a equipe.

### Papéis
Conjunto fixo, comparado por igualdade exata: `Lider`, `Dev`, `Designer`, `Arquiteto`, `QA`, `DevOps`,
`Dados`. Cada papel tem uma cor fixa na interface.

---

## 3. Formato dos arquivos

Arquivos texto simples, um por grafo, sem depender de nenhuma biblioteca de JSON. Linhas que começam
com `#` são comentários.

```
name Squad Ágil
desc Um líder que já trabalhou com dois devs e um designer
vertices 4
# id  papel     nome     x     y      (nome, x e y são opcionais; x/y normalizados em 0..1)
0     Lider     -        0.50  0.15
1     Dev       -        0.20  0.75
2     Dev       -        0.50  0.75
3     Designer  -        0.80  0.75
edges 3
0 1
0 2
0 3
```

- `data/empresa.txt` é o grafo social fixo (~14 funcionários) usado na demonstração, com posições
  desenhadas à mão para ficar legível.
- `data/presets/*.txt` são os moldes de equipe.
- Empresas maiores, para o benchmark, vêm do **gerador** (seção 5), não de arquivos.

### Presets iniciais
| Preset | Forma | k |
|---|---|---|
| Squad Ágil | Líder ligado a 2 Devs e 1 Designer (estrela) | 4 |
| Trio de Arquitetura | Arquiteto, Dev e DevOps todos ligados entre si (triângulo) | 3 |
| Cadeia de Mentoria | Caminho Líder → Dev → Dev → QA | 4 |
| Time Estrela | Clique de 4 (todos já trabalharam com todos) | 4 |
| Squad de Produto | Líder, Designer, 2 Devs, QA, com ciclo + ramificação | 5–6 |

---

## 4. Contrato entre o algoritmo e o resto do sistema

A ideia central é que **o algoritmo não desenha nada**. A cada candidato testado, ele informa o
`SearchContext`, e é o contexto que decide o que fazer com essa informação:

- **Modo visual (interface):** grava cada passo num *trace* em memória (limite de ~500 mil passos;
  acima disso só conta). A interface depois **reproduz** o trace na velocidade escolhida, então não
  importa se o algoritmo levou 2 ms.
- **Modo benchmark:** só conta os passos e verifica o relógio. Quando o tempo limite estoura,
  `step` retorna `false` e o algoritmo deve parar. Isso mantém a medição limpa.

**Regra única para o algoritmo:** depois de cada `step`, se o retorno for `false`, pare. Isso cobre os
dois casos:
- **Tempo limite:** `step` retorna `false` em qualquer passo.
- **Equipe encontrada (`Match`):** o contexto decide. Por padrão retorna `false` e a busca para na
  primeira equipe. No modo "listar todas" retorna `true` e a busca continua, sem mudar o algoritmo.

```cpp
// src/solver.hpp (esboço)
enum class StepResult { Testing, Fail, Match };

class SearchContext {
public:
    // map[i] = funcionário atribuído ao vértice i do molde (-1 = ainda não atribuído)
    // failU/failV = aresta do molde que falhou, se houver (a interface pinta de vermelho)
    // Retorna false se a busca deve parar (tempo limite, ou equipe encontrada no modo padrão).
    bool step(const int* map, int k, StepResult r, int failU = -1, int failV = -1);
};

// Implementado pelo Kleiton. Retorna true e preenche `mapping` se achou uma equipe.
bool bruteForce(const Graph& pattern, const Graph& company,
                SearchContext& ctx, std::vector<int>& mapping);
```

Os solvers ficam registrados numa lista (`src/solvers/registry.cpp`). Nas próximas etapas da disciplina,
basta adicionar outro solver (backtracking, poda por papel, VF2…) e ele aparece automaticamente na
interface e no benchmark.

---

## 5. Gerador de instâncias

### Empresas
Serve para criar empresas de tamanho `n` arbitrário para o benchmark:
- Parâmetros: `n`, **grau médio** `d` (padrão 6), distribuição de papéis e `seed`, para que o resultado
  seja reproduzível.
- A probabilidade de cada aresta é `p = d / (n − 1)`. Assim cada funcionário tem em média `d`
  colaboradores, seja qual for o tamanho da empresa, como numa rede social real (esparsa). Com `p`
  fixo, empresas grandes ficariam densas demais.
- **Modo "solução plantada":** embute o molde em posições aleatórias, garantindo que existe resposta.
- **Modo "sem solução":** garante que não existe resposta, o que força a busca a percorrer tudo (pior caso).

### Moldes
Para a série com `k` crescente (Fase 6), gera moldes conexos aleatórios de `k` vértices, com papéis
sorteados e `seed`.

---

## 6. Interface (raylib + raygui, 1280×720)

```
┌──────────────┬─────────────────────────────────────┬──────────────┐
│ PRESETS      │                                     │ MOLDE        │
│ [Squad Ágil] │        GRAFO DA EMPRESA             │   (L)        │
│ [Trio Arq.]  │   vértices coloridos por papel      │   / | \      │
│ [Mentoria]   │   candidato atual destacado         │ (D)(D)(Ds)   │
│ ...          │   aresta ok = verde                 │              │
│ Solver: [▼]  │   aresta faltando = vermelho        │ EQUIPE       │
│ Empresa: [▼] │                                     │ FORMADA      │
│ [Executar]   │                                     │ (ao final)   │
├──────────────┴─────────────────────────────────────┴──────────────┤
│ |◀  ◀  ▶/❚❚  ▶  ▶|   velocidade ━━━●━━   passo 1.234 / 5.040    │
│ candidatos testados · tempo real da busca · [pular p/ solução]    │
└───────────────────────────────────────────────────────────────────┘
          Abas:  [ Visualização ]  [ Benchmark ]
```

- A cor de cada vértice do molde se repete no funcionário mapeado, então dá para ver a correspondência `i ↔ map[i]`.
- A velocidade vai de 1 a 10.000 passos/s, em escala logarítmica.
- A aba **Benchmark** lê `results/bench.csv` e plota as duas séries (tempo × n e tempo × k) em escala
  log, marcando a partir de que tamanho a busca passa do tempo limite.

---

## 7. Build

- **Linguagem:** C++17 (C++14 se o compilador exigir).
- **raylib 5.5** e **raygui** ficam **vendorizados** em `third_party/`, e o Makefile compila tudo. Assim o
  professor só precisa de um compilador C/C++ e do `make`.
- Alvos do Makefile:

| Comando | O que faz |
|---|---|
| `make` | compila a interface (`teams_gui`) e a CLI de benchmark (`teams_bench`) |
| `make run` | abre a interface |
| `make bench` | roda o benchmark e gera `results/bench.csv` |
| `make clean` | apaga os artefatos de build |

- **Windows:** MinGW-w64 ou w64devkit. O MinGW.org GCC 6.3 que está instalado hoje provavelmente é antigo
  demais; isso é testado na Fase 1.
- **Linux:** `build-essential` + `libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev`.

---

## 8. Estrutura de pastas (alvo)

```
Makefile
README.md
SPEC.md
src/
  graph.hpp / graph.cpp          Graph + leitura de arquivos
  solver.hpp / context.cpp       SearchContext, trace, timeout
  generator.hpp / generator.cpp  gerador de empresas
  solvers/registry.cpp           lista de solvers
  solvers/brute_force.cpp        ← ALGORITMO DO KLEITON
  gui/                           interface (main, graph_view, panels)
  bench/main.cpp                 CLI de benchmark
data/empresa.txt
data/presets/*.txt
assets/fonts/                    fonte TTF livre com acentos
third_party/raylib/, third_party/raygui/
```

---

## 9. Convenções

- **Idioma:** o código é escrito em inglês (identificadores, nomes de arquivos, palavras-chave do formato
  de dados, colunas do CSV). Comentários, documentação, textos da interface e mensagens da CLI são em
  português, com acentuação.
- **Commits:** gitmoji com shortcode e mensagem em inglês.

---

## 10. Fases de implementação

**Fluxo de trabalho:** cada fase é feita numa branch `fase-N-<nome>` e vira um **Pull Request** no GitHub.
O Kleiton revisa o diff e faz o merge, e só então começa a fase seguinte. Os commits seguem o **gitmoji**.

### ✅ Fase 0 — Repositório
- Repositório público no GitHub, gitmoji-cli instalado, `.gitignore`, README básico e licença MIT.

### Fase 1 — Toolchain e esqueleto de build
- Testar se o compilador atual compila a raylib. Se não compilar, instalar o w64devkit e documentar.
- Vendorizar raylib 5.5 e raygui. Escrever o Makefile multiplataforma.
- `src/gui/main.cpp` abre uma janela vazia com título e fonte carregada.
- **Checkpoint:** `make && make run` abre a janela.
- **Para revisar:** Makefile (alvos, flags, detecção de SO) e o README com as instruções de build.

### Fase 2 — Núcleo de dados e contrato
- `Graph`, leitor dos arquivos `.txt` com mensagens de erro claras (linha e motivo).
- `data/empresa.txt` e os presets.
- `solver.hpp`, `SearchContext` (trace, contador, timeout), registro de solvers e o **stub** de
  `brute_force.cpp` com comentários explicando o contrato.
- Gerador de empresas (plantada / sem solução) e gerador de moldes.
- **Checkpoint:** `teams_bench --list` lista os presets e a empresa; rodar o stub retorna "sem solução"
  sem travar.
- **Para revisar:** o formato dos arquivos, os presets fazem sentido? O contrato da seção 4 está confortável para implementar?

### Fase 3 — Algoritmo de força bruta (Kleiton)
- Implementar `bruteForce` seguindo o contrato. Pode rodar em paralelo com as Fases 4 e 5.
- **Checkpoint:** a CLI encontra uma equipe válida para cada preset na `empresa.txt`.

### Fase 4 — Desenho dos grafos
- Renderizar a empresa e o molde: cores por papel, nomes, legenda, layout a partir das posições do
  arquivo (ou layout automático para empresas geradas).
- Painel de presets com miniatura do molde e seleção de solver/empresa.
- **Checkpoint:** a interface mostra a empresa e o preset selecionado, sem execução ainda.
- **Para revisar:** visual geral (cores, legibilidade, disposição dos painéis).

### Fase 5 — Execução e reprodução passo a passo
- Botão Executar: roda o solver em modo visual e guarda o trace.
- Controles de reprodução, destaque do candidato atual, arestas verde/vermelho, painel "Equipe formada",
  aviso de trace truncado.
- **Checkpoint:** com o algoritmo da Fase 3 (ou o solver descartável), a busca é reproduzida do início
  até a solução.
- **Para revisar:** a animação explica bem o que a força bruta está fazendo?

### Fase 6 — Benchmark
- CLI `teams_bench` com duas séries, ambas nos modos plantada e sem solução:
  - **n crescente, k fixo:** para cada preset, aumenta o tamanho da empresa até estourar o tempo
    limite. Responde "até que tamanho de empresa é viável".
  - **k crescente, n fixo:** aumenta o tamanho do molde (moldes gerados) numa empresa de tamanho fixo.
    Com `k` fixo, a força bruta cresce como `n^k` (polinomial). A explosão exponencial típica de um
    problema NP-Completo aparece quando `k` cresce.
- Tempo limite padrão de **10 s** por execução, configurável com `--timeout`. Os números do relatório
  usam `--timeout 60`.
- Grava `results/bench.csv` com as colunas
  `solver,series,preset,mode,n,k,m,found,candidates,time_ms,timeout`.
- Aba Benchmark na interface, com os gráficos tempo × n e tempo × k.
- **Checkpoint:** `make bench` gera o CSV e a aba plota os dados.
- **Para revisar:** o gráfico responde claramente "até que tamanho é viável"?

### Fase 7 — Polimento e entrega
- README completo (como compilar, rodar, formato dos arquivos, prints da interface).
- Build limpo testado do zero, no Windows e se possível no Linux.
- **Checkpoint:** um clone novo do repositório compila e roda seguindo só o README.

---

## 11. Decisões tomadas
- **Tempo limite do benchmark:** 10 s por padrão, configurável; relatório com 60 s (seção 10, Fase 6).
- **Densidade das empresas geradas:** grau médio fixo `d = 6`, configurável (seção 5).
- **Primeira equipe ou todas:** o contexto decide pelo retorno de `step` no `Match`; o padrão é parar
  na primeira (seção 4).
- **Benchmark com duas séries:** n crescente com k fixo, e k crescente com n fixo (seção 10, Fase 6).
