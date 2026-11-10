/*
 * logger.h — Interface do logger assíncrono com semáforos POSIX
 *
 * O logger usa uma fila circular separada sincronizada exclusivamente
 * com semáforos POSIX (sem_t), demonstrando essa primitiva ao lado do
 * mutex+condição do buffer principal.
 *
 * Produtores e consumidores enviam eventos com log_evento() de forma
 * não-bloqueante. Uma thread dedicada consome esses eventos e os exibe
 * no terminal com timestamp em milissegundos.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>
#include "config.h"

/* ---- Tipos de eventos de log ---- */
typedef enum {
    LOG_PRODUZIU,   /* produtor inseriu um item no buffer          */
    LOG_CONSUMIU,   /* consumidor retirou um item do buffer        */
    LOG_BUF_CHEIO,  /* produtor encontrou buffer cheio e aguardou  */
    LOG_BUF_VAZIO,  /* consumidor encontrou buffer vazio e aguardou*/
    LOG_SHUTDOWN    /* evento sentinela: encerra a thread de log   */
} TipoEvento;

/* ---- Estrutura de um evento de log ---- */
typedef struct {
    TipoEvento tipo;           /* tipo do evento                           */
    int        thread_id;      /* id da thread que gerou o evento          */
    int        valor;          /* valor do item envolvido (0 se n/a)       */
    int        quantidade_no_buffer; /* ocupação do buffer no momento      */
    double     timestamp_ms;   /* milissegundos desde o início da execução */
} EventoLog;

/* ---- Interface pública ---- */

/* Inicializa a fila de log, semáforos e registra o instante de início */
void logger_init(void);

/* Inicia a thread de log — deve ser chamado após logger_init() */
void logger_iniciar_thread(void);

/* Envia um evento para a fila de log (não-bloqueante: descarta se fila cheia) */
void log_evento(TipoEvento tipo, int thread_id, int valor, int qtd_buffer);

/* Envia evento sentinela e aguarda a thread de log terminar */
void logger_shutdown(void);

/* Destrói semáforos e mutex do logger */
void logger_destroy(void);

#endif /* LOGGER_H */
