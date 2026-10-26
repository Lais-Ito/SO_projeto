/* 
 * ============================================================================
 * Arquivo: frontend.c
 * Descrição: Renderização ANSI no terminal, navegação interativa e geração SVG.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#include "frontend.h"
#include "engine.h"

/* Limpa o terminal e move o cursor para o topo (Sequência ANSI padrão) */
void frontend_limpar_tela(void) {
    printf("\033[2J\033[H");
}

/* Converte a string hexadecimal de 6 caracteres (ex: "F0E0D0") em inteiros R, G, B */
void frontend_converter_hex_rgb(const char *hex, int *r, int *g, int *b) {
    *r = 0; *g = 0; *b = 0;
    if (!hex || strlen(hex) < 6) return;

    char str_r[3] = {hex[0], hex[1], '\0'};
    char str_g[3] = {hex[2], hex[3], '\0'};
    char str_b[3] = {hex[4], hex[5], '\0'};

    *r = (int)strtol(str_r, NULL, 16);
    *g = (int)strtol(str_g, NULL, 16);
    *b = (int)strtol(str_b, NULL, 16);
}

/* Exibe a legenda explicativa dos elementos gráficos (Requisito 2) */
void frontend_desenhar_legenda(void) {
    printf("\n=== LEGENDA DOS ELEMENTOS GRÁFICOS (REQUISITO 2) ===\n");
    printf(" [ Cor da Tarefa ] Executando na CPU (Fundo RGB Hexadecimal)\n");
    printf(" [    Vazio     ] Na Fila de Prontos (Aguardando Alocação de CPU)\n");
    printf(" [ ▒▒ (Hachura)  ] Tarefa Suspensa/Bloqueada (Preenchimento Preto)\n");
    printf(" [  v  ] Ingresso / Ativação da Tarefa\n");
    printf(" [  x  ] Término de Instância da Tarefa\n");
    printf(" [  !  ] Perda de Prazo (Deadline Miss - Continua Executando)\n");
    printf(" [  S  ] Escolhida por Sorteio no 5º Critério de Desempate\n");
    printf("====================================================\n\n");
}

/* Exibe o status atual das CPUs e a lista de tarefas no tick corrente */
void frontend_exibir_status_cpus_e_tarefas(const HistoricoTick *snapshot, int qtde_cpus) {
    if (!snapshot || !snapshot->cpus_desligadas || !snapshot->tarefas) return;

    printf("\n--- STATUS PROCESSADORES / CPUS (TICK %d) ---\n", snapshot->tick);
    for (int c = 0; c < qtde_cpus; c++) {
        if (snapshot->cpus_desligadas[c]) {
            printf("  CPU %d: [ DESLIGADA / OCIOSA ] (Tempo total desligada: %d ticks)\n", 
                   c + 1, snapshot->tempo_desligado_acumulado[c]);
        } else {
            printf("  CPU %d: Executando Tarefa ID %d (Tempo total desligada: %d ticks)\n", 
                   c + 1, snapshot->cpu_executando[c], snapshot->tempo_desligado_acumulado[c]);
        }
    }

    printf("\n--- ESTADO DAS TAREFAS NO TCB (TICK %d) ---\n", snapshot->tick);
    printf(" ID | Estado     | CPU | Dur. Rest. | Prox. Deadline | Concluídas (max 10)\n");
    printf("----+------------+-----+------------+----------------+--------------------\n");
    for (int i = 0; i < snapshot->qtde_tarefas; i++) {
        TCB *t = &snapshot->tarefas[i];
        const char *nome_estado = "DESCONHECIDO";
        switch (t->estado) {
            case ESTADO_PRONTA:     nome_estado = "PRONTA"; break;
            case ESTADO_EXECUTANDO: nome_estado = "EXECUTANDO"; break;
            case ESTADO_SUSPENSA:   nome_estado = "SUSPENSA"; break;
            case ESTADO_FINALIZADA: nome_estado = "FINALIZADA"; break;
        }
        
        char cpu_str[32];
        if (t->cpu_atual >= 0) sprintf(cpu_str, "CPU %d", t->cpu_atual + 1);
        else strcpy(cpu_str, " - ");

        printf(" %2d | %-10s | %-3s | %10d | %14d | %18d/10\n",
               t->id, nome_estado, cpu_str, t->tempo_restante_instancia,
               t->proximo_deadline_absoluto, t->execucoes_concluidas);
    }
    printf("----------------------------------------------------------------------\n");
}

/* 
 * Desenha o Gráfico de Gantt no Terminal para o intervalo de ticks [0 .. tick_atual]
 * Requisito 2.5: Eixo Y com ordem DECRESCENTE em relação ao ID (menor ID mais próximo do eixo X).
 */
