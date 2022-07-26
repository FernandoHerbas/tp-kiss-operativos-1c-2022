#include "../include/memoria_controlador.h"

t_frame *recorrer_frames() {
    int i;
    t_frame *frame = NULL;
    pthread_mutex_lock(&mutex_mp);
    for(i=0;i<memoria_principal->cantidad_frames;i++) {
		frame = (t_frame*)list_get(memoria_principal->frames, i);
		if(frame->is_free) {
            pthread_mutex_unlock(&mutex_mp);
            return frame;
		}
	}
    pthread_mutex_unlock(&mutex_mp);
    return NULL;
}

t_frame *realizar_algoritmo_reemplazo(t_tabla_pagina *tabla_pagina, t_col_pagina *pagina,
                                      accion accion, int32_t entrada_tabla_1n, int32_t entrada_tabla_2n) {
    t_frame *frame = NULL;

    switch (tipo_algoritmo) {
    case CLOCK:
            sem_wait(&sem_algoritmo);
            frame = realizar_algoritmo_clock(tabla_pagina, pagina, accion, entrada_tabla_1n, entrada_tabla_2n);
            sem_post(&sem_algoritmo);
        break;
    case CLOCKM:
            sem_wait(&sem_algoritmo);
            frame = realizar_algoritmo_clock_mejorado(tabla_pagina, pagina, accion, entrada_tabla_1n, entrada_tabla_2n);
            sem_post(&sem_algoritmo);
        break;
    }
    return frame;
}

void liberar_frame_asignado(void *frame_asignado) {
    free(frame_asignado);
}
