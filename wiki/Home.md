# Interfaz Gráfica de Procesamiento de Imágenes BMP

Bienvenido a la wiki del **Sistema de Procesamiento Paralelo de Imágenes BMP**. Este proyecto implementa una interfaz gráfica multiplataforma (Windows/Linux/macOS) que transforma imágenes BMP utilizando paralelización con OpenMP.

## 🎯 Descripción rápida

Este proyecto evalúa el rendimiento de transformaciones de imágenes en paralelo en tres dispositivos diferentes, identificando cuál es más eficiente para operar como nodo principal de una red de procesamiento distribuido.

**Transformaciones disponibles:**
- ✅ Inversión vertical (grayscale/color)
- ✅ Flip horizontal (grayscale/color)  
- ✅ Desenfoque/Blur configurable (grayscale/color)
- ✅ Control de hilos OpenMP ajustable (6, 12, 18 o personalizado)

## 📋 Tabla de Contenidos

| Sección | Descripción |
|---------|-------------|
| [Objetivos](1-Objetivos) | Metas del experimento y preguntas de investigación |
| [Contexto Experimental](2-Contexto-Experimental) | Hardware probado, programa y metodología |
| [Resultados y Datos](3-Resultados-y-Datos) | Tabla consolidada de tiempos y hallazgos principales |
| [Análisis Detallado](4-Análisis-Detallado) | Interpretación por dispositivo y factor OS |
| [Conclusiones](5-Conclusiones) | Recomendaciones y selección de nodos |
| [Arquitectura](6-Arquitectura) | Cómo funciona la UI y su conexión al backend |
| [Guía de Instalación](7-Guía-Instalación) | Compilación y ejecución por plataforma |

## 🚀 Inicio rápido

### Compilar en Windows
```powershell
gcc interfaz_grafica.c -o interfaz_grafica.exe -fopenmp -lgdiplus -lgdi32 -lcomdlg32 -lshell32 -mwindows
./interfaz_grafica.exe
```

### Compilar en Linux/macOS
```bash
gcc interfaz_grafica.c -o interfaz_grafica $(pkg-config --cflags --libs gtk+-3.0) -fopenmp
./interfaz_grafica
```

## 🔑 Conclusiones principales

- **Mejor desempeño:** Laptop B (Francisco) con **6.8342s a 18 hilos**
- **Mejor escalabilidad:** Linux Bodhi supera Windows en eficiencia
- **Configuración óptima:** 18 hilos paralelos en todos los equipos
- **Recomendación:** Nodo maestro = Laptop B, Nodo secundario principal = Laptop C, Nodo de apoyo = Laptop A

## 📁 Estructura del repositorio

```
procesamiento-de-imagenes/
├── interfaz_grafica.c          # GUI multiplataforma
├── selec_proc.h                # Transformaciones backend (grayscale)
├── selec_proc_1.h              # Transformaciones backend (color)
├── src/
│   └── logo.bmp                # Logo Tec de Monterrey (180×150)
├── img/                        # Directorio de salida de resultados
├── README.md                   # Documentación principal
└── wiki/                       # Esta wiki
```

## 💡 Notas importantes

- El logo se renderiza en un lienzo de **180×150 píxeles** con escalado proporcional sin recortes
- Los archivos de salida se guardan en el directorio `img/` relativo al ejecutable
- OpenMP se configura dinámicamente desde la UI (por defecto 18 hilos)
- Soporte para hasta **10 imágenes BMP** simultaneamente
- Tiempo de ejecución medido con precisión mediante `omp_get_wtime()`

---

**Última actualización:** Abril 2026  
**Versión:** 1.0 (Interfaz Gráfica Multiplataforma)
