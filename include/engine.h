/* 
 * ============================================================================
 * Arquivo: engine.h
 * Descrição: Protótipos das funções do motor de simulação por ticks.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "escalonador.h"

/* Forward declaration com tag para compatibilidade total entre módulos */
typedef struct MotorSimulacao MotorSimulacao;

/* Estrutura Principal do Motor de Simulação */
struct MotorSimulacao {
    ConfiguracaoSistema config;     /* Configuração do SO (Algoritmo, Quantum, CPUs) */
    TCB *tarefas;                   /* Vetor principal de TCBs */
    int total_tarefas;
    
    int tick_atual;                 /* Relógio global da simulação */
    int max_ticks_alocados;         /* Capacidade alocada do vetor de histórico */
    HistoricoTick *historico;       /* Vetor com histórico de snapshots por tick */
    
    int *cpu_executando;            /* ID da tarefa em execução em cada CPU (-1 se nenhuma) */
    int *cpus_desligadas;           /* Indicador se a CPU foi desligada no tick */
    int *tempo_desligado_acumulado; /* Tempo acumulado de desligamento por CPU */
};

/* Inicializa o motor de simulação a partir das configurações e tarefas */
int engine_inicializar(MotorSimulacao *motor, ConfiguracaoSistema config, TCB *tarefas_carregadas);

/* Executa um único tick de relógio da simulação e armazena o snapshot */
int engine_executar_tick(MotorSimulacao *motor);

/* Executa a simulação completa até todas as tarefas concluírem 10 execuções ou limite max */
int engine_executar_simulacao_completa(MotorSimulacao *motor, int limite_max_ticks);

/* Recalcula a simulação a partir do tick informado caso o usuário edite uma tarefa */
void engine_recalcular_a_partir_do_tick(MotorSimulacao *motor, int tick_edicao);

/* Libera a memória utilizada pelo motor e seu histórico */
void engine_destruir(MotorSimulacao *motor);

#endif /* ENGINE_H */
