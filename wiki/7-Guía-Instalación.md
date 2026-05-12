# Guía de Instalación y Uso

## Requisitos previos

### Windows
- Python 3.10 o superior.
- PyQt5 instalado en el entorno de Python.
- GCC con soporte para OpenMP.
- MSYS2 o MinGW para compilar el backend.

### Linux
- Python 3.10 o superior.
- PyQt5.
- GCC con OpenMP.

### macOS
- Python 3.10 o superior.
- PyQt5.
- GCC o Clang con OpenMP, según el toolchain disponible.

---

## Instalación de dependencias

### Windows

```powershell
python -m pip install PyQt5
```

### Linux y macOS

```bash
python3 -m pip install PyQt5
```

Si tu sistema no tiene un compilador con OpenMP listo, instala el toolchain correspondiente antes de compilar `para_image_parra.c`.

---

## Compilación del backend

### Windows

```powershell
cd "ruta\al\proyecto"
gcc para_image_parra.c -o main.exe -fopenmp
```

### Linux y macOS

```bash
cd /ruta/al/proyecto
gcc para_image_parra.c -o main -fopenmp
```

El ejecutable debe quedar en la misma carpeta que [Interfaz.py](../Interfaz.py).

---

## Ejecución

### Windows

```powershell
python Interfaz.py
```

### Linux y macOS

```bash
python3 Interfaz.py
```

La interfaz:
- permite arrastrar y soltar BMP,
- admite hasta 10 imágenes,
- guarda la salida en `resultados/`,
- y lee el tiempo total del backend desde `TIEMPO_TOTAL`.

---

## Uso rápido

1. Selecciona o arrastra imágenes BMP.
2. Marca una o varias transformaciones.
3. Ajusta los kernels si vas a usar blur.
4. Ejecuta el procesamiento.
5. Revisa los archivos generados en `resultados/`.

---

## Solución de problemas

### No encuentra el ejecutable

- Verifica que `main.exe` o `main` esté junto a `Interfaz.py`.
- Confirma que la compilación no falló.

### PyQt5 no está instalado

- Ejecuta `python -m pip install PyQt5` o `python3 -m pip install PyQt5`.

### No se generan archivos

- Revisa que las imágenes sean BMP válidas.
- Verifica que al menos una transformación esté marcada.
- Confirma que la carpeta `resultados/` sea escribible.

### El tiempo sale como error

- Compila de nuevo el backend.
- Asegúrate de que `main.exe`/`main` corresponda al sistema operativo actual.

---

[[6-Arquitectura]] | [[Home]]

## 📊 Validación post-compilación

### Test rápido (60 segundos)

```bash
# 1. Crear imagen de prueba (BMP simple 100×100)
# 2. Seleccionar solo esa imagen
# 3. Marcar solo "Vertical grayscale"
# 4. Ejecutar
# 5. Verificar que archivo se creó en resultados/
```

### Test completo (5 minutos)

```bash
# 1. Agregar 3 imágenes
# 2. Marcar todas las transformaciones
# 3. Usar threads = 18, kernel = 27
# 4. Ejecutar
# 5. Verificar:
#    - Tiempo razonable (< 30s para 3 imágenes)
#    - 18 archivos generados (3 × 6)
#    - Sin mensajes de error
```

---

## 📦 Información de versión

| Componente | Versión mínima |
|---|---|
| GCC | 7.0+ |
| OpenMP | 4.5+ |
| GTK+ | 3.10+ (Linux/macOS) |
| GDI+ | Windows 7+ |
| Windows | Windows 7+ |
| Linux kernel | 4.0+ |

---

[[6-Arquitectura]] | [[Home]]
