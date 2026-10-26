/* 
 * ============================================================================
 * Arquivo: engine.c
 * Descrição: Implementação do motor de simulação discreta por ticks.
 * Gerencia a Fila de Prontos Global, alocação de CPUs, desligamento de CPUs ociosas,
 * encerramento após 10 execuções e histórico de snapshots para Undo/Redo.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#include "engine.h"

int engine_inicializar(MotorSimulacao *motor, ConfiguracaoSistema config, TCB *tarefas_carregadas) {
    if (!motor || !tarefas_carregadas) return 0;

    motor->config = config;
    motor->total_tarefas = config.total_tarefas;
    motor->tick_atual = 0;

    /* Aloca o vetor principal de TCBs */
    motor->tarefas = (TCB *)malloc(motor->total_tarefas * sizeof(TCB));
    if (!motor->tarefas) return 0;
    memcpy(motor->tarefas, tarefas_carregadas, motor->total_tarefas * sizeof(TCB));

    /* Aloca estado das CPUs */
    motor->cpu_executando = (int *)malloc(config.qtde_cpus * sizeof(int));
    motor->cpus_desligadas = (int *)malloc(config.qtde_cpus * sizeof(int));
    motor->tempo_desligado_acumulado = (int *)malloc(config.qtde_cpus * sizeof(int));

    for (int c = 0; c < config.qtde_cpus; c++) {
        motor->cpu_executando[c] = -1;
        motor->cpus_desligadas[c] = 1;
        motor->tempo_desligado_acumulado[c] = 0;
    }

    /* Aloca estrutura do histórico de snapshots */
    motor->max_ticks_alocados = 1000;
    motor->historico = (HistoricoTick *)malloc(motor->max_ticks_alocados * sizeof(HistoricoTick));
    if (!motor->historico) return 0;

    return 1;
}

/* Cria uma cópia em memória (snapshot) do estado atual do sistema no histórico */
static void engine_salvar_snapshot(MotorSimulacao *motor, RegistroEvento *eventos_tick, int qtd_eventos) {
    int t = motor->tick_atual;

    /* Expande o histórico dinamicamente se necessário */
    if (t >= motor->max_ticks_alocados) {
        motor->max_ticks_alocados *= 2;
        motor->historico = (HistoricoTick *)realloc(motor->historico, motor->max_ticks_alocados * sizeof(HistoricoTick));
    }

    HistoricoTick *snap = &motor->historico[t];
    snap->tick = t;
    snap->qtde_tarefas = motor->total_tarefas;

    /* Cópias profundas das estruturas */
    snap->tarefas = (TCB *)malloc(motor->total_tarefas * sizeof(TCB));
    memcpy(snap->tarefas, motor->tarefas, motor->total_tarefas * sizeof(TCB));

    snap->cpu_executando = (int *)malloc(motor->config.qtde_cpus * sizeof(int));
    memcpy(snap->cpu_executando, motor->cpu_executando, motor->config.qtde_cpus * sizeof(int));

    snap->cpus_desligadas = (int *)malloc(motor->config.qtde_cpus * sizeof(int));
    memcpy(snap->cpus_desligadas, motor->cpus_desligadas, motor->config.qtde_cpus * sizeof(int));

    snap->tempo_desligado_acumulado = (int *)malloc(motor->config.qtde_cpus * sizeof(int));
    memcpy(snap->tempo_desligado_acumulado, motor->tempo_desligado_acumulado, motor->config.qtde_cpus * sizeof(int));

    snap->qtde_eventos = qtd_eventos;
    if (qtd_eventos > 0) {
        snap->eventos = (RegistroEvento *)malloc(qtd_eventos * sizeof(RegistroEvento));
        memcpy(snap->eventos, eventos_tick, qtd_eventos * sizeof(RegistroEvento));
    } else {
        snap->eventos = NULL;
    }
}

