/* 
 * ============================================================================
 * Arquivo: escalonador.h
 * Descrição: Protótipos das funções do módulo de escalonamento (RM e EDF)
 * com aplicação estrita dos 5 critérios de desempate.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include "types.h"

/* Assinatura para funções genéricas de ordenação/comparações de escalonadores */
typedef int (*FuncaoComparacaoTarefas)(const TCB *a, const TCB *b, int cpu_id_atual, int tick_atual);

/* Seleciona a melhor tarefa da Fila de Prontos para ser alocada em uma CPU */
TCB *escalonador_selecionar_proxima_tarefa(TCB *tarefas_prontas[], int total_prontas,
                                           const char *algoritmo, int cpu_id, int tick_atual);

/* Função de comparação segundo as regras de prioridade do RM + 5 Critérios de Desempate */
int escalonador_comparar_rm(const TCB *a, const TCB *b, int cpu_id, int tick_atual);

/* Função de comparação segundo as regras de prioridade do EDF + 5 Critérios de Desempate */
int escalonador_comparar_edf(const TCB *a, const TCB *b, int cpu_id, int tick_atual);

#endif /* ESCALONADOR_H */
