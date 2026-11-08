// Algoritmo de força bruta - implementado por Kleiton.
//
// ---------------------------------------------------------------------------------------------
// O que a função recebe
// ---------------------------------------------------------------------------------------------
//   pattern  molde da equipe, com k = pattern.vertexCount() vértices
//   company  grafo social da empresa, com n = company.vertexCount() funcionários
//   ctx      contexto da busca (passo a passo dos candidatos testados, contagem de passos, etc.)
//   mapping  saída: se achar uma equipe, mapping[j] = funcionário escolhido para o vértice j do molde
// ---------------------------------------------------------------------------------------------

#include "solver.hpp"
#include <utility>

namespace
{
    /**
     * Verifica se o mapeamento atual é válido, ou seja, se os papéis correspondem e se todas as arestas do molde existem na empresa.
     *
     * @param pattern O molde da equipe.
     * @param company O grafo social da empresa.
     * @param ctx O contexto da busca, usado para registrar os passos.
     * @param mapping O mapeamento atual dos vértices do molde para os funcionários.
     * @return Um par de booleanos:
     *         - O primeiro indica se o mapeamento é válido.
     *         - O segundo indica se a busca deve parar (true se deve continuar, false caso contrário).
     */
    std::pair<bool, bool> isValidMapping(const Graph &pattern, const Graph &company, SearchContext &ctx, const std::vector<int> &mapping)
    {
        int k = pattern.vertexCount();

        // Verifica se os papéis nas devidas posições correspondem.
        for (int i = 0; i < k; ++i)
        {
            if (company.role(mapping[i]) != pattern.role(i))
            {
                // Retorna um par com os dois bools indicando que a verificação falhou devido ao papel do vértice i.
                return {false, ctx.step(mapping.data(), k, StepResult::Fail, i)};
            }
        }

        // Verifica se todas as arestas do molde existem na empresa.
        for (const auto &edge : pattern.edges())
        {
            int u = edge.first;
            int v = edge.second;
            if (!company.hasEdge(mapping[u], mapping[v]))
            {
                return {false, ctx.step(mapping.data(), k, StepResult::Fail, u, v)};
            }
        }

        return {true, ctx.step(mapping.data(), k, StepResult::Match)};
    }

    /**
     * Verifica se um determinado valor já foi usado no mapeamento atual até o índice especificado.
     *
     * @param mapping O mapeamento atual dos vértices do molde para os funcionários.
     * @param index O índice até o qual verificar (exclusivo).
     * @param value O valor a ser verificado.
     * @return true se o valor já foi usado, false caso contrário.
     */
    bool alreadyInUse(const std::vector<int> &mapping, int index, int value)
    {
        for (int i = 0; i < index; ++i)
        {
            if (mapping[i] == value)
            {
                return true;
            }
        }
        return false;
    }

    /**
     * Função recursiva que testa todos os candidatos possíveis para preencher o molde da equipe.
     *
     * @param pattern O molde da equipe.
     * @param company O grafo social da empresa.
     * @param ctx O contexto da busca, usado para registrar os passos.
     * @param mapping O mapeamento atual dos vértices do molde para os funcionários.
     * @param index O índice atual no mapeamento.
     * @param foundMatch Um booleano que indica se uma equipe válida foi encontrada.
     * @return true se a busca deve parar, false caso contrário. Pode parar por encontrar uma equipe válida ou por atingir o limite de tempo.
     */
    bool testCandidates(const Graph &pattern, const Graph &company, SearchContext &ctx, std::vector<int> &mapping, int index, bool &foundMatch)
    {
        int k = pattern.vertexCount();
        int n = company.vertexCount();

        if (index == k)
        {
            // Todos os vértices do molde foram preenchidos. Hora de verificar.
            std::pair<bool, bool> res = isValidMapping(pattern, company, ctx, mapping);
            foundMatch = foundMatch || res.first; // Atualiza foundMatch se encontrou uma equipe válida
            return !res.second;                   // Retorna true se a busca deve parar.
        }

        for (int i = 0; i < n; ++i)
        {
            // Antes de atribuir o funcionário, verificamos se ele já é usado no mapping atual.
            if (alreadyInUse(mapping, index, i))
            {
                continue;
            }
            mapping[index] = i;
            if (testCandidates(pattern, company, ctx, mapping, index + 1, foundMatch))
            {
                return true;
            }
        }

        return false;
    }
}

/**
 * O algoritmo de força bruta para encontrar equipes válidas no grafo social da empresa que correspondam ao molde da equipe.
 *
 * @param pattern O molde da equipe.
 * @param company O grafo social da empresa.
 * @param ctx O contexto da busca, usado para registrar os passos.
 * @param mapping O mapeamento atual dos vértices do molde para os funcionários.
 * @return true se uma equipe válida for encontrada, false caso contrário.
 */
bool bruteForce(const Graph &pattern, const Graph &company, SearchContext &ctx, std::vector<int> &mapping)
{
    mapping.resize(pattern.vertexCount(), -1); // Inicializa o mapping com -1 (sem atribuição)
    int k = pattern.vertexCount();
    int n = company.vertexCount();

    if (k > n)
    {
        // Não há funcionários suficientes para preencher o molde.
        return false;
    }

    bool foundMatch = false;
    testCandidates(pattern, company, ctx, mapping, 0, foundMatch);

    if (foundMatch)
    {
        // Se encontrou pelo menos uma equipe, a primeira encontrada ficará em mapping.
        mapping = ctx.firstMatch();
    }

    return foundMatch;
}