int engine_executar_tick(MotorSimulacao *motor) {
    int t = motor->tick_atual;
    int qtde_cpus = motor->config.qtde_cpus;

    RegistroEvento eventos_tick[64];
    int qtd_eventos = 0;

    /* 1. Ativação de Tarefas Periódicas no Tick Atual */
    for (int i = 0; i < motor->total_tarefas; i++) {
        TCB *tarefa = &motor->tarefas[i];
        tarefa->perdeu_prazo_atual = 0;
        tarefa->foi_sorteada_atual = 0;

        /* Verifica se completou as 10 execuções do Projeto A (Requisito 4.4) */
        if (tarefa->execucoes_concluidas >= 10) {
            tarefa->estado = ESTADO_FINALIZADA;
            tarefa->cpu_atual = -1;
            continue;
        }

        /* Se for o momento de uma nova ativação periódica */
        if (t >= tarefa->ingresso && (t - tarefa->ingresso) % tarefa->periodo == 0) {
            /* Se a tarefa já estava pronta para iniciar uma nova instância */
            if (tarefa->tempo_restante_instancia <= 0) {
                tarefa->estado = ESTADO_PRONTA;
                tarefa->tempo_restante_instancia = tarefa->duracao;
                tarefa->proximo_deadline_absoluto = t + tarefa->prazo;

                /* Registra Evento de Chegada/Ingresso */
                if (qtd_eventos < 64) {
                    eventos_tick[qtd_eventos].id_tarefa = tarefa->id;
                    eventos_tick[qtd_eventos].evento = EVENTO_CHEGADA;
                    qtd_eventos++;
                }
            }
        }

        /* Verificação de Perda de Prazo / Deadline Miss (Requisito 3.3) */
        if (tarefa->estado != ESTADO_FINALIZADA && tarefa->tempo_restante_instancia > 0) {
            if (t > tarefa->proximo_deadline_absoluto) {
                tarefa->perdeu_prazo_atual = 1;
                if (qtd_eventos < 64) {
                    eventos_tick[qtd_eventos].id_tarefa = tarefa->id;
                    eventos_tick[qtd_eventos].evento = EVENTO_PERDA_DEADLINE;
                    qtd_eventos++;
                }
            }
        }
    }

    /* 2. Montagem da Fila Global de Prontos */
    TCB *fila_prontos[64];
    int qtd_prontas = 0;

    for (int i = 0; i < motor->total_tarefas; i++) {
        TCB *tarefa = &motor->tarefas[i];
        if (tarefa->estado == ESTADO_PRONTA || tarefa->estado == ESTADO_EXECUTANDO) {
            if (qtd_prontas < 64) {
                fila_prontos[qtd_prontas++] = tarefa;
            }
        }
    }

    /* Reseta alocação das CPUs para recálculo do tick */
    for (int c = 0; c < qtde_cpus; c++) {
        motor->cpu_executando[c] = -1;
        motor->cpus_desligadas[c] = 1;
    }

    /* 3. Escalonamento e Alocação das CPUs (Requisito 1.2) */
    for (int c = 0; c < qtde_cpus; c++) {
        if (qtd_prontas == 0) {
            /* Sem tarefas na fila de prontos -> Desliga a CPU e acumula tempo ocioso */
            motor->tempo_desligado_acumulado[c]++;
            continue;
        }

        /* Seleciona a melhor tarefa segundo o algoritmo (RM ou EDF + 5 critérios) */
        TCB *escolhida = escalonador_selecionar_proxima_tarefa(fila_prontos, qtd_prontas,
                                                              motor->config.algoritmo_escalonamento,
                                                              c, t);

        if (escolhida) {
            escolhida->estado = ESTADO_EXECUTANDO;
            escolhida->cpu_atual = c;
            motor->cpu_executando[c] = escolhida->id;
            motor->cpus_desligadas[c] = 0;

            /* Se a tarefa foi selecionada por sorteio no 5º critério */
            if (escolhida->foi_sorteada_atual && qtd_eventos < 64) {
                eventos_tick[qtd_eventos].id_tarefa = escolhida->id;
                eventos_tick[qtd_eventos].evento = EVENTO_SORTEIO_DESEMPATE;
                qtd_eventos++;
            }

            /* Remove a tarefa alocada da fila temporária de prontos */
            for (int k = 0; k < qtd_prontas; k++) {
                if (fila_prontos[k] == escolhida) {
                    for (int m = k; m < qtd_prontas - 1; m++) fila_prontos[m] = fila_prontos[m + 1];
                    qtd_prontas--;
                    break;
                }
            }
        } else {
            motor->tempo_desligado_acumulado[c]++;
        }
    }

    /* 4. Processamento do Avanço do Tempo (1 Tick de Execução) */
    for (int i = 0; i < motor->total_tarefas; i++) {
        TCB *tarefa = &motor->tarefas[i];
        if (tarefa->estado == ESTADO_EXECUTANDO) {
            tarefa->tempo_restante_instancia--;

            /* Se a instância atual terminou de executar */
            if (tarefa->tempo_restante_instancia <= 0) {
                tarefa->execucoes_concluidas++;
                tarefa->estado = (tarefa->execucoes_concluidas >= 10) ? ESTADO_FINALIZADA : ESTADO_PRONTA;
                tarefa->cpu_atual = -1;

                if (qtd_eventos < 64) {
                    eventos_tick[qtd_eventos].id_tarefa = tarefa->id;
                    eventos_tick[qtd_eventos].evento = EVENTO_TERMINO_INSTANCIA;
                    qtd_eventos++;
                }
            }
        }
    }

    /* 5. Salva o Snapshot no Histórico para o Debugger */
    engine_salvar_snapshot(motor, eventos_tick, qtd_eventos);

    motor->tick_atual++;
    return 1;
}

