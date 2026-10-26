/* 
 * ============================================================================
 * Arquivo: escalonador.c
 * Descrição: Lógica modular dos algoritmos Rate Monotonic (RM) e Earliest
 * Deadline First (EDF) e implementação estrita da sequência de 5 critérios de desempate.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#include "escalonador.h"

/* 
 * Aplica os 5 Critérios de Desempate (Requisito 4.3):
 * Retorna:
 *  -1 se a tarefa A é preferível
 *   1 se a tarefa B é preferível
 *   0 se empatou absolutamente nos 4 primeiros critérios (sinal de sorteio)
 */
static int aplicar_criterios_desempate(const TCB *a, const TCB *b, int cpu_id) {
    /* Critério 1: Manutenção na CPU (Evita troca desnecessária de contexto) */
    int a_na_cpu = (a->cpu_atual == cpu_id) ? 1 : 0;
    int b_na_cpu = (b->cpu_atual == cpu_id) ? 1 : 0;
    if (a_na_cpu && !b_na_cpu) return -1;
    if (!a_na_cpu && b_na_cpu) return 1;

    /* Critério 2: Menor Prazo (Deadline Relativo) */
    if (a->prazo < b->prazo) return -1;
    if (a->prazo > b->prazo) return 1;

    /* Critério 3: Menor Instante de Ingresso (Quem chegou primeiro) */
    if (a->ingresso < b->ingresso) return -1;
    if (a->ingresso > b->ingresso) return 1;

    /* Critério 4: Menor Duração de Computação */
    if (a->duracao < b->duracao) return -1;
    if (a->duracao > b->duracao) return 1;

    /* Critério 5: Sorteio (Caso permaneça empatado nos 4 critérios anteriores) */
    return 0;
}

/* 
 * Comparação pelo Rate Monotonic (RM Preemptivo):
 * Prioridade Fixa: Menor período = Maior prioridade.
 */
int escalonador_comparar_rm(const TCB *a, const TCB *b, int cpu_id, int tick_atual) {
    (void)tick_atual; /* RM não altera prioridade com o relógio */

    if (a->periodo < b->periodo) return -1;
    if (a->periodo > b->periodo) return 1;

    /* Períodos iguais = Empate de prioridade base. Aplica os 5 critérios de desempate */
    return aplicar_criterios_desempate(a, b, cpu_id);
}

/* 
 * Comparação pelo Earliest Deadline First (EDF Preemptivo):
 * Prioridade Dinâmica: Menor deadline absoluto corrente = Maior prioridade.
 */
int escalonador_comparar_edf(const TCB *a, const TCB *b, int cpu_id, int tick_atual) {
    (void)tick_atual;

    if (a->proximo_deadline_absoluto < b->proximo_deadline_absoluto) return -1;
    if (a->proximo_deadline_absoluto > b->proximo_deadline_absoluto) return 1;

    /* Deadlines absolutos iguais = Empate de prioridade base. Aplica os 5 critérios */
    return aplicar_criterios_desempate(a, b, cpu_id);
}

/* 
 * Função Principal de Seleção:
 * Percorre a lista de tarefas prontas e retorna a tarefa de maior prioridade.
 */
TCB *escalonador_selecionar_proxima_tarefa(TCB *tarefas_prontas[], int total_prontas,
                                           const char *algoritmo, int cpu_id, int tick_atual) {
    if (!tarefas_prontas || total_prontas <= 0) return NULL;

    FuncaoComparacaoTarefas funcao_comp = escalonador_comparar_rm;
    if (algoritmo && strcmp(algoritmo, "EDF") == 0) {
        funcao_comp = escalonador_comparar_edf;
    }

    int melhor_idx = 0;

    for (int i = 1; i < total_prontas; i++) {
        int res = funcao_comp(tarefas_prontas[i], tarefas_prontas[melhor_idx], cpu_id, tick_atual);

        if (res < 0) {
            melhor_idx = i;
        } else if (res == 0) {
            /* 5º Critério de Desempate: Sorteio (rand) */
            if (rand() % 2 == 0) {
                melhor_idx = i;
            }
            tarefas_prontas[melhor_idx]->foi_sorteada_atual = 1;
        }
    }

    return tarefas_prontas[melhor_idx];
}
