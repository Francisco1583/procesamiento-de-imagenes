#ifndef SELEC_PROC_H
#define SELEC_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void manejar_encabezado(FILE *in, FILE *out, int *ancho, int *alto, int *offset) {
    unsigned char fileHeader[14];
    fread(fileHeader, 1, 14, in);
    *offset = *(int*)&fileHeader[10];
    
    unsigned char* fullHeader = (unsigned char*)malloc(*offset);
    memcpy(fullHeader, fileHeader, 14);
    fread(fullHeader + 14, 1, *offset - 14, in);
    fwrite(fullHeader, 1, *offset, out);
    
    *ancho = *(int*)&fullHeader[18];
    *alto = *(int*)&fullHeader[22];
    free(fullHeader);
}

// 1. Inversión vertical en escala de grises
extern void inv_img(const char* mask, const char* path) {
    FILE *image = fopen(path, "rb");
    if (!image) return;
    
    char add_char[80] = "./img/";
    strcat(add_char, mask); strcat(add_char, ".bmp");
    FILE *outputImage = fopen(add_char, "wb");

    int ancho, alto, offset;
    manejar_encabezado(image, outputImage, &ancho, &alto, &offset);

    int padding = (4 - (ancho * 3) % 4) % 4;
    unsigned char* arr_in = (unsigned char*)malloc(ancho * alto);
    
    for (int i = 0; i < alto; i++) {
        for (int j = 0; j < ancho; j++) {
            unsigned char b = fgetc(image);
            unsigned char g = fgetc(image);
            unsigned char r = fgetc(image);
            arr_in[i * ancho + j] = 0.21 * r + 0.72 * g + 0.07 * b;
        }
        // Saltar padding en lectura
        for (int p = 0; p < padding; p++) fgetc(image);
    }

    for (int i = alto - 1; i >= 0; i--) {
        for (int x = 0; x < ancho; x++) {
            unsigned char pixel = arr_in[i * ancho + x];
            fputc(pixel, outputImage); fputc(pixel, outputImage); fputc(pixel, outputImage);
        }
        // Escribir padding en salida
        for (int p = 0; p < padding; p++) fputc(0, outputImage);
    }

    free(arr_in); fclose(image); fclose(outputImage);
}

// 2. Inversión vertical a color
extern void inv_img_color(const char* mask, const char* path) {
    FILE *image = fopen(path, "rb");
    char add_char[80] = "./img/";
    strcat(add_char, mask); strcat(add_char, ".bmp");
    FILE *outputImage = fopen(add_char, "wb");

    int ancho, alto, offset;
    manejar_encabezado(image, outputImage, &ancho, &alto, &offset);

    int padding = (4 - (ancho * 3) % 4) % 4;
    unsigned char *b_arr = malloc(ancho * alto), *g_arr = malloc(ancho * alto), *r_arr = malloc(ancho * alto);

    for (int i = 0; i < alto; i++) {
        for (int j = 0; j < ancho; j++) {
            b_arr[i * ancho + j] = fgetc(image);
            g_arr[i * ancho + j] = fgetc(image);
            r_arr[i * ancho + j] = fgetc(image);
        }
        for (int p = 0; p < padding; p++) fgetc(image);
    }

    for (int i = alto - 1; i >= 0; i--) {
        for (int y = 0; y < ancho; y++) {
            fputc(b_arr[i * ancho + y], outputImage);
            fputc(g_arr[i * ancho + y], outputImage);
            fputc(r_arr[i * ancho + y], outputImage);
        }
        for (int p = 0; p < padding; p++) fputc(0, outputImage);
    }

    free(b_arr); free(g_arr); free(r_arr);
    fclose(image); fclose(outputImage);
}

// 3. Desenfoque a color (Mantenido porque ya manejaba el padding bien)
extern void desenfoque_color(const char* input_path, const char* name_output, int kernel_size) {
    FILE *image = fopen(input_path, "rb");
    char output_path[100] = "./img/";
    strcat(output_path, name_output); strcat(output_path, ".bmp");
    FILE *outputImage = fopen(output_path, "wb");

    int ancho, alto, offset;
    manejar_encabezado(image, outputImage, &ancho, &alto, &offset);
    
    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char** input_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** output_rows = malloc(alto * sizeof(unsigned char*));
    unsigned char** temp_rows = malloc(alto * sizeof(unsigned char*));

    for (int i = 0; i < alto; i++) {
        input_rows[i] = malloc(row_padded);
        output_rows[i] = malloc(row_padded);
        temp_rows[i] = malloc(row_padded);
        fread(input_rows[i], 1, row_padded, image);
    }

    int k = kernel_size / 2;
    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sB = 0, sG = 0, sR = 0, count = 0;
            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < ancho) {
                    sB += input_rows[y][nx*3]; sG += input_rows[y][nx*3+1]; sR += input_rows[y][nx*3+2]; count++;
                }
            }
            temp_rows[y][x*3] = sB/count; temp_rows[y][x*3+1] = sG/count; temp_rows[y][x*3+2] = sR/count;
        }
    }

    for (int y = 0; y < alto; y++) {
        for (int x = 0; x < ancho; x++) {
            int sB = 0, sG = 0, sR = 0, count = 0;
            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < alto) {
                    sB += temp_rows[ny][x*3]; sG += temp_rows[ny][x*3+1]; sR += temp_rows[ny][x*3+2]; count++;
                }
            }
            output_rows[y][x*3] = sB/count; output_rows[y][x*3+1] = sG/count; output_rows[y][x*3+2] = sR/count;
        }
        fwrite(output_rows[y], 1, row_padded, outputImage);
    }

    for(int i=0; i<alto; i++) { free(input_rows[i]); free(temp_rows[i]); free(output_rows[i]); }
    free(input_rows); free(temp_rows); free(output_rows);
    fclose(image); fclose(outputImage);
}
#endif
