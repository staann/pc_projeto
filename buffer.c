/*
 * buffer.c — Implementação do buffer circular compartilhado
 *
 * O buffer é protegido por mutex e variáveis de condição Pthreads:
 *   - mutex_buffer : exclusão mútua na região crítica
 *   - nao_cheio    : bloqueia produtores quando buffer está cheio
 *   - nao_vazio    : bloqueia consumidores quando buffer está vazio
 *
 * Os eventos de inserção/remoção são reportados ao logger assíncrono
 * via log_evento(), substituindo printf() direto neste módulo.
 */

#include "buffer.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>

/* ---- Instância do buffer compartilhado ---- */
Buffer buffer_compartilhado;

/* ---- Mecanismos de sincronização ---- */
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  nao_cheio    = PTHREAD_COND_INITIALIZER;
pthread_cond_t  nao_vazio    = PTHREAD_COND_INITIALIZER;

/* ================================================================
 * buffer_init — Inicializa o buffer e seus campos
 * ================================================================ */
void buffer_init(void) {
    buffer_compartilhado.cabeca        = 0;
    buffer_compartilhado.cauda         = 0;
    buffer_compartilhado.quantidade    = 0;
    buffer_compartilhado.proximo_valor = 1;
}

/* ================================================================
 * buffer_destroy — Destrói mutex e variáveis de condição
 * ================================================================ */
void buffer_destroy(void) {
    pthread_mutex_destroy(&mutex_buffer);
    pthread_cond_destroy(&nao_cheio);
    pthread_cond_destroy(&nao_vazio);
}

/* ================================================================
 * buffer_inserir — Insere um item no buffer (bloqueia se cheio)
 *
 * Protocolo:
 *   1. Adquire o mutex.
 *   2. Aguarda em nao_cheio enquanto buffer estiver cheio.
 *   3. Insere o item e avança o ponteiro de cauda.
 *   4. Sinaliza nao_vazio.
 *   5. Libera o mutex.
 *   6. Envia evento de log (fora do mutex).
 *
 * Retorna o valor inserido.
 * ================================================================ */
int buffer_inserir(int id_thread) {
    pthread_mutex_lock(&mutex_buffer);

    /* Aguarda enquanto o buffer estiver cheio */
    while (buffer_compartilhado.quantidade == TAMANHO_BUFFER) {
        /* Notifica o logger que o produtor está bloqueado */
        log_evento(LOG_BUF_CHEIO, id_thread, 0,
                   buffer_compartilhado.quantidade);
        pthread_cond_wait(&nao_cheio, &mutex_buffer);
    }

    /* Seção crítica: insere item no slot de cauda */
    int valor = buffer_compartilhado.proximo_valor++;
    buffer_compartilhado.dados[buffer_compartilhado.cauda] = valor;
    buffer_compartilhado.cauda =
        (buffer_compartilhado.cauda + 1) % TAMANHO_BUFFER;
    buffer_compartilhado.quantidade++;

    int qtd_atual = buffer_compartilhado.quantidade;

    /* Sinaliza consumidores: agora há pelo menos um item */
    pthread_cond_signal(&nao_vazio);

    pthread_mutex_unlock(&mutex_buffer);

    /* Registra o evento de inserção no logger (fora da seção crítica) */
    log_evento(LOG_PRODUZIU, id_thread, valor, qtd_atual);

    return valor;
}

/* ================================================================
 * buffer_retirar — Retira um item do buffer (bloqueia se vazio)
 *
 * Protocolo simétrico ao de inserção.
 * Retorna o valor retirado.
 * ================================================================ */
int buffer_retirar(int id_thread) {
    pthread_mutex_lock(&mutex_buffer);

    /* Aguarda enquanto o buffer estiver vazio */
    while (buffer_compartilhado.quantidade == 0) {
        /* Notifica o logger que o consumidor está bloqueado */
        log_evento(LOG_BUF_VAZIO, id_thread, 0,
                   buffer_compartilhado.quantidade);
        pthread_cond_wait(&nao_vazio, &mutex_buffer);
    }

    /* Seção crítica: retira item do slot de cabeça */
    int valor = buffer_compartilhado.dados[buffer_compartilhado.cabeca];
    buffer_compartilhado.cabeca =
        (buffer_compartilhado.cabeca + 1) % TAMANHO_BUFFER;
    buffer_compartilhado.quantidade--;

    int qtd_atual = buffer_compartilhado.quantidade;

    /* Sinaliza produtores: agora há pelo menos um espaço livre */
    pthread_cond_signal(&nao_cheio);

    pthread_mutex_unlock(&mutex_buffer);

    /* Registra o evento de remoção no logger (fora da seção crítica) */
    log_evento(LOG_CONSUMIU, id_thread, valor, qtd_atual);

    return valor;
}
