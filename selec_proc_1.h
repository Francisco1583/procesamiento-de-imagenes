#ifndef SELEC_PROC_1_H
#define SELEC_PROC_1_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1. Convertir a Escala de Grises
extern void gray_img(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto) {
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    fwrite(header, 1, offset, outputImage);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded);

    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            unsigned char b = pixels[row_start + x * 3];
            unsigned char g = pixels[row_start + x * 3 + 1];
            unsigned char r = pixels[row_start + x * 3 + 2];
            
            // OPTIMIZACIÓN: Matemática entera
            unsigned char gray = (r * 54 + g * 183 + b * 19) >> 8;
            
            out_row[x * 3]     = gray;
            out_row[x * 3 + 1] = gray;
            out_row[x * 3 + 2] = gray;
        }
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(out_row);
    fclose(outputImage);
}

// 2. Inversión Horizontal en Escala de Grises (Efecto Espejo)
extern void inv_img_grey_horizontal(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto) {
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    fwrite(header, 1, offset, outputImage);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded);

    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            int src_x = ancho - 1 - x;
            unsigned char b = pixels[row_start + src_x * 3];
            unsigned char g = pixels[row_start + src_x * 3 + 1];
            unsigned char r = pixels[row_start + src_x * 3 + 2];
            
            // OPTIMIZACIÓN: Matemática entera
            unsigned char gray = (r * 54 + g * 183 + b * 19) >> 8;
            
            out_row[x * 3]     = gray;
            out_row[x * 3 + 1] = gray;
            out_row[x * 3 + 2] = gray;
        }
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(out_row);
    fclose(outputImage);
}

// 3. Inversión Horizontal a Color (Efecto Espejo)
extern void inv_img_color_horizontal(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto) {
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    fwrite(header, 1, offset, outputImage);

    int row_padded = (ancho * 3 + 3) & (~3);
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded);

    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            int src_x = ancho - 1 - x;
            out_row[x * 3]     = pixels[row_start + src_x * 3];
            out_row[x * 3 + 1] = pixels[row_start + src_x * 3 + 1];
            out_row[x * 3 + 2] = pixels[row_start + src_x * 3 + 2];
        }
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(out_row);
    fclose(outputImage);
}

// 4. Desenfoque (Convierte a escala de grises y aplica Blur)
extern void desenfoque(const char* output_path, const unsigned char* header, int offset, const unsigned char* pixels, int ancho, int alto, int kernel_size) {
    FILE *outputImage = fopen(output_path, "wb");
    if (!outputImage) return;

    fwrite(header, 1, offset, outputImage);

    int row_padded = (ancho * 3 + 3) & (~3);
    
    unsigned char* temp_img = (unsigned char*)malloc(alto * row_padded);
    unsigned char* out_row = (unsigned char*)malloc(row_padded);
    memset(out_row, 0, row_padded);

    int k = kernel_size / 2;

    // Paso Horizontal
    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            int sum = 0, count = 0;
            for (int dx = -k; dx <= k; dx++) {
                int nx = x + dx;
                if (nx >= 0 && nx < ancho) {
                    unsigned char b = pixels[row_start + nx * 3];
                    unsigned char g = pixels[row_start + nx * 3 + 1];
                    unsigned char r = pixels[row_start + nx * 3 + 2];
                    
                    // OPTIMIZACIÓN: Matemática entera aquí también
                    sum += (r * 54 + g * 183 + b * 19) >> 8;
                    count++;
                }
            }
            unsigned char avg_gray = sum / count;
            temp_img[row_start + x * 3]     = avg_gray;
            temp_img[row_start + x * 3 + 1] = avg_gray;
            temp_img[row_start + x * 3 + 2] = avg_gray;
        }
    }

    // Paso Vertical y Escritura Final
    for (int y = 0; y < alto; y++) {
        int row_start = y * row_padded;
        for (int x = 0; x < ancho; x++) {
            int sum = 0, count = 0;
            for (int dy = -k; dy <= k; dy++) {
                int ny = y + dy;
                if (ny >= 0 && ny < alto) {
                    sum += temp_img[ny * row_padded + x * 3];
                    count++;
                }
            }
            unsigned char avg_gray = sum / count;
            out_row[x * 3]     = avg_gray;
            out_row[x * 3 + 1] = avg_gray;
            out_row[x * 3 + 2] = avg_gray;
        }
        fwrite(out_row, 1, row_padded, outputImage);
    }

    free(temp_img);
    free(out_row);
    fclose(outputImage);
}
#endif
