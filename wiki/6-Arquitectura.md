# Arquitectura de la Interfaz Gráfica

## 🏗️ Estructura general

La interfaz gráfica es un **puente multiplataforma** que conecta la selección del usuario con funciones backend de procesamiento de imágenes.

```
┌────────────────────────────────────────────────────────────┐
│                  INTERFAZ GRÁFICA (UI)                     │
│              interfaz_grafica.c (multiplataforma)          │
├────────────────────────────────────────────────────────────┤
│ • Windows: WinAPI + GDI+ para renderizado                  │
│ • Linux/macOS: GTK3 + GdkPixbuf para renderizado           │
└────────────────────┬─────────────────────────────────────┘
                     │ Captura de parámetros
                     ▼
         ┌──────────────────────────┐
         │   TransformConfig struct │
         │ (selecciones del usuario)│
         └────────────┬─────────────┘
                      │
                      ▼
         ┌──────────────────────────┐
         │  run_transformations()   │
         │    (coordinador)         │
         └────────────┬─────────────┘
                      │ Llamadas al backend
                      ▼
    ┌─────────────────────────────────────┐
    │    BACKEND (Funciones kernel)       │
    │ selec_proc.h / selec_proc_1.h       │
    │                                     │
    │ • inv_img()                         │
    │ • inv_img_color()                   │
    │ • inv_img_grey_horizontal()         │
    │ • inv_img_color_horizontal()        │
    │ • desenfoque()                      │
    │ • desenfoque_color()                │
    └────────────┬────────────────────────┘
                 │ Escriben resultados
                 ▼
         ┌──────────────┐
         │   img/       │
         │ (Directorio) │
         └──────────────┘
```

---

## 🔄 Flujo de ejecución detallado

### 1. **Usuario abre la interfaz**

```c
create_ui(HWND hwnd)
```

**Qué sucede:**
- ✅ Se crea lista de archivos (máx 10 BMP)
- ✅ Se crean checkboxes para 6 transformaciones
- ✅ Se crean campos para hilos OpenMP y kernels
- ✅ Se carga y escala el logo (180×150 píxeles)
- ✅ Se inicializa el directorio `img/`

**Logo rendering:**
```
Windows: GDI+ (GdipLoadImageFromFile → escala proporcional → HBITMAP)
Linux/macOS: GdkPixbuf (gdk_pixbuf_new_from_file_at_scale)
Resultado: 180×150 sin distorsión, centrado en canvas
```

---

### 2. **Usuario selecciona archivos**

```c
add_files_from_dialog(HWND hwnd)
```

**Flujo:**
```
Usuario click "Add images..."
    ↓
Diálogo multiselector (OFN_ALLOWMULTISELECT)
    ↓
Validar formato (*.bmp)
    ↓
Agregar a ImageSelection struct
    ↓
Refrescar listbox (refresh_file_list)
```

**Estructura:**
```c
typedef struct {
    char files[MAX_IMAGES][MAX_PATH];  // Array de rutas
    int count;                          // Cantidad (max 10)
} ImageSelection;
```

---

### 3. **Usuario marca opciones**

| Control | Lectura | Valor |
|---|---|---|
| Checkboxes (6) | `is_checked(hwnd)` | 0 o 1 (checked) |
| Campo "OpenMP threads" | `get_int_from_edit()` | 1–64 (default 18) |
| Campo "Kernel grayscale" | `get_int_from_edit()` | 1–∞ (default 27) |
| Campo "Kernel color" | `get_int_from_edit()` | 1–∞ (default 27) |

**Validación de kernel:**
```c
set_even_or_odd_kernel_text()
// Si kernel es par, suma 1 (los filtros requieren kernel impar)
```

---

### 4. **Usuario hace clic "Execute"**

```c
execute_processing(HWND hwnd)
    ↓
1. Validar que hay archivos seleccionados
2. Capturar estado de cada checkbox
3. Construir TransformConfig
4. Validar que hay al menos 1 transformación activada
5. Cambiar directorio de trabajo
6. Crear/verificar carpeta img/
7. Llamar run_transformations()
8. Mostrar resultados
```

**TransformConfig struct:**
```c
typedef struct {
    int use_invert_gray;           // checkbox[0]
    int use_invert_color;          // checkbox[1]
    int use_flip_gray;             // checkbox[2]
    int use_flip_color;            // checkbox[3]
    int use_blur_gray;             // checkbox[4]
    int use_blur_color;            // checkbox[5]
    int threads;                   // campo threads
    int kernel_gray;               // campo kernel 1
    int kernel_color;              // campo kernel 2
} TransformConfig;
```

---

### 5. **run_transformations() coordina el backend**

```c
run_transformations(const ImageSelection *sel, 
                    const TransformConfig *cfg, 
                    const char *outdir)
```

**Paso 1: Configurar OpenMP**
```c
omp_set_num_threads(cfg->threads);  // 18 por default
double start = omp_get_wtime();
```

