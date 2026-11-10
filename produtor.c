/*
 * produtor.c — Implementação da thread produtora
 *
 * Cada thread produtora gera ITENS_POR_THREAD valores e os insere no
 * buffer compartilhado via buffer_inserir(). Em vez de usar printf()
 * diretamente, o log é enviado ao logger assíncrono dentro de buffer_inserir().
 */

#include "produtor.h"
#include "buffer.h"
#include "config.h"

#include <stdlib.h>
#include <unistd.h>

/* ================================================================
 * produtor — Função da thread produtora
 *
 * Argumento: ponteiro para int com o id da thread (1-based).
 * ================================================================ */
void *produtor(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < ITENS_POR_THREAD; i++) {
        /* Simula tempo variável de produção (100–400 ms) */
        usleep((rand() % 300 + 100) * 1000);

        /* Insere item no buffer (bloqueia se cheio; loga internamente) */
        buffer_inserir(id);
    }

    return NULL;
}
