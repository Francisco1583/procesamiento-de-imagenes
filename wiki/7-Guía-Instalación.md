# Guía de Instalación y Uso

## 🖥️ Requisitos previos

### Windows (MinGW/MSYS2)
- **Compilador:** GCC 9.0+ (con OpenMP)
- **Librerías:** GDI+ (incluido en Windows)
- **Herramientas:** MSYS2 o MinGW

### Linux
- **Compilador:** GCC 7.0+ (con OpenMP)
- **GTK3:** Development headers (`libgtk-3-dev`)
- **GdkPixbuf:** Incluido con GTK3

### macOS
- **Compilador:** GCC via Homebrew
- **GTK3:** Via Homebrew
- **Xcode Command Line Tools:** Recomendado

---

## ⚙️ Instalación de dependencias

### Windows (MSYS2)

1. **Abrir MSYS2 MinGW 64-bit**

2. **Actualizar repositorios:**
   ```bash
   pacman -Syu
   ```

3. **Instalar GCC y OpenMP:**
   ```bash
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-omp
   ```

4. **Verificar instalación:**
   ```bash
   gcc --version
   gcc -Q --help=warning | grep fopenmp
   ```

**Nota:** GDI+ viene con Windows, no requiere instalación.

---

### Linux (Debian/Ubuntu)

1. **Actualizar gestor de paquetes:**
   ```bash
   sudo apt update
   ```

2. **Instalar dependencias:**
   ```bash
   sudo apt install -y \
     build-essential \
     pkg-config \
     libgtk-3-dev \
     libgdk-pixbuf2.0-dev \
     libomp-dev
   ```

3. **Verificar instalación:**
   ```bash
   gcc --version
   pkg-config --modversion gtk+-3.0
   ```

---

### macOS (Homebrew)

1. **Instalar Homebrew** (si no está):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Instalar GTK3:**
   ```bash
   brew install gtk+3
   brew install pkg-config
   brew install libomp
   ```

3. **Verificar instalación:**
   ```bash
   gcc --version
   pkg-config --modversion gtk+-3.0
   ```

---

## 🔨 Compilación

### Windows (Release - Sin consola)

```powershell
cd "ruta\al\proyecto"

gcc interfaz_grafica.c `
  -o interfaz_grafica.exe `
  -fopenmp `
  -lgdiplus `
  -lgdi32 `
  -lcomdlg32 `
  -lshell32 `
  -mwindows

# Ejecutar
.\interfaz_grafica.exe
```

**Flags explicados:**
- `-fopenmp`: Habilita paralelización OpenMP
- `-lgdiplus`: GDI+ para renderizado de imágenes
- `-lgdi32`: Gráficos de Windows
- `-lcomdlg32`: Diálogos de archivo
- `-lshell32`: Integración Shell
- `-mwindows`: No abre consola

### Windows (Debug - Con consola)

```powershell
gcc interfaz_grafica.c `
  -o interfaz_grafica_debug.exe `
  -fopenmp `
  -lgdiplus `
  -lgdi32 `
  -lcomdlg32 `
  -lshell32 `
  -g

# Ejecutar (mostrará mensajes de depuración)
.\interfaz_grafica_debug.exe
```

---

### Linux

```bash
cd /ruta/al/proyecto

gcc interfaz_grafica.c \
  -o interfaz_grafica \
  -fopenmp \
  $(pkg-config --cflags --libs gtk+-3.0)

# Ejecutar
./interfaz_grafica
```

---

### macOS

```bash
cd /ruta/al/proyecto

gcc interfaz_grafica.c \
  -o interfaz_grafica \
  -fopenmp \
  $(pkg-config --cflags --libs gtk+-3.0)

# Ejecutar
./interfaz_grafica
```

---

## 🚀 Uso de la interfaz

### 1. **Seleccionar imágenes**

```
Interfaz
├── Botón "Add images..."
│   └── Se abre diálogo de selección múltiple
│       └── Seleccionar 1-10 archivos *.bmp
└── Botón "Clear"
    └── Vaciar lista de archivos
```

**Límites:**
- Máximo 10 imágenes simultáneamente
- Solo formato BMP (`.bmp`)
- Ruta completa se valida automáticamente

### 2. **Seleccionar transformaciones**

| Checkbox | Descripción | Resultado |
|---|---|---|
| ✓ 1 - Vertical grayscale | Inversión vertical en B&W | `*_inv_gray.bmp` |
| ✓ 2 - Vertical color | Inversión RGB vertical | `*_inv_color.bmp` |
| ✓ 3 - Horizontal grayscale | Espejo horizontal B&W | `*_flip_gray.bmp` |
| ✓ 4 - Horizontal color | Espejo horizontal RGB | `*_flip_color.bmp` |
| ✓ 5 - Grayscale blur | Desenfoque B&W | `*_blur_gray.bmp` |
| ✓ 6 - Color blur | Desenfoque RGB | `*_blur_color.bmp` |

