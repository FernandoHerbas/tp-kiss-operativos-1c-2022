#include "../include/cpu.h"


void iniciar() {
    iniciar_mmu();
    conectar_a_memoria_y_recibir_config();
    esperar_a_kernel();
}

void esperar_a_kernel() {
    log_info(logger_cpu,"CPU a la espera de Kernel");

    pthread_t *hilo_dispatch = malloc(sizeof(pthread_t));
    pthread_t *hilo_interrupt = malloc(sizeof(pthread_t));
    pthread_t *hilo_ciclo_de_instruccion = malloc(sizeof(pthread_t));

    pthread_create(hilo_dispatch, NULL, &ejecutar_pcb, NULL);
    pthread_create(hilo_interrupt, NULL, &ejecutar_interrupcion, NULL);
    pthread_create(hilo_ciclo_de_instruccion, NULL, &ciclo_de_instruccion, NULL);

    pthread_join(*hilo_interrupt, NULL);
    pthread_join(*hilo_dispatch, NULL);
    pthread_join(*hilo_ciclo_de_instruccion, NULL);
}

void enviar_confirmacion(int *socket, modulo modulo_solicitante) {

    if(modulo_solicitante == KERNEL) {
        pthread_mutex_lock(&mutex_logger);
        log_info(logger_cpu,"HANDSHAKE con Kernel realizado");
        pthread_mutex_unlock(&mutex_logger);
        void *buffer = malloc(sizeof(int)*2);
        modulo modulo_actual = CPU;
        codigo_operacion handshake = HANDSHAKE;
        memcpy(buffer, &handshake, sizeof(int));
        memcpy(buffer + sizeof(int), &modulo_actual, sizeof(int));
        send(*socket, buffer, sizeof(int)*2, 0);
        free(buffer);
    } else {
        pthread_mutex_lock(&mutex_logger);
        log_error(logger_cpu, "Error al realizar el HANDSHAKE con Kernel");
        pthread_mutex_unlock(&mutex_logger);
    }
}

void *ejecutar_pcb(void *arg) {
    int cpu_dispatch = iniciar_servidor(config_cpu.ip_cpu, config_cpu.puerto_escucha_dispatch);
    socket_kernel_dispatch = esperar_cliente(cpu_dispatch);
    esperar_handshake(&socket_kernel_dispatch, enviar_confirmacion);

	while(true) {

		sem_wait(&sem_busqueda_proceso_nuevo);
        pthread_mutex_lock(&mutex_flag_interrupcion);
        hay_interrupcion = false;
        pthread_mutex_unlock(&mutex_flag_interrupcion);
		int operacion = recibir_operacion(socket_kernel_dispatch);
        switch (operacion) {
			case PCB:
                proceso_pcb = deserializar_proceso_pcb(socket_kernel_dispatch);
                pcb = proceso_pcb->pcb;
                pthread_mutex_lock(&mutex_logger);
                log_warning(logger_cpu, "Atendiendo nuevo proceso con PID %d ", pcb->pid);
                pthread_mutex_unlock(&mutex_logger);
                sem_post(&sem_ciclo_de_instruccion);
				break;
            case -1:
                pthread_mutex_lock(&mutex_logger);
                log_error(logger_cpu, "El puerto dispatch se desconecto. Terminando Hilo.");
                pthread_mutex_unlock(&mutex_logger);
                break;
			default:
                pthread_mutex_lock(&mutex_logger);
                log_warning(logger_cpu, "Operacion desconocida.");
                pthread_mutex_unlock(&mutex_logger);
				break;
		}
	}
}


void *ciclo_de_instruccion(void *arg) {
    while (true) {
        sem_wait(&sem_ciclo_de_instruccion);

        pthread_mutex_lock(&mutex_logger);
        log_info(logger_cpu,"Iniciando ciclo de instruccion");
        pthread_mutex_unlock(&mutex_logger);

        t_instruccion *instruccion = (t_instruccion *) queue_pop(pcb->consola->instrucciones); // FETCH

        operacion_a_enviar = UNDEFINED;
        //proceso_a_enviar = malloc(sizeof(t_proceso_pcb));
        uint32_t valor_a_copiar;

        if(necesita_fetch_operands(instruccion->instruc)) { // DECODE
        	valor_a_copiar = obtener_dato_memoria(instruccion->parametro2);
        }

        ejecutar_instruccion(instruccion, valor_a_copiar); // EXECUTE
        pcb->program_counter ++;
        free(instruccion);
        if(hay_que_desalojar_cpu()) {
            desalojar_cpu();
        } 
        /*else {
            free(proceso_a_enviar);
        }*/
    }
}

