/*
 * Programacao Concorrente - Trabalho 1
 * Sincronizacao entre Processos
 *
 * Problema: Produtor-Consumidor com Buffer Limitado
 *
 * Descricao do Problema:
 *   Multiplos produtores e consumidores compartilham um buffer de tamanho
 *   fixo na memoria. Os produtores inserem itens no buffer e os consumidores
 *   retiram itens do buffer.
 *
 *   Condicoes de corrida ocorrem quando:
 *     - Dois produtores tentam inserir no mesmo slot simultaneamente.
 *     - Dois consumidores tentam retirar o mesmo item simultaneamente.
 *     - Um produtor insere em buffer cheio (sobrescrevendo dados).
 *     - Um consumidor retira de buffer vazio (lendo lixo).
 *
 * Solucao:
 *   Utilizamos mutex (pthread_mutex_t) para exclusao mutua na regiao critica
 *   e variaveis condicao (pthread_cond_t) para sincronizar as esperas de
 *   buffer cheio e buffer vazio.
 *
 * Compilar: gcc -o produtor_consumidor produtor_consumidor.c -lpthread
 * Executar: ./produtor_consumidor
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* ---- Configuracoes ---- */
#define TAMANHO_BUFFER  5    /* capacidade maxima do buffer compartilhado */
#define NUM_PRODUTORES  3    /* quantidade de threads produtoras           */
#define NUM_CONSUMIDORES 3   /* quantidade de threads consumidoras         */
#define ITENS_POR_THREAD 4   /* cada produtor produz esta quantidade       */

/* ---- Memoria compartilhada ---- */
int buffer[TAMANHO_BUFFER]; /* buffer circular compartilhado              */
int cabeca = 0;             /* proximo indice para consumo                */
int cauda  = 0;             /* proximo indice para insercao               */
int quantidade = 0;         /* numero de itens atualmente no buffer       */

/* ---- Mecanismos de sincronizacao ---- */
pthread_mutex_t mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  nao_cheio = PTHREAD_COND_INITIALIZER; /* sinaliza: ha espaco */
pthread_cond_t  nao_vazio = PTHREAD_COND_INITIALIZER; /* sinaliza: ha item   */

/* Contador global para gerar valores unicos */
int proximo_valor = 1;

/* ================================================================
 * Funcao auxiliar: imprime o estado atual do buffer (protegida
 * pelo mutex — deve ser chamada DENTRO da secao critica).
 * ================================================================ */
void imprimir_buffer(void) {
    printf("  Buffer [");
    for (int i = 0; i < TAMANHO_BUFFER; i++) {
        int idx = (cabeca + i) % TAMANHO_BUFFER;
        if (i < quantidade) {
            printf("%3d", buffer[idx]);
        } else {
            printf("  _");
        }
    }
    printf(" ] (%d/%d)\n", quantidade, TAMANHO_BUFFER);
}

/* ================================================================
 * Thread Produtora
 *
 * Cada produtor gera ITENS_POR_THREAD valores e os insere no
 * buffer. Se o buffer estiver cheio, aguarda na variavel condicao
 * "nao_cheio" ate que um consumidor libere espaco.
 * ================================================================ */
void *produtor(void *arg) {
    int id = *(int *)arg;

    for (int i = 0; i < ITENS_POR_THREAD; i++) {
        /* Simula tempo de producao */
        usleep((rand() % 300 + 100) * 1000);

        pthread_mutex_lock(&mutex);

        /* Espera enquanto buffer estiver cheio */
        while (quantidade == TAMANHO_BUFFER) {
            printf("[Produtor %d] Buffer cheio. Aguardando...\n", id);
            pthread_cond_wait(&nao_cheio, &mutex);
        }

        /* Secao critica: insere item no buffer circular */
        int valor = proximo_valor++;
        buffer[cauda] = valor;
        cauda = (cauda + 1) % TAMANHO_BUFFER;
        quantidade++;

        printf("[Produtor %d] Inseriu: %d | ", id, valor);
        imprimir_buffer();

        /* Sinaliza que o buffer nao esta mais vazio */
        pthread_cond_signal(&nao_vazio);

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

/* ================================================================
 * Thread Consumidora
 *
 * Cada consumidor retira itens do buffer enquanto houver itens a
 * consumir. Se o buffer estiver vazio, aguarda na variavel
 * condicao "nao_vazio".
 * ================================================================ */
void *consumidor(void *arg) {
    int id = *(int *)arg;
    int total_esperado = (NUM_PRODUTORES * ITENS_POR_THREAD) / NUM_CONSUMIDORES;

    for (int i = 0; i < total_esperado; i++) {
        /* Simula tempo de consumo */
        usleep((rand() % 400 + 200) * 1000);

        pthread_mutex_lock(&mutex);

        /* Espera enquanto buffer estiver vazio */
        while (quantidade == 0) {
            printf("[Consumidor %d] Buffer vazio. Aguardando...\n", id);
            pthread_cond_wait(&nao_vazio, &mutex);
        }

        /* Secao critica: retira item do buffer circular */
        int valor = buffer[cabeca];
        cabeca = (cabeca + 1) % TAMANHO_BUFFER;
        quantidade--;

        printf("[Consumidor %d] Retirou: %d | ", id, valor);
        imprimir_buffer();

        /* Sinaliza que o buffer nao esta mais cheio */
        pthread_cond_signal(&nao_cheio);

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

/* ================================================================
 * Main
 * ================================================================ */
int main(void) {
    pthread_t threads_prod[NUM_PRODUTORES];
    pthread_t threads_cons[NUM_CONSUMIDORES];
    int ids_prod[NUM_PRODUTORES];
    int ids_cons[NUM_CONSUMIDORES];

    srand(42); /* semente fixa para reproducibilidade */

    printf("=== Produtor-Consumidor com Buffer Limitado ===\n");
    printf("Buffer: %d posicoes | %d produtores | %d consumidores\n\n",
           TAMANHO_BUFFER, NUM_PRODUTORES, NUM_CONSUMIDORES);

    /* Cria threads consumidoras */
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        ids_cons[i] = i + 1;
        pthread_create(&threads_cons[i], NULL, consumidor, &ids_cons[i]);
    }

    /* Cria threads produtoras */
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        ids_prod[i] = i + 1;
        pthread_create(&threads_prod[i], NULL, produtor, &ids_prod[i]);
    }

    /* Aguarda todas as threads produtoras terminarem */
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        pthread_join(threads_prod[i], NULL);
    }

    /* Aguarda todas as threads consumidoras terminarem */
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        pthread_join(threads_cons[i], NULL);
    }

    /* Libera recursos */
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&nao_cheio);
    pthread_cond_destroy(&nao_vazio);

    printf("\n=== Execucao concluida com sucesso! ===\n");
    return 0;
}
