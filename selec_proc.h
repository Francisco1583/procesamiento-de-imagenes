#ifndef SELEC_PROC_H
#define SELEC_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1. Inversión vertical en escala de grises
extern void inv_img(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto) {
    // Se elimina el uso de mask y strcat. Abrimos la ruta final directamente.
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    // Escribir el encabezado completo de una sola vez
    fwrite(header, 1, offset, outputImage);

    // Calcular padding matemáticamente
    int row_padded = (ancho * 3 + 3) & (~3);
    
    // Crear una fila temporal en memoria para escribirla de golpe
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded); // Garantiza que el padding final sea siempre '0'

    // Leer de abajo hacia arriba (inversión vertical)
    for (int i = alto - 1; i >= 0; i--) {
        int row_start = i * row_padded;
        for (int x = 0; x < ancho; x++) {
            unsigned char b = pixels[row_start + x * 3];
            unsigned char g = pixels[row_start + x * 3 + 1];
            unsigned char r = pixels[row_start + x * 3 + 2];
            
            // OPTIMIZACIÓN: Matemática entera con bitwise shift (equivalente a / 256)
            unsigned char gray = (r * 54 + g * 183 + b * 19) >> 8;
            
            out_row[x * 3]     = gray;
            out_row[x * 3 + 1] = gray;
            out_row[x * 3 + 2] = gray;
        }
        // Escribir toda la fila (incluyendo padding) con 1 sola llamada al sistema
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(out_row);
    fclose(outputImage);
}

// 2. Inversión vertical a color
extern void inv_img_color(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto) {
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    fwrite(header, 1, offset, outputImage);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded);

    for (int i = alto - 1; i >= 0; i--) {
        int row_start = i * row_padded;
        for (int x = 0; x < ancho; x++) {
            out_row[x * 3]     = pixels[row_start + x * 3];
            out_row[x * 3 + 1] = pixels[row_start + x * 3 + 1];
            out_row[x * 3 + 2] = pixels[row_start + x * 3 + 2];
        }
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(out_row);
    fclose(outputImage);
}

// 3. Desenfoque a color
extern void desenfoque_color(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto, int kernel_size) {
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    fwrite(header, 1, offset, outputImage);
    
    int row_padded = (ancho * 3 + 3) & (~3);
    
    // Eliminamos los multiples mallocs (adiós fragmentación).
    // Usamos un solo bloque de memoria para el cálculo temporal.
    unsigned char* temp_img = (unsigned char*)malloc(alto * row_padded);
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded);

    int k = kernel_size / 2;
    
    // Paso Horizontal
    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            int sB = 0, sG = 0, sR = 0, count = 0;
            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < ancho) {
                    sB += pixels[row_start + nx * 3]; 
                    sG += pixels[row_start + nx * 3 + 1]; 
                    sR += pixels[row_start + nx * 3 + 2]; 
                    count++;
                }
            }
            temp_img[row_start + x * 3]     = sB / count; 
            temp_img[row_start + x * 3 + 1] = sG / count; 
            temp_img[row_start + x * 3 + 2] = sR / count;
        }
    }

    // Paso Vertical y Escritura final
    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            int sB = 0, sG = 0, sR = 0, count = 0;
            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < alto) {
                    int ny_start = ny * row_padded;
                    sB += temp_img[ny_start + x * 3]; 
                    sG += temp_img[ny_start + x * 3 + 1]; 
                    sR += temp_img[ny_start + x * 3 + 2]; 
                    count++;
                }
            }
            out_row[x * 3]     = sB / count; 
            out_row[x * 3 + 1] = sG / count; 
            out_row[x * 3 + 2] = sR / count;
        }
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(temp_img); 
    free(out_row);
    fclose(outputImage);
}
#endif
