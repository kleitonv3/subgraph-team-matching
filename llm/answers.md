# Experimento com LLM — respostas certas

Gabarito dos casos de [prompt.md](prompt.md). Para cada caso estão **todas** as equipes válidas, encontradas
de duas formas independentes (o solver de força bruta do projeto com `--all` e um script de conferência que
testa todas as combinações), com o mesmo resultado.

## Como corrigir uma resposta

Uma resposta de equipe está certa se, e somente se:
1. ninguém se repete;
2. o papel de cada pessoa é o papel da posição;
3. cada colaboração exigida pelo molde aparece na lista de colaborações da empresa.

Uma resposta certa é sempre uma das equipes listadas abaixo (elas são todas as que existem). Se a LLM
responder SEM SOLUÇÃO num caso que tem equipe, ou inventar uma equipe num caso sem solução, a resposta está
errada.

Para conferir com o programa, use os arquivos indicados em cada caso:

```sh
./bin/teams_bench --run PRESET --company EMPRESA --all --timeout 0
```

## Resumo

| Caso | Molde | Empresa | Resposta certa | Formas válidas |
|---|---|---|---|---|
| 1 | Trio de Arquitetura (k = 3) | A | EQUIPE | 1 |
| 2 | Squad Ágil (k = 4) | A | EQUIPE | 4 |
| 3 | Time Completo (k = 4) | A | **SEM SOLUÇÃO** | 0 |
| 4 | Time Estrela (k = 4) | A | EQUIPE | 1 |
| 5 | Squad de Produto (k = 5) | A | EQUIPE | 2 |
| 6 | Squad Ágil (k = 4) | B | **SEM SOLUÇÃO** | 0 |
| 7 | Squad de Produto (k = 5) | C | EQUIPE | 7 |

## Caso 1 — Trio de Arquitetura (Empresa A)

Arquivos: `data/presets/architecture_trio.txt` e `data/company.txt`.

- 1 (Arquiteto) = Heitor · 2 (Dev) = Elisa · 3 (DevOps) = Nuno

Única equipe. Heitor–Elisa, Heitor–Nuno e Elisa–Nuno existem.

## Caso 2 — Squad Ágil (Empresa A)

Arquivos: `data/presets/agile_squad.txt` e `data/company.txt`.

- 1 (Líder) = Fábio · 2 (Dev) = Elisa · 3 (Dev) = Gabi · 4 (Designer) = Marina
- 1 (Líder) = Fábio · 2 (Dev) = Gabi · 3 (Dev) = Elisa · 4 (Designer) = Marina
- 1 (Líder) = Lucas · 2 (Dev) = Ana · 3 (Dev) = Carla · 4 (Designer) = Diego
- 1 (Líder) = Lucas · 2 (Dev) = Carla · 3 (Dev) = Ana · 4 (Designer) = Diego

Duas equipes; cada uma aparece duas vezes porque as posições 2 e 3 (Dev) podem ser trocadas entre si, e as duas formas estão certas.

## Caso 3 — Time Completo (Empresa A)

Arquivos: `llm/cases/full_team.txt` e `data/company.txt`.

**Não existe equipe.** O caso foi feito para ser um quase-encaixe: os dois líderes têm devs e designers por perto, mas sempre falta pelo menos uma colaboração.

| Tentativa | Colaborações que faltam |
|---|---|
| Fábio, Elisa, Gabi, Marina | Elisa–Marina (as outras 5 existem) |
| Lucas, Ana, Carla, Diego | Ana–Diego e Carla–Diego |

Misturar os grupos não ajuda: Fábio não colabora com Ana, Carla nem Diego, e Lucas não colabora com Elisa, Gabi nem Marina. A resposta errada mais provável é **Fábio, Elisa, Gabi, Marina**, que viola a colaboração Elisa–Marina.

## Caso 4 — Time Estrela (Empresa A)

Arquivos: `data/presets/star_team.txt` e `data/company.txt`.

- 1 (Líder) = Fábio · 2 (Dev) = Gabi · 3 (Designer) = Marina · 4 (QA) = João

Única equipe. As 6 colaborações (todos com todos) entre Fábio, Gabi, Marina e João existem.

## Caso 5 — Squad de Produto (Empresa A)

Arquivos: `data/presets/product_squad.txt` e `data/company.txt`.

- 1 (Líder) = Fábio · 2 (Designer) = Marina · 3 (Dev) = Elisa · 4 (Dev) = Gabi · 5 (QA) = João
- 1 (Líder) = Lucas · 2 (Designer) = Diego · 3 (Dev) = Ana · 4 (Dev) = Carla · 5 (QA) = Bruno

Duas equipes, uma em cada grupo. Na segunda: Lucas–Diego, Lucas–Ana, Ana–Carla, Carla–Bruno e Bruno–Lucas existem.

## Caso 6 — Squad Ágil (Empresa B)

Arquivos: `data/presets/agile_squad.txt` e `llm/cases/company_20_nosolution.txt`.

**Não existe equipe.** O Squad Ágil exige uma colaboração entre o Líder (posição 1) e o Designer (posição 4), e na empresa B **nenhum** líder (P00, P02, P03, P17) colabora com **nenhum** designer (P06, P12, P19). A empresa foi gerada no modo "sem solução", que remove todas as colaborações entre esses dois papéis. Os outros requisitos (um líder com dois devs vizinhos) são fáceis de satisfazer, então uma LLM pode "achar" uma equipe trocando a colaboração exigida por outra parecida.

## Caso 7 — Squad de Produto (Empresa C)

Arquivos: `data/presets/product_squad.txt` e `llm/cases/company_30_planted.txt`.

- 1 (Líder) = P15 · 2 (Designer) = P26 · 3 (Dev) = P09 · 4 (Dev) = P29 · 5 (QA) = P04
- 1 (Líder) = P15 · 2 (Designer) = P26 · 3 (Dev) = P12 · 4 (Dev) = P08 · 5 (QA) = P04
- 1 (Líder) = P15 · 2 (Designer) = P26 · 3 (Dev) = P29 · 4 (Dev) = P08 · 5 (QA) = P04
- 1 (Líder) = P25 · 2 (Designer) = P26 · 3 (Dev) = P24 · 4 (Dev) = P29 · 5 (QA) = P01
- 1 (Líder) = P25 · 2 (Designer) = P26 · 3 (Dev) = P24 · 4 (Dev) = P29 · 5 (QA) = P04
- 1 (Líder) = P28 · 2 (Designer) = P07 · 3 (Dev) = P23 · 4 (Dev) = P24 · 5 (QA) = P27
- 1 (Líder) = P28 · 2 (Designer) = P07 · 3 (Dev) = P23 · 4 (Dev) = P29 · 5 (QA) = P01

Sete formas válidas, todas com conjuntos de pessoas diferentes. Qualquer uma delas está certa; para conferir outra resposta, verifique as três regras uma a uma com as listas do prompt.