**Botones rápidos:**
- **"All"**: Marca todas las transformaciones
- **"Clear"**: Desmarca todas

### 3. **Configurar parámetros**

| Campo | Rango | Default | Notas |
|---|---|---|---|
| OpenMP threads | 1–64 | 18 | Recomendado: 6, 12 o 18 |
| Kernel (grayscale) | Impar, 3+ | 27 | Debe ser número impar |
| Kernel (color) | Impar, 3+ | 27 | Debe ser número impar |

**Validación:**
- Si kernel es par → Se suma 1 automáticamente
- Si threads ≤ 0 → Se usa 18 por defecto
- Si kernel < 3 → Se usa 27 por defecto

### 4. **Ejecutar procesamiento**

```
Botón "Execute"
    ↓
Validar entrada
    ↓
Crear directorio img/
    ↓
Procesar todas las imágenes
    ↓
Mostrar diálogo de resultados
    ├── Tiempo transcurrido (segundos)
    ├── Archivos generados (N/total)
    └── Archivos saltados
```

### 5. **Verificar resultados**

Los archivos se guardan en: `img/` (relativo al ejecutable)

```
img/
├── imagen1_inv_gray.bmp
├── imagen1_inv_color.bmp
├── imagen1_flip_gray.bmp
├── imagen1_flip_color.bmp
├── imagen1_blur_gray.bmp
├── imagen1_blur_color.bmp
├── imagen2_inv_gray.bmp
├── imagen2_inv_color.bmp
... (y así para cada imagen)
```

---

## 📋 Ejemplos de uso

### Ejemplo 1: Conversión simple

```
1. Agregar: foto.bmp, retrato.bmp
2. Marcar: ✓ 1 - Vertical grayscale
3. Dejar: OpenMP threads = 18, Kernel = 27
4. Ejecutar
5. Resultado: foto_inv_gray.bmp, retrato_inv_gray.bmp
```

### Ejemplo 2: Procesamiento completo

```
1. Agregar: imagen1.bmp, imagen2.bmp, imagen3.bmp
2. Marcar: Todos (botón "All")
3. Configurar: threads = 18, kernel = 21
4. Ejecutar
5. Resultado: 18 archivos (3 imágenes × 6 transformaciones)
```

### Ejemplo 3: Ajuste fino para equipos lentos

```
1. Si tiempo > 30 segundos, probar:
   - Reducir hilos a 12
   - Reducir kernel a 15
   - Procesar 2-3 imágenes en lugar de 10
```

---

## 🔍 Solución de problemas

### El logo no aparece

**Síntomas:** Área en blanco donde debería estar el logo

**Causas y soluciones:**
```
❌ Archivo no encontrado:
   ✓ Verificar que src/logo.bmp existe
   ✓ Ejecutar desde carpeta del proyecto

❌ Formato incorrecto:
   ✓ Logo debe ser 24-bit BMP
   ✓ Verificar: Windows → propiedades → 866×650 píxeles

❌ En Windows, usar debug:
   ./interfaz_grafica_debug.exe
   └─ Mostrará ruta exacta que buscó
```

### Mensaje "Limit reached"

```
No puedo agregar más imágenes

Solución: Máximo 10 imágenes permitidas
✓ Agregar un segundo lote después de ejecutar
```

### Tiempo de ejecución muy alto

```
Si elapsed > 60 segundos:

1. Verificar especificaciones (ver Conclusiones):
   - ¿Laptop A? → Usar 2-3 imágenes máx
   - ¿Laptop B? → Considerar como maestro

2. Reducir paralelismo:
   threads = 12 (en lugar de 18)

3. Reducir kernel:
   kernel = 15 (en lugar de 27)

4. Monitorear temperatura:
   - Windows: Task Manager → Performance
   - Linux: watch -n 1 'sensors'
   - macOS: sudo powermetrics
```

### Archivos no se generan

```
GeneratedFiles: 0/12

Posibles causas:

1. Permisos en directorio:
   ls -la img/
   chmod 755 img/

2. Disco lleno:
   df -h (Linux/macOS)
   Get-PSDrive (Windows)

3. Archivo de entrada corrupto:
   file imagen.bmp
   hexdump -C imagen.bmp | head
```

---

## 📊 Validación post-compilación

### Test rápido (60 segundos)

```bash
# 1. Crear imagen de prueba (BMP simple 100×100)
# 2. Seleccionar solo esa imagen
# 3. Marcar solo "Vertical grayscale"
# 4. Ejecutar
# 5. Verificar que archivo se creó en img/
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
