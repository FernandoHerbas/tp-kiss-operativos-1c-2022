#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <general/carpisLib.h>
#include <utils/serverutils.h>
#include <utils/utilslib.h>
#include <pthread.h>
#include <sys/syscall.h>

///////////////////////////////////////////

typedef struct config_kernel
{
    char *ip_memoria;
    char *puerto_memoria;
    char *ip_cpu;
    char *puerto_cpu_dispatch;
    char *puerto_cpu_interrupt;
    char *puerto_escucha;
    char *algoritmo_planificacion;
    int estimacion_inicial;
    int alfa_plani;
    int grado_multiprogramacion;
    int tiempo_max_bloqueado;
} t_config_kernel;

typedef struct proceso
{
    int socket_proceso;
    t_pcb *un_pcb;
} t_proceso;

///////////////////////////////////////////

t_log *un_logger;
t_config_kernel una_config_kernel;

t_list *procesos_en_new;
t_list *procesos_en_ready;
t_list *procesos_en_bloq;
t_list *procesos_en_exit;

pthread_mutex_t mutex_log;
pthread_mutex_t mutex_procesos_en_new;

///////////////////////////////////////////

void iniciar_logger();
void iniciar_config();
void iniciar_mutex();

void iniciar_estructuras();

void liberar_memoria();

///////////////////////////////////////////

#include <conexion.h>

#endif