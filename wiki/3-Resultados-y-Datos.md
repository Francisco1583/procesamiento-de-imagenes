# Resultados y Datos

## Tabla consolidada de tiempos en segundos

| Hilos | Laptop A (Alejandro) | Laptop B (Francisco) | Laptop C (Yahel) |
|:---:|:---:|:---:|:---:|
| **6** | 10.5160 | 7.3627 | 7.7310 |
| **12** | 9.8120 | 7.1117 | 8.2830 |
| **18** | 9.6230 | **6.8342** | 6.9550 |

## Lectura general

Las mediciones se mantienen como referencia del experimento original. La diferencia actual es que la interfaz en Python solo ejecuta el backend C optimizado; por eso, los tiempos reflejan el costo real del procesamiento y no el de la UI.

### Tendencias observadas

- Laptop B obtuvo el mejor tiempo global con 6.8342 segundos a 18 hilos.
- Laptop A fue la más lenta en todas las configuraciones.
- Laptop C mostró una regresión a 12 hilos, pero recuperó rendimiento a 18.

### Visualización de tendencias

```
LAPTOP A (AMD Ryzen 5 4500U - Windows)
10.5160 ──────────────
  9.8120 ─────────
  9.6230 ────────

LAPTOP B (Intel i5-1135G7 - Linux) ⭐ MEJOR
 7.3627 ────────────
 7.1117 ──────────
 6.8342 ────────

LAPTOP C (Intel i5-1235U - Windows)
 7.7310 ────────────
 8.2830 ──────────────  ⚠️ Anomalía a 12 hilos
 6.9550 ───────
```

## Hallazgos principales

### Laptop B obtuvo los mejores resultados

- Mejor tiempo global: 6.8342 segundos (18 hilos)
- Mejor consistencia: reducción progresiva al subir hilos
- Ventaja: Linux Bodhi con menor overhead del sistema operativo

### El incremento de hilos siguió siendo favorable

| Dispositivo | Mejora 6→18 hilos |
|---|---|
| Laptop A | 8.4% |
| Laptop B | 7.1% |
| Laptop C | 9.9% |

### Laptop C registró una anomalía a 12 hilos

- 6 hilos: 7.7310s
- 12 hilos: 8.2830s
- 18 hilos: 6.9550s

La causa más probable sigue siendo la combinación de Windows 11, procesos en segundo plano y variación térmica.

### Laptop A fue la más limitada

- Rango: 9.6 a 10.5 segundos
- Límite físico: solo 6 hilos lógicos reales
- Efecto: menor capacidad de ocultar la latencia de I/O y del planificador

## Análisis de escalabilidad

| Paso | Laptop A | Laptop B | Laptop C |
|---|---|---|---|
| 6→12 hilos | -0.6040s (↓5.8%) | -0.2510s (↓3.4%) | +0.5520s (↑7.1%) |
| 12→18 hilos | -0.1890s (↓1.9%) | -0.2775s (↓3.9%) | -1.3280s (↓16.0%) |
| Total 6→18 | -0.7930s (↓7.5%) | -0.5285s (↓7.2%) | -0.7760s (↓10.0%) |

## Consideraciones sobre la medición

> Los resultados dependen de la carga del sistema operativo en el momento de la ejecución. Las variaciones pequeñas, sobre todo en Windows, siguen siendo esperables.

Medidas tomadas:
- 3 repeticiones por configuración.
- Misma carpeta de salida y mismas imágenes.
- Backend optimizado para reducir ruido por I/O.

---

[[2-Contexto-Experimental]] | [[Home]] | [[4-Análisis-Detallado]]