void frontend_desenhar_gantt_terminal(const HistoricoTick *historico_completo, int tick_atual, int total_tarefas, int qtde_cpus) {
    (void)qtde_cpus;
    if (!historico_completo || tick_atual < 0) return;

    printf("\n======================================================================\n");
    printf("                 GRÁFICO DE GANTT (TICK 0 ATÉ %d)\n", tick_atual);
    printf("======================================================================\n\n");

    /* Eixo Y Decrescente em relação ao ID */
    for (int id_alvo = total_tarefas; id_alvo >= 1; id_alvo--) {
        printf("Tarefa ID %2d |", id_alvo);

        for (int t = 0; t <= tick_atual; t++) {
            const HistoricoTick *snap = &historico_completo[t];
            if (!snap || !snap->tarefas) {
                printf("  ");
                continue;
            }

            TCB *tarefa = NULL;
            for (int i = 0; i < snap->qtde_tarefas; i++) {
                if (snap->tarefas[i].id == id_alvo) {
                    tarefa = &snap->tarefas[i];
                    break;
                }
            }

            if (!tarefa) {
                printf("  ");
                continue;
            }

            /* Identifica evento notável do tick para este ID */
            char simbolo_evento = ' ';
            for (int e = 0; e < snap->qtde_eventos; e++) {
                if (snap->eventos[e].id_tarefa == id_alvo) {
                    switch (snap->eventos[e].evento) {
                        case EVENTO_CHEGADA:           simbolo_evento = 'v'; break;
                        case EVENTO_TERMINO_INSTANCIA: simbolo_evento = 'x'; break;
                        case EVENTO_PERDA_DEADLINE:    simbolo_evento = '!'; break;
                        case EVENTO_SORTEIO_DESEMPATE: simbolo_evento = 'S'; break;
                        default: break;
                    }
                }
            }

            /* Preenchimento visual segundo os Requisitos 2.1 e 2.2 */
            if (tarefa->estado == ESTADO_EXECUTANDO) {
                int r, g, b;
                frontend_converter_hex_rgb(tarefa->cor_hex, &r, &g, &b);
                if (simbolo_evento != ' ') {
                    printf("\033[48;2;%d;%d;%dm\033[38;2;255;255;255m%c%d\033[0m", 
                           r, g, b, simbolo_evento, tarefa->cpu_atual + 1);
                } else {
                    printf("\033[48;2;%d;%d;%dm%d \033[0m", r, g, b, tarefa->cpu_atual + 1);
                }
            } else if (tarefa->estado == ESTADO_SUSPENSA) {
                if (simbolo_evento != ' ') printf("\033[40m\033[37m%c▒\033[0m", simbolo_evento);
                else printf("\033[40m\033[37m▒▒\033[0m");
            } else if (tarefa->estado == ESTADO_PRONTA) {
                if (simbolo_evento != ' ') printf("%c ", simbolo_evento);
                else printf("  ");
            } else {
                printf("..");
            }
        }
        printf("|\n");
    }

    printf("--------------+");
    for (int t = 0; t <= tick_atual; t++) printf("--");
    printf("\n      Ticks   |");
    for (int t = 0; t <= tick_atual; t++) printf("%-2d", t % 100);
    printf("\n");

    frontend_desenhar_legenda();
}

/* 
 * Menu do Modo Passo-a-Passo (Debugger da Simulação)
 * Permite avançar (Next), retroceder (Undo/Redo), editar tarefa e sair.
 */
