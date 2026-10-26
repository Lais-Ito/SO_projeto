# Simulador Visual de Sistema Operacional Multitarefa (Projeto A)

Este repositório contém a implementação completa em C do Simulador Visual de Sistema Operacional Multitarefa Preemptivo de Tempo Compartilhado (Projeto A).

---

## Visão Geral do Projeto

O simulador modela a execução de tarefas periódicas em um sistema operacional com múltiplos processadores/CPUs. Ele implementa algoritmos de escalonamento de tempo real (Rate Monotonic - RM e Earliest Deadline First - EDF), com uma fila global de prontos e 5 critérios estritos de desempate.

O projeto conta com uma interface visual interativa no terminal, um debugger passo-a-passo com navegação temporal Undo/Redo e edição ao vivo de tarefas, além de exportação automática do gráfico de Gantt em vetor SVG ao final da simulação.

---

## Arquitetura do Código-Fonte e Estrutura de Diretórios

```text
.
├── include/
│   ├── types.h          # Definições centrais (TCB, Ticks, Histórico, Configurações, Eventos)
│   ├── parser.h         # Protótipos para leitura e validação do arquivo de configuração
│   ├── escalonador.h    # Protótipos dos algoritmos RM, EDF e 5 critérios de desempate
│   ├── engine.h         # Protótipos do motor de simulação discreta por ticks e snapshots
│   └── frontend.h       # Protótipos da interface terminal ANSI, menu interativo e gerador SVG
├── src/
│   ├── main.c           # Ponto de entrada e gerenciamento dos modos (Passo-a-Passo / Completa)
│   ├── parser.c         # Leitor robusto (case-insensitive, ignora aperiódicas, trata ';')
│   ├── escalonador.c    # Implementação modular das regras de prioridade e desempate
│   ├── engine.c         # Loop de simulação, alocação em CPUs, desligamento ocioso e Undo/Redo
│   └── frontend.c       # Renderizador de Gantt, tabela TCB, menu e exportação SVG
├── examples/
│   └── config_exemplo.txt # Arquivo de entrada de exemplo
├── Makefile             # Script de automação de compilação sem dependências externas
└── README.md            # Documentação completa do projeto
```

---

## Funcionamento dos Módulos

### 1. Estrutura de Dados da Tarefa (TCB) — `include/types.h`
Toda informação de uma tarefa é mantida em uma estrutura única chamada **Task Control Block (TCB)**:
* `id`, `cor_hex`, `ingresso`, `duracao`, `periodo`, `prazo`.
* `estado`: `ESTADO_PRONTA`, `ESTADO_EXECUTANDO`, `ESTADO_SUSPENSA` ou `ESTADO_FINALIZADA`.
* `execucoes_concluidas`: Contador incremental. Periódicas encerram a simulação após **10 execuções completas**.
* `proximo_deadline_absoluto`: Calculado a cada nova ativação periódica.

### 2. Leitor de Configuração (Parser) — `src/parser.c`
* **Leitura da 1ª Linha**: `algoritmo_escalonamento;quantum;qtde_cpus`.
* **Tratamento Insensível a Maiúsculas/Minúsculas**: Converte strings para caixa alta ("RM", "rm", "Rm" são equivalentes).
* **Filtro de Tarefas Aperiódicas**: Se `periodo == 0`, o parser emite um aviso claro ao usuário e ignora a tarefa para o Projeto A (que suporta apenas tarefas periódicas).
* **Flexibilidade**: Aceita linhas terminadas ou não por `;`, ignora linhas em branco e aceita arquivos de qualquer diretório.

### 3. Escalonador e 5 Critérios de Desempate — `src/escalonador.c`
* **Rate Monotonic (RM)**: Prioridade fixa baseada no menor período.
* **Earliest Deadline First (EDF)**: Prioridade dinâmica baseada no menor deadline absoluto corrente.
* **Sequência Rigorosa dos 5 Critérios de Desempate**:
  1. **Manutenção na CPU**: Se a tarefa já estava executando na CPU antes do escalonamento, ela tem preferência para evitar troca desnecessária de contexto.
  2. **Menor Prazo (Deadline Relativo)**.
  3. **Menor Instante de Ingresso** (chegou primeiro).
  4. **Menor Duração** de computação.
  5. **Sorteio (`rand()`)**: Se persistir o empate nos 4 critérios anteriores, sorteia e registra o evento `S` no gráfico de Gantt.

### 4. Motor de Simulação por Ticks — `src/engine.c`
* **Relógio Global Discreto**: O tempo avança em `ticks` não atrelados ao tempo real.
* **Fila Global de Prontos**: Fila única compartilhada entre todas as CPUs.
* **Minimização de Ociosidade**: Aloca tarefas disponíveis nas CPUs livres. Se faltar tarefa, a CPU é marcada como **desligada/ociosa** e o tempo ocioso acumulado é contabilizado.
* **Undo/Redo (Mecanismo Debugger)**: A cada tick, grava um snapshot em memória (`HistoricoTick`). Permite navegar no tempo (`p` para retroceder, `n` para avançar) e editar tarefas ao vivo (`e`).

### 5. Frontend & Exportação SVG — `src/frontend.c`
* **Eixo Y Decrescente por ID**: Tarefas ordenadas da maior para a menor no topo, fazendo com que a tarefa de **menor ID fique posicionada na linha inferior, mais próxima do eixo X**.
* **Mapeamento de Cores e Estados**:
  * **Em Execução**: Preenchido com a cor RGB Hex da tarefa (`\033[48;2;R;G;Bm`).
  * **Fila de Prontos**: Sem preenchimento de cor.
  * **Suspensa**: Cor preta com padrão hachurado `▒▒` no terminal.
* **Eventos Notáveis**: Renderiza `v` (ingresso), `x` (término de instância), `!` (deadline miss) e `S` (sorteio).
* **Exportação SVG**: Ao final, constrói um XML/SVG vetorial integral contendo todas as barras, cores e legendas sem truncamento de tela.

---

## Como Compilar e Executar

### Pré-requisitos
* Compilador GCC.
* Utilitário `make`.

### Comandos de Compilação
```bash
# Compilar o projeto
make

# Executar os testes automatizados
make test

# Limpar binários gerados
make clean
```

### Execução do Simulador

#### Modo 1: Execução Passo-a-Passo (Debugger Interativo)
```bash
./simulador examples/config_exemplo.txt -p
```
* **Teclas do Menu**:
  * `n`: Avança 1 tick no relógio.
  * `p`: Retrocede 1 tick (Undo temporal).
  * `e`: Edita o estado de qualquer tarefa em tempo de execução.
  * `q`: Encerra o menu interativo e gera o arquivo SVG final.

#### Modo 2: Execução Completa
```bash
./simulador examples/config_exemplo.txt -c
```
Executa silenciosamente até a conclusão das tarefas e gera o arquivo `gantt_final.svg`.
