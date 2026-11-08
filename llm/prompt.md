# Experimento com LLM — mensagem

Parte ii do trabalho: verificar como uma LLM generalista se sai ao resolver os mesmos casos fictícios do
problema (isomorfismo de subgrafos aplicado à formação de equipes). As respostas certas estão em
[answers.md](answers.md), e os resultados de cada teste são registrados em [results.md](results.md).

## Como usar

1. Abra uma **conversa nova** com o agente/modelo (sem histórico, para não haver contexto extra).
2. Copie a mensagem inteira do bloco abaixo e envie como uma única mensagem.
3. Registre a resposta em [results.md](results.md), conferindo cada caso com o [answers.md](answers.md).

Variações possíveis do experimento:
- **Sem ferramentas** (padrão): a mensagem pede para resolver sem escrever nem executar código.
- **Com ferramentas**: apague a linha "Resolva raciocinando, sem escrever nem executar código." para ver se
  o agente escreve um programa para resolver.
- **Um caso por vez**: envie o texto até "## Dados das empresas", os dados da empresa usada e um único caso.
  Serve para ver se o tamanho da mensagem atrapalha.

Os casos 3 e 6 **não têm solução** (servem para ver se a LLM inventa uma equipe), e o caso 7 usa uma empresa
de 30 pessoas, em que montar a resposta de cabeça fica bem mais difícil. Não conte isso para a LLM.

Os dados vêm dos arquivos do projeto: `data/company.txt` (empresa A), `llm/cases/company_20_nosolution.txt`
(empresa B), `llm/cases/company_30_planted.txt` (empresa C), os presets de `data/presets/` e
`llm/cases/full_team.txt` (caso 3). Nas empresas geradas, os funcionários não têm nome e aparecem como
P00, P01, ... (o número é o id no arquivo). Na mensagem, as colaborações estão em ordem alfabética: a ordem
dos arquivos agrupa as pessoas por equipe e, nas empresas geradas, deixa as colaborações da equipe embutida no
fim, o que daria pistas da resposta.

## Mensagem