int frontend_menu_passo_a_passo(MotorSimulacao *motor) {
    if (!motor || motor->tick_atual == 0) return 0;

    int tick_visao = motor->tick_atual - 1; /* Exibe o último tick simulado */
    char opcao[100];

    while (1) {
        frontend_limpar_tela();
        printf("=== SIMULADOR DE ESCALONAMENTO DE SO - MODO PASSO-A-PASSO (DEBUGGER) ===\n");
        printf("Algoritmo: %s | CPUs: %d | Quantum: %d | Visualizando Tick: %d / %d\n",
               motor->config.algoritmo_escalonamento, motor->config.qtde_cpus, motor->config.quantum, 
               tick_visao, motor->tick_atual - 1);

        frontend_desenhar_gantt_terminal(motor->historico, tick_visao, motor->total_tarefas, motor->config.qtde_cpus);
        frontend_exibir_status_cpus_e_tarefas(&motor->historico[tick_visao], motor->config.qtde_cpus);

        printf("\n[COMANDOS DO DEBUGGER]:\n");
        printf("  [n] Avançar 1 Tick (Next)\n");
        printf("  [p] Retroceder 1 Tick (Undo/Previous)\n");
        printf("  [e] Modificar Estado/Características de uma Tarefa (Edit)\n");
        printf("  [q] Sair do Modo Passo-a-Passo / Finalizar Simulação\n");
        printf("Opção > ");

        if (!fgets(opcao, sizeof(opcao), stdin)) return 0;

        char c = opcao[0];
        if (c == 'n' || c == 'N') {
            if (tick_visao < motor->tick_atual - 1) {
                tick_visao++;
            } else {
                /* Avança o tempo simulando o próximo tick */
                if (engine_executar_tick(motor)) {
                    tick_visao = motor->tick_atual - 1;
                } else {
                    printf("\n[AVISO] Todas as tarefas finalizaram suas 10 execuções! Pressione ENTER...");
                    getchar();
                }
            }
        } else if (c == 'p' || c == 'P') {
            if (tick_visao > 0) {
                tick_visao--;
            } else {
                printf("\n[AVISO] Já está no tick inicial (0)! Pressione ENTER...");
                getchar();
            }
        } else if (c == 'e' || c == 'E') {
            int id_edicao, novo_estado_num;
            printf("\n--- EDIÇÃO EM TEMPO REAL DE TAREFA ---\n");
            printf("Informe o ID da Tarefa que deseja modificar: ");
            if (scanf("%d", &id_edicao) != 1) {
                while (getchar() != '\n');
                continue;
            }
            
            /* Localiza a tarefa no snapshot do tick de visualização */
            HistoricoTick *snap = &motor->historico[tick_visao];
            TCB *alvo = NULL;
            for (int i = 0; i < snap->qtde_tarefas; i++) {
                if (snap->tarefas[i].id == id_edicao) {
                    alvo = &snap->tarefas[i];
                    break;
                }
            }

            if (!alvo) {
                printf("\n[ERRO] Tarefa ID %d não encontrada! Pressione ENTER...", id_edicao);
                while (getchar() != '\n');
                getchar();
                continue;
            }

            printf("Selecione o novo Estado:\n");
            printf("  0 - PRONTA\n  1 - EXECUTANDO\n  2 - SUSPENSA\n  3 - FINALIZADA\nEscolha: ");
            if (scanf("%d", &novo_estado_num) != 1) {
                while (getchar() != '\n');
                continue;
            }
            while (getchar() != '\n'); /* Limpa buffer */

            if (novo_estado_num < 0 || novo_estado_num > 3) {
                printf("\n[ERRO] Estado numérico inválido informado! Pressione ENTER...");
                getchar();
                continue;
            }

            alvo->estado = (EstadoTarefa)novo_estado_num;
            if (alvo->estado == ESTADO_SUSPENSA) alvo->cpu_atual = -1;

            /* Atualiza o motor e recalcula o futuro da simulação a partir desta edição */
            engine_recalcular_a_partir_do_tick(motor, tick_visao);

            printf("\n[SUCESSO] Tarefa ID %d atualizada! Pressione ENTER...", id_edicao);
            getchar();
        } else if (c == 'q' || c == 'Q') {
            break;
        }
    }
    return 1;
}

/* 
 * Exportador de Imagem Integral em SVG (Requisito 2.4)
 * Gera imagem de alta resolução sem cortes de tempo e sem depender de print de tela.
 */
