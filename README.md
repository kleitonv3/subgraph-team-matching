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

## Licença

[MIT](LICENSE)
