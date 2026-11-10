/*
 * main.c — Ponto de entrada do sistema produtor-consumidor v2.0
 *
 * Ordem de inicialização:
 *   1. Inicializa buffer e logger.
 *   2. Inicia thread de log.
 *   3. Cria threads consumidoras.
 *   4. Cria threads produtoras.
 *
 * Ordem de terminação (evita perda de logs e race conditions):
 *   1. Join das threads produtoras (garante que toda a produção terminou).
 *   2. Join das threads consumidoras (garante que todo o consumo terminou).
 *   3. Envia LOG_SHUTDOWN ao logger e faz join (processa e exibe todos
 *      os eventos pendentes da fila antes de finalizar).
 *   4. Destrói recursos de buffer e logger.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "config.h"
#include "buffer.h"
#include "logger.h"
#include "produtor.h"
#include "consumidor.h"

int main(void) {
    /* Arrays de threads e ids */
    pthread_t threads_prod[NUM_PRODUTORES];
    pthread_t threads_cons[NUM_CONSUMIDORES];
    int ids_prod[NUM_PRODUTORES];
    int ids_cons[NUM_CONSUMIDORES];

    /* Semente fixa para reprodutibilidade */
    srand(42);

    /* ---- Inicialização ---- */
    buffer_init();
    logger_init();
    logger_iniciar_thread();

    printf("=== Produtor-Consumidor v2.0 com Logger Assincrono ===\n");
    printf("Buffer: %d posicoes | %d produtores | %d consumidores\n\n",
           TAMANHO_BUFFER, NUM_PRODUTORES, NUM_CONSUMIDORES);

    /* ---- Cria threads consumidoras primeiro ---- */
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        ids_cons[i] = i + 1;
        pthread_create(&threads_cons[i], NULL, consumidor, &ids_cons[i]);
    }

    /* ---- Cria threads produtoras ---- */
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        ids_prod[i] = i + 1;
        pthread_create(&threads_prod[i], NULL, produtor, &ids_prod[i]);
    }

    /* ---- Aguarda produtores terminarem ---- */
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        pthread_join(threads_prod[i], NULL);
    }

    /* ---- Aguarda consumidores terminarem (processam todo o buffer) ---- */
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        pthread_join(threads_cons[i], NULL);
    }

    /* ---- Encerra o logger (após toda produção e consumo concluídos) ---- */
    logger_shutdown();

    /* ---- Libera recursos ---- */
    buffer_destroy();
    logger_destroy();

    printf("\n=== Execucao concluida com sucesso! ===\n");
    return 0;
}
