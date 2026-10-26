/* 
 * ============================================================================
 * Arquivo: main.c
 * Descrição: Ponto de entrada do Simulador de Sistema Operacional (Projeto A).
 * Suporta o Modo (a) Passo-a-Passo (Debugger) e Modo (b) Execução Completa.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "parser.h"
#include "engine.h"
#include "frontend.h"

static void exibir_ajuda_uso(const char *nome_prog) {
    printf("USO: %s <caminho_arquivo_config.txt> [opção_modo]\n", nome_prog);
    printf("Opções de modo:\n");
    printf("  -p, --passo-a-passo : Modo Debugger interativo com navegação Undo/Redo e edição ao vivo.\n");
    printf("  -c, --completa      : Execução automática rápida com geração do SVG final.\n");
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL)); /* Inicializa semente para sorteio no 5º critério */

    if (argc < 2) {
        printf("======================================================================\n");
        printf(" SIMULADOR VISUAL DE SISTEMA OPERACIONAL MULTITAREFA (PROJETO A)\n");
        printf("======================================================================\n");
        exibir_ajuda_uso(argv[0]);
        return 1;
    }

    const char *caminho_config = argv[1];
    int modo = 0; /* 1 = Passo-a-Passo, 2 = Completa */

    if (argc >= 3) {
        if (strcmp(argv[2], "-p") == 0 || strcmp(argv[2], "--passo-a-passo") == 0) {
            modo = 1;
        } else if (strcmp(argv[2], "-c") == 0 || strcmp(argv[2], "--completa") == 0) {
            modo = 2;
        }
    }

    /* 1. Carregamento do Arquivo de Configuração */
    ConfiguracaoSistema config;
    TCB *tarefas_carregadas = NULL;

    if (!parser_carregar_arquivo(caminho_config, &config, &tarefas_carregadas)) {
        fprintf(stderr, "[FALHA CRÍTICA] Erro ao processar o arquivo de configuração.\n");
        return 1;
    }

    /* 2. Escolha do Modo caso não informado nos argumentos */
    if (modo == 0) {
        printf("Escolha o Modo de Execução da Simulação (Requisito 1.5):\n");
        printf("  [1] Execução Passo-a-Passo (Debugger Interativo com Undo/Redo)\n");
        printf("  [2] Execução Completa (Direto para o resultado final e imagem SVG)\n");
        printf("Opção > ");
        int op = 1;
        if (scanf("%d", &op) == 1 && (op == 1 || op == 2)) {
            modo = op;
        } else {
            modo = 1;
        }
        while (getchar() != '\n');
    }

    /* 3. Inicialização do Motor de Simulação */
    MotorSimulacao motor;
    if (!engine_inicializar(&motor, config, tarefas_carregadas)) {
        fprintf(stderr, "[FALHA CRÍTICA] Erro ao inicializar o motor de simulação.\n");
        parser_liberar_tarefas(tarefas_carregadas);
        return 1;
    }

    /* 4. Execução do Modo Escolhido */
    if (modo == 1) {
        /* Executa o primeiro tick (tick 0) para registrar a situação inicial */
        engine_executar_tick(&motor);
        frontend_menu_passo_a_passo(&motor);
    } else {
        printf("[EXECUÇÃO COMPLETA] Rodando simulação até o encerramento das tarefas...\n");
        int total_ticks = engine_executar_simulacao_completa(&motor, 1000);
        printf("[EXECUÇÃO COMPLETA] Finalizada em %d ticks de relógio.\n", total_ticks);
    }

    /* 5. Exportação da Imagem Integral do Gantt em SVG (Requisito 2.4) */
    int ticks_validos = (motor.tick_atual > 0) ? motor.tick_atual - 1 : 0;
    frontend_exportar_gantt_svg("gantt_final.svg", motor.historico, ticks_validos, motor.total_tarefas, motor.config.qtde_cpus);

    /* 6. Limpeza de Memória */
    engine_destruir(&motor);
    parser_liberar_tarefas(tarefas_carregadas);

    printf("\n[FIM] Simulação concluída com sucesso.\n");
    return 0;
}
