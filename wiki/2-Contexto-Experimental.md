# Contexto Experimental

## Programa utilizado

### Arquitectura actual

La solución quedó dividida en dos partes:
- **Interfaz:** [Interfaz.py](../Interfaz.py) con PyQt5, encargada de seleccionar imágenes, opciones y lanzar el backend.
- **Backend:** [para_image_parra.c](../para_image_parra.c) junto con [selec_proc.h](../selec_proc.h) y [selec_proc_1.h](../selec_proc_1.h), donde vive el procesamiento pesado.

### Transformaciones implementadas

El código procesa imágenes BMP y genera hasta 6 salidas por imagen:

| # | Transformación | Tipo | Descripción |
|---|---|---|---|
| 1 | Inversión vertical | Grayscale | Invierte los píxeles de arriba hacia abajo |
| 2 | Inversión horizontal | Grayscale | Espejo de izquierda a derecha |
| 3 | Inversión vertical | Color | Inversión RGB completa |
| 4 | Inversión horizontal | Color | Espejo en color |
| 5 | Desenfoque | Grayscale | Convolución con kernel configurable |
| 6 | Desenfoque | Color | Convolución RGB |

### Paralelización

- **Modelo:** OpenMP con tareas independientes (`#pragma omp parallel` + `#pragma omp single` + `#pragma omp task`)
- **Granularidad:** Una tarea por transformación e imagen
- **Medición:** `omp_get_wtime()` antes y después del bloque de trabajo
- **Configuración:** La UI solo lanza el ejecutable; el backend controla el número de hilos configurado para el experimento

### Optimizaciones del backend

- Cada imagen se carga una sola vez en RAM.
- Los filtros escriben una fila completa por vez para reducir llamadas de I/O.
- El cálculo de escala de grises usa aritmética entera.
- El blur separa el proceso en paso horizontal y vertical para mejorar localidad de memoria.

---

## Hardware evaluado

### Especificaciones técnicas

| Dispositivo | Persona | OS | RAM | Hilos lógicos | Núcleos físicos | Frecuencia | Procesador |
|---|---|---|---|---|---|---|---|
| **A** | Alejandro | Windows 11 Home | 16 GB | 6 | 6 | Base | AMD Ryzen 5 4500U |
| **B** | Francisco | Linux Bodhi | 16 GB | 8 | 4 | 2.4–4.2 GHz | Intel Core i5-1135G7 (Gen 11) |
| **C** | Yahel | Windows 11 | 16 GB | 12 | 10 | 1.3–4.4 GHz | Intel Core i5-1235U (Gen 12) |

### Diferencias estructurales relevantes

1. Laptop A no tiene hyperthreading, así que su techo práctico es menor.
2. Laptop B corre Linux Bodhi, que ofrece menor overhead de sistema.
3. Laptop C tiene más núcleos físicos, pero Windows 11 introduce variabilidad.

---

## Configuración de prueba

### Variables controladas
- Imágenes BMP usadas en la campaña experimental: 4
- Transformaciones: las 6 disponibles
- Salida: directorio `resultados/` creado por la interfaz

### Variables ajustadas
- Hilos OpenMP: 6, 12 y 18
- Repeticiones: 3 por configuración
- Entorno: mismas imágenes y mismos filtros para todas las ejecuciones

### Factores no controlados
- Procesos en segundo plano, especialmente en Windows
- Temperatura del CPU
- Carga de otros programas
- Caché del sistema de archivos

---

## Metodología de medición

```c
double tiempo_inicio = omp_get_wtime();
/* procesamiento paralelo con tareas */
double tiempo_final = omp_get_wtime();
```

**Precisión:** 4 decimales

**Repeticiones:** 3 por cada combinación dispositivo × hilos

**Total de ejecuciones:** 3 dispositivos × 3 configuraciones × 3 repeticiones = 27 mediciones

---

[[1-Objetivos]] | [[Home]] | [[3-Resultados-y-Datos]]
