#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "selec_proc.h"
#include "selec_proc_1.h"
#include <mpi.h>

#define NUM_THREADS 18
#define MAX_IMGS 150

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

// Estructura para almacenar los detalles temporales de cada tarea (para el log)
typedef struct {
    char nombre_img[128];
    char transformacion[32];
    long long pixeles;
    double tiempo;
    char archivo_salida[256];
} LogDetail;

// Estructura esperada de argumentos CLI enviados por la interfaz gráfica:
// ./main [ruta_salida] [kernel_gris] [kernel_color] [f1] [f2] [f3] [f4] [f5] [f6] [img1] [img2] ... [img10]
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

    omp_set_num_threads(NUM_THREADS);

    // Variables maestras para el registro de logs
    LogDetail detalles[MAX_IMGS * 6]; // Capacidad para 6 transformaciones por imagen
    int num_detalles = 0;
    long long pixeles_totales = 0;
    
    // Obtener nombre del host
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Sincronización inicial y toma de tiempo global
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_inicio = MPI_Wtime();

    // Distribución de carga: Cada nodo procesa (i % num_procs == my_rank)
    for (int i = my_rank; i < num_imgs; i += num_procs) {
        const char *ruta_completa = argv[10 + i];
        unsigned char *header;
        unsigned char *pixels;
        int offset, ancho, alto;

        // Extraer el nombre base para reporte y logs
        char nombre_base[128];
        const char *slash = strrchr(ruta_completa, '/');
        const char *inicio_nombre = (slash != NULL) ? slash + 1 : ruta_completa;
        strncpy(nombre_base, inicio_nombre, 127);
        nombre_base[127] = '\0';
        char *punto = strrchr(nombre_base, '.');
        if (punto != NULL) *punto = '\0';

        // Imprimir para que lo lea la consola de Python
        printf("[Rank %d en %s] Procesando la imagen: %s\n", my_rank, processor_name, nombre_base);
        fflush(stdout);

        cargar_imagen(ruta_completa, &header, &offset, &pixels, &ancho, &alto);
        long long pixeles_imagen = (long long)ancho * alto;

        #pragma omp parallel
        {
            #pragma omp single
            {
                if (f1) {
                    #pragma omp task
                    {
                        double t_ini = omp_get_wtime();
                        char temp_path[512], out_path[512], cmd[1024];
                        snprintf(temp_path, sizeof(temp_path), "/tmp/%s_VG_%d.bmp", nombre_base, my_rank);
                        snprintf(out_path, sizeof(out_path), "%s/%s_VG.bmp", ruta_salida, nombre_base);
                        
                        // Escribe a velocidad de RAM/Disco local sin tocar la red
                        gray_img(temp_path, header, offset, pixels, ancho, alto);
                        double t_fin = omp_get_wtime();

                        #pragma omp critical
                        {
                            // AQUI ESTA LA CORRECCIÓN DE LAS COMILLAS
                            snprintf(cmd, sizeof(cmd), "mv \"%s\" \"%s\"", temp_path, out_path);
                            system(cmd);

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
                        char temp_path[512], out_path[512], cmd[1024];
                        snprintf(temp_path, sizeof(temp_path), "/tmp/%s_VC_%d.bmp", nombre_base, my_rank);
                        snprintf(out_path, sizeof(out_path), "%s/%s_VC.bmp", ruta_salida, nombre_base);
                        
                        inv_img_color(temp_path, header, offset, pixels, ancho, alto);
                        double t_fin = omp_get_wtime();

                        #pragma omp critical
                        {
                            snprintf(cmd, sizeof(cmd), "mv \"%s\" \"%s\"", temp_path, out_path);
                            system(cmd);

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
                        char temp_path[512], out_path[512], cmd[1024];
                        snprintf(temp_path, sizeof(temp_path), "/tmp/%s_HG_%d.bmp", nombre_base, my_rank);
                        snprintf(out_path, sizeof(out_path), "%s/%s_HG.bmp", ruta_salida, nombre_base);
                        
                        inv_img_grey_horizontal(temp_path, header, offset, pixels, ancho, alto);
                        double t_fin = omp_get_wtime();

                        #pragma omp critical
                        {
                            snprintf(cmd, sizeof(cmd), "mv \"%s\" \"%s\"", temp_path, out_path);
                            system(cmd);

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
                        char temp_path[512], out_path[512], cmd[1024];
                        snprintf(temp_path, sizeof(temp_path), "/tmp/%s_HC_%d.bmp", nombre_base, my_rank);
                        snprintf(out_path, sizeof(out_path), "%s/%s_HC.bmp", ruta_salida, nombre_base);
                        
                        inv_img_color_horizontal(temp_path, header, offset, pixels, ancho, alto);
                        double t_fin = omp_get_wtime();

                        #pragma omp critical
                        {
                            snprintf(cmd, sizeof(cmd), "mv \"%s\" \"%s\"", temp_path, out_path);
                            system(cmd);

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
                        char temp_path[512], out_path[512], cmd[1024];
                        snprintf(temp_path, sizeof(temp_path), "/tmp/%s_DG_%d.bmp", nombre_base, my_rank);
                        snprintf(out_path, sizeof(out_path), "%s/%s_DG.bmp", ruta_salida, nombre_base);
                        
                        desenfoque(temp_path, header, offset, pixels, ancho, alto, k_gris);
                        double t_fin = omp_get_wtime();

                        #pragma omp critical
                        {
                            snprintf(cmd, sizeof(cmd), "mv \"%s\" \"%s\"", temp_path, out_path);
                            system(cmd);

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
                        char temp_path[512], out_path[512], cmd[1024];
                        snprintf(temp_path, sizeof(temp_path), "/tmp/%s_DC_%d.bmp", nombre_base, my_rank);
                        snprintf(out_path, sizeof(out_path), "%s/%s_DC.bmp", ruta_salida, nombre_base);
                        
                        desenfoque_color(temp_path, header, offset, pixels, ancho, alto, k_color);
                        double t_fin = omp_get_wtime();

                        #pragma omp critical
                        {
                            snprintf(cmd, sizeof(cmd), "mv \"%s\" \"%s\"", temp_path, out_path);
                            system(cmd);

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

    // Esperar a que todos terminen
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Cálculo de métricas globales del rango
    double tiempo_final = MPI_Wtime();
    double tiempo_efectivo = tiempo_final - tiempo_inicio;
    double pixeles_por_segundo = (tiempo_efectivo > 0) ? ((double)pixeles_totales / tiempo_efectivo) : 0;

    // ====================================================================
    // ESCRITURA DEL ARCHIVO LOG
    // ====================================================================
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
        fprintf(log_file, "Pixeles/segundo:        %.3e\n\n", pixeles_por_segundo); // Formato Científico
        
        fprintf(log_file, "--- Detalle por imagen ---\n");
        fprintf(log_file, "%-15s %-20s %-15s %-10s %s\n", "Imagen", "Transform.", "Pixeles", "Tiempo(s)", "Archivo salida");
        fprintf(log_file, "------------------------------------------------------------------------------------------------\n");
        
        for(int k = 0; k < num_detalles; k++) {
            // Extraer solo el nombre del archivo final para la tabla
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
    // ====================================================================

    // Solo el Master imprime la salida final para la GUI de Python
    if (my_rank == 0) {
        printf("TIEMPO_TOTAL:%.4f\n", tiempo_efectivo);
    }

    MPI_Finalize();
    return 0;
}