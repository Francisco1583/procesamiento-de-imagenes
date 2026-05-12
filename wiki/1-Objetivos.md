# Objetivos del Proyecto

## Pregunta principal de investigación

> **¿Qué dispositivo del equipo es más eficiente para operar como nodo maestro en un sistema de transformación paralela de imágenes?**

## Objetivos específicos

### 1. Comparar rendimiento entre dispositivos
Evaluar el tiempo de ejecución de 6 transformaciones BMP en tres equipos distintos:
- Laptop A (Alejandro) - AMD Ryzen 5 4500U / Windows 11
- Laptop B (Francisco) - Intel Core i5-1135G7 / Linux Bodhi
- Laptop C (Yahel) - Intel Core i5-1235U / Windows 11

### 2. Analizar el impacto del paralelismo
Medir cómo el incremento de hilos OpenMP (6, 12 y 18) afecta el tiempo de ejecución en cada dispositivo:
- ¿La escalabilidad es lineal?
- ¿Existe un punto óptimo de hilos?
- ¿Las limitaciones de hardware se hacen evidentes?

### 3. Evaluar el factor sistema operativo
Determinar si existen diferencias significativas entre:
- Windows 11 en las laptops A y C
- Linux Bodhi en la laptop B

### 4. Identificar el nodo maestro
Basado en rendimiento, estabilidad y escalabilidad:
- ¿Cuál dispositivo debería distribuir las tareas?
- ¿Cuál tiene mejor consistencia en las mediciones?
- ¿Cuál ofrece mejor relación rendimiento/costo computacional?

### 5. Proponer configuración óptima
Recomendar:
- Número de hilos para maximizar rendimiento
- Distribución de carga entre nodos
- Especificaciones mínimas para nuevos nodos

## Metodología

**Variable independiente:** Número de hilos OpenMP (6, 12, 18)

**Variable dependiente:** Tiempo de ejecución total en segundos

**Repeticiones:** 3 ejecuciones por configuración y dispositivo

**Medición:** `omp_get_wtime()` con precisión de 4 decimales

**Nota actual:** La interfaz ya no participa en el cómputo. La UI en Python solo captura parámetros y lanza el backend C optimizado.

---

[[Home]] | [[2-Contexto-Experimental]]