void frontend_exportar_gantt_svg(const char *nome_arquivo, const HistoricoTick *historico_completo, int total_ticks, int total_tarefas, int qtde_cpus) {
    (void)qtde_cpus;
    if (!historico_completo || total_ticks < 0 || !nome_arquivo) return;

    FILE *f = fopen(nome_arquivo, "w");
    if (!f) return;

    int largura_celula = 30;
    int altura_linha = 40;
    int margem_esq = 160;
    int margem_topo = 80;
    int largura_total = margem_esq + (total_ticks + 1) * largura_celula + 60;
    int altura_total = margem_topo + total_tarefas * altura_linha + 160;

    fprintf(f, "<?xml version='1.0' encoding='UTF-8'?>\n");
    fprintf(f, "<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d' style='background-color:#ffffff; font-family:monospace;'>\n", largura_total, altura_total);
    
    /* Definição do padrão pontilhado/hachurado para tarefas suspensas */
    fprintf(f, "  <defs>\n");
    fprintf(f, "    <pattern id='hatch_suspensa' width='8' height='8' patternUnits='userSpaceOnUse'>\n");
    fprintf(f, "      <rect width='8' height='8' fill='#1a1a1a'/>\n");
    fprintf(f, "      <circle cx='2' cy='2' r='1.5' fill='#ffffff'/>\n");
    fprintf(f, "      <circle cx='6' cy='6' r='1.5' fill='#ffffff'/>\n");
    fprintf(f, "    </pattern>\n");
    fprintf(f, "  </defs>\n\n");

    /* Título do Gráfico */
    fprintf(f, "  <text x='%d' y='35' font-size='20' font-weight='bold' fill='#212529'>Simulador de Escalonamento de SO - Gráfico de Gantt Completo</text>\n", margem_esq);
    fprintf(f, "  <text x='%d' y='55' font-size='13' fill='#6c757d'>Resultado integral da simulação (Requisito 2.4 - Exportação Vetorial)</text>\n\n", margem_esq);

    /* Linhas das Tarefas (Eixo Y Decrescente em relação ao ID) */
    for (int idx = 0; idx < total_tarefas; idx++) {
        int id_alvo = total_tarefas - idx;
        int pos_y = margem_topo + idx * altura_linha;

        fprintf(f, "  <text x='20' y='%d' font-size='14' font-weight='bold' fill='#333333'>Tarefa ID %02d</text>\n", pos_y + 25, id_alvo);

        for (int t = 0; t <= total_ticks; t++) {
            const HistoricoTick *snap = &historico_completo[t];
            int pos_x = margem_esq + t * largura_celula;
            TCB *tarefa = NULL;

            if (snap && snap->tarefas) {
                for (int i = 0; i < snap->qtde_tarefas; i++) {
                    if (snap->tarefas[i].id == id_alvo) {
                        tarefa = &snap->tarefas[i];
                        break;
                    }
                }
            }

            if (!tarefa) continue;

            if (tarefa->estado == ESTADO_EXECUTANDO) {
                fprintf(f, "  <rect x='%d' y='%d' width='%d' height='%d' fill='#%s' stroke='#495057' stroke-width='1'/>\n",
                        pos_x, pos_y + 5, largura_celula, altura_linha - 10, tarefa->cor_hex);
                fprintf(f, "  <text x='%d' y='%d' font-size='11' font-weight='bold' fill='#ffffff' text-anchor='middle'>C%d</text>\n",
                        pos_x + largura_celula / 2, pos_y + 24, tarefa->cpu_atual + 1);
            } else if (tarefa->estado == ESTADO_SUSPENSA) {
                fprintf(f, "  <rect x='%d' y='%d' width='%d' height='%d' fill='url(#hatch_suspensa)' stroke='#000000' stroke-width='1'/>\n",
                        pos_x, pos_y + 5, largura_celula, altura_linha - 10);
            } else if (tarefa->estado == ESTADO_PRONTA) {
                fprintf(f, "  <rect x='%d' y='%d' width='%d' height='%d' fill='none' stroke='#dee2e6' stroke-width='1'/>\n",
                        pos_x, pos_y + 5, largura_celula, altura_linha - 10);
            }

            /* Marcadores de Eventos */
            for (int e = 0; e < snap->qtde_eventos; e++) {
                if (snap->eventos[e].id_tarefa == id_alvo) {
                    if (snap->eventos[e].evento == EVENTO_CHEGADA) {
                        fprintf(f, "  <polygon points='%d,%d %d,%d %d,%d' fill='#0d6efd'/>\n",
                                pos_x + 5, pos_y + 2, pos_x + 15, pos_y + 2, pos_x + 10, pos_y + 8);
                    } else if (snap->eventos[e].evento == EVENTO_TERMINO_INSTANCIA) {
                        fprintf(f, "  <circle cx='%d' cy='%d' r='4' fill='#198754'/>\n",
                                pos_x + largura_celula - 6, pos_y + altura_linha - 12);
                    } else if (snap->eventos[e].evento == EVENTO_PERDA_DEADLINE) {
                        fprintf(f, "  <text x='%d' y='%d' font-size='13' font-weight='bold' fill='#dc3545'>!</text>\n",
                                pos_x + 3, pos_y + 16);
                    }
                }
            }
        }
    }

    /* Linha do Eixo X e Ticks */
    int eixo_y_pos = margem_topo + total_tarefas * altura_linha + 5;
    fprintf(f, "  <line x1='%d' y1='%d' x2='%d' y2='%d' stroke='#212529' stroke-width='2'/>\n",
            margem_esq, eixo_y_pos, margem_esq + (total_ticks + 1) * largura_celula, eixo_y_pos);

    for (int t = 0; t <= total_ticks; t++) {
        int pos_x = margem_esq + t * largura_celula;
        fprintf(f, "  <text x='%d' y='%d' font-size='10' fill='#495057' text-anchor='middle'>%d</text>\n",
                pos_x + largura_celula / 2, eixo_y_pos + 15, t);
    }

    fprintf(f, "</svg>\n");
    fclose(f);
    printf("\n[SUCESSO] Gráfico de Gantt vetorial integral gerado em '%s'.\n", nome_arquivo);
}
