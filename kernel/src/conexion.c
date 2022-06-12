#include "../include/conexion.h"

////////////////////////////////////////////

void conexion(void)
{
    int socket_kernel_serv = iniciar_servidor("127.0.0.1", "25655");
    log_info(un_logger, "Kernel a la espera de conexiones ... \n");

    int socket_proceso = 0;

    conectar_con_cpu(socket_kernel_serv);

    // Aca tendriamos que conectarnos con MEMORIA y CPU
    // En caso de no poder realizar la conexion, error!! Kernel Panic (?

    while(true)
    {
        t_com_proceso *comunicacion_proceso = malloc(sizeof(t_com_proceso));
        pthread_mutex_init(&comunicacion_proceso->mutex_socket_proceso,NULL);

        pthread_mutex_lock(&comunicacion_proceso->mutex_socket_proceso);
        socket_proceso = esperar_cliente(socket_kernel_serv);
        pthread_mutex_unlock(&comunicacion_proceso->mutex_socket_proceso);

        pthread_t *hilo_proceso = malloc(sizeof(pthread_t));

        comunicacion_proceso->socket_proceso = socket_proceso;
        comunicacion_proceso->hilo_com_proceso = hilo_proceso;

        pthread_create(hilo_proceso, NULL, gestionar_comunicacion_con_proceso, (void*)comunicacion_proceso);
        pthread_detach(*hilo_proceso);
    }
}

int conectar_con_cpu(int socket_kernel_serv)
{
    socket_dispatch = crear_conexion(una_config_kernel.ip_cpu, una_config_kernel.puerto_cpu_dispatch);
    socket_interrupt = crear_conexion(una_config_kernel.ip_cpu, una_config_kernel.puerto_cpu_interrupt);
    log_info(un_logger, "Enviando HANDSHAKE a CPU \n");
    enviar_handshake(&socket_dispatch, KERNEL);
    return esperar_handshake(&socket_dispatch, confirmar_modulo);
}

void confirmar_modulo(int *socket, modulo modulo) {
    if(modulo == CPU) {
        log_info(un_logger, "HANDSHAKE exitoso con CPU \n");
    }
    else {
        log_error(un_logger,"KERNEL PANIC -> Error al realizar el HANDSHAKE con CPU");
        exit(EXIT_FAILURE);
    }
}

void *gestionar_comunicacion_con_proceso(void* com_proceso_param)
{
    t_com_proceso *comunicacion_proceso = (t_com_proceso*) com_proceso_param;

    pthread_mutex_lock(&comunicacion_proceso->mutex_socket_proceso);
    codigo_operacion un_codigo = (codigo_operacion) recibir_operacion(comunicacion_proceso->socket_proceso);
    pthread_mutex_unlock(&comunicacion_proceso->mutex_socket_proceso);

    switch(un_codigo)
    {
        case T_CONSOLA:
            pthread_mutex_lock(&mutex_log);
            log_info(un_logger,"Recibi unas instrucciones!");
            pthread_mutex_unlock(&mutex_log);
            inicializar_proceso(comunicacion_proceso);
            break;
        default:
            pthread_mutex_lock(&mutex_log);
            log_error(un_logger,"Mensaje no entendidoooo!!!");
            pthread_mutex_unlock(&mutex_log);
            break;
    }
    return NULL;
}

////////////////////////////////////////////

void inicializar_proceso(t_com_proceso *comunicacion_proceso)
{
    t_proceso *un_proceso = malloc(sizeof (t_proceso));

    un_proceso->un_pcb = inicializar_pcb(comunicacion_proceso);
    un_proceso->comunicacion_proceso = comunicacion_proceso;
    un_proceso->tiempo_ejecutando_estimacion = 0;

    pasar_proceso_a_new(un_proceso);
}

void pasar_proceso_a_new(t_proceso *un_proceso)
{
    pthread_mutex_lock(&mutex_procesos_en_new);
    queue_push(procesos_en_new,(void*) un_proceso);
    pthread_mutex_unlock(&mutex_procesos_en_new);

    //Le avisamos al hilo de largo plazo que se encarga de pasar a ready, que
    //Llego un proceso y asi NO puede encontrar la queue vacia!!
    sem_post(&llego_un_proceso);
}

t_pcb *inicializar_pcb(t_com_proceso *comunicacion_proceso)
{
    pthread_mutex_lock(&comunicacion_proceso->mutex_socket_proceso);
    t_consola *una_consola = recibir_datos_consola(comunicacion_proceso->socket_proceso);
    pthread_mutex_unlock(&comunicacion_proceso->mutex_socket_proceso);
    t_pcb *un_pcb = malloc(sizeof(t_pcb));

    asignar_pid(un_pcb);
    un_pcb->program_counter = 0;
    un_pcb->una_estimacion = una_config_kernel.estimacion_inicial;
    un_pcb->un_estado = NEW;
    un_pcb->consola = una_consola;

    un_pcb->id_tabla_1n = UNDEFINED;

    //probar_comunicacion_instrucciones(un_pcb);
    return un_pcb;
}

////////////////////////////////////////////

void mostrar_en_pantalla(t_instruccion *una_instruccion)
{
    pthread_mutex_lock(&mutex_log);
    log_info(un_logger,"El valor de instruccion es: %d      | %d      | %d",una_instruccion->instruc,una_instruccion->parametro1, una_instruccion->parametro2);
    pthread_mutex_unlock(&mutex_log);
}

void probar_comunicacion_instrucciones(t_pcb * un_pcb)
{
    for(int i = 0; queue_size(un_pcb->consola->instrucciones) > 0;i++)
    {
        t_instruccion *una_instruccion = (t_instruccion*) queue_pop(un_pcb->consola->instrucciones);
        mostrar_en_pantalla(una_instruccion);
    }
}

void asignar_pid(t_pcb *un_pcb)
{
    pthread_mutex_lock(&mutex_contador_pid);
    un_pcb->pid = contador_pid;
    contador_pid++;
    pthread_mutex_unlock(&mutex_contador_pid);
}

////////////////////////////////////////////////

//Funcion temporal (?
void responder_fin_proceso(int socket_proceso)
{
    int estado_finalizacion = 1;
    send(socket_proceso, &estado_finalizacion, sizeof(int), MSG_WAITALL);

    pthread_mutex_lock(&mutex_log);
    log_info(un_logger,"Se envio mensaje de finalizacion!!");
    pthread_mutex_unlock(&mutex_log);

    sem_post(&grado_multiprog_lo_permite);
}

//////////////////////////////////////////////////
// Utils handshake

void realizar_handshake(int socket_proceso)
{

    responder_handshake(socket_proceso);
}

void mapeador(int *un_socket,modulo un_modulo)
{
    pthread_mutex_lock(&mutex_log);
    log_info(un_logger,"Se conecto un proceso!! 😂");
    pthread_mutex_unlock(&mutex_log);
    return;
}

void responder_handshake(int socket_proceso)
{
    modulo id_modulo_solicitante = KERNEL;
    enviar_handshake(&socket_proceso,id_modulo_solicitante);
}

//////////////////////////////////////////////////
