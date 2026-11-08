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

Arquivos texto simples, um por grafo, sem depender de nenhuma biblioteca de JSON. Tudo depois de `#`
é comentário. Os nomes não podem ter espaço: `_` vira espaço (`Zé_Silva`) e `-` significa sem nome.
Erros de formato são informados com arquivo, linha e motivo (ex.: `company.txt:12: papel desconhecido "Chefe"`).

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

- `data/company.txt` é o grafo social fixo (14 funcionários em três grupos) usado na demonstração, com
  posições desenhadas à mão para ficar legível. A numeração foi escolhida para a demonstração: numa força
  bruta que testa os candidatos em ordem (papéis antes das arestas), cada preset passa por candidatos com
  os papéis certos mas com alguma colaboração faltando (arestas vermelhas) antes de encontrar a equipe,
  que aparece entre 38% e 53% da busca. Outra ordem de enumeração muda esses números.
- `data/presets/*.txt` são os moldes de equipe. O nome do arquivo (sem `.txt`) é o id do preset na CLI.
- Empresas maiores, para o benchmark, vêm do **gerador** (seção 5), não de arquivos.

### Presets
Todos têm pelo menos uma equipe válida em `company.txt`.

| Arquivo | Preset | Forma | k |
|---|---|---|---|
| `architecture_trio` | Trio de Arquitetura | Arquiteto, Dev e DevOps todos ligados entre si (triângulo) | 3 |
| `agile_squad` | Squad Ágil | Líder ligado a 2 Devs e 1 Designer (estrela) | 4 |
| `mentoring_chain` | Cadeia de Mentoria | Caminho Líder → Dev → Dev → QA | 4 |
| `star_team` | Time Estrela | Líder, Dev, Designer e QA, todos ligados entre si (clique) | 4 |
| `product_squad` | Squad de Produto | Ciclo Líder → Dev → Dev → QA → Líder, com Designer ligado ao Líder | 5 |

O Squad de Produto ficou com k = 5: com k = 6, a força bruta passaria de 2 milhões de candidatos
na empresa de 14 pessoas, acima do limite do trace (seção 4).

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
// src/solver.hpp (resumo)
enum class StepResult { Testing, Fail, Match };

class SearchContext {
public:
    // map[j] = funcionário atribuído ao vértice j do molde (-1 = ainda não atribuído)
    // Em Fail: failU/failV = aresta do molde que não existe na empresa; ou failU = vértice do molde
    // com papel errado e failV = -1. A interface pinta de vermelho o que falhou.
    // Retorna false se a busca deve parar (tempo limite, ou equipe encontrada no modo padrão).
    bool step(const int* map, int k, StepResult r, int failU = -1, int failV = -1);
};

// Implementado pelo Kleiton. Retorna true e preenche `mapping` com a primeira equipe encontrada.
bool bruteForce(const Graph& pattern, const Graph& company,
                SearchContext& ctx, std::vector<int>& mapping);
