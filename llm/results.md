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
| 1 | ChatGPT (modelo não informado) | 08/11/2026 | sem ferramentas, todos os casos | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | 7/7 | 100% de confiança em todos; no caso 3, conclusão certa com justificativa parcialmente falsa |

Referência (força bruta do projeto, `teams_bench --run ... --all`): acerta os 7 casos e encontra todas as
equipes de cada um em menos de 1 segundo. O caso 7, o maior, testa todos os 30·29·28·27·26 = 17.100.720
candidatos em cerca de 0,13 s.

## Respostas

### Teste 1 — ChatGPT (modelo não informado), 08/11/2026, sem ferramentas

Acertou os 7 casos, todos com 100% de confiança.

- Casos 1, 2 e 4: equipes certas (nos casos 1 e 4, a única que existe).
- Caso 3: respondeu SEM SOLUÇÃO, que está certo, mas a justificativa afirma que nem Elisa nem Gabi colaboram
  com Marina. **Gabi–Marina existe**; a única colaboração que falta no grupo de Fábio é Elisa–Marina. A
  conclusão certa veio de um raciocínio parcialmente falso, com 100% de confiança. Para o grupo de Lucas, a
  justificativa está certa (faltam Ana–Diego e Carla–Diego).
- Caso 5: respondeu a equipe do grupo de Lucas (Lucas, Diego, Ana, Carla, Bruno), a segunda das duas que existem.
- Caso 6: SEM SOLUÇÃO, com o motivo certo (nenhum dos quatro líderes colabora com um Designer). Ao listar os
  vizinhos de P02, omitiu P00, o que não muda a conclusão.
- Caso 7: P15, P26, P09, P29, P04, uma das 7 formas válidas (é a equipe que o gerador embutiu na empresa).

<details><summary>Resposta completa</summary>

```text
### Caso 1: EQUIPE

1 (Arquiteto) = Heitor\
2 (Dev) = Elisa\
3 (DevOps) = Nuno

Justificativa: As três colaborações exigidas existem:

- Heitor–Elisa: colaboração registrada.
- Heitor–Nuno: colaboração registrada.
- Elisa–Nuno: colaboração registrada.

Os três funcionários possuem papéis distintos e formam um triângulo completo de colaborações.

Confiança: 100%.

### Caso 2: EQUIPE

1 (Líder) = Fábio\
2 (Dev) = Elisa\
3 (Dev) = Gabi\
4 (Designer) = Marina

Justificativa: As três colaborações exigidas existem:

- Fábio–Elisa: colaboração registrada.
- Fábio–Gabi: colaboração registrada.
- Fábio–Marina: colaboração registrada.

Cada funcionário ocupa uma única posição e todos possuem o papel correspondente.

Confiança: 100%.

### Caso 3: SEM SOLUÇÃO

Justificativa: Existem apenas dois líderes possíveis, e nenhum deles permite formar o grupo completo:

- Fábio: suas únicas opções de Dev que colaboraram com ele são Elisa e Gabi. Embora Elisa–Gabi seja uma colaboração existente, nenhuma das duas possui colaboração com Marina, a única Designer que já trabalhou com Fábio.
- Lucas: suas únicas opções de Dev são Ana e Carla. Embora Ana–Carla seja uma colaboração existente, nenhuma delas possui colaboração com Diego, o único Designer que já trabalhou com Lucas.

Portanto, não é possível formar uma equipe em que todos os quatro funcionários tenham colaborado entre si.

Confiança: 100%.

### Caso 4: EQUIPE

1 (Líder) = Fábio\
2 (Dev) = Gabi\
3 (Designer) = Marina\
4 (QA) = João

Justificativa: Todas as seis colaborações exigidas existem:

- Fábio–Gabi: colaboração registrada.
- Fábio–Marina: colaboração registrada.
- Fábio–João: colaboração registrada.
- Gabi–Marina: colaboração registrada.
- Gabi–João: colaboração registrada.
- Marina–João: colaboração registrada.

Os quatro funcionários formam um grupo em que todos já trabalharam com todos.

Confiança: 100%.

### Caso 5: EQUIPE

1 (Líder) = Lucas\
2 (Designer) = Diego\
3 (Dev) = Ana\
4 (Dev) = Carla\
5 (QA) = Bruno

Justificativa: Todas as cinco colaborações exigidas existem:

- Lucas–Diego: colaboração registrada.
- Lucas–Ana: colaboração registrada.
- Ana–Carla: colaboração registrada.
- Carla–Bruno: colaboração registrada.
- Bruno–Lucas: colaboração registrada.

O ciclo completo Lucas → Ana → Carla → Bruno → Lucas está presente. Além disso, Diego, o Designer, também possui colaboração com Lucas.

Confiança: 100%.

### Caso 6: SEM SOLUÇÃO

Justificativa: Nenhum dos quatro líderes disponíveis possui colaboração com um Designer.

Verificando os líderes:

- P00: possui colaboração com P02, P09, P11 e P18. Nenhum deles é Designer.
- P02: possui colaboração com P04, P10, P13 e P16. Nenhum deles é Designer.
- P03: possui colaboração com P05, P14 e P16. Nenhum deles é Designer.
- P17: possui colaboração com P04, P08, P10, P11, P13 e P16. Nenhum deles é Designer.

Como a posição 4 exige um Designer que tenha colaborado com o Líder, e nenhum líder atende a essa condição, não existe equipe possível.

Confiança: 100%.

### Caso 7: EQUIPE

1 (Líder) = P15\
2 (Designer) = P26\
3 (Dev) = P09\
4 (Dev) = P29\
5 (QA) = P04

Justificativa: Todas as cinco colaborações exigidas existem:

- P15–P26: colaboração registrada.
- P15–P09: colaboração registrada.
- P09–P29: colaboração registrada.
- P29–P04: colaboração registrada.
- P04–P15: colaboração registrada.

O ciclo completo P15 → P09 → P29 → P04 → P15 está presente. Além disso, P26, o Designer, também possui colaboração com P15, o Líder.

Todos os funcionários ocupam posições distintas e possuem os papéis exigidos.

Confiança: 100%.
```

</details>

<!-- Modelo para os próximos testes:

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
