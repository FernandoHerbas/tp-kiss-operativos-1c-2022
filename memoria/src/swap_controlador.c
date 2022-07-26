#include "../include/swap_controlador.h"

void realizar_page_fault(int32_t id_tabla_1n, int nro_pagina, void *a_leer) {
    log_warning(logger_memoria,"Solicitando pág a SWAP. ID Tabla: %d - Página nro: %d", id_tabla_1n, nro_pagina);
    usleep(config_memoria.retardo_swap*1000);
    t_particion* particion = encontrar_particion_de(id_tabla_1n);

    if (!particion){
        log_error(logger_memoria,"El proceso no tiene páginas en swap que pueda pedir.");
    }
    else{
        int nro_pag_en_swap = nro_pagina_en_swap(particion, nro_pagina);
        if(nro_pag_en_swap > -1) {
            int inicio_pag = nro_pag_en_swap * config_memoria.tamanio_pagina;

            int file;
            int mode = 0x0777;
            if ((file = open (particion->fcb->path_archivo, O_RDWR,mode)) < 0)
            {
            	imprimir_error_archivo(errno);
            	log_error(logger_memoria,"No se pudo abrir el archivo %s para leerlo.", particion->fcb->path_archivo);
                return;
            }

            char* ptro_archivo = (char*)mmap(0, config_memoria.tamanio_pagina, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

            if (ptro_archivo != 0){
                memcpy(a_leer,ptro_archivo + inicio_pag, config_memoria.tamanio_pagina);

                int cod = munmap(ptro_archivo, config_memoria.tamanio_pagina);
                if (cod != 0){ log_error(logger_memoria,"No se pudo 'desmapear' el archivo."); }

                int cerrado = close (file);
                if (cerrado != 0){ log_error(logger_memoria,"No se pudo cerrar el archivo."); }
            }
            return;
        }
        log_info(logger_memoria,"El proceso no tiene la pagina pedida en swap.");
    }
}

void escribir_pagina_en_swap(int32_t id_tabla_1n, int nro_pagina, void *a_escribir){
    log_warning(logger_memoria,"Escribiendo pág en SWAP. ID Tabla: %d - Página nro: %d", id_tabla_1n, nro_pagina);
    usleep(config_memoria.retardo_swap*1000);
    t_particion* particion = encontrar_particion_de(id_tabla_1n);

    if (!particion){
    	log_error(logger_memoria,"Error en la recuperación del archivo en swap para este proceso.");
    }

    int nro_pag_en_swap = nro_pagina_en_swap(particion, nro_pagina);
    if (nro_pag_en_swap == -1){                                            // Página nueva
		int nro_pagina_libre = obtener_nro_pagina_libre(particion);
		if (nro_pagina_libre > -1){
			asignar_pagina_a(particion, nro_pagina);
		}
		nro_pag_en_swap = nro_pagina_libre;
    }
	else{
		log_warning(logger_memoria,"El proceso alcanzó el máximo nro de páginas que puede pedir a SWAP");
	}
    int inicio_pag = nro_pag_en_swap * config_memoria.tamanio_pagina;
    int file;
    int mode = 0x0777;
    if ((file = open (particion->fcb->path_archivo, O_RDWR,mode)) < 0)//edited here
    {
        imprimir_error_archivo(errno);
        log_error(logger_memoria,"No se pudo abrir el archivo %s para escribir.", particion->fcb->path_archivo);
        return;
    }
    char* ptro_archivo = mmap(0, config_memoria.tamanio_pagina, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

    if (ptro_archivo != 0){

        memcpy(ptro_archivo + inicio_pag, a_escribir, config_memoria.tamanio_pagina);

        int cod = munmap(ptro_archivo, config_memoria.tamanio_pagina);
        if (cod != 0){ log_error(logger_memoria,"No se pudo 'desmapear' el archivo."); }

        int cerrado = close (file);
        if (cerrado != 0){ log_error(logger_memoria,"No se pudo cerrar el archivo."); }

        log_info(logger_memoria,"La escritura en swap se realizó con éxito.");
    }
}

void swapear_proceso(t_tabla_pagina *tabla_1n){
    int nro_pagina;
    for (int i = 0; i < dictionary_size(tabla_1n->tabla); ++i) {
        char *i_s = string_itoa(i);
        t_tabla_pagina *tabla_2n = dictionary_get(tabla_1n->tabla, i_s);
        for (int j = 0; j < dictionary_size(tabla_2n->tabla); ++j) {
            char* j_s = string_itoa(j);
            t_col_pagina *registro_pagina = dictionary_get(tabla_2n->tabla, j_s);
            free(j_s);
            if(registro_pagina->presencia){
                registro_pagina->presencia = false;
                t_frame *frame = list_get(memoria_principal->frames,registro_pagina->nro_frame);
                //El numero de pagina esta formado por: entrada_tabla_1n:entrada_tabla_2n
                nro_pagina = calcular_nro_pagina(i,j);
                escribir_pagina_en_swap(tabla_1n->id_tabla, nro_pagina, frame->base);
                frame->is_free = true;
            }
            registro_pagina->nro_frame = UNDEFINED;
        }
        free(i_s);
    }
    tabla_1n->fue_suspendido = true;
    list_clean_and_destroy_elements(tabla_1n->frames_asignados, liberar_frame_asignado);
}

void marcar_pag_ocupada(int pid, int nro_pagina_en_memoria){
	t_particion* particion = encontrar_particion_de(pid);
    int nro_pag_en_swap = nro_pagina_en_swap(particion, nro_pagina_en_memoria);

    t_pagina_swap* pagina_swap = list_get(particion->fcb->pags_en_archivo, nro_pag_en_swap);
    pagina_swap->id_memoria = nro_pagina_en_memoria;
    pagina_swap->is_free = 0;
}


/* ------ Auxiliares ------ */
t_particion* encontrar_particion_de(int tabla_1n){
    int cantidad_de_particiones = list_size(swap.particiones);
    for (int i = 0; i < cantidad_de_particiones; i++){
    	t_particion* particion_aux = (t_particion *)list_get(swap.particiones,i);
        int id_tabla_1n_aux = particion_aux->fcb->id_tabla;
        if (tabla_1n == id_tabla_1n_aux){
        	return list_get(swap.particiones,i);
        }
    }

    return NULL;
}

int obtener_indice_particion(int tabla_1n){
    int cantidad_de_particiones = list_size(swap.particiones);
    for (int i = 0; i < cantidad_de_particiones; i++){
        t_particion* particion_aux = (t_particion *)list_get(swap.particiones,i);
        int id_tabla_1n_aux = particion_aux->fcb->id_tabla;
        if (tabla_1n == id_tabla_1n_aux){
            return i;
        }
    }
    return UNDEFINED;
}


int obtener_nro_pagina_libre(t_particion* particion){
    for (int i = 0; i < list_size(particion->fcb->pags_en_archivo); i++){
    	t_pagina_swap* pagina_aux = list_get(particion->fcb->pags_en_archivo,i);
        if (pagina_aux->is_free){
            return i;
        }
    }
    return -1;
}

int nro_pagina_en_swap(t_particion *particion, int nro_pag_memoria){
	int pags_por_archivo = particion->tamanio / config_memoria.entradas_por_tabla;
	for (int i = 0; i < pags_por_archivo; i++){
        t_pagina_swap* pag_aux = list_get(particion->fcb->pags_en_archivo,i);
        if (pag_aux->id_memoria == nro_pag_memoria){
            return i;
        }
    }
    return -1;
}

void asignar_pagina_a(t_particion *particion, int nro_pagina){
    t_list* paginas = particion->fcb->pags_en_archivo;
    int i;
    for (i = 0; i < list_size(paginas); i++){
        t_pagina_swap* pag_aux = list_get(paginas,i);
        if (pag_aux->is_free){
            pag_aux->id_memoria = nro_pagina;
            pag_aux->is_free = 0;
            return;
        }
    }
}

int calcular_pags_libres(t_particion* particion){
    t_list* paginas = particion->fcb->pags_en_archivo;
    int paginas_libres = 0, i;
        for (i = 0; i < list_size(paginas); i++){
            t_pagina_swap* pag_aux = list_get(paginas,i);
            if (pag_aux->is_free){
                paginas_libres++;
            }
        }

    return paginas_libres;
}

void imprimir_error_archivo(int error){
	int errnum = errno;
	fprintf(stderr, "Value of errno: %d\n", errno);
	perror("Error printed by perror");
	fprintf(stderr, "Error opening file: %s\n", strerror( errnum ));
}
