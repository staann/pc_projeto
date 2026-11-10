/*
 * consumidor.h — Interface da thread consumidora
 */

#ifndef CONSUMIDOR_H
#define CONSUMIDOR_H

/* Função da thread consumidora (assinatura compatível com pthread_create) */
void *consumidor(void *arg);

#endif /* CONSUMIDOR_H */
