/*
 * config.h — Constantes e configurações globais do sistema produtor-consumidor
 *
 * Este cabeçalho centraliza todas as constantes de configuração utilizadas
 * pelos demais módulos do projeto.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* ---- Configurações do buffer compartilhado ---- */
#define TAMANHO_BUFFER   5   /* capacidade máxima do buffer principal        */

/* ---- Configurações das threads ---- */
#define NUM_PRODUTORES   3   /* número de threads produtoras                 */
#define NUM_CONSUMIDORES 3   /* número de threads consumidoras               */
#define ITENS_POR_THREAD 4   /* itens que cada produtor gera                 */

/* ---- Configurações do logger assíncrono ---- */
#define TAMANHO_FILA_LOG 20  /* capacidade da fila circular de eventos de log */

#endif /* CONFIG_H */
