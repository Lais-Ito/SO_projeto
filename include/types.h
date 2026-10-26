/* 
 * ============================================================================
 * Arquivo: types.h
 * Descrição: Definição centralizada de tipos, estruturas de dados e TCB.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * Autores: Equipe de Desenvolvimento
 * ============================================================================
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Estado da Tarefa dentro do Task Control Block (TCB) */
typedef enum {
    ESTADO_PRONTA = 0,      /* Aguardando alocação na Fila de Prontos */
    ESTADO_EXECUTANDO,     /* Alocada e rodando em uma CPU */
    ESTADO_SUSPENSA,        /* Bloqueada pelo usuário ou evento */
    ESTADO_FINALIZADA       /* Concluiu todas as 10 execuções do Projeto A */
} EstadoTarefa;

/* Tipos de Eventos Notáveis para Exibição Gráfica no Gantt */
typedef enum {
    EVENTO_NENHUM = 0,
    EVENTO_CHEGADA,          /* Ingresso/Ativação da tarefa no tick */
    EVENTO_TERMINO_INSTANCIA,/* Fim de uma execução/instância da tarefa */
    EVENTO_PERDA_DEADLINE,   /* Estouro de prazo (Deadline Miss) */
    EVENTO_SORTEIO_DESEMPATE /* Tarefa escolhida por sorteio (5º critério) */
} TipoEvento;

/* Estrutura do TCB (Task Control Block) - Requisito 1.3 */
typedef struct {
    int id;                         /* Identificador único da tarefa */
    char cor_hex[7];                /* Cor em formato RGB Hexadecimal (ex: "F0E0D0") */
    int ingresso;                   /* Instante de criação/ingresso no sistema */
    int duracao;                    /* Tempo de computação necessário por instância */
    int periodo;                    /* Período (>0 periódica, 0 aperiódica, <0 erro) */
    int prazo;                      /* Deadline relativo a partir da ativação */

    /* Campos Dinâmicos do Estado Interno */
    EstadoTarefa estado;            /* Estado corrente (Pronta, Executando, Suspensa, etc.) */
    int execucoes_concluidas;       /* Contador de execuções completas (limite = 10 no Projeto A) */
    int tempo_restante_instancia;   /* Ticks restantes para concluir a instância atual */
    int proximo_deadline_absoluto;  /* Instante limite (tick) para término da instância */
    int cpu_atual;                  /* Índice da CPU onde executa (-1 se nenhuma) */
    
    /* Flags do Tick Corrente (para notificação visual no Gantt) */
    int perdeu_prazo_atual;         /* 1 se ocorreu deadline miss neste tick */
    int foi_sorteada_atual;         /* 1 se foi selecionada por sorteio neste tick */
} TCB;

/* Evento Notável Ocorrido em um Registro de Histórico */
typedef struct {
    int id_tarefa;
    TipoEvento evento;
} RegistroEvento;

/* Snapshot Completo do Sistema em um Tick de Relógio (Suporte ao Debugger / Undo-Redo) */
typedef struct {
    int tick;                       /* Relógio global do sistema */
    int qtde_tarefas;               /* Número total de tarefas */
    TCB *tarefas;                   /* Cópia do estado de todas as tarefas */
    int *cpu_executando;            /* ID da tarefa em execução por CPU (-1 se ociosa) */
    int *cpus_desligadas;           /* 1 se a CPU foi desligada por falta de tarefas, 0 caso contrário */
    int *tempo_desligado_acumulado; /* Acumulado de ticks em que a CPU esteve desligada */
    RegistroEvento *eventos;        /* Lista de eventos ocorridos neste tick */
    int qtde_eventos;               /* Quantidade de eventos registrados */
} HistoricoTick;

/* Configuração Geral do Sistema Operacional */
typedef struct {
    char algoritmo_escalonamento[16]; /* "RM" ou "EDF" (case-insensitive) */
    int quantum;                       /* Quantum de tempo do sistema */
    int qtde_cpus;                     /* Número de CPUs configuradas (1 a N) */
    int total_tarefas;                 /* Número de tarefas válidas carregadas */
} ConfiguracaoSistema;

#endif /* TYPES_H */
