/* 
 * ============================================================================
 * Arquivo: frontend.h
 * Descrição: Interface gráfica do terminal, modo interativo e renderização do Gantt.
 * ============================================================================
 */

#ifndef FRONTEND_H
#define FRONTEND_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"

/* Forward declaration da estrutura MotorSimulacao */
typedef struct MotorSimulacao MotorSimulacao;

/* Protótipos das Funções do Frontend */
void frontend_limpar_tela(void);
void frontend_converter_hex_rgb(const char *hex, int *r, int *g, int *b);
void frontend_desenhar_legenda(void);
void frontend_desenhar_gantt_terminal(const HistoricoTick *historico_completo, int tick_atual, int total_tarefas, int qtde_cpus);
void frontend_exibir_status_cpus_e_tarefas(const HistoricoTick *snapshot, int qtde_cpus);
int frontend_menu_passo_a_passo(MotorSimulacao *motor);
void frontend_exportar_gantt_svg(const char *nome_arquivo, const HistoricoTick *historico_completo, int total_ticks, int total_tarefas, int qtde_cpus);

#endif /* FRONTEND_H */
