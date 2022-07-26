#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <general/carpisLib.h>
#include <utils/serverutils.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <semaphore.h>
#include <errno.h>

#define size_dato sizeof(uint32_t)

typedef int32_t dir_logica;
/**
 * @DESC Quizas este en carpisLib
 *
 */
typedef enum
{
    LRU,
    FIFO
}t_algoritmo;

typedef struct
{
    char* ip_memoria;
    char* puerto_memoria;
    int cantidad_entradas_tlb;
    char *algoritmo_reemplazo_tlb;
    int retardo_noop;
    char* puerto_escucha_dispatch;
    char* puerto_escucha_interrupt;
    int entradas_por_tabla;
    int tamanio_pagina;
    char* ip_cpu;
    int log_habilitado;
} t_config_cpu;

typedef struct t_columna_tlb {
    int entrada;
    bool is_free;
    int pagina;
    unsigned int nro_marco;
    long tiempo_usado;
}t_columna_tlb;

/////////////////////////////////////////////////////////////
t_pcb *pcb;
int operacion_a_enviar;
t_proceso_pcb *proceso_pcb;;
bool hay_interrupcion;

t_list *tlb;
t_algoritmo tipo_algoritmo_tlb;

t_config_cpu config_cpu;
t_log *logger_cpu;

int socket_memoria;
int socket_kernel_dispatch;
int socket_kernel_interrupt;

sem_t sem_interrupt;
sem_t sem_interrupt_fin;
sem_t sem_ciclo_de_instruccion;
sem_t sem_busqueda_proceso_nuevo;

pthread_mutex_t mutex_logger;
pthread_mutex_t mutex_flag_interrupcion;
////////////////////////////////////////////////////////////

void leer_configuracion(char *);
void arrancar_logger();
void conectar_a_memoria_y_recibir_config();
void obtener_configuracion(int *socket, modulo modulo);
void iniciar_semaforos();

void liberar_semaforos();
void liberar_configuracion_y_log();

#include "mmu.h"
#include "cpu.h"
#include "tlb.h"
#include "conector_memoria.h"

#endif
