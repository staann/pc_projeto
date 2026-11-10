/*
 * consumidor.c — Implementação da thread consumidora
 *
 * Cada thread consumidora retira itens do buffer compartilhado via
 * buffer_retirar(). O total de itens a consumir é distribuído
 * igualmente entre os consumidores (NUM_PRODUTORES * ITENS_POR_THREAD
 * / NUM_CONSUMIDORES). O log é delegado ao logger assíncrono.
 */

#include "consumidor.h"
#include "buffer.h"
#include "config.h"

#include <stdlib.h>
#include <unistd.h>

/* ================================================================
 * consumidor — Função da thread consumidora
 *
 * Argumento: ponteiro para int com o id da thread (1-based).
 * ================================================================ */
void *consumidor(void *arg) {
    int id = *(int *)arg;

    /* Cada consumidor retira uma fração igual do total produzido */
    int total_esperado = (NUM_PRODUTORES * ITENS_POR_THREAD) / NUM_CONSUMIDORES;

    for (int i = 0; i < total_esperado; i++) {
        /* Simula tempo variável de consumo (200–600 ms) */
        usleep((rand() % 400 + 200) * 1000);

        /* Retira item do buffer (bloqueia se vazio; loga internamente) */
        buffer_retirar(id);
    }

    return NULL;
}