**Paso 2: Iterar cada imagen**
```c
for (int i = 0; i < selection->count; i++) {
    // Verificar que archivo existe
    // Extraer nombre sin extensión y sanitizar
    // Construir nombres de salida
    // LLAMAR FUNCIONES DEL BACKEND
}
```

**Paso 3: Llamadas al backend (condicional por checkbox)**

Si `config.use_invert_gray == true`:
```c
inv_img(output_name, selection->files[i]);
// Backend: Lee BMP, invierte píxeles verticalmente, escribe resultado
```

Si `config.use_blur_gray == true`:
```c
desenfoque(selection->files[i], output_name, config->kernel_gray);
// Backend: Lee BMP, aplica convolución, escribe resultado
```

**Todas las transformaciones disponibles:**

| Checkbox | Función backend | Parámetros | Salida |
|---|---|---|---|
| 1 - Vertical grayscale | `inv_img()` | output_name, input_path | `*_inv_gray.bmp` |
| 2 - Vertical color | `inv_img_color()` | output_name, input_path | `*_inv_color.bmp` |
| 3 - Horizontal grayscale | `inv_img_grey_horizontal()` | output_name, input_path | `*_flip_gray.bmp` |
| 4 - Horizontal color | `inv_img_color_horizontal()` | output_name, input_path | `*_flip_color.bmp` |
| 5 - Grayscale blur | `desenfoque()` | input_path, output_name, kernel | `*_blur_gray.bmp` |
| 6 - Color blur | `desenfoque_color()` | input_path, output_name, kernel | `*_blur_color.bmp` |

**Paso 4: Validar resultados**
```c
update_stats_for_output(&stats, outdir, output_name)
// Incrementa expected_outputs
// Si file_exists() → incrementa generated_outputs
```

**Paso 5: Calcular tiempo total**
```c
stats.elapsed_seconds = omp_get_wtime() - start;
return stats;
```

---

### 6. **Mostrar resultados al usuario**

```c
char message[512];
snprintf(message, sizeof(message),
    "Processing completed in %.4f seconds.\n"
    "Generated files: %d/%d\n"
    "Skipped input files: %d\n"
    "Output folder: %s",
    stats.elapsed_seconds,
    stats.generated_outputs,
    stats.expected_outputs,
    stats.skipped_inputs,
    outdir);
MessageBoxA(hwnd, message, "Completed", MB_ICONINFORMATION | MB_OK);
```

---

## 📊 Estadísticas retornadas

```c
typedef struct {
    int expected_outputs;      // Transformaciones × imágenes
    int generated_outputs;     // Archivos que existen en disco
    int skipped_inputs;        // Imágenes que no se procesaron
    double elapsed_seconds;    // Tiempo total (omp_get_wtime)
} RunStats;
```

**Ejemplo:**
- Seleccionar 3 imágenes
- Marcar 4 transformaciones
- `expected_outputs = 3 × 4 = 12`
- Si 1 imagen falló: `skipped_inputs = 1`, `generated_outputs = 8`

---

## 🔌 Puntos clave de conexión

### 1. **Captura de UI** → **TransformConfig**
```c
is_checked(g_checks[0])  // Checkbox → bool
get_int_from_edit(...)   // Campo de texto → int
```

### 2. **TransformConfig** → **run_transformations()**
```c
if (config.use_invert_gray) {
    inv_img(...);  // ← BACKEND
}
```

### 3. **Backend** → **Archivo de salida**
```c
inv_img(output_name, input_file);
// Escribe automáticamente a: img/output_name.bmp
```

### 4. **Validación** → **Estadísticas**
```c
file_exists("img/output_name.bmp")  // ✓ o ✗
→ stats.generated_outputs++
```

### 5. **OpenMP** → **Backend**
```c
omp_set_num_threads(18);  // Configurado en run_transformations()
// Las funciones backend usan el valor configurado
```

---

## 📁 Directorios y archivos

```
procesamiento-de-imagenes/
├── interfaz_grafica.c          ← UI principal
├── selec_proc.h                ← Backend grayscale
├── selec_proc_1.h              ← Backend color
├── src/
│   └── logo.bmp                ← Logo Tec (866×650, 24-bit)
│
├── img/                        ← Creado automáticamente
│   ├── imagen1_inv_gray.bmp
│   ├── imagen1_inv_color.bmp
│   ├── imagen1_flip_gray.bmp
│   ├── imagen1_flip_color.bmp
│   ├── imagen1_blur_gray.bmp
│   └── imagen1_blur_color.bmp
```

**Nota:** `make_output_directory()` crea `img/` si no existe, garantizando que las funciones backend encuentren la ruta.

---

## 🔐 Manejo de errores

```c
// Validación de entrada
if (g_selection.count == 0) 
    → "Please add at least one BMP file"

if (!config.use_invert_gray && ...) 
    → "Select at least one transformation"

// Validación de archivo
if (!fopen(file)) 
    → stats.skipped_inputs++

// Validación de salida
if (!file_exists(output_file)) 
    → No incrementa generated_outputs
```

---

[[5-Conclusiones]] | [[Home]] | [[7-Guía-Instalación]]
