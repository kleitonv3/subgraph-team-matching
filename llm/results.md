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
| 2 | Gemini 3.5 Flash Lite (o modelo se identificou só como "Gemini 3") | 08/11/2026 | sem ferramentas, todos os casos | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | 7/7 | 100% de confiança em todos; nos casos sem solução, só afirma a conclusão, sem demonstrar |
| 3 | Claude Sonnet 5.5 | 08/11/2026 | sem ferramentas, todos os casos | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | 7/7 | 97% a 99% de confiança (menor nos casos sem solução); justificativas corretas e verificáveis em todos os casos |

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

### Teste 2 — Gemini 3.5 Flash Lite, 08/11/2026, sem ferramentas

O modelo se identificou só como "Gemini 3". Acertou os 7 casos, todos com 100% de confiança.

- Casos 1 e 4: as únicas equipes que existem.
- Caso 2: respondeu a equipe do grupo de Lucas (Lucas, Ana, Carla, Diego); o ChatGPT tinha respondido a do
  grupo de Fábio. As duas estão certas.
- Casos 3 e 6: SEM SOLUÇÃO, que está certo, mas as justificativas só **repetem a conclusão** ("não existe
  nenhum grupo... onde todos tenham colaborado mutuamente"; "nenhum dos líderes... possui colaboração com
  nenhum designer"). Nenhum candidato ou colaboração faltante é citado, então não dá para conferir o
  raciocínio pelo texto.
- Caso 5: equipe do grupo de Lucas, certa.
- Caso 7: P15, P26, P09, P29, P04, a mesma resposta do ChatGPT (a equipe que o gerador embutiu na empresa).

<details><summary>Resposta completa</summary>

```text
Caso 1: EQUIPE
1 (Arquiteto) = Heitor
2 (Dev) = Elisa
3 (DevOps) = Nuno
Justificativa: Heitor é Arquiteto e colabora com Elisa (Dev) e Nuno (DevOps); Elisa e Nuno também colaboram entre si (Elisa–Nuno), preenchendo todas as arestas do triângulo exigido.
Confiança: 100%

Caso 2: EQUIPE
1 (Líder) = Lucas
2 (Dev) = Ana
3 (Dev) = Carla
4 (Designer) = Diego
Justificativa: Lucas é Líder e colabora diretamente com Ana (Dev), Carla (Dev) e Diego (Designer), satisfazendo todas as colaborações exigidas pelo molde.
Confiança: 100%

Caso 3: SEM SOLUÇÃO
Justificativa: Não existe nenhum grupo na Empresa A formado por um líder, dois desenvolvedores e um designer onde todos os membros tenham colaborado mutuamente entre si (clique completo de 4 vértices).
Confiança: 100%

Caso 4: EQUIPE
1 (Líder) = Fábio
2 (Dev) = Gabi
3 (Designer) = Marina
4 (QA) = João
Justificativa: Fábio (Líder), Gabi (Dev), Marina (Designer) e João (QA) formam um grupo totalmente interconectado pelas colaborações exigidas (Fábio–Gabi, Fábio–Marina, Fábio–João, Gabi–Marina, Gabi–João e João–Marina).
Confiança: 100%

Caso 5: EQUIPE
1 (Líder) = Lucas
2 (Designer) = Diego
3 (Dev) = Ana
4 (Dev) = Carla
5 (QA) = Bruno
Justificativa: Lucas (Líder) colabora com Diego (Designer), Ana (Dev) e Bruno (QA); Ana colabora com Carla (Dev); e Carla colabora com Bruno (QA), fechando o ciclo de colaborações exigidas.
Confiança: 100%

Caso 6: SEM SOLUÇÃO
Justificativa: Nenhum dos líderes da Empresa B possui colaboração registrada com nenhum designer da empresa, tornando impossível atender à exigência de colaboração entre o líder e o designer.
Confiança: 100%

Caso 7: EQUIPE
1 (Líder) = P15
2 (Designer) = P26
3 (Dev) = P09
4 (Dev) = P29
5 (QA) = P04
Justificativa: P15 (Líder) colabora com P26 (Designer), P09 (Dev) e P04 (QA); P09 colabora com P29 (Dev); e P29 colabora com P04 (QA), completando o ciclo de colaborações exigidas.
Confiança: 100%
```

</details>

### Teste 3 — Claude Sonnet 5.5, 08/11/2026, sem ferramentas

Acertou os 7 casos, com 99% de confiança nos casos com equipe, 98% no caso 7 e 97% nos dois casos sem
solução. Todas as afirmações das justificativas foram conferidas com os dados e estão corretas.

- Casos 1, 2, 4 e 5: equipes certas. No caso 5, respondeu a equipe do grupo de Fábio (os testes 1 e 2
  responderam a do grupo de Lucas); as duas estão certas.
- Caso 3: SEM SOLUÇÃO, com a justificativa completa e correta: os únicos devs vizinhos de Fábio são Elisa e
  Gabi, o único designer vizinho é Marina, e Elisa–Marina não existe; Diego, o único designer vizinho de
  Lucas, só colabora com Lucas, então faltam Diego–Ana e Diego–Carla.
- Caso 6: SEM SOLUÇÃO, partindo dos 3 designers (o lado com menos pessoas para conferir) e listando os
  vizinhos de cada um sem erro: nenhum é líder.
- Caso 7: P15, P26, P09, P29, P04, a mesma resposta dos testes 1 e 2 (a equipe que o gerador embutiu na
  empresa), conferindo também papéis e repetição.

<details><summary>Resposta completa</summary>

```text
Caso 1: EQUIPE
1 (Arquiteto) = Heitor
2 (Dev) = Elisa
3 (DevOps) = Nuno
Justificativa: 1–2 existe porque Heitor–Elisa está na lista. 1–3 existe porque Heitor–Nuno está na lista. 2–3 existe porque Elisa–Nuno está na lista.
Confiança: 99%

Caso 2: EQUIPE
1 (Líder) = Fábio
2 (Dev) = Elisa
3 (Dev) = Gabi
4 (Designer) = Marina
Justificativa: 1–2 existe porque Fábio–Elisa está na lista. 1–3 existe porque Fábio–Gabi está na lista. 1–4 existe porque Fábio–Marina está na lista.
Confiança: 99%

Caso 3: SEM SOLUÇÃO
Justificativa: O líder precisa ter colaboração com dois devs e com o designer, e os dois devs e o designer precisam colaborar entre si. Há dois líderes na empresa.
- Fábio tem como devs vizinhos apenas Elisa e Gabi, e como designer vizinho apenas Marina. Elisa–Marina não existe, então falha.
- Lucas tem como designer vizinho apenas Diego, e Diego só colaborou com Lucas. Então Diego–Ana e Diego–Carla não existem, e falha.
Nenhuma escolha funciona.
Confiança: 97%

Caso 4: EQUIPE
1 (Líder) = Fábio
2 (Dev) = Gabi
3 (Designer) = Marina
4 (QA) = João
Justificativa: 1–2 existe porque Fábio–Gabi está na lista. 1–3 existe porque Fábio–Marina está na lista. 1–4 existe porque Fábio–João está na lista. 2–3 existe porque Gabi–Marina está na lista. 2–4 existe porque Gabi–João está na lista. 3–4 existe porque João–Marina está na lista.
Confiança: 99%

Caso 5: EQUIPE
1 (Líder) = Fábio
2 (Designer) = Marina
3 (Dev) = Elisa
4 (Dev) = Gabi
5 (QA) = João
Justificativa: 1–2 existe porque Fábio–Marina está na lista. 1–3 existe porque Fábio–Elisa está na lista. 3–4 existe porque Elisa–Gabi está na lista. 4–5 existe porque Gabi–João está na lista. 5–1 existe porque Fábio–João está na lista.
Confiança: 99%

Caso 6: SEM SOLUÇÃO
Justificativa: O líder precisa ter colaboração com um designer. Os designers da empresa B são P06, P12 e P19, e os líderes são P00, P02, P03 e P17.
- P06 colaborou apenas com P08 e P18.
- P12 colaborou apenas com P04, P10, P11, P13 e P15.
- P19 colaborou apenas com P01 e P14.
Nenhum desses vizinhos é líder, então nenhum líder tem colaboração com um designer.
Confiança: 97%

Caso 7: EQUIPE
1 (Líder) = P15
2 (Designer) = P26
3 (Dev) = P09
4 (Dev) = P29
5 (QA) = P04
Justificativa: 1–2 existe porque P15–P26 está na lista. 1–3 existe porque P15–P09 está na lista. 3–4 existe porque P09–P29 está na lista. 4–5 existe porque P04–P29 está na lista. 5–1 existe porque P04–P15 está na lista. Os papéis conferem (P15 Líder, P26 Designer, P09 e P29 Dev, P04 QA) e ninguém se repete.
Confiança: 98%
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
