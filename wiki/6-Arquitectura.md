# Arquitectura de la Interfaz y del Backend

## Estructura general

La arquitectura actual separa responsabilidades con más claridad:

```
┌────────────────────────────────────────────────────┐
│ Interfaz en Python                                 │
│ Interfaz.py + PyQt5                                │
│ - arrastrar y soltar imágenes BMP                  │
│ - seleccionar hasta 10 archivos                    │
│ - activar transformaciones                         │
│ - capturar kernels                                 │
│ - lanzar el backend por subprocess                 │
└───────────────────────┬────────────────────────────┘
                        │
                        ▼
┌────────────────────────────────────────────────────┐
│ Backend en C/OpenMP                                │
│ para_image_parra.c                                 │
│ selec_proc.h / selec_proc_1.h                      │
│ - carga BMP en RAM una sola vez                    │
│ - crea tareas OpenMP por imagen y transformación   │
│ - escribe resultados a disco                       │
│ - imprime TIEMPO_TOTAL                             │
└───────────────────────┬────────────────────────────┘
                        │
                        ▼
                 resultados/
```

## Flujo de ejecución

### 1. La interfaz recoge datos

`Interfaz.py` crea la ventana, muestra el logo y prepara los controles. La ruta de salida se resuelve desde la carpeta del script para evitar depender del directorio actual de la terminal.

### 2. El usuario selecciona archivos y opciones

- Hasta 10 imágenes BMP.
- Seis transformaciones disponibles.
- Kernels configurables para blur.
- Botón para seleccionar todas las transformaciones.

### 3. La interfaz invoca el backend

La aplicación construye un comando con esta forma general:

```text
main.exe <ruta_salida> <kernel_gris> <kernel_color> <f1> <f2> <f3> <f4> <f5> <f6> <img1> ...
```

En Windows se busca `main.exe`; en otros sistemas se busca `main`.

### 4. El backend procesa con OpenMP

`para_image_parra.c` carga cada BMP una sola vez, crea tareas independientes por imagen y transformación, y delega el trabajo a las funciones definidas en los headers.

Funciones principales:

| Función | Archivo | Propósito |
|---|---|---|
| `gray_img()` | `selec_proc_1.h` | Conversión a escala de grises |
| `inv_img_color()` | `selec_proc.h` | Inversión vertical a color |
| `inv_img_grey_horizontal()` | `selec_proc_1.h` | Inversión horizontal en gris |
| `inv_img_color_horizontal()` | `selec_proc_1.h` | Inversión horizontal a color |
| `desenfoque()` | `selec_proc_1.h` | Blur en gris |
| `desenfoque_color()` | `selec_proc.h` | Blur a color |

## Optimizaciones del backend

- Lectura única de cada imagen en memoria.
- Uso de buffers por fila para escritura más eficiente.
- Conversión de gris con aritmética entera.
- Blur separable por pasos horizontal y vertical.
- Tareas OpenMP para repartir trabajo sin depender de un único loop.

## Salida y reporte

El backend escribe los archivos resultantes en `resultados/` y devuelve el tiempo total por stdout usando `TIEMPO_TOTAL:`. La interfaz extrae ese valor y lo muestra en la ventana.

## Estructura del proyecto

```
procesamiento-de-imagenes/
├── Interfaz.py
├── para_image_parra.c
├── selec_proc.h
├── selec_proc_1.h
├── resultados/
└── src/
```

---

[[5-Conclusiones]] | [[Home]] | [[7-Guía-Instalación]]
