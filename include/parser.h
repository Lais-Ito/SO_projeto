/* 
 * ============================================================================
 * Arquivo: parser.h
 * Descrição: Protótipos das funções do leitor de arquivos de configuração.
 * Projeto: Simulador Visual de Sistema Operacional (Projeto A)
 * ============================================================================
 */

#ifndef PARSER_H
#define PARSER_H

#include "types.h"

/* Realiza a leitura e validação do arquivo .txt contendo a configuração */
int parser_carregar_arquivo(const char *caminho_arquivo, ConfiguracaoSistema *config, TCB **vetor_tarefas);

/* Libera a memória alocada para o vetor de tarefas */
void parser_liberar_tarefas(TCB *vetor_tarefas);

#endif /* PARSER_H */
