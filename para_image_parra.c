#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "selec_proc.h"
#include "selec_proc_1.h"
#include <mpi.h>

#define NUM_THREADS 18
#define MAX_IMGS 10

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
    
    // Sincronización inicial y toma de tiempo
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_inicio = MPI_Wtime();

    // Distribución de carga: Cada nodo procesa (i % num_procs == my_rank)
    for (int i = my_rank; i < num_imgs; i += num_procs) {
        const char *ruta_completa = argv[10 + i];
        unsigned char *header;
        unsigned char *pixels;
        int offset, ancho, alto;

        // Solo el nodo asignado carga la imagen a su RAM
        cargar_imagen(ruta_completa, &header, &offset, &pixels, &ancho, &alto);

        char nombre_base[128];
        const char *slash = strrchr(ruta_completa, '/');
        const char *inicio_nombre = (slash != NULL) ? slash + 1 : ruta_completa;
        strncpy(nombre_base, inicio_nombre, 127);
        nombre_base[127] = '\0';
        char *punto = strrchr(nombre_base, '.');
        if (punto != NULL) *punto = '\0';

        // Procesamiento local con OpenMP
        #pragma omp parallel
        {
            #pragma omp single
            {
                if (f1) {
                    #pragma omp task
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_VG.bmp", ruta_salida, nombre_base);
                        gray_img(out_path, header, offset, pixels, ancho, alto);
                    }
                }
                if (f2) {
                    #pragma omp task
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_VC.bmp", ruta_salida, nombre_base);
                        inv_img_color(out_path, header, offset, pixels, ancho, alto);
                    }
                }
                if (f3) {
                    #pragma omp task
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_HG.bmp", ruta_salida, nombre_base);
                        inv_img_grey_horizontal(out_path, header, offset, pixels, ancho, alto);
                    }
                }
                if (f4) {
                    #pragma omp task
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_HC.bmp", ruta_salida, nombre_base);
                        inv_img_color_horizontal(out_path, header, offset, pixels, ancho, alto);
                    }
                }
                if (f5) {
                    #pragma omp task
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_DG.bmp", ruta_salida, nombre_base);
                        desenfoque(out_path, header, offset, pixels, ancho, alto, k_gris);
                    }
                }
                if (f6) {
                    #pragma omp task
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_DC.bmp", ruta_salida, nombre_base);
                        desenfoque_color(out_path, header, offset, pixels, ancho, alto, k_color);
                    }
                }
            }
        }
        free(header);
        free(pixels);
    }

    // Esperar a que todos terminen para calcular el tiempo total
    MPI_Barrier(MPI_COMM_WORLD);
    double tiempo_final = MPI_Wtime();

    // Solo el Master imprime la salida para que la GUI la intercepte
    if (my_rank == 0) {
        printf("TIEMPO_TOTAL:%.4f\n", tiempo_final - tiempo_inicio);
    }

    MPI_Finalize();
    return 0;
}