```text
Você vai resolver um problema de formação de equipes em uma empresa.

A empresa é descrita por uma lista de funcionários, cada um com um papel (Líder, Dev, Designer, Arquiteto, QA, DevOps ou Dados), e por uma lista de colaborações: pares de pessoas que já trabalharam bem juntas. Uma colaboração vale nos dois sentidos (Ana–Lucas é o mesmo que Lucas–Ana).

Cada caso pede uma equipe no formato de um "molde": uma lista de posições numeradas, cada uma com um papel, e uma lista de colaborações exigidas entre posições.

Uma equipe é uma escolha de um funcionário para cada posição do molde que respeita TODAS as regras:
1. Cada funcionário ocupa no máximo uma posição (ninguém se repete).
2. O papel do funcionário é exatamente o papel da posição.
3. Para cada colaboração exigida entre duas posições, os dois funcionários escolhidos para elas aparecem como colaboração na lista da empresa.

Colaborações a mais entre os escolhidos não atrapalham. Se existir mais de uma equipe possível, basta indicar uma. Se nenhuma escolha respeitar todas as regras, responda SEM SOLUÇÃO.

Resolva raciocinando, sem escrever nem executar código.

Responda a cada caso exatamente neste formato:

Caso N: EQUIPE
1 (papel) = nome
2 (papel) = nome
...
Justificativa: por que cada colaboração exigida existe.
Confiança: de 0 a 100%.

ou, se não houver equipe:

Caso N: SEM SOLUÇÃO
Justificativa: por que nenhuma escolha funciona.
Confiança: de 0 a 100%.

## Dados das empresas

### Empresa A — Empresa Exemplo (usada nos casos 1 a 5)

Funcionários (14): Ana (Dev), Bruno (QA), Carla (Dev), Diego (Designer), Elisa (Dev), Fábio (Líder), Gabi (Dev), Heitor (Arquiteto), Íris (Dados), João (QA), Karen (DevOps), Lucas (Líder), Marina (Designer), Nuno (DevOps).

Colaborações (23 pares de pessoas que já trabalharam bem juntas):
Ana–Carla, Ana–Íris, Ana–Lucas, Bruno–Carla, Bruno–Lucas, Carla–Karen, Carla–Lucas, Diego–Lucas
Elisa–Fábio, Elisa–Gabi, Elisa–Heitor, Elisa–Nuno, Fábio–Gabi, Fábio–Íris, Fábio–João, Fábio–Marina
Gabi–João, Gabi–Marina, Heitor–Íris, Heitor–Karen, Heitor–Nuno, João–Marina, Karen–Nuno

### Empresa B — empresa maior (usada no caso 6)

Funcionários (20): P00 (Líder), P01 (QA), P02 (Líder), P03 (Líder), P04 (Dev), P05 (Arquiteto), P06 (Designer), P07 (DevOps), P08 (Dev), P09 (DevOps), P10 (QA), P11 (QA), P12 (Designer), P13 (Dev), P14 (QA), P15 (Dev), P16 (Dev), P17 (Líder), P18 (Dev), P19 (Designer).

Colaborações (55 pares de pessoas que já trabalharam bem juntas):
P00–P02, P00–P09, P00–P11, P00–P18, P01–P05, P01–P14, P01–P15, P01–P19
P02–P04, P02–P10, P02–P13, P02–P16, P03–P05, P03–P14, P03–P16, P04–P05
P04–P09, P04–P12, P04–P13, P04–P14, P04–P16, P04–P17, P05–P07, P05–P09
P05–P18, P06–P08, P06–P18, P07–P11, P07–P13, P07–P15, P08–P10, P08–P11
P08–P14, P08–P16, P08–P17, P08–P18, P09–P10, P09–P11, P09–P16, P09–P18
P10–P12, P10–P17, P11–P12, P11–P13, P11–P16, P11–P17, P12–P13, P12–P15
P13–P15, P13–P17, P14–P15, P14–P16, P14–P18, P14–P19, P16–P17

### Empresa C — empresa maior (usada no caso 7)

Funcionários (30): P00 (Designer), P01 (QA), P02 (Dados), P03 (Dev), P04 (QA), P05 (QA), P06 (DevOps), P07 (Designer), P08 (Dev), P09 (Dev), P10 (Dev), P11 (Arquiteto), P12 (Dev), P13 (Designer), P14 (Dev), P15 (Líder), P16 (Arquiteto), P17 (DevOps), P18 (QA), P19 (DevOps), P20 (Líder), P21 (Líder), P22 (Designer), P23 (Dev), P24 (Dev), P25 (Líder), P26 (Designer), P27 (QA), P28 (Líder), P29 (Dev).

Colaborações (103 pares de pessoas que já trabalharam bem juntas):
P00–P03, P00–P08, P00–P09, P00–P11, P00–P12, P00–P21, P00–P29, P01–P02
P01–P06, P01–P09, P01–P12, P01–P21, P01–P25, P01–P28, P01–P29, P02–P12
P02–P13, P02–P20, P02–P22, P02–P24, P02–P25, P03–P19, P03–P21, P03–P22
P04–P05, P04–P08, P04–P11, P04–P12, P04–P15, P04–P18, P04–P25, P04–P29
P05–P06, P05–P08, P05–P12, P05–P16, P05–P19, P05–P20, P05–P22, P05–P25
P05–P28, P06–P07, P06–P10, P06–P14, P06–P20, P06–P22, P06–P25, P06–P29
P07–P19, P07–P22, P07–P24, P07–P28, P08–P11, P08–P12, P08–P13, P08–P29
P09–P14, P09–P15, P09–P16, P09–P29, P10–P14, P10–P19, P10–P25, P10–P26
P11–P16, P11–P21, P11–P24, P11–P29, P12–P13, P12–P14, P12–P15, P12–P22
P13–P26, P13–P27, P13–P29, P15–P16, P15–P19, P15–P20, P15–P25, P15–P26
P15–P29, P16–P26, P16–P28, P17–P18, P17–P19, P17–P20, P17–P24, P17–P26
P18–P20, P18–P21, P19–P20, P19–P26, P20–P27, P21–P26, P23–P24, P23–P28
P23–P29, P24–P25, P24–P27, P24–P29, P25–P26, P26–P27, P27–P28

## Casos

### Caso 1 — Trio de Arquitetura (Empresa A)
Arquiteto, dev e devops que já trabalharam todos juntos.
Posições: 1 = Arquiteto, 2 = Dev, 3 = DevOps.
Colaborações exigidas entre as posições: 1–2, 1–3, 2–3.

### Caso 2 — Squad Ágil (Empresa A)
Um líder que já trabalhou com dois devs e com um designer.
Posições: 1 = Líder, 2 = Dev, 3 = Dev, 4 = Designer.
Colaborações exigidas entre as posições: 1–2, 1–3, 1–4.

### Caso 3 — Time Completo (Empresa A)
Líder, dois devs e um designer em que todos já trabalharam com todos.
Posições: 1 = Líder, 2 = Dev, 3 = Dev, 4 = Designer.
Colaborações exigidas entre as posições: 1–2, 1–3, 1–4, 2–3, 2–4, 3–4.

### Caso 4 — Time Estrela (Empresa A)
Líder, dev, designer e QA em que todos já trabalharam com todos.
Posições: 1 = Líder, 2 = Dev, 3 = Designer, 4 = QA.
Colaborações exigidas entre as posições: 1–2, 1–3, 1–4, 2–3, 2–4, 3–4.

### Caso 5 — Squad de Produto (Empresa A)
Um ciclo líder → dev → dev → QA → líder, com um designer que também já trabalhou com o líder.
Posições: 1 = Líder, 2 = Designer, 3 = Dev, 4 = Dev, 5 = QA.
Colaborações exigidas entre as posições: 1–2, 1–3, 3–4, 4–5, 5–1.

### Caso 6 — Squad Ágil (Empresa B)
Um líder que já trabalhou com dois devs e com um designer.
Posições: 1 = Líder, 2 = Dev, 3 = Dev, 4 = Designer.
Colaborações exigidas entre as posições: 1–2, 1–3, 1–4.

### Caso 7 — Squad de Produto (Empresa C)
Um ciclo líder → dev → dev → QA → líder, com um designer que também já trabalhou com o líder.
Posições: 1 = Líder, 2 = Designer, 3 = Dev, 4 = Dev, 5 = QA.
Colaborações exigidas entre as posições: 1–2, 1–3, 3–4, 4–5, 5–1.
```
