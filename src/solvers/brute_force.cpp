// Algoritmo de força bruta — implementado pelo Kleiton.
//
// ---------------------------------------------------------------------------------------------
// O que a função recebe
// ---------------------------------------------------------------------------------------------
//   pattern  molde da equipe, com k = pattern.vertexCount() vértices
//   company  grafo social da empresa, com n = company.vertexCount() funcionários
//   ctx      contexto da busca (veja abaixo)
//   mapping  saída: se achar uma equipe, mapping[j] = funcionário escolhido para o vértice j do molde
//
// API do Graph (src/graph.hpp) mais útil aqui:
//   g.vertexCount()        número de vértices
//   g.role(v)              papel do vértice v (Role::Leader, Role::Dev, ...)
//   g.hasEdge(a, b)        existe a aresta {a, b}? custo O(1)
//   g.edges()              lista de arestas {a, b}, cada uma uma vez
//   g.neighbors(v)         vizinhos de v
//
// ---------------------------------------------------------------------------------------------
// O que é uma solução (SPEC, seção 2)
// ---------------------------------------------------------------------------------------------
// Um mapeamento map: vértices do molde -> funcionários tal que:
//   1. injetivo: dois vértices do molde nunca recebem o mesmo funcionário;
//   2. papéis:   company.role(map[j]) == pattern.role(j) para todo j;
//   3. arestas:  para toda aresta {u, v} do molde, company.hasEdge(map[u], map[v]).
// Arestas extras entre os escolhidos são permitidas (versão não induzida).
//
// ---------------------------------------------------------------------------------------------
// Como falar com o contexto
// ---------------------------------------------------------------------------------------------
// Para cada candidato testado, chame:
//
//     bool keepGoing = ctx.step(map, k, StepResult::Fail, u, v);   // descartado pela aresta {u, v}
//     bool keepGoing = ctx.step(map, k, StepResult::Fail, j);      // descartado pelo papel do vértice j
//     bool keepGoing = ctx.step(map, k, StepResult::Fail);         // descartado por outro motivo
//     bool keepGoing = ctx.step(map, k, StepResult::Match);        // equipe válida
//
// onde map é um array de k inteiros (map[j] = funcionário do vértice j; -1 se ainda não atribuído).
// Os índices u, v e j são vértices do MOLDE; a interface usa essa informação para pintar de vermelho
// o que falhou.
//
// Regra única: se ctx.step(...) retornar false, pare a busca e retorne. Isso acontece quando o tempo
// limite estoura, ou logo depois de um Match (por padrão a busca para na primeira equipe; no modo
// "listar todas" o contexto retorna true e a busca continua).
//
// Retorno: true se encontrou pelo menos uma equipe; nesse caso, preencha `mapping` com a primeira
// equipe encontrada. O sistema confere a equipe devolvida contra as três regras acima.

#include "solver.hpp"

bool bruteForce(const Graph& pattern, const Graph& company, SearchContext& ctx, std::vector<int>& mapping) {
    // TODO(Kleiton): implementar a busca por força bruta.
    (void)pattern;
    (void)company;
    (void)ctx;
    (void)mapping;
    return false;
}