void *ejecutar_interrupcion(void *arg) {
    int cpu_interrupt = iniciar_servidor(config_cpu.ip_cpu, config_cpu.puerto_escucha_interrupt);
    socket_kernel_interrupt = esperar_cliente(cpu_interrupt);
    pthread_mutex_lock(&mutex_logger);
    log_info(logger_cpu,"Puerto de interrupcion conectado");
    pthread_mutex_unlock(&mutex_logger);
	while(true) {

		int operacion = recibir_operacion(socket_kernel_interrupt);

		if(operacion == INTERRUPCION) {
            pthread_mutex_lock(&mutex_logger);
            log_warning(logger_cpu,"Hubo una interrupcion");
            pthread_mutex_unlock(&mutex_logger);
            pthread_mutex_lock(&mutex_flag_interrupcion);
            hay_interrupcion =  recibir_interrupcion(socket_kernel_interrupt);
            pthread_mutex_unlock(&mutex_flag_interrupcion);
            sem_wait(&sem_interrupt);
            log_warning(logger_cpu,"Atendiendo Interrupcion");
			proceso_pcb->tiempo_bloqueo = UNDEFINED;
			operacion_a_enviar = PCB;
            sem_post(&sem_interrupt_fin);
		}
	}
}

int necesita_fetch_operands(instruccion instruction) {
	return instruction == COPY;
}

void ejecutar_instruccion(t_instruccion *instruccion, uint32_t valor_a_copiar) {
	int resultado;

    switch (instruccion->instruc) {
        case NO_OP:
            pthread_mutex_lock(&mutex_logger);
            log_info(logger_cpu,"CPU ejecutando NO_OP -> PID: %d", pcb->pid);
            pthread_mutex_unlock(&mutex_logger);
            resultado = usleep(config_cpu.retardo_noop * 1000);
            if(resultado == -1 ) {
                pthread_mutex_lock(&mutex_logger);
                log_error(logger_cpu, "Error al realizar usleep");
                pthread_mutex_unlock(&mutex_logger);
            }
            chequear_si_hay_interrupcion();
           break;

        case IO:
            pthread_mutex_lock(&mutex_logger);
            log_info(logger_cpu,"CPU ejecutando IO -> PID %d", pcb->pid);
            pthread_mutex_unlock(&mutex_logger);
        	proceso_pcb->tiempo_bloqueo = instruccion->parametro1;
        	operacion_a_enviar = BLOQUEO;
           break;

        case READ:
            pthread_mutex_lock(&mutex_logger);
            log_info(logger_cpu,"CPU ejecutando READ en la direccion: %d", instruccion->parametro1);
        	log_info(logger_cpu, "Valor leido de memoria: %i ",obtener_dato_memoria(instruccion->parametro1));
            pthread_mutex_unlock(&mutex_logger);
            chequear_si_hay_interrupcion();
           break;

        case WRITE:
            pthread_mutex_lock(&mutex_logger);
            log_info(logger_cpu,"CPU ejecutando WRITE en la direccion: %d", instruccion->parametro1);
            pthread_mutex_unlock(&mutex_logger);
            escribir_dato_memoria(instruccion->parametro1, instruccion->parametro2);
        	chequear_si_hay_interrupcion();
           break;

        case COPY:
            pthread_mutex_lock(&mutex_logger);
            log_info(logger_cpu,"CPU ejecutando COPY en la direccion: %d con el valor de la direccion %d ", instruccion->parametro1, instruccion->parametro2);
            pthread_mutex_unlock(&mutex_logger);
            escribir_dato_memoria(instruccion->parametro1, valor_a_copiar);
        	chequear_si_hay_interrupcion();
           break;

        case I_EXIT:
            pthread_mutex_lock(&mutex_logger);
            log_info(logger_cpu,"CPU ejecutando EXIT -> PID %d", pcb->pid);
            pthread_mutex_unlock(&mutex_logger);
            proceso_pcb->tiempo_bloqueo = UNDEFINED;
			operacion_a_enviar = FIN_PROCESO;
           break;
    }
}

void desalojar_cpu() {
		//proceso_a_enviar->pcb = pcb;
		enviar_proceso_pcb(socket_kernel_dispatch, proceso_pcb, operacion_a_enviar);
        queue_destroy_and_destroy_elements(pcb->consola->instrucciones, borrar_instruccion_consola);
        free(proceso_pcb->pcb->consola);
        free(proceso_pcb->pcb);
		free(proceso_pcb);
        limpiar_tlb();
        pthread_mutex_lock(&mutex_logger);
        log_warning(logger_cpu, "Desalojando PCB");
        pthread_mutex_unlock(&mutex_logger);
		sem_post(&sem_busqueda_proceso_nuevo);
}

bool hay_que_desalojar_cpu() {
	return operacion_a_enviar != UNDEFINED;
}

void chequear_si_hay_interrupcion() {

    pthread_mutex_lock(&mutex_flag_interrupcion);
	if(hay_interrupcion) {
        pthread_mutex_unlock(&mutex_flag_interrupcion);
		sem_post(&sem_interrupt);
        sem_wait(&sem_interrupt_fin);
	} else {
        pthread_mutex_unlock(&mutex_flag_interrupcion);
		sem_post(&sem_ciclo_de_instruccion);
	}
}

