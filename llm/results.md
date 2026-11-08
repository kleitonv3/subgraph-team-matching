# Experimento com LLM — resultados

Registro dos testes feitos com a mensagem de [prompt.md](prompt.md), corrigidos com o gabarito de
[answers.md](answers.md).

## Como registrar

Para cada teste (um agente/modelo, uma conversa nova), preencha uma linha da tabela e, se quiser, guarde a
resposta completa na seção "Respostas". Na coluna de cada caso, use:

| Símbolo | Significado |
|---|---|
| ✓ | resposta certa (equipe válida, ou SEM SOLUÇÃO nos casos 3 e 6) |
| ✗ inválida | equipe que viola alguma regra: anote qual (pessoa repetida, papel errado ou colaboração que não existe) |
| ✗ inventada | equipe proposta num caso sem solução (casos 3 e 6) |
| ✗ sem solução | disse SEM SOLUÇÃO num caso que tem equipe |
| — | não respondeu o caso |

Também vale anotar a **confiança** que a LLM declarou em cada caso: errar com 95% de confiança é bem
diferente de errar com 40%.

## Resumo

| # | Agente / modelo | Data | Variação | C1 | C2 | C3 | C4 | C5 | C6 | C7 | Acertos | Observações |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 |  |  | sem ferramentas |  |  |  |  |  |  |  | /7 |  |

Referência (força bruta do projeto, `teams_bench --run ... --all`): acerta os 7 casos e encontra todas as
equipes de cada um em menos de 1 segundo. O caso 7, o maior, testa todos os 30·29·28·27·26 = 17.100.720
candidatos em cerca de 0,13 s.

## Respostas

<!-- Cole aqui as respostas completas, uma subseção por teste. Exemplo:

### Teste 1 — <agente/modelo>, <data>

- Caso 3: respondeu Fábio, Elisa, Gabi, Marina com 90% de confiança; a colaboração Elisa–Marina não existe.

<details><summary>Resposta completa</summary>

(texto da resposta)

</details>
-->

## Análise

Perguntas do enunciado, para responder com base na tabela:

- **A LLM consegue solucionar?** Em quais casos acertou? O tamanho da empresa (casos 1 a 5 com 14 pessoas,
  caso 6 com 20, caso 7 com 30) fez diferença?
- **Ela propõe soluções incorretas?** Inventou equipes nos casos 3 e 6? Que tipo de erro aparece mais
  (colaboração inexistente, papel errado, pessoa repetida)? A justificativa "conferia" colaborações que não
  existem?
- **Quão eficiente é usar uma LLM generalista para esse problema?** Compare com a força bruta: tempo, custo e,
  principalmente, a garantia de correção (a força bruta nunca erra e sabe afirmar "sem solução"; a LLM não
  oferece essa garantia, e a resposta precisa ser conferida de qualquer forma).
