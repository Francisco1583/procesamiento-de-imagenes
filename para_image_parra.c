#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "selec_proc.h"
#include "selec_proc_1.h"

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
    // Archivo de registro silencioso
    FILE *fptr = fopen("arc1.txt", "w");
    if (fptr != NULL){
        fprintf(fptr, "Ejemplo escribir\n");
        fprintf(fptr, "Emmanuel Torres Rios\n");
        fclose(fptr);
    }

    // Validación mínima de argumentos (1 ejecutable + 1 ruta + 2 kernels + 6 flags + mínimo 1 imagen = 11)
    if (argc < 11) {
        printf("ERROR_ARGUMENTOS\n");
        return 1;
    }

    // Parseo de los argumentos de configuración
    char *ruta_salida = argv[1];
    int k_gris        = atoi(argv[2]);
    int k_color       = atoi(argv[3]);
    
    // Banderas booleanas (1 = ejecutar, 0 = omitir)
    int f1 = atoi(argv[4]); // 1- Vertical escala de grises
    int f2 = atoi(argv[5]); // 2- Vertical escala a colores
    int f3 = atoi(argv[6]); // 3- Horizontal escala de grises
    int f4 = atoi(argv[7]); // 4- Horizontal escala a colores
    int f5 = atoi(argv[8]); // 5- Desenfoque escala de grises
    int f6 = atoi(argv[9]); // 6- Desenfoque escala a colores

    // Parseo dinámico de imágenes
    int num_imgs = argc - 10;
    if (num_imgs > MAX_IMGS) num_imgs = MAX_IMGS; // Tope de seguridad

    // Arreglos para almacenar los datos de hasta 10 imágenes en RAM
    unsigned char *headers[MAX_IMGS];
    unsigned char *pixels[MAX_IMGS];
    int offsets[MAX_IMGS], anchos[MAX_IMGS], altos[MAX_IMGS];
    
    // Arreglo para guardar el nombre original de la imagen (sin ruta y sin .bmp)
    char nombres_base[MAX_IMGS][128];

    // Cargar todas las imágenes en RAM secuencialmente y extraer su nombre original
    for (int i = 0; i < num_imgs; i++) {
        const char *ruta_completa = argv[10 + i];
        
        // 1. Cargar datos a la RAM
        cargar_imagen(ruta_completa, &headers[i], &offsets[i], &pixels[i], &anchos[i], &altos[i]);
        
        // 2. Extraer el nombre base (limpiar diagonales dependiendo si es Linux o Windows)
        const char *slash = strrchr(ruta_completa, '/');
        const char *backslash = strrchr(ruta_completa, '\\');
        const char *inicio_nombre = ruta_completa;
        
        if (slash != NULL) inicio_nombre = slash + 1;
        if (backslash != NULL && backslash > slash) inicio_nombre = backslash + 1;
        
        strncpy(nombres_base[i], inicio_nombre, 127);
        nombres_base[i][127] = '\0'; // Asegurar fin de cadena
        
        // 3. Quitar el ".bmp"
        char *punto = strrchr(nombres_base[i], '.');
        if (punto != NULL) *punto = '\0';
    }

    omp_set_num_threads(NUM_THREADS);
    double tiempo_inicio = omp_get_wtime();

    // Procesamiento paralelo con validación de banderas
    #pragma omp parallel
    {
        #pragma omp single
        {
            for (int i = 0; i < num_imgs; i++) {
                
                if (f1) {
                    #pragma omp task firstprivate(i)
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_VG.bmp", ruta_salida, nombres_base[i]);
                        gray_img(out_path, headers[i], offsets[i], pixels[i], anchos[i], altos[i]);
                    }
                }

                if (f2) {
                    #pragma omp task firstprivate(i)
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_VC.bmp", ruta_salida, nombres_base[i]);
                        inv_img_color(out_path, headers[i], offsets[i], pixels[i], anchos[i], altos[i]);
                    }
                }

                if (f3) {
                    #pragma omp task firstprivate(i)
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_HG.bmp", ruta_salida, nombres_base[i]);
                        inv_img_grey_horizontal(out_path, headers[i], offsets[i], pixels[i], anchos[i], altos[i]);
                    }
                }

                if (f4) {
                    #pragma omp task firstprivate(i)
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_HC.bmp", ruta_salida, nombres_base[i]);
                        inv_img_color_horizontal(out_path, headers[i], offsets[i], pixels[i], anchos[i], altos[i]);
                    }
                }

                if (f5) {
                    #pragma omp task firstprivate(i)
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_DG.bmp", ruta_salida, nombres_base[i]);
                        desenfoque(out_path, headers[i], offsets[i], pixels[i], anchos[i], altos[i], k_gris);
                    }
                }

                if (f6) {
                    #pragma omp task firstprivate(i)
                    {
                        char out_path[512];
                        snprintf(out_path, sizeof(out_path), "%s/%s_DC.bmp", ruta_salida, nombres_base[i]);
                        desenfoque_color(out_path, headers[i], offsets[i], pixels[i], anchos[i], altos[i], k_color);
                    }
                }
            }
        }
    }

    double tiempo_final = omp_get_wtime();

    // Liberar memoria
    for (int i = 0; i < num_imgs; i++) {
        free(headers[i]);
        free(pixels[i]);
    }

    // Única salida de texto que leerá la interfaz gráfica
    printf("TIEMPO_TOTAL:%.4f\n", tiempo_final - tiempo_inicio);

    return 0;
}
