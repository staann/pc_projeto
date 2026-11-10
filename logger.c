/*
 * logger.c — Implementação do logger assíncrono com semáforos POSIX
 *
 * Arquitetura:
 *   - Fila circular de EventoLog com TAMANHO_FILA_LOG posições.
 *   - Dois semáforos POSIX contadores:
 *       sem_espacos_log : espaços livres (início: TAMANHO_FILA_LOG)
 *       sem_itens_log   : itens pendentes (início: 0)
 *   - Um mutex para proteger a leitura/escrita na fila circular.
 *   - Thread de log: aguarda em sem_wait(&sem_itens_log) e exibe o evento.
 *
 * Envio (produtores/consumidores):
 *   - Usa sem_trywait(&sem_espacos_log) — não-bloqueante.
 *   - Se a fila estiver cheia, o evento é descartado silenciosamente.
 *
 * Encerramento:
 *   - logger_shutdown() insere um evento LOG_SHUTDOWN na fila e faz join
 *     da thread de log.
 */

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

/* ---- Fila circular de eventos ---- */
static EventoLog fila_log[TAMANHO_FILA_LOG]; /* array circular de eventos  */
static int cabeca_log = 0;                   /* índice de leitura          */
static int cauda_log  = 0;                   /* índice de escrita          */

/* ---- Semáforos POSIX (sem_t) ---- */
static sem_t sem_espacos_log; /* conta espaços livres na fila de log       */
static sem_t sem_itens_log;   /* conta itens pendentes na fila de log      */

/* ---- Mutex para exclusão mútua na fila circular ---- */
static pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;

/* ---- Thread de log ---- */
static pthread_t thread_log;

/* ---- Instante de início para cálculo de timestamp_ms ---- */
static struct timespec tempo_inicio;

/* ================================================================
 * Função auxiliar: retorna milissegundos desde o início da execução
 * ================================================================ */
static double ms_desde_inicio(void) {
    struct timespec agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    double diff_s  = (double)(agora.tv_sec  - tempo_inicio.tv_sec);
    double diff_ns = (double)(agora.tv_nsec - tempo_inicio.tv_nsec);
    return diff_s * 1000.0 + diff_ns / 1.0e6;
}

/* ================================================================
 * Função da thread de log
 *
 * Loop principal:
 *   1. Aguarda sem_wait(&sem_itens_log) — bloqueia enquanto fila vazia.
 *   2. Adquire mutex_log para leitura segura da fila.
 *   3. Lê e avança cabeca_log.
 *   4. Libera mutex_log.
 *   5. Libera um espaço com sem_post(&sem_espacos_log).
 *   6. Exibe o evento (após liberar o mutex, para não bloquear produtores).
 *   7. Repete até receber LOG_SHUTDOWN.
 * ================================================================ */
static void *thread_log_func(void *arg) {
    (void)arg; /* parâmetro não utilizado */

    while (1) {
        /* Aguarda um item disponível na fila de log */
        sem_wait(&sem_itens_log);

        /* Adquire o mutex para leitura segura */
        pthread_mutex_lock(&mutex_log);

        /* Lê o evento da fila circular */
        EventoLog ev = fila_log[cabeca_log];
        cabeca_log = (cabeca_log + 1) % TAMANHO_FILA_LOG;

        pthread_mutex_unlock(&mutex_log);

        /* Libera uma posição na fila */
        sem_post(&sem_espacos_log);

        /* Verifica evento sentinela de encerramento */
        if (ev.tipo == LOG_SHUTDOWN) {
            break;
        }

        /* Exibe o evento com timestamp e informações contextuais */
        switch (ev.tipo) {
            case LOG_PRODUZIU:
                printf("[%7.1fms] [Produtor %d] Inseriu: %d | Buffer: %d/%d\n",
                       ev.timestamp_ms, ev.thread_id, ev.valor,
                       ev.quantidade_no_buffer, TAMANHO_BUFFER);
                break;

            case LOG_CONSUMIU:
                printf("[%7.1fms] [Consumidor %d] Retirou: %d | Buffer: %d/%d\n",
                       ev.timestamp_ms, ev.thread_id, ev.valor,
                       ev.quantidade_no_buffer, TAMANHO_BUFFER);
                break;

            case LOG_BUF_CHEIO:
                printf("[%7.1fms] [Produtor %d] Buffer cheio. Aguardando...\n",
                       ev.timestamp_ms, ev.thread_id);
                break;

            case LOG_BUF_VAZIO:
                printf("[%7.1fms] [Consumidor %d] Buffer vazio. Aguardando...\n",
                       ev.timestamp_ms, ev.thread_id);
                break;

            default:
                break;
        }
        fflush(stdout);
    }

    return NULL;
}

