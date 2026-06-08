#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "selec_proc.h"
#include "selec_proc_1.h"

#define MAX_IMGS 600

// Estructura para almacenar los detalles temporales de cada tarea (para el log)
typedef struct {
    char nombre_img[128];
    char transformacion[32];
    long long pixeles;
    double tiempo;
    char archivo_salida[256];
} LogDetail;

// Función auxiliar para leer la imagen en RAM una sola vez con validaciones
void cargar_imagen(const char* path, unsigned char** header, int* offset, unsigned char** pixels, int* ancho, int* alto) {
    FILE *in = fopen(path, "rb");
    if (!in) {
        printf("ERROR_LECTURA\n");
        exit(1);
    }

    unsigned char fileHeader[14];
    if (fread(fileHeader, 1, 14, in) != 14) exit(1);
    *offset = *(int*)&fileHeader[10];

    *header = (unsigned char*)malloc(*offset);
    memcpy(*header, fileHeader, 14);
    if (fread(*header + 14, 1, *offset - 14, in) != (size_t)(*offset - 14)) exit(1);

    *ancho = *(int*)&((*header)[18]);
    *alto = *(int*)&((*header)[22]);

    int row_padded = ((*ancho) * 3 + 3) & (~3);
    int pixel_data_size = row_padded * (*alto);

    *pixels = (unsigned char*)malloc(pixel_data_size);
    if (fread(*pixels, 1, pixel_data_size, in) != (size_t)pixel_data_size) exit(1);

    fclose(in);
}

