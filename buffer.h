/*
 * buffer.h — Interface do buffer circular compartilhado
 *
 * Declara as estruturas de dados, variáveis globais e funções que
 * implementam o buffer limitado protegido por mutex e variáveis de condição.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include "config.h"

/* ---- Estrutura do buffer circular ---- */
typedef struct {
    int dados[TAMANHO_BUFFER]; /* vetor circular de itens                   */
    int cabeca;                /* índice do próximo item a ser consumido     */
    int cauda;                 /* índice da próxima posição livre            */
    int quantidade;            /* número de itens presentes no buffer        */
    int proximo_valor;         /* contador global para gerar valores únicos  */
} Buffer;

/* ---- Mecanismos de sincronização do buffer (definidos em buffer.c) ---- */
extern Buffer buffer_compartilhado;
extern pthread_mutex_t mutex_buffer;
extern pthread_cond_t  nao_cheio;
extern pthread_cond_t  nao_vazio;

/* ---- Interface pública ---- */

/* Inicializa o buffer e os mecanismos de sincronização */
void buffer_init(void);

/* Destrói os mecanismos de sincronização do buffer */
void buffer_destroy(void);

/* Insere um item no buffer (bloqueia se buffer cheio) — retorna o valor inserido */
int buffer_inserir(int id_thread);

/* Retira um item do buffer (bloqueia se buffer vazio) — retorna o valor retirado */
int buffer_retirar(int id_thread);

#endif /* BUFFER_H */
