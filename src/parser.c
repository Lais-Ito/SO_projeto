/* 
 * ============================================================================
 * Arquivo: parser.c
 * Descrição: Implementação do leitor de arquivos de configuração em C puro.
 * Trata insensibilidade a maiúsculas/minúsculas, linhas em branco, ponto-e-vírgula
 * opcional e filtragem de tarefas aperiódicas (Projeto A).
 * ============================================================================
 */

#include "parser.h"

/* Remove espaços em branco do início e do fim de uma string */
static char *trim_whitespace(char *str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/* Converte string para maiúsculas (para tornar a leitura case-insensitive) */
static void str_to_uppercase(char *str) {
    if (!str) return;
    for (int i = 0; str[i]; i++) {
        str[i] = (char)toupper((unsigned char)str[i]);
    }
}

int parser_carregar_arquivo(const char *caminho_arquivo, ConfiguracaoSistema *config, TCB **vetor_tarefas) {
    if (!caminho_arquivo || !config || !vetor_tarefas) {
        fprintf(stderr, "[ERRO PARSER] Ponteiros nulos informados para a leitura.\n");
        return 0;
    }

    FILE *f = fopen(caminho_arquivo, "r");
    if (!f) {
        fprintf(stderr, "[ERRO PARSER] Não foi possível abrir o arquivo '%s'. Verifique o caminho.\n", caminho_arquivo);
        return 0;
    }

    char linha[512];
    int linha_num = 0;
    int leu_cabecalho = 0;

    int capacidade_tarefas = 10;
    int qtd_tarefas_validas = 0;
    TCB *tarefas = (TCB *)malloc(capacidade_tarefas * sizeof(TCB));
    if (!tarefas) {
        fprintf(stderr, "[ERRO PARSER] Falha ao alocar memória para o vetor de tarefas.\n");
        fclose(f);
        return 0;
    }

    /* Valores padrão de fallback (Requisito 3.2) */
    strcpy(config->algoritmo_escalonamento, "RM");
    config->quantum = 1;
    config->qtde_cpus = 1;
    config->total_tarefas = 0;

    while (fgets(linha, sizeof(linha), f)) {
        linha_num++;
        char *linha_limpa = trim_whitespace(linha);

        /* Ignora linhas em branco (Requisito 3.3.6) */
        if (strlen(linha_limpa) == 0) continue;

        /* Se a linha termina com ';', removemos para padronizar (Requisito 3.3.3) */
        size_t len = strlen(linha_limpa);
        if (len > 0 && linha_limpa[len - 1] == ';') {
            linha_limpa[len - 1] = '\0';
            linha_limpa = trim_whitespace(linha_limpa);
        }

        /* 1ª Linha válida: Parâmetros Gerais do Sistema (Requisito 3.3.1) */
        if (!leu_cabecalho) {
            char alg[32];
            int q = 1, cpus = 1;

            if (sscanf(linha_limpa, "%31[^;];%d;%d", alg, &q, &cpus) < 3) {
                fprintf(stderr, "[AVISO PARSER] Linha 1 possui formato incompleto. Usando valores padrão.\n");
            }

            char *alg_clean = trim_whitespace(alg);
            str_to_uppercase(alg_clean); /* Insensível a maiúsculas/minúsculas (Requisito 3.3.2) */

            if (strcmp(alg_clean, "RM") != 0 && strcmp(alg_clean, "EDF") != 0) {
                fprintf(stderr, "[AVISO PARSER] Algoritmo '%s' desconhecido. Defino o padrão para 'RM'.\n", alg_clean);
                strcpy(config->algoritmo_escalonamento, "RM");
            } else {
                strcpy(config->algoritmo_escalonamento, alg_clean);
            }

            config->quantum = (q > 0) ? q : 1;
            config->qtde_cpus = (cpus > 0) ? cpus : 1;
            leu_cabecalho = 1;
            continue;
        }

        /* Linhas seguintes: Especificação de cada Tarefa */
        int id, ingresso, duracao, periodo, prazo;
        char cor[32];
        char lista_eventos[256] = "";

        int lidos = sscanf(linha_limpa, "%d;%31[^;];%d;%d;%d;%d;%255[^\n]",
                           &id, cor, &ingresso, &duracao, &periodo, &prazo, lista_eventos);

        if (lidos < 6) {
            fprintf(stderr, "[AVISO PARSER] Linha %d ignorada por sintaxe incorreta: '%s'\n", linha_num, linha_limpa);
            continue;
        }

        /* Validação do Período (Requisito 3.3 & 4.4) */
        if (periodo < 0) {
            fprintf(stderr, "[ERRO PARSER] Linha %d: Tarefa ID %d possui período negativo (%d). Registro inválido.\n", linha_num, id, periodo);
            continue;
        } else if (periodo == 0) {
            /* Tarefa aperiódica: Projeto A suporta apenas tarefas periódicas. Ignorar com aviso claro (Requisito 4.4) */
            printf("[AVISO SIMULADOR] Tarefa aperiódica ID %d (período = 0) ignorada para o Projeto A.\n", id);
            continue;
        }

        /* Ajuste Dinâmico de Capacidade do Vetor de Tarefas */
        if (qtd_tarefas_validas >= capacidade_tarefas) {
            capacidade_tarefas *= 2;
            TCB *novas_tarefas = (TCB *)realloc(tarefas, capacidade_tarefas * sizeof(TCB));
            if (!novas_tarefas) {
                fprintf(stderr, "[ERRO PARSER] Falha ao realocar memória para tarefas.\n");
                free(tarefas);
                fclose(f);
                return 0;
            }
            tarefas = novas_tarefas;
        }

        /* Inicialização dos Campos do TCB para a Tarefa */
        TCB *t = &tarefas[qtd_tarefas_validas];
        t->id = id;
        
        char *cor_clean = trim_whitespace(cor);
        snprintf(t->cor_hex, sizeof(t->cor_hex), "%-6s", cor_clean);
        /* Substitui eventuais espaços por '0' para garantir 6 dígitos hex */
        for (int i = 0; i < 6; i++) {
            if (t->cor_hex[i] == ' ' || t->cor_hex[i] == '\0') t->cor_hex[i] = '0';
        }
        t->cor_hex[6] = '\0';

        t->ingresso = (ingresso >= 0) ? ingresso : 0;
        t->duracao = (duracao > 0) ? duracao : 1;
        t->periodo = periodo;
        t->prazo = (prazo > 0) ? prazo : t->duracao;

        /* Estado Inicial no TCB */
        t->estado = ESTADO_PRONTA;
        t->execucoes_concluidas = 0;
        t->tempo_restante_instancia = 0;
        t->proximo_deadline_absoluto = 0;
        t->cpu_atual = -1;
        t->perdeu_prazo_atual = 0;
        t->foi_sorteada_atual = 0;

        qtd_tarefas_validas++;
    }

    fclose(f);

    if (qtd_tarefas_validas == 0) {
        fprintf(stderr, "[ERRO PARSER] Nenhuma tarefa periódica válida encontrada no arquivo.\n");
        free(tarefas);
        return 0;
    }

    config->total_tarefas = qtd_tarefas_validas;
    *vetor_tarefas = tarefas;

    printf("[SUCESSO PARSER] Arquivo '%s' carregado com sucesso!\n", caminho_arquivo);
    printf("                Algoritmo: %s | CPUs: %d | Quantum: %d | Tarefas Ativas: %d\n\n",
           config->algoritmo_escalonamento, config->qtde_cpus, config->quantum, config->total_tarefas);

    return 1;
}

void parser_liberar_tarefas(TCB *vetor_tarefas) {
    if (vetor_tarefas) free(vetor_tarefas);
}