int main(int argc, char *argv[]) {
    int my_rank, num_procs;
    
    // Inicialización de MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    if (argc < 11) {
        if (my_rank == 0) printf("ERROR_ARGUMENTOS\n");
        MPI_Finalize();
        return 1;
    }

    char *ruta_salida = argv[1];
    int k_gris        = atoi(argv[2]);
    int k_color       = atoi(argv[3]);
    int f1 = atoi(argv[4]), f2 = atoi(argv[5]), f3 = atoi(argv[6]);
    int f4 = atoi(argv[7]), f5 = atoi(argv[8]), f6 = atoi(argv[9]);

    int num_imgs = argc - 10;
    if (num_imgs > MAX_IMGS) num_imgs = MAX_IMGS;

    // Variables maestras para el registro de logs
    LogDetail detalles[MAX_IMGS * 6]; 
    int num_detalles = 0;
    long long pixeles_totales = 0;
    
    // Obtener nombre del host para la trazabilidad
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Sincronización inicial y toma de tiempo global
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_inicio = MPI_Wtime();

    // Etiquetas para el paso de mensajes (Master-Worker)
    int WORK_TAG = 1;
    int DATA_TAG = 2;
    int END_TAG = 3;

    if (my_rank == 0) {
        // =========================================================
        // NODO 0: EL DESPACHADOR (JEFE) - Solo reparte trabajo
        // =========================================================
        int imagenes_despachadas = 0;
        int trabajadores_activos = num_procs - 1;

        while (trabajadores_activos > 0) {
            int peticion;
            MPI_Status status;
            
            // Escucha a cualquier trabajador que esté libre y pida trabajo
            MPI_Recv(&peticion, 1, MPI_INT, MPI_ANY_SOURCE, WORK_TAG, MPI_COMM_WORLD, &status);
            int worker_id = status.MPI_SOURCE;

            if (imagenes_despachadas < num_imgs) {
                // Hay trabajo: Enviar el índice de la siguiente imagen
                MPI_Send(&imagenes_despachadas, 1, MPI_INT, worker_id, DATA_TAG, MPI_COMM_WORLD);
                imagenes_despachadas++;
            } else {
                // No hay trabajo: Enviar señal de terminación (-1)
                int fin = -1;
                MPI_Send(&fin, 1, MPI_INT, worker_id, END_TAG, MPI_COMM_WORLD);
                trabajadores_activos--;
            }
        }
    } else {
        // =========================================================
        // NODOS > 0: TRABAJADORES - Piden y procesan imágenes
        // =========================================================
        while (1) {
            int peticion = 1;
            
            // Avisar al Master que estoy libre
            MPI_Send(&peticion, 1, MPI_INT, 0, WORK_TAG, MPI_COMM_WORLD);

            int img_idx;
            MPI_Status status;
            
            // Recibir la imagen asignada
            MPI_Recv(&img_idx, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == END_TAG) {
                break; // Ya no hay imágenes, el trabajador puede descansar
            }

            // --- PROCESAMIENTO DE LA IMAGEN ASIGNADA ---
            const char *ruta_completa = argv[10 + img_idx];
            unsigned char *header;
            unsigned char *pixels;
            int offset, ancho, alto;

            // Extraer el nombre base
            char nombre_base[128];
            const char *slash = strrchr(ruta_completa, '/');
            const char *inicio_nombre = (slash != NULL) ? slash + 1 : ruta_completa;
            strncpy(nombre_base, inicio_nombre, 127);
            nombre_base[127] = '\0';
            char *punto = strrchr(nombre_base, '.');
            if (punto != NULL) *punto = '\0';

            printf("[Rank %d en %s] Procesando la imagen: %s\n", my_rank, processor_name, nombre_base);
            fflush(stdout);

            cargar_imagen(ruta_completa, &header, &offset, &pixels, &ancho, &alto);
            long long pixeles_imagen = (long long)ancho * alto;

            // OpenMP usará automáticamente todos los núcleos físicos de la laptop donde esté corriendo
            #pragma omp parallel
            {
                #pragma omp single
                {
                    if (f1) {
                        #pragma omp task
                        {
                            double t_ini = omp_get_wtime();
                            char out_path[512];
                            snprintf(out_path, sizeof(out_path), "%s/%s_VG.bmp", ruta_salida, nombre_base);
                            gray_img(out_path, header, offset, pixels, ancho, alto);
                            double t_fin = omp_get_wtime();

                            #pragma omp critical
                            {
                                strncpy(detalles[num_detalles].nombre_img, nombre_base, 127);
                                strcpy(detalles[num_detalles].transformacion, "gris_vertical");
                                detalles[num_detalles].pixeles = pixeles_imagen;
                                detalles[num_detalles].tiempo = t_fin - t_ini;
                                strcpy(detalles[num_detalles].archivo_salida, out_path);
                                pixeles_totales += pixeles_imagen;
                                num_detalles++;
                            }
                        }
                    }
                    if (f2) {
                        #pragma omp task
                        {
                            double t_ini = omp_get_wtime();
                            char out_path[512];
                            snprintf(out_path, sizeof(out_path), "%s/%s_VC.bmp", ruta_salida, nombre_base);
                            inv_img_color(out_path, header, offset, pixels, ancho, alto);
                            double t_fin = omp_get_wtime();

                            #pragma omp critical
                            {
                                strncpy(detalles[num_detalles].nombre_img, nombre_base, 127);
                                strcpy(detalles[num_detalles].transformacion, "color_vertical");
                                detalles[num_detalles].pixeles = pixeles_imagen;
                                detalles[num_detalles].tiempo = t_fin - t_ini;
                                strcpy(detalles[num_detalles].archivo_salida, out_path);
                                pixeles_totales += pixeles_imagen;
                                num_detalles++;
                            }
                        }
                    }
                    if (f3) {
                        #pragma omp task
                        {
                            double t_ini = omp_get_wtime();
                            char out_path[512];
                            snprintf(out_path, sizeof(out_path), "%s/%s_HG.bmp", ruta_salida, nombre_base);
                            inv_img_grey_horizontal(out_path, header, offset, pixels, ancho, alto);
                            double t_fin = omp_get_wtime();

                            #pragma omp critical
                            {
                                strncpy(detalles[num_detalles].nombre_img, nombre_base, 127);
                                strcpy(detalles[num_detalles].transformacion, "gris_horizontal");
                                detalles[num_detalles].pixeles = pixeles_imagen;
                                detalles[num_detalles].tiempo = t_fin - t_ini;
                                strcpy(detalles[num_detalles].archivo_salida, out_path);
                                pixeles_totales += pixeles_imagen;
                                num_detalles++;
                            }
                        }
                    }
                    if (f4) {
                        #pragma omp task
                        {
                            double t_ini = omp_get_wtime();
                            char out_path[512];
                            snprintf(out_path, sizeof(out_path), "%s/%s_HC.bmp", ruta_salida, nombre_base);
                            inv_img_color_horizontal(out_path, header, offset, pixels, ancho, alto);
                            double t_fin = omp_get_wtime();

                            #pragma omp critical
                            {
                                strncpy(detalles[num_detalles].nombre_img, nombre_base, 127);
                                strcpy(detalles[num_detalles].transformacion, "color_horizontal");
                                detalles[num_detalles].pixeles = pixeles_imagen;
                                detalles[num_detalles].tiempo = t_fin - t_ini;
                                strcpy(detalles[num_detalles].archivo_salida, out_path);
                                pixeles_totales += pixeles_imagen;
                                num_detalles++;
                            }
                        }
                    }
                    if (f5) {
                        #pragma omp task
                        {
                            double t_ini = omp_get_wtime();
                            char out_path[512];
                            snprintf(out_path, sizeof(out_path), "%s/%s_DG.bmp", ruta_salida, nombre_base);
                            desenfoque(out_path, header, offset, pixels, ancho, alto, k_gris);
                            double t_fin = omp_get_wtime();

                            #pragma omp critical
                            {
                                strncpy(detalles[num_detalles].nombre_img, nombre_base, 127);
                                strcpy(detalles[num_detalles].transformacion, "desenfoque_gris");
                                detalles[num_detalles].pixeles = pixeles_imagen;
                                detalles[num_detalles].tiempo = t_fin - t_ini;
                                strcpy(detalles[num_detalles].archivo_salida, out_path);
                                pixeles_totales += pixeles_imagen;
                                num_detalles++;
                            }
                        }
                    }
                    if (f6) {
                        #pragma omp task
                        {
                            double t_ini = omp_get_wtime();
                            char out_path[512];
                            snprintf(out_path, sizeof(out_path), "%s/%s_DC.bmp", ruta_salida, nombre_base);
                            desenfoque_color(out_path, header, offset, pixels, ancho, alto, k_color);
                            double t_fin = omp_get_wtime();

                            #pragma omp critical
                            {
                                strncpy(detalles[num_detalles].nombre_img, nombre_base, 127);
                                strcpy(detalles[num_detalles].transformacion, "desenfoque_color");
                                detalles[num_detalles].pixeles = pixeles_imagen;
                                detalles[num_detalles].tiempo = t_fin - t_ini;
                                strcpy(detalles[num_detalles].archivo_salida, out_path);
                                pixeles_totales += pixeles_imagen;
                                num_detalles++;
                            }
                        }
                    }
                }
            }
            free(header);
            free(pixels);
        }
    }

    // Esperar a que todos terminen para las métricas finales
    MPI_Barrier(MPI_COMM_WORLD);
    
    double tiempo_final = MPI_Wtime();
    double tiempo_efectivo = tiempo_final - tiempo_inicio;
    double pixeles_por_segundo = (tiempo_efectivo > 0) ? ((double)pixeles_totales / tiempo_efectivo) : 0;

    // Solo los trabajadores (Rank > 0) que procesaron algo escriben un archivo LOG
    if (my_rank > 0 && num_detalles > 0) {
        char log_filename[256];
        snprintf(log_filename, sizeof(log_filename), "%s/rank_%d.log", ruta_salida, my_rank);
        
        FILE *log_file = fopen(log_filename, "w");
        if (log_file != NULL) {
            fprintf(log_file, "====== MPI RANK %d | HOST: %s ======\n", my_rank, processor_name);
            fprintf(log_file, "Procesos MPI totales:   %d\n", num_procs);
            fprintf(log_file, "Threads OpenMP:         %d\n", omp_get_max_threads());
            fprintf(log_file, "Tareas ejecutadas:      %d\n", num_detalles);
            fprintf(log_file, "Pixeles procesados:     %lld\n", pixeles_totales);
            fprintf(log_file, "Tiempo efectivo:        %f s\n", tiempo_efectivo);
            fprintf(log_file, "Pixeles/segundo:        %.3e\n\n", pixeles_por_segundo); 
            
            fprintf(log_file, "--- Detalle por imagen ---\n");
            fprintf(log_file, "%-15s %-20s %-15s %-10s %s\n", "Imagen", "Transform.", "Pixeles", "Tiempo(s)", "Archivo salida");
            fprintf(log_file, "------------------------------------------------------------------------------------------------\n");
            
            for(int k = 0; k < num_detalles; k++) {
                char *nombre_archivo = strrchr(detalles[k].archivo_salida, '/');
                nombre_archivo = (nombre_archivo != NULL) ? nombre_archivo + 1 : detalles[k].archivo_salida;

                fprintf(log_file, "%-15.15s %-20s %-15lld %-10.4f %s\n",
                        detalles[k].nombre_img,
                        detalles[k].transformacion,
                        detalles[k].pixeles,
                        detalles[k].tiempo,
                        nombre_archivo);
            }
            fclose(log_file);
        }
    }

    // El Jefe (Rank 0) imprime el tiempo total para que la Interfaz de Python lo capture
    if (my_rank == 0) {
        printf("TIEMPO_TOTAL:%.4f\n", tiempo_efectivo);
    }

    MPI_Finalize();
    return 0;
}