int engine_executar_simulacao_completa(MotorSimulacao *motor, int limite_max_ticks) {
    if (!motor) return 0;

    while (motor->tick_atual < limite_max_ticks) {
        /* Verifica se todas as tarefas concluíram suas 10 execuções */
        int todas_concluidas = 1;
        for (int i = 0; i < motor->total_tarefas; i++) {
            if (motor->tarefas[i].execucoes_concluidas < 10) {
                todas_concluidas = 0;
                break;
            }
        }

        if (todas_concluidas) break;

        engine_executar_tick(motor);
    }

    return motor->tick_atual;
}

void engine_recalcular_a_partir_do_tick(MotorSimulacao *motor, int tick_edicao) {
    if (!motor || tick_edicao < 0 || tick_edicao >= motor->tick_atual) return;

    /* Restaura o estado das tarefas a partir do snapshot editado */
    HistoricoTick *snap = &motor->historico[tick_edicao];
    memcpy(motor->tarefas, snap->tarefas, motor->total_tarefas * sizeof(TCB));
    memcpy(motor->cpu_executando, snap->cpu_executando, motor->config.qtde_cpus * sizeof(int));
    memcpy(motor->cpus_desligadas, snap->cpus_desligadas, motor->config.qtde_cpus * sizeof(int));
    memcpy(motor->tempo_desligado_acumulado, snap->tempo_desligado_acumulado, motor->config.qtde_cpus * sizeof(int));

    int antigo_total_ticks = motor->tick_atual;
    motor->tick_atual = tick_edicao;

    /* Recalcula os ticks subsequentes a partir do novo estado editado */
    while (motor->tick_atual < antigo_total_ticks) {
        engine_executar_tick(motor);
    }
}

void engine_destruir(MotorSimulacao *motor) {
    if (!motor) return;

    if (motor->tarefas) free(motor->tarefas);
    if (motor->cpu_executando) free(motor->cpu_executando);
    if (motor->cpus_desligadas) free(motor->cpus_desligadas);
    if (motor->tempo_desligado_acumulado) free(motor->tempo_desligado_acumulado);

    if (motor->historico) {
        for (int i = 0; i < motor->tick_atual; i++) {
            if (motor->historico[i].tarefas) free(motor->historico[i].tarefas);
            if (motor->historico[i].cpu_executando) free(motor->historico[i].cpu_executando);
            if (motor->historico[i].cpus_desligadas) free(motor->historico[i].cpus_desligadas);
            if (motor->historico[i].tempo_desligado_acumulado) free(motor->historico[i].tempo_desligado_acumulado);
            if (motor->historico[i].eventos) free(motor->historico[i].eventos);
        }
        free(motor->historico);
    }
}
