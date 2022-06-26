#include "../../include/utils/utilslib.h"

void mostrar_error(char *exmsj, t_log *logger) {
	log_error(logger, exmsj);
}

/*********************************************/
void enviar_operacion(t_operacion *operacion, int socket_cliente) {
	int bytes;
	void* a_enviar;
	bytes = operacion->buffer->size + 2*sizeof(int);
	a_enviar = serializar_operacion(operacion, bytes);
	send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
}

void *serializar_operacion(t_operacion *operacion, int bytes) {
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(operacion->cod_op), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(operacion->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, operacion->buffer->stream, operacion->buffer->size);

	return magic;
}

void setear_operacion(t_operacion *operacion, void *valor) {
    codigo_operacion op_code = operacion->cod_op;
    int desplazamiento = 0, size;
    t_consola *consola;
    void *stream;

    switch (op_code) {
		case T_CONSOLA:
            stream = serializar_consola((t_consola *)valor, &size);
            operacion->buffer->size = size;
            operacion->buffer->stream = stream;
            break;
        case PCB:
		case BLOQUEO:
		case FIN_PROCESO:
			serializar_proceso_pcb((t_proceso_pcb *)valor,operacion);
			break;
		case INTERRUPCION:
			size = sizeof(int);
			stream = malloc(size);
			memcpy(stream, valor, size);
			operacion->buffer->size = size;
            operacion->buffer->stream = stream;
			break;
        case PRIMERA_SOLICITUD:
        case SEGUNDA_SOLICITUD:
            stream = serializar_solicitud((t_solicitud *)valor, &size);
            operacion->buffer->size = size;
            operacion->buffer->stream = stream;
            break;
        case TERCERA_SOLICITUD:
            stream = serializar_tercera_solicitud((t_tercera_solicitud *)valor, &size);
            operacion->buffer->size = size;
            operacion->buffer->stream = stream;
            break;
        case INICIO_PROCESO:
            stream = malloc(sizeof(uint32_t));
            memcpy(stream, valor, sizeof(uint32_t));
            operacion->buffer->size = sizeof(uint32_t);
            operacion->buffer->stream = stream;
    }
    return;
}

t_operacion *crear_operacion(codigo_operacion cod_op) {
	t_operacion *operacion = malloc(sizeof(t_operacion));
    operacion->cod_op = cod_op;
    crear_buffer_operacion(operacion);
	return operacion;
}

void crear_buffer_operacion(t_operacion *operacion) {
    operacion->buffer = malloc(sizeof(t_buffer));
    operacion->buffer->size = 0;
    operacion->buffer->stream = NULL;
}

/*********************************************/
//-----------------Liberaciones de memoria---------------------/

void eliminar_operacion(t_operacion *operacion) {
	free(operacion->buffer->stream);
	free(operacion->buffer);
	free(operacion);
}


