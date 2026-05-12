# Procesamiento Paralelo de Imágenes BMP

Bienvenido a la wiki del **Sistema de Procesamiento Paralelo de Imágenes BMP**. La versión actual separa claramente la interfaz y el backend: la UI está hecha en **Python con PyQt5** y el procesamiento pesado permanece en **C/OpenMP**.

## Descripción rápida

El usuario selecciona imágenes BMP desde la interfaz, define kernels y transformaciones, y luego la aplicación invoca el ejecutable del backend para generar los resultados en disco. La interfaz solo orquesta la ejecución; el trabajo intensivo ocurre en el backend optimizado.

**Transformaciones disponibles:**
- Inversión vertical en escala de grises
- Inversión vertical a color
- Inversión horizontal en escala de grises
- Inversión horizontal a color
- Desenfoque en escala de grises
- Desenfoque a color

## Tabla de contenidos

| Sección | Descripción |
|---------|-------------|
| [Objetivos](1-Objetivos) | Meta del experimento y preguntas de investigación |
| [Contexto Experimental](2-Contexto-Experimental) | Hardware, programa actual y metodología |
| [Resultados y Datos](3-Resultados-y-Datos) | Tiempos consolidados y lectura de resultados |
| [Análisis Detallado](4-Análisis-Detallado) | Interpretación por dispositivo y sistema operativo |
| [Conclusiones](5-Conclusiones) | Recomendaciones de nodos y configuración |
| [Arquitectura](6-Arquitectura) | Flujo Python UI -> backend C |
| [Guía de Instalación](7-Guía-Instalación) | Dependencias, compilación y uso |

## Inicio rápido

### Compilar el backend en Windows
```powershell
gcc para_image_parra.c -o main.exe -fopenmp
python -m pip install PyQt5
python Interfaz.py
```

### Compilar el backend en Linux/macOS
```bash
gcc para_image_parra.c -o main -fopenmp
python3 -m pip install PyQt5
python3 Interfaz.py
```

## Conclusiones principales

- Mejor desempeño: Laptop B (Francisco) con 6.8342s a 18 hilos.
- Mejor escalabilidad: Linux Bodhi mostró la ejecución más estable.
- Configuración óptima: 18 hilos sigue siendo el mejor balance en las pruebas.
- Recomendación: Laptop B como nodo maestro, Laptop C como nodo secundario de alto rendimiento y Laptop A como apoyo.

## Estructura del repositorio

```
procesamiento-de-imagenes/
├── Interfaz.py                 # Interfaz gráfica en Python/PyQt5
├── para_image_parra.c          # Orquestador del backend OpenMP
├── selec_proc.h                # Transformaciones en gris y blur
├── selec_proc_1.h              # Transformaciones horizontales y color
├── resultados/                 # Salida generada por la interfaz
├── src/                        # Recursos gráficos
├── README.md                   # Documentación general
└── wiki/                       # Esta wiki
```

## Notas importantes

- La interfaz resuelve rutas absolutas desde la carpeta del script para evitar errores por el directorio de trabajo.
- El backend carga cada imagen una sola vez en memoria y escribe los resultados por filas para reducir I/O innecesario.
- La interfaz acepta hasta 10 imágenes BMP simultáneamente.
- El tiempo total se lee desde la salida estándar del ejecutable del backend.

---

**Última actualización:** Mayo 2026  
**Versión:** 2.0 (UI en Python + backend C/OpenMP)
