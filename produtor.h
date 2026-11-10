/*
 * produtor.h — Interface da thread produtora
 */

#ifndef PRODUTOR_H
#define PRODUTOR_H

/* Função da thread produtora (assinatura compatível com pthread_create) */
void *produtor(void *arg);

#endif /* PRODUTOR_H */
