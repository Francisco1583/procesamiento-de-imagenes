#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "selec_proc.h"
#include "selec_proc_1.h"

#define NUM_THREADS 4

int main(){
    // Variables tiempo
    double tiempo_inicio, tiempo_final;
    
    omp_set_num_threads(NUM_THREADS);
    
    FILE *fptr;
    char data[80] = "arc1.txt";
    fptr = fopen(data, "w");
    if (fptr != NULL){
        fprintf(fptr, "Ejemplo escribir\n");
        fprintf(fptr, "Emmanuel Torres Rios\n");
        fclose(fptr);
    }

    const char *imagenes_entrada[4] = {
        "./img/prueba1.bmp",
        "./img/prueba2.bmp",
        "./img/prueba3.bmp",
        "./img/prueba4.bmp"
    };

    const char *prefijos[4] = {
        "img1",
        "img2",
        "img3",
        "img4"
    };

    printf("Iniciando procesamiento secuencial de 4 imagenes...\n");

    
    tiempo_inicio = omp_get_wtime();

    for (int i = 0; i < 4; i++) {
        printf("\n--- Procesando imagen %d: %s ---\n", i + 1, imagenes_entrada[i]);

        char out_inv[50], out_espejo[50], out_inv_color[50];
        char out_espejo_color[50], out_desenfoque[50], out_desenfoque_color[50];

        sprintf(out_inv, "%s_inv_1", prefijos[i]);
        sprintf(out_espejo, "%s_espejo", prefijos[i]);
        sprintf(out_inv_color, "%s_inv_color_1", prefijos[i]);
        sprintf(out_espejo_color, "%s_espejo_color", prefijos[i]);
        sprintf(out_desenfoque, "%s_desenfoque", prefijos[i]);
        sprintf(out_desenfoque_color, "%s_desenfoque_color", prefijos[i]);

        #pragma omp parallel
        {
            #pragma omp sections
            {
                #pragma omp section
                inv_img(out_inv, imagenes_entrada[i]); 
                
                #pragma omp section
                inv_img_grey_horizontal(out_espejo, imagenes_entrada[i]); 
                
                #pragma omp section
                inv_img_color(out_inv_color, imagenes_entrada[i]); 

                #pragma omp section
                inv_img_color_horizontal(out_espejo_color, imagenes_entrada[i]); 
                
                #pragma omp section
                desenfoque(imagenes_entrada[i], out_desenfoque, 27); 

                #pragma omp section
                desenfoque_color(imagenes_entrada[i], out_desenfoque_color, 27);
            }
        }
        printf("Transformaciones de la imagen %d completadas.\n", i + 1);
    }

    
    tiempo_final = omp_get_wtime();

    printf("\n=============================================\n");
    printf("Procesamiento total finalizado.\n");
    printf("Tiempo total de ejecucion: %.4f segundos\n", tiempo_final - tiempo_inicio);
    printf("=============================================\n");

    return 0;
}