/* ================================================================
 * logger_init — Inicializa fila, semáforos e registra tempo de início
 * ================================================================ */
void logger_init(void) {
    /* Registra o instante de início para cálculo de timestamps */
    clock_gettime(CLOCK_MONOTONIC, &tempo_inicio);

    /* Inicializa semáforo de espaços: começa com TAMANHO_FILA_LOG espaços */
    if (sem_init(&sem_espacos_log, 0, TAMANHO_FILA_LOG) != 0) {
        perror("sem_init sem_espacos_log");
        exit(EXIT_FAILURE);
    }

    /* Inicializa semáforo de itens: começa vazio (0 itens) */
    if (sem_init(&sem_itens_log, 0, 0) != 0) {
        perror("sem_init sem_itens_log");
        exit(EXIT_FAILURE);
    }

    /* Inicializa índices da fila circular */
    cabeca_log = 0;
    cauda_log  = 0;
}

/* ================================================================
 * logger_iniciar_thread — Cria a thread de log
 * ================================================================ */
void logger_iniciar_thread(void) {
    if (pthread_create(&thread_log, NULL, thread_log_func, NULL) != 0) {
        perror("pthread_create thread_log");
        exit(EXIT_FAILURE);
    }
}

/* ================================================================
 * log_evento — Envia um evento para a fila (não-bloqueante)
 *
 * Usa sem_trywait para tentar reservar um espaço. Se a fila estiver
 * cheia (sem_trywait retorna EAGAIN), o evento é descartado.
 * ================================================================ */
void log_evento(TipoEvento tipo, int thread_id, int valor, int qtd_buffer) {
    /* Tenta reservar um espaço livre — não bloqueia */
    if (sem_trywait(&sem_espacos_log) != 0) {
        /* Fila cheia: descarta o evento silenciosamente */
        return;
    }

    /* Preenche o evento com timestamp atual */
    EventoLog ev;
    ev.tipo               = tipo;
    ev.thread_id          = thread_id;
    ev.valor              = valor;
    ev.quantidade_no_buffer = qtd_buffer;
    ev.timestamp_ms       = ms_desde_inicio();

    /* Adquire o mutex para escrita segura na fila circular */
    pthread_mutex_lock(&mutex_log);

    fila_log[cauda_log] = ev;
    cauda_log = (cauda_log + 1) % TAMANHO_FILA_LOG;

    pthread_mutex_unlock(&mutex_log);

    /* Sinaliza que há um novo item disponível na fila de log */
    sem_post(&sem_itens_log);
}

/* ================================================================
 * logger_shutdown — Envia evento sentinela e aguarda thread terminar
 * ================================================================ */
void logger_shutdown(void) {
    /* Insere o evento sentinela — usa sem_wait para garantir a inserção */
    sem_wait(&sem_espacos_log);

    pthread_mutex_lock(&mutex_log);

    EventoLog ev_shutdown;
    ev_shutdown.tipo               = LOG_SHUTDOWN;
    ev_shutdown.thread_id          = -1;
    ev_shutdown.valor              = 0;
    ev_shutdown.quantidade_no_buffer = 0;
    ev_shutdown.timestamp_ms       = ms_desde_inicio();

    fila_log[cauda_log] = ev_shutdown;
    cauda_log = (cauda_log + 1) % TAMANHO_FILA_LOG;

    pthread_mutex_unlock(&mutex_log);

    sem_post(&sem_itens_log);

    /* Aguarda a thread de log finalizar */
    pthread_join(thread_log, NULL);
}

/* ================================================================
 * logger_destroy — Libera os semáforos e o mutex do logger
 * ================================================================ */
void logger_destroy(void) {
    sem_destroy(&sem_espacos_log);
    sem_destroy(&sem_itens_log);
    pthread_mutex_destroy(&mutex_log);
}
