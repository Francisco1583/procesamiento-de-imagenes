# Contexto Experimental

## Programa utilizado

### Transformaciones implementadas

El código procesa 4 imágenes BMP aplicándole 6 filtros distintos a cada una, generando **24 tareas independientes**:

| # | Transformación | Tipo | Descripción |
|---|---|---|---|
| 1 | Inversión vertical | Grayscale | Invierte los píxeles de arriba hacia abajo |
| 2 | Flip horizontal | Grayscale | Espejo de izquierda a derecha |
| 3 | Inversión vertical | Color | Inversión RGB completa |
| 4 | Flip horizontal | Color | Espejo en color |
| 5 | Desenfoque | Grayscale | Convolución con kernel configurable |
| 6 | Desenfoque | Color | Convolución RGB |

### Paralelización

- **Modelo:** OpenMP con `#pragma omp sections`
- **Granularidad:** 24 secciones independientes (1 filtro/imagen por sección)
- **Medición:** `omp_get_wtime()` antes y después del bloque paralelo
- **Configuración:** Variable desde UI (6, 12, 18 hilos)

---

## Hardware evaluado

### Especificaciones técnicas

| Dispositivo | Persona | OS | RAM | Hilos lógicos | Núcleos físicos | Frecuencia | Procesador |
|---|---|---|---|---|---|---|---|
| **A** | Alejandro | Windows 11 Home | 16 GB | **6** | 6 | Base | AMD Ryzen 5 4500U |
| **B** | Francisco | Linux Bodhi | 16 GB | 8 | 4 | 2.4–4.2 GHz | Intel Core i5-1135G7 (Gen 11) |
| **C** | Yahel | Windows 11 | 16 GB | **12** | 10 | 1.3–4.4 GHz | Intel Core i5-1235U (Gen 12) |

### Diferencias estructurales relevantes

1. **Laptop A:** Sin hyperthreading (1 núcleo = 1 hilo lógico) → Límite duro de paralelismo en 6
2. **Laptop B:** Linux Bodhi con planificador más eficiente que Windows
3. **Laptop C:** Generación más reciente (12va) con más núcleos pero limitada por Windows 11

---

## Configuración de prueba

### Variables controladas
- **Imágenes:** Siempre 4 BMPs
- **Transformaciones:** Las 6 filtros en todas las pruebas
- **I/O:** Lectura desde disco, escritura en `img/`

### Variables ajustadas
- **Hilos OpenMP:** 6, 12, 18
- **Repeticiones:** 3 por configuración
- **Momento:** Durante días laborales para evitar interferencia de procesos en segundo plano

### Factores no controlados (se reportan como limitaciones)
- Procesos en segundo plano (especialmente en Windows)
- Temperatura del CPU
- Otros programas abiertos
- Caché del sistema de archivos

---

## Metodología de medición

```c
double start = omp_get_wtime();
#pragma omp parallel sections num_threads(N)
{
    // 24 secciones con transformaciones
}
double elapsed = omp_get_wtime() - start;
```

**Precisión:** 4 decimales (milisegundos)  
**Repeticiones:** 3 por cada combinación dispositivo × hilos  
**Total de ejecuciones:** 3 dispositivos × 3 configuraciones × 3 repeticiones = **27 mediciones**

---

[[1-Objetivos]] | [[Home]] | [[3-Resultados-y-Datos]]
