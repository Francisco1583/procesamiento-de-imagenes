#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "selec_proc.h"
#include "selec_proc_1.h"

#define NUM_THREADS 18

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

    printf("Iniciando procesamiento paralelo de 4 imagenes...\n");

    tiempo_inicio = omp_get_wtime();


    #pragma omp parallel
    {
        #pragma omp sections
        {
            // ================= IMAGEN 1 =================
            #pragma omp section
            inv_img("img1_inv_1", "./img/prueba1.bmp"); 
            
            #pragma omp section
            inv_img_grey_horizontal("img1_espejo", "./img/prueba1.bmp"); 
            
            #pragma omp section
            inv_img_color("img1_inv_color_1", "./img/prueba1.bmp"); 

            #pragma omp section
            inv_img_color_horizontal("img1_espejo_color", "./img/prueba1.bmp"); 
            
            #pragma omp section
            desenfoque("./img/prueba1.bmp", "img1_desenfoque", 27); 

            #pragma omp section
            desenfoque_color("./img/prueba1.bmp", "img1_desenfoque_color", 27);

            // ================= IMAGEN 2 =================
            #pragma omp section
            inv_img("img2_inv_1", "./img/prueba2.bmp"); 
            
            #pragma omp section
            inv_img_grey_horizontal("img2_espejo", "./img/prueba2.bmp"); 
            
            #pragma omp section
            inv_img_color("img2_inv_color_1", "./img/prueba2.bmp"); 

            #pragma omp section
            inv_img_color_horizontal("img2_espejo_color", "./img/prueba2.bmp"); 
            
            #pragma omp section
            desenfoque("./img/prueba2.bmp", "img2_desenfoque", 27); 

            #pragma omp section
            desenfoque_color("./img/prueba2.bmp", "img2_desenfoque_color", 27);

            // ================= IMAGEN 3 =================
            #pragma omp section
            inv_img("img3_inv_1", "./img/prueba3.bmp"); 
            
            #pragma omp section
            inv_img_grey_horizontal("img3_espejo", "./img/prueba3.bmp"); 
            
            #pragma omp section
            inv_img_color("img3_inv_color_1", "./img/prueba3.bmp"); 

            #pragma omp section
            inv_img_color_horizontal("img3_espejo_color", "./img/prueba3.bmp"); 
            
            #pragma omp section
            desenfoque("./img/prueba3.bmp", "img3_desenfoque", 27); 

            #pragma omp section
            desenfoque_color("./img/prueba3.bmp", "img3_desenfoque_color", 27);

            // ================= IMAGEN 4 =================
            #pragma omp section
            inv_img("img4_inv_1", "./img/prueba4.bmp"); 
            
            #pragma omp section
            inv_img_grey_horizontal("img4_espejo", "./img/prueba4.bmp"); 
            
            #pragma omp section
            inv_img_color("img4_inv_color_1", "./img/prueba4.bmp"); 

            #pragma omp section
            inv_img_color_horizontal("img4_espejo_color", "./img/prueba4.bmp"); 
            
            #pragma omp section
            desenfoque("./img/prueba4.bmp", "img4_desenfoque", 27); 

            #pragma omp section
            desenfoque_color("./img/prueba4.bmp", "img4_desenfoque_color", 27);
        }
    }

    tiempo_final = omp_get_wtime();

    printf("\n=============================================\n");
    printf("Procesamiento total finalizado.\n");
    printf("Tiempo total de ejecucion: %.4f segundos\n", tiempo_final - tiempo_inicio);
    printf("=============================================\n");

    return 0;
}