```

A documentação completa do contrato fica no começo de `src/solvers/brute_force.cpp`.

Depois da busca, `runSolver` confere a equipe devolvida contra as três regras da seção 2 e avisa se ela
for inválida. Essa conferência não fica disponível para os solvers: cada algoritmo faz a sua própria
verificação.

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
  Escolhe uma aresta `{u, v}` do molde e remove da empresa todas as colaborações entre alguém com o papel
  de `u` e alguém com o papel de `v` (ex.: nenhum Líder trabalhou com nenhum Designer). Todos os papéis
  continuam presentes, então a falta de um papel nunca resolve o problema sozinha.
- Em todos os modos com molde, a empresa tem pelo menos tantos funcionários de cada papel quanto o molde exige.
- O gerador de números aleatórios é próprio (não o `<random>`): a mesma `seed` gera o mesmo grafo, byte a
  byte, no Windows e no Linux.

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

- A cor de cada vértice indica o **papel** (mesma cor na empresa, no molde e na legenda). Na Fase 5, a
  correspondência `j ↔ map[j]` é mostrada destacando os funcionários do candidato atual com o número da
  posição do molde.
- **Seletor de empresa:** a empresa de demonstração (`company.txt`, posições do arquivo) ou empresas geradas
  com 25, 50 e 100 pessoas (modo plantada com o molde escolhido, `seed` 1, grau médio 4 para o desenho
  ficar legível). As geradas usam layout automático de forças (Fruchterman–Reingold), determinístico.
- Passar o mouse sobre um funcionário mostra nome, papel e número de colaborações.
- A velocidade vai de 1 a 10.000 passos/s, em escala logarítmica.
- A aba **Benchmark** lê `results/bench.csv` e plota as duas séries (tempo × n e tempo × k) em escala
  log, marcando a partir de que tamanho a busca passa do tempo limite (detalhes na Fase 6).

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

- **Windows:** w64devkit (versão de referência: 2.10.1). O MinGW.org GCC 6.3 foi testado na Fase 1 e **não**
  compila a raylib: faltam headers do Windows (`minwindef.h`, `dinput.h`).
- O módulo de áudio da raylib (`raudio`) não é compilado, porque o projeto não usa som.
- No Windows, o `teams_gui.exe` leva ícone e informações de versão (`src/gui/teams_gui.rc`). Na Fase 4, o
  Kaspersky marcou como `VHO:Trojan-Dropper` (detecção heurística, falso positivo) uma versão que capturava
  a tela para salvar imagens. A captura foi removida; as imagens do README são tiradas à mão.
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
  generator.hpp / generator.cpp  gerador de empresas e moldes
  solvers/registry.cpp           lista de solvers
  solvers/brute_force.cpp        ← ALGORITMO DO KLEITON
  gui/                           interface (main, graph_view, panels)
  bench/main.cpp                 CLI de benchmark
data/company.txt
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

### ✅ Fase 1 — Toolchain e esqueleto de build
- Testar se o compilador atual compila a raylib. Se não compilar, instalar o w64devkit e documentar.
- Vendorizar raylib 5.5 e raygui. Escrever o Makefile multiplataforma.
- `src/gui/main.cpp` abre uma janela vazia com título e fonte carregada.
- **Checkpoint:** `make && make run` abre a janela.
- **Para revisar:** Makefile (alvos, flags, detecção de SO) e o README com as instruções de build.

### ✅ Fase 2 — Núcleo de dados e contrato
- `Graph`, leitor dos arquivos `.txt` com mensagens de erro claras (linha e motivo).
- `data/company.txt` e os presets.
- `solver.hpp`, `SearchContext` (trace, contador, timeout), registro de solvers e o **stub** de
  `brute_force.cpp` com comentários explicando o contrato.
- Gerador de empresas (plantada / sem solução) e gerador de moldes.
- CLI `teams_bench` com `--list`, `--run` (empresa do arquivo ou gerada), `--generate-company` e
  `--generate-pattern`. As séries do benchmark ficam para a Fase 6.
- **Checkpoint:** `teams_bench --list` lista os presets e a empresa; rodar o stub retorna "sem solução"
  sem travar.
- **Para revisar:** o formato dos arquivos, os presets fazem sentido? O contrato da seção 4 está confortável para implementar?

### ✅ Fase 3 — Algoritmo de força bruta (Kleiton)
- Implementar `bruteForce` seguindo o contrato. Pode rodar em paralelo com as Fases 4 e 5.
- **Checkpoint:** `teams_bench --run <preset>` encontra uma equipe válida para cada preset na
  `company.txt`, e `--run <preset> --n 30 --mode nosolution` responde "sem solução".

### ✅ Fase 4 — Desenho dos grafos
- Renderizar a empresa e o molde: cores por papel, nomes, legenda, layout a partir das posições do
  arquivo (ou layout automático para empresas geradas).
- Painel de presets com miniatura do molde e seleção de solver/empresa.
- **Checkpoint:** a interface mostra a empresa e o preset selecionado, sem execução ainda.
- **Para revisar:** visual geral (cores, legibilidade, disposição dos painéis).

### ✅ Fase 5 — Execução e reprodução passo a passo
- Botão Executar: roda o solver em modo visual e guarda o trace. A busca roda numa thread separada (a
  janela continua respondendo), com botão **Cancelar** e tempo limite de 60 s.
- Controles de reprodução: início, passo anterior, reproduzir/pausar, próximo passo, fim, velocidade
  (1 a 10.000 passos/s), linha do tempo arrastável e "Pular para a solução". Atalhos: Espaço, ←/→,
  Home/End. A reprodução pausa sozinha ao chegar na equipe encontrada.
- Destaques do passo atual:
  - os funcionários do candidato ficam em destaque, com o número da posição do molde (1, 2, 3...);
    os demais ficam esmaecidos;
  - papel errado: anel vermelho no funcionário e na posição do molde (as arestas não são destacadas);
  - papéis certos: cada aresta do molde fica verde se a colaboração existe, ou vermelha (tracejada na
    empresa) se falta;
  - uma frase no rodapé explica o passo (ex.: "Falha: falta a colaboração entre Fábio (1) e Diego (4)").
- O resultado ("Equipe encontrada", painel "Equipe formada") só aparece quando a reprodução chega nele.
- Trace truncado: aviso com quantos passos foram gravados; "Pular para a solução" mostra a equipe final
  mesmo que ela tenha sido encontrada depois do limite do trace.
- **Checkpoint:** com o algoritmo da Fase 3 (ou o solver descartável), a busca é reproduzida do início
  até a solução.
- **Para revisar:** a animação explica bem o que a força bruta está fazendo?

### ✅ Fase 6 — Benchmark
- CLI `teams_bench` com duas séries, ambas nos modos plantada e sem solução:
  - **n crescente, k fixo:** para cada preset, aumenta o tamanho da empresa até estourar o tempo
    limite. Responde "até que tamanho de empresa é viável".
  - **k crescente, n fixo:** aumenta o tamanho do molde (moldes gerados) numa empresa de tamanho fixo.
    Com `k` fixo, a força bruta cresce como `n^k` (polinomial). A explosão exponencial típica de um
    problema NP-Completo aparece quando `k` cresce.
- Tempo limite padrão de **10 s** por execução, configurável com `--timeout`. Os números do relatório
  usam `--timeout 60` (`make bench BENCH_ARGS="--timeout 60"`). Com 10 s, o benchmark completo leva uns
  5 minutos.
- Tamanhos da série n: 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, ... até 4096 (crescimento de ~1,5x).
  Série k: n = 20 e k = 2, 3, 4, ... Cada série para no primeiro tamanho em que alguma execução estoura
  o tempo limite.
- O modo plantada roda **3 repetições** (seeds 1, 2, 3; `--reps`), porque o tempo até achar depende de
  onde a equipe foi embutida; os gráficos usam a mediana. O modo sem solução roda uma vez: o trabalho é
  sempre percorrer todos os `n!/(n-k)!` candidatos.
- Se o solver não testar nenhum candidato (o stub), o benchmark avisa e para, em vez de varrer todos os
  tamanhos à toa.
- Grava `results/bench.csv` com as colunas
  `solver,series,preset,mode,n,k,m,seed,found,candidates,time_ms,timeout` (uma linha por execução).
- **Aba Benchmark** na interface:
  - filtro de modo (sem solução / plantada) numa linha acima dos gráficos;
  - gráfico de linhas tempo × n (uma linha por preset, cor fixa por preset, eixo de tempo em log) e
    tempo × k, com a linha do tempo limite e marcador vazado nas execuções que estouraram;
  - legenda, tooltip ao passar o mouse e uma tabela-resumo "Até que tamanho a busca é viável?";
  - a paleta de cores foi validada para daltonismo (as cores de menor contraste são compensadas pela
    legenda, pelo tooltip e pela tabela);
  - o CSV é relido ao entrar na aba e pelo botão Recarregar.
- Observação que aparece nos resultados: no pior caso, moldes com o mesmo `k` levam o mesmo tempo, porque
  a força bruta testa todos os candidatos, sejam quais forem as arestas.
- **Checkpoint:** `make bench` gera o CSV e a aba plota os dados.
- **Para revisar:** o gráfico responde claramente "até que tamanho é viável"?

### ✅ Fase 7 — Polimento e entrega
- README completo: o problema, como compilar (Linux primeiro, que é o mais provável para o professor), a
  interface passo a passo, a linha de comando com entradas e saídas esperadas, o benchmark, os resultados
  (tempo limite de 60 s), o formato dos arquivos, o experimento com LLM e a estrutura do projeto.
- Imagens da interface em `docs/images/`, geradas com o solver real. Elas são capturadas num build Linux
  (WSL), porque o código de captura de tela dispara o falso positivo do antivírus no Windows (seção 7).
- Build limpo testado do zero, a partir de um clone novo do GitHub, no Windows (w64devkit) e no Linux
  (WSL Ubuntu 22.04).
- **Checkpoint:** um clone novo do repositório compila e roda seguindo só o README.

### ✅ Fase 8 — Experimento com LLM (parte ii do trabalho)
- Enunciado: verificar como uma LLM generalista se sai ao resolver os mesmos casos fictícios. Ela consegue
  solucionar? Propõe soluções incorretas? Quão eficiente é usá-la para esse problema?
- Não implementamos um modelo: a pasta `llm/` traz uma mensagem pronta para colar em agentes/modelos, o
  gabarito e um modelo para registrar os resultados.
  - `llm/prompt.md`: a mensagem, com as regras do problema, o formato de resposta e 7 casos;
  - `llm/answers.md`: todas as equipes válidas de cada caso, conferidas pelo solver de força bruta
    (`--all`) e por um script independente;
  - `llm/results.md`: tabela para registrar cada teste e as perguntas do enunciado para a análise;
  - `llm/cases/`: os arquivos de dados criados para o experimento (um molde e duas empresas geradas).
- Os 7 casos:

  | Caso | Molde | Empresa | Resposta |
  |---|---|---|---|
  | 1 | Trio de Arquitetura (k = 3) | exemplo (14 pessoas) | 1 equipe |
  | 2 | Squad Ágil (k = 4) | exemplo | 2 equipes (4 formas) |
  | 3 | Time Completo (k = 4, clique) | exemplo | **sem solução** (quase-encaixe: falta 1 colaboração) |
  | 4 | Time Estrela (k = 4, clique) | exemplo | 1 equipe |
  | 5 | Squad de Produto (k = 5) | exemplo | 2 equipes |
  | 6 | Squad Ágil (k = 4) | gerada, 20 pessoas | **sem solução** (nenhum Líder colabora com Designer) |
  | 7 | Squad de Produto (k = 5) | gerada, 30 pessoas | 7 formas |

- Os casos sem solução testam se a LLM inventa uma equipe; o caso 7 testa um tamanho em que conferir de
  cabeça fica difícil.
- Na mensagem, as colaborações aparecem em ordem alfabética: a ordem dos arquivos agrupa as pessoas por
  equipe e, nas empresas geradas, deixa as colaborações da equipe embutida no fim, o que daria pistas.
- **Checkpoint:** a mensagem pode ser colada como está, e cada resposta pode ser corrigida só com o gabarito.

---

## 11. Decisões tomadas
- **Tempo limite do benchmark:** 10 s por padrão, configurável; relatório com 60 s (seção 10, Fase 6).
- **Densidade das empresas geradas:** grau médio fixo `d = 6`, configurável (seção 5).
- **Primeira equipe ou todas:** o contexto decide pelo retorno de `step` no `Match`; o padrão é parar
  na primeira (seção 4).
- **Benchmark com duas séries:** n crescente com k fixo, e k crescente com n fixo (seção 10, Fase 6).
