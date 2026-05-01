#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <gdiplus.h>
#else
#include <gtk/gtk.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#include "selec_proc.h"
#include "selec_proc_1.h"

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

#define MAX_IMAGES 10

typedef struct {
    char files[MAX_IMAGES][MAX_PATH];
    int count;
} ImageSelection;

typedef struct {
    int use_invert_gray;
    int use_invert_color;
    int use_flip_gray;
    int use_flip_color;
    int use_blur_gray;
    int use_blur_color;
    int threads;
    int kernel_gray;
    int kernel_color;
} TransformConfig;

typedef struct {
    double elapsed_seconds;
    int expected_outputs;
    int generated_outputs;
    int skipped_inputs;
} RunStats;

static const char *k_output_folder_name = "img";

/* Copia segura de strings evitando overflow y asegurando terminación nula
Se usa en todo el programa para prevenir vulnerabilidades de memoria */
static void safe_copy(char *destination, size_t destination_size, const char *source) {
    if (destination_size == 0) {
        return;
    }
    strncpy(destination, source ? source : "", destination_size - 1);
    destination[destination_size - 1] = '\0';
}

/* Asegura que el kernel sea válido para convolución:
- Mínimo 1
- Siempre impar (requisito en filtros como blur) */
static void normalize_kernel(int *kernel) {
    if (*kernel < 1) {
        *kernel = 1;
    }
    if ((*kernel % 2) == 0) {
        (*kernel)++;
    }
}

/* Obtiene el directorio donde se ejecuta el programa,
permitiendo trabajar con rutas relativas de forma consistente */
/* Obtiene el directorio donde se ejecuta el programa,
permitiendo trabajar con rutas relativas de forma consistente */
static void get_executable_directory(char *buffer, size_t buffer_size) {
#if defined(_WIN32)
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        safe_copy(buffer, buffer_size, ".");
        return;
    }
    char *last_slash = strrchr(path, '\\');
    if (last_slash) {
        *last_slash = '\0';
    }
    safe_copy(buffer, buffer_size, path);
#else
    // Implementación agregada para Linux / Unix
    char path[MAX_PATH];
    ssize_t count = readlink("/proc/self/exe", path, MAX_PATH);
    if (count != -1) {
        path[count] = '\0';
        char *last_slash = strrchr(path, '/');
        if (last_slash) {
            *last_slash = '\0';
        }
        safe_copy(buffer, buffer_size, path);
    } else {
        // Fallback por si readlink falla
        safe_copy(buffer, buffer_size, ".");
    }
#endif
}

/* Crea el directorio de salida "img" relativo al ejecutable.
Todas las funciones de transformación (inv_img, inv_img_color, etc.)
escriben sus resultados en este directorio.
Cambia el directorio de trabajo para garantizar que las rutas relativas funcionen. */
static void make_output_directory(char *output_directory, size_t output_directory_size) {
    char executable_directory[MAX_PATH];
    get_executable_directory(executable_directory, sizeof(executable_directory));

#if defined(_WIN32)
    SetCurrentDirectoryA(executable_directory);
    snprintf(output_directory, output_directory_size, "%s\\%s", executable_directory, k_output_folder_name);
    CreateDirectoryA(k_output_folder_name, NULL);
#else
    chdir(executable_directory);
    snprintf(output_directory, output_directory_size, "%s/%s", executable_directory, k_output_folder_name);
    mkdir(k_output_folder_name, 0755);
#endif
}

/* Extrae el nombre del archivo sin ruta ni extensión
Se usa para generar nombres de salida */
static const char *basename_without_extension(const char *path, char *buffer, size_t buffer_size) {
    const char *file_name = path;
    const char *slash_a = strrchr(path, '/');
    const char *slash_b = strrchr(path, '\\');
    const char *slash = slash_a;
    if (slash_b && (!slash || slash_b > slash)) {
        slash = slash_b;
    }
    if (slash) {
        file_name = slash + 1;
    }

    safe_copy(buffer, buffer_size, file_name);
    char *dot = strrchr(buffer, '.');
    if (dot) {
        *dot = '\0';
    }
    return buffer;
}

static void set_even_or_odd_kernel_text(int *value) {
    normalize_kernel(value);
}

static int file_exists(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

/* Convierte el nombre a un formato seguro para archivos:
// elimina caracteres inválidos y evita nombres vacíos */
static void make_safe_stem(const char *raw_stem, char *safe_stem, size_t safe_stem_size) {
    size_t limit = safe_stem_size > 0 ? safe_stem_size - 1 : 0;
    size_t out_idx = 0;

    for (size_t i = 0; raw_stem[i] != '\0' && out_idx < limit; i++) {
        unsigned char ch = (unsigned char)raw_stem[i];
        if (isalnum(ch) || ch == '_' || ch == '-') {
            safe_stem[out_idx++] = (char)ch;
        } else {
            safe_stem[out_idx++] = '_';
        }
    }

    if (out_idx == 0 && limit > 0) {
        safe_stem[out_idx++] = 'i';
        if (out_idx < limit) safe_stem[out_idx++] = 'm';
        if (out_idx < limit) safe_stem[out_idx++] = 'g';
    }

    safe_stem[out_idx] = '\0';
}

/* Verifica si el archivo de salida realmente fue creado:
Valida existencia física en disco */
static void update_stats_for_output(RunStats *stats, const char *output_directory, const char *output_name) {
    char output_file[MAX_PATH];
#if defined(_WIN32)
    snprintf(output_file, sizeof(output_file), "%s\\%s.bmp", output_directory, output_name);
#endif

    stats->expected_outputs++;
    if (file_exists(output_file)) {
        stats->generated_outputs++;
    }
}

/* CONEXIÓN UI ↔ BACKEND:
   Las funciones de transformación se importan de selec_proc.h y selec_proc_1.h:
   - inv_img() / inv_img_color()        → Inversión vertical (grayscale/color)
   - inv_img_grey_horizontal() / inv_img_color_horizontal() → Flip horizontal
   - desenfoque() / desenfoque_color()  → Blur/Convolución (grayscale/color)
   
   Cada función recibe:
     - output_name: Nombre base del archivo de salida (ej: "foto_inv_gray")
     - input_file: Ruta al BMP original
     - kernel (para blur): Tamaño de filtro
   
   Las funciones escriben automáticamente en el directorio "img" y retornan void.
   update_stats_for_output() verifica que el archivo se creó en disco.

   OpenMP se configura una sola vez al inicio con omp_set_num_threads()
   para paralelizar el trabajo dentro de cada función de transformación. */
static RunStats run_transformations(const ImageSelection *selection, const TransformConfig *config, const char *output_directory) {
    RunStats stats;
    memset(&stats, 0, sizeof(stats));

    omp_set_num_threads(config->threads > 0 ? config->threads : 18); // Configura número de hilos OpenMP

    double start_time = omp_get_wtime(); // Inicio de medición de tiempo usando OpenMP

    // Procesa cada imagen seleccionada por el usuario
    for (int i = 0; i < selection->count; i++) {
        FILE *input_test = fopen(selection->files[i], "rb");
        if (!input_test) {
            stats.skipped_inputs++;
            continue;
        }
        fclose(input_test);

        char raw_stem[MAX_PATH];
        char safe_stem[40];
        basename_without_extension(selection->files[i], raw_stem, sizeof(raw_stem));
        make_safe_stem(raw_stem, safe_stem, sizeof(safe_stem));

        char output_name[MAX_PATH];

        /* Llamadas al BACKEND: cada checkbox activado en la UI dispara una función de transformación */
        if (config->use_invert_gray) {
            snprintf(output_name, sizeof(output_name), "%s_inv_gray", safe_stem);
            inv_img(output_name, selection->files[i]); // Llama función del backend (selec_proc.h)
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_invert_color) {
            snprintf(output_name, sizeof(output_name), "%s_inv_color", safe_stem);
            inv_img_color(output_name, selection->files[i]); // Llama función del backend
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_flip_gray) {
            snprintf(output_name, sizeof(output_name), "%s_flip_gray", safe_stem);
            inv_img_grey_horizontal(output_name, selection->files[i]); // Llama función del backend
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_flip_color) {
            snprintf(output_name, sizeof(output_name), "%s_flip_color", safe_stem);
            inv_img_color_horizontal(output_name, selection->files[i]); // Llama función del backend
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_blur_gray) {
            snprintf(output_name, sizeof(output_name), "%s_blur_gray", safe_stem);
            desenfoque(selection->files[i], output_name, config->kernel_gray); // Llama función del backend
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_blur_color) {
            snprintf(output_name, sizeof(output_name), "%s_blur_color", safe_stem);
            desenfoque_color(selection->files[i], output_name, config->kernel_color); // Llama función del backend
            update_stats_for_output(&stats, output_directory, output_name);
        }
    }

    stats.elapsed_seconds = omp_get_wtime() - start_time; // Calcula tiempo total de ejecución
    return stats;
}

/* Logo rendering constants (shared between Windows and Linux/macOS) */
#define LOGO_CANVAS_WIDTH 180
#define LOGO_CANVAS_HEIGHT 150
#define LOGO_RIGHT_MARGIN 28
#define LOGO_BOTTOM_MARGIN 60

#if defined(_WIN32)

#define ID_BTN_ADD       101
#define ID_BTN_CLEAR     102
#define ID_BTN_ALL       103
#define ID_BTN_EXEC      104
#define ID_LIST_FILES    200
#define ID_EDIT_THREADS  201
#define ID_EDIT_KERNEL_1 202
#define ID_EDIT_KERNEL_2 203
#define ID_CHECK_INV_G   300
#define ID_CHECK_INV_C   301
#define ID_CHECK_H_G     302
#define ID_CHECK_H_C     303
#define ID_CHECK_BLUR_G  304
#define ID_CHECK_BLUR_C  305
#define ID_BTN_ABOUT     400
#define ID_STATIC_LOGO   401

static ImageSelection g_selection = {0};
static HWND g_list_files = NULL;
static HWND g_edit_threads = NULL;
static HWND g_edit_kernel_gray = NULL;
static HWND g_edit_kernel_color = NULL;
static HWND g_output_dir_label = NULL;
static HWND g_runtime_label = NULL;
static HWND g_checks[6] = {0};
static char g_output_directory[MAX_PATH] = {0};
static char g_executable_directory[MAX_PATH] = {0};
static HBITMAP g_logo_bitmap = NULL;
static ULONG_PTR g_gdiplus_token = 0;
static int g_gdiplus_started = 0;

static HBITMAP create_scaled_hbitmap_from_file_w(const wchar_t *image_path, int canvas_width, int canvas_height) {
    if (!image_path || canvas_width <= 0 || canvas_height <= 0) {
        return NULL;
    }

    if (!g_gdiplus_started) {
        GdiplusStartupInput gdiplusStartupInput = {0};
        gdiplusStartupInput.GdiplusVersion = 1;
        if (GdiplusStartup(&g_gdiplus_token, &gdiplusStartupInput, NULL) == 0) {
            g_gdiplus_started = 1;
        }
    }

    if (!g_gdiplus_started) {
        return NULL;
    }

    GpImage *source_image = NULL;
    if (GdipLoadImageFromFile(image_path, &source_image) != 0 || source_image == NULL) {
        return NULL;
    }

    UINT source_width = 0;
    UINT source_height = 0;
    if (GdipGetImageWidth(source_image, &source_width) != 0 || GdipGetImageHeight(source_image, &source_height) != 0 || source_width == 0 || source_height == 0) {
        GdipDisposeImage(source_image);
        return NULL;
    }

    GpBitmap *scaled_bitmap = NULL;
    if (GdipCreateBitmapFromScan0(canvas_width, canvas_height, 0, PixelFormat32bppARGB, NULL, &scaled_bitmap) != 0 || scaled_bitmap == NULL) {
        GdipDisposeImage(source_image);
        return NULL;
    }

    GpGraphics *graphics = NULL;
    if (GdipGetImageGraphicsContext((GpImage *)scaled_bitmap, &graphics) != 0 || graphics == NULL) {
        GdipDisposeImage((GpImage *)scaled_bitmap);
        GdipDisposeImage(source_image);
        return NULL;
    }

    GdipSetInterpolationMode(graphics, InterpolationModeHighQualityBicubic);
    GdipSetCompositingQuality(graphics, CompositingQualityHighQuality);
    GdipSetSmoothingMode(graphics, SmoothingModeHighQuality);
    GdipGraphicsClear(graphics, 0xFFFFFFFF);

    double scale_x = (double)canvas_width / (double)source_width;
    double scale_y = (double)canvas_height / (double)source_height;
    double scale = scale_x < scale_y ? scale_x : scale_y;
    int draw_width = (int)(source_width * scale + 0.5);
    int draw_height = (int)(source_height * scale + 0.5);
    int draw_x = (canvas_width - draw_width) / 2;
    int draw_y = (canvas_height - draw_height) / 2;

    GdipDrawImageRectI(graphics, source_image, draw_x, draw_y, draw_width, draw_height);

    HBITMAP result_bitmap = NULL;
    if (GdipCreateHBITMAPFromBitmap(scaled_bitmap, &result_bitmap, 0xFFFFFFFF) != 0) {
        result_bitmap = NULL;
    }

    GdipDeleteGraphics(graphics);
    GdipDisposeImage((GpImage *)scaled_bitmap);
    GdipDisposeImage(source_image);
    return result_bitmap;
}

static void refresh_file_list(void) {
    SendMessageA(g_list_files, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_selection.count; i++) {
        SendMessageA(g_list_files, LB_ADDSTRING, 0, (LPARAM)g_selection.files[i]);
    }
}

static int is_checked(HWND hwnd) {
    return SendMessageA(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

static int get_int_from_edit(HWND hwnd, int fallback) {
    char buffer[64];
    GetWindowTextA(hwnd, buffer, sizeof(buffer));
    char *endptr = NULL;
    long value = strtol(buffer, &endptr, 10);
    if (endptr == buffer || value <= 0) {
        return fallback;
    }
    return (int)value;
}

// Limita a MAX_IMAGES (10) para evitar sobrecarga
static void add_path_to_selection(HWND hwnd, const char *path) {
    if (g_selection.count >= MAX_IMAGES) {
        MessageBoxA(hwnd, "You can select up to 10 BMP files.", "Limit reached", MB_ICONWARNING | MB_OK);
        return;
    }

    for (int i = 0; i < g_selection.count; i++) {
        if (lstrcmpiA(g_selection.files[i], path) == 0) {
            return;
        }
    }

    safe_copy(g_selection.files[g_selection.count], sizeof(g_selection.files[g_selection.count]), path);
    g_selection.count++;
    refresh_file_list();
}

/* Manejo especial de buffer cuando se seleccionan múltiples archivos:
Formato: [carpeta]\0[file1]\0[file2]...\0\0 */
static void add_files_from_dialog(HWND hwnd) {
    char file_buffer[8192] = {0};
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = file_buffer;
    ofn.nMaxFile = sizeof(file_buffer);
    ofn.lpstrFilter = "BMP Images\0*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&ofn)) {
        return;
    }

    if (file_buffer[0] == '\0') {
        return;
    }

    size_t first_len = strlen(file_buffer);
    if (file_buffer[first_len + 1] == '\0') {
        add_path_to_selection(hwnd, file_buffer);
        return;
    }

    char folder[MAX_PATH];
    safe_copy(folder, sizeof(folder), file_buffer);

    char *cursor = file_buffer + first_len + 1;
    while (*cursor) {
        char full_path[MAX_PATH + 256];
        snprintf(full_path, sizeof(full_path), "%s\\%s", folder, cursor);
        add_path_to_selection(hwnd, full_path);
        cursor += strlen(cursor) + 1;
    }
}

static void clear_selection(void) {
    memset(&g_selection, 0, sizeof(g_selection));
    refresh_file_list();
}

static void set_all_checks(int checked) {
    for (int i = 0; i < 6; i++) {
        SendMessageA(g_checks[i], BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static void update_runtime_label(double elapsed_seconds) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Execution time: %.4f seconds", elapsed_seconds);
    SetWindowTextA(g_runtime_label, buffer);
}

/* CAPTURA DE DATOS DE LA UI → CONFIGURACIÓN DEL BACKEND
   Esta función es el "puente" entre la interfaz gráfica y las funciones de transformación:
   
   1. Lee estado de checkboxes (qué transformaciones seleccionar el usuario)
   2. Lee valores numéricos (hilos, tamaños de kernel)
   3. Construye un objeto TransformConfig
   4. Llama a run_transformations() que ejecuta todas las transformaciones
   5. Actualiza etiquetas de la UI con los resultados (tiempo, cantidad de archivos)
*/
static void execute_processing(HWND hwnd) {
    if (g_selection.count == 0) {
        MessageBoxA(hwnd, "Please add at least one BMP file.", "Missing input", MB_ICONWARNING | MB_OK);
        return;
    }

    /* Captura estado de cada control de la UI */
    TransformConfig config = {0};
    config.use_invert_gray = is_checked(g_checks[0]);      // Checkbox "1 - Vertical grayscale"
    config.use_invert_color = is_checked(g_checks[1]);    // Checkbox "2 - Vertical color"
    config.use_flip_gray = is_checked(g_checks[2]);       // Checkbox "3 - Horizontal grayscale"
    config.use_flip_color = is_checked(g_checks[3]);      // Checkbox "4 - Horizontal color"
    config.use_blur_gray = is_checked(g_checks[4]);       // Checkbox "5 - Grayscale blur"
    config.use_blur_color = is_checked(g_checks[5]);      // Checkbox "6 - Color blur"
    config.threads = get_int_from_edit(g_edit_threads, 18);           // Campo "OpenMP threads"
    config.kernel_gray = get_int_from_edit(g_edit_kernel_gray, 27);   // Campo "Kernel" (grayscale)
    config.kernel_color = get_int_from_edit(g_edit_kernel_color, 27); // Campo "Kernel" (color)
    set_even_or_odd_kernel_text(&config.kernel_gray);
    set_even_or_odd_kernel_text(&config.kernel_color);

    /* Valida que al menos una transformación esté seleccionada */
    if (!config.use_invert_gray && !config.use_invert_color && !config.use_flip_gray &&
        !config.use_flip_color && !config.use_blur_gray && !config.use_blur_color) {
        MessageBoxA(hwnd, "Select at least one transformation or click All.", "Missing option", MB_ICONWARNING | MB_OK);
        return;
    }

    /* Prepara el directorio de salida y cambios de trabajo */
    SetCurrentDirectoryA(g_executable_directory);
    make_output_directory(g_output_directory, sizeof(g_output_directory));
    SetWindowTextA(g_output_dir_label, g_output_directory);

    /* LLAMADA AL BACKEND: run_transformations() ejecuta todas las transformaciones
       usando los parámetros capturados de la UI */
    RunStats stats = run_transformations(&g_selection, &config, g_output_directory);
    update_runtime_label(stats.elapsed_seconds); 

    /* Muestra resultados: archivos generados, archivos saltados, tiempo total */
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
             g_output_directory);
    MessageBoxA(hwnd, message, "Completed", MB_ICONINFORMATION | MB_OK);
}

static void create_ui(HWND hwnd) {
    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    get_executable_directory(g_executable_directory, sizeof(g_executable_directory));
    make_output_directory(g_output_directory, sizeof(g_output_directory));

    CreateWindowA("STATIC", "BMP Image Processing", WS_CHILD | WS_VISIBLE,
                  20, 15, 220, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "Working directory:", WS_CHILD | WS_VISIBLE,
                  20, 45, 120, 20, hwnd, NULL, NULL, NULL);
    HWND workdir_label = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE,
                                       20, 65, 560, 20, hwnd, NULL, NULL, NULL);

    CreateWindowA("STATIC", "Selected BMP files (max 10):", WS_CHILD | WS_VISIBLE,
                  20, 100, 200, 20, hwnd, NULL, NULL, NULL);
    g_list_files = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
                                 20, 125, 440, 190, hwnd, (HMENU)ID_LIST_FILES, NULL, NULL);

    CreateWindowA("BUTTON", "Add images...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  20, 325, 120, 30, hwnd, (HMENU)ID_BTN_ADD, NULL, NULL);
    CreateWindowA("BUTTON", "Clear", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  150, 325, 80, 30, hwnd, (HMENU)ID_BTN_CLEAR, NULL, NULL);
    CreateWindowA("BUTTON", "All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  240, 325, 80, 30, hwnd, (HMENU)ID_BTN_ALL, NULL, NULL);

    CreateWindowA("STATIC", "Transformations", WS_CHILD | WS_VISIBLE,
                  500, 15, 140, 20, hwnd, NULL, NULL, NULL);

    g_checks[0] = CreateWindowA("BUTTON", "1 - Vertical grayscale", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                500, 45, 240, 24, hwnd, (HMENU)ID_CHECK_INV_G, NULL, NULL);
    g_checks[1] = CreateWindowA("BUTTON", "2 - Vertical color", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                500, 75, 240, 24, hwnd, (HMENU)ID_CHECK_INV_C, NULL, NULL);
    g_checks[2] = CreateWindowA("BUTTON", "3 - Horizontal grayscale", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                500, 105, 240, 24, hwnd, (HMENU)ID_CHECK_H_G, NULL, NULL);
    g_checks[3] = CreateWindowA("BUTTON", "4 - Horizontal color", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                500, 135, 240, 24, hwnd, (HMENU)ID_CHECK_H_C, NULL, NULL);
    g_checks[4] = CreateWindowA("BUTTON", "5 - Grayscale blur", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                500, 165, 240, 24, hwnd, (HMENU)ID_CHECK_BLUR_G, NULL, NULL);
    g_checks[5] = CreateWindowA("BUTTON", "6 - Color blur", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                500, 195, 240, 24, hwnd, (HMENU)ID_CHECK_BLUR_C, NULL, NULL);

    CreateWindowA("STATIC", "Kernel:", WS_CHILD | WS_VISIBLE,
                  760, 165, 90, 20, hwnd, NULL, NULL, NULL);
    g_edit_kernel_gray = CreateWindowA("EDIT", "27", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                       850, 162, 70, 24, hwnd, (HMENU)ID_EDIT_KERNEL_1, NULL, NULL);

    CreateWindowA("STATIC", "Kernel:", WS_CHILD | WS_VISIBLE,
                  760, 195, 90, 20, hwnd, NULL, NULL, NULL);
    g_edit_kernel_color = CreateWindowA("EDIT", "27", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                        850, 192, 70, 24, hwnd, (HMENU)ID_EDIT_KERNEL_2, NULL, NULL);

    CreateWindowA("STATIC", "OpenMP threads:", WS_CHILD | WS_VISIBLE,
                  500, 240, 110, 20, hwnd, NULL, NULL, NULL);
    g_edit_threads = CreateWindowA("EDIT", "18", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                   615, 237, 70, 24, hwnd, (HMENU)ID_EDIT_THREADS, NULL, NULL);

    CreateWindowA("BUTTON", "Execute", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                  500, 285, 130, 35, hwnd, (HMENU)ID_BTN_EXEC, NULL, NULL);

    CreateWindowA("BUTTON", "About", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                  640, 285, 80, 35, hwnd, (HMENU)ID_BTN_ABOUT, NULL, NULL);

    CreateWindowA("STATIC", "Output directory:", WS_CHILD | WS_VISIBLE,
                  20, 395, 110, 20, hwnd, NULL, NULL, NULL);
    g_output_dir_label = CreateWindowA("STATIC", g_output_directory, WS_CHILD | WS_VISIBLE,
                                       130, 395, 620, 20, hwnd, NULL, NULL, NULL);

    g_runtime_label = CreateWindowA("STATIC", "Execution time: not run yet", WS_CHILD | WS_VISIBLE,
                                    20, 425, 260, 20, hwnd, NULL, NULL, NULL);

    {
        wchar_t exe_w[MAX_PATH];
        if (GetModuleFileNameW(NULL, exe_w, MAX_PATH) > 0) {
            wchar_t *last = wcsrchr(exe_w, L'\\');
            if (last) *last = L'\0';
            wchar_t logo_path_w[MAX_PATH + 32];
            wcscpy_s(logo_path_w, MAX_PATH + 32, exe_w);
            wcscat_s(logo_path_w, MAX_PATH + 32, L"\\src\\logo.bmp");
            g_logo_bitmap = create_scaled_hbitmap_from_file_w(logo_path_w, LOGO_CANVAS_WIDTH, LOGO_CANVAS_HEIGHT);
        }

        if (!g_logo_bitmap) {
            char logo_path_a[MAX_PATH + 32];
            snprintf(logo_path_a, sizeof(logo_path_a), "%s\\src\\logo.bmp", g_executable_directory);
            wchar_t logo_path_w[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, logo_path_a, -1, logo_path_w, MAX_PATH);
            g_logo_bitmap = create_scaled_hbitmap_from_file_w(logo_path_w, LOGO_CANVAS_WIDTH, LOGO_CANVAS_HEIGHT);
        }

        if (g_logo_bitmap) {
            HWND logo_hwnd = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
                                           980 - LOGO_CANVAS_WIDTH - LOGO_RIGHT_MARGIN, 520 - LOGO_CANVAS_HEIGHT - LOGO_BOTTOM_MARGIN,
                                           LOGO_CANVAS_WIDTH, LOGO_CANVAS_HEIGHT, hwnd, (HMENU)ID_STATIC_LOGO, NULL, NULL);
            SendMessageA(logo_hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_logo_bitmap);
            InvalidateRect(hwnd, NULL, TRUE);
        } else {
            /* Debug: inform which paths were tried when logo failed to load */
            char tried_msg[1024] = "Tried paths:\n";
            char logo_path_a[MAX_PATH + 32];
            snprintf(logo_path_a, sizeof(logo_path_a), "%s\\src\\logo.bmp", g_executable_directory);
            strncat(tried_msg, logo_path_a, sizeof(tried_msg) - strlen(tried_msg) - 1);
            MessageBoxA(hwnd, tried_msg, "Logo not loaded", MB_OK | MB_ICONWARNING);
        }
    }

    SendMessageA(workdir_label, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageA(g_output_dir_label, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageA(g_runtime_label, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageA(g_list_files, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageA(g_edit_threads, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageA(g_edit_kernel_gray, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageA(g_edit_kernel_color, WM_SETFONT, (WPARAM)font, TRUE);

    char working_directory[MAX_PATH];
    get_executable_directory(working_directory, sizeof(working_directory));
    SetWindowTextA(workdir_label, working_directory);
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            create_ui(hwnd);
            DragAcceptFiles(hwnd, TRUE);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_ADD:
                    add_files_from_dialog(hwnd);
                    return 0;
                case ID_BTN_CLEAR:
                    clear_selection();
                    return 0;
                case ID_BTN_ALL:
                    set_all_checks(1);
                    return 0;
                case ID_BTN_EXEC:
                    execute_processing(hwnd);
                    return 0;
                case ID_BTN_ABOUT: {
                    const wchar_t about_text[] = L"TC3003\n"
                        L"Tecnol\u00f3gico de Monterrey\n"
                        L"Campus Puebla\n"
                        L"Mayo 2026\n\n"
                        L"- Alejandro Santana Moreno (A01733717)\n"
                        L"- Francisco Antonio Lopez Ricardez (A01737275)\n"
                        L"- Humberto P\u00e9rez Galindo (A01732526)\n"
                        L"- Yahel Alejandro Jim\u00e9nez Fern\u00e1ndez (A01736980)";
                    MessageBoxW(hwnd, about_text, L"About", MB_ICONINFORMATION | MB_OK);
                    return 0;
                }
                default:
                    return 0;
            }

        // Solo acepta archivos .bmp.
        case WM_DROPFILES: {
            HDROP drop = (HDROP)wParam;
            UINT file_count = DragQueryFileA(drop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < file_count && g_selection.count < MAX_IMAGES; i++) {
                char path[MAX_PATH];
                if (DragQueryFileA(drop, i, path, MAX_PATH)) {
                    size_t length = strlen(path);
                    if (length >= 4 && _stricmp(path + length - 4, ".bmp") == 0) {
                        add_path_to_selection(hwnd, path);
                    }
                }
            }
            DragFinish(drop);
            return 0;
        }

        case WM_DESTROY:
            if (g_logo_bitmap) {
                DeleteObject(g_logo_bitmap);
            }
            if (g_gdiplus_started) {
                GdiplusShutdown(g_gdiplus_token);
                g_gdiplus_started = 0;
            }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ImageProcessingGUI";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    /* Initialize GDI+ for image loading (used as a fallback) */
    {
        GdiplusStartupInput gdiplusStartupInput = {0};
        gdiplusStartupInput.GdiplusVersion = 1;
        if (GdiplusStartup(&g_gdiplus_token, &gdiplusStartupInput, NULL) == 0) {
            g_gdiplus_started = 1;
        }
    }

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Unable to register the window class.", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    HWND hwnd = CreateWindowA(
        wc.lpszClassName,
        "Image Processing",
        WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_SIZEBOX),
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        980,
        520,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxA(NULL, "Unable to create the main window.", "Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    /* Loop principal de eventos (message loop).
    Controla toda la interacción de la UI. */
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}

#else

typedef struct {
    GtkWidget *window;
    GtkWidget *file_view;
    GtkWidget *runtime_label;
    GtkWidget *output_label;
    GtkWidget *threads_entry;
    GtkWidget *kernel_gray_entry;
    GtkWidget *kernel_color_entry;
    GtkWidget *checks[6];
    ImageSelection selection;
    char output_directory[MAX_PATH];
} GtkAppState;

static GtkAppState g_app;

static void gtk_refresh_file_view(void) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_app.file_view));
    GString *text = g_string_new("");
    for (int i = 0; i < g_app.selection.count; i++) {
        g_string_append_printf(text, "%d. %s\n", i + 1, g_app.selection.files[i]);
    }
    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}

static void gtk_add_path_to_selection(const char *path) {
    if (g_app.selection.count >= MAX_IMAGES) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_app.window), GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                   "You can select up to 10 BMP files.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    for (int i = 0; i < g_app.selection.count; i++) {
        if (g_ascii_strcasecmp(g_app.selection.files[i], path) == 0) {
            return;
        }
    }

    safe_copy(g_app.selection.files[g_app.selection.count], sizeof(g_app.selection.files[g_app.selection.count]), path);
    g_app.selection.count++;
    gtk_refresh_file_view();
}

static void on_add_files_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select BMP images",
        GTK_WINDOW(g_app.window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Open", GTK_RESPONSE_ACCEPT,
        NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList *item = files; item != NULL; item = item->next) {
            char *path = (char *)item->data;
            size_t length = strlen(path);
            if (length >= 4 && g_ascii_strcasecmp(path + length - 4, ".bmp") == 0) {
                gtk_add_path_to_selection(path);
            }
            g_free(path);
        }
        g_slist_free(files);
    }

    gtk_widget_destroy(dialog);
}

static void on_clear_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    memset(&g_app.selection, 0, sizeof(g_app.selection));
    gtk_refresh_file_view();
}

static void on_all_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    for (int i = 0; i < 6; i++) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_app.checks[i]), TRUE);
    }
}

static int gtk_get_int_from_entry(GtkWidget *entry, int fallback) {
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    char *endptr = NULL;
    long value = strtol(text, &endptr, 10);
    if (endptr == text || value <= 0) {
        return fallback;
    }
    return (int)value;
}

static void gtk_update_runtime_label(double elapsed_seconds) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Execution time: %.4f seconds", elapsed_seconds);
    gtk_label_set_text(GTK_LABEL(g_app.runtime_label), buffer);
}

static void on_execute_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;

    if (g_app.selection.count == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_app.window), GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                   "Please add at least one BMP file.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    TransformConfig config = {0};
    config.use_invert_gray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_app.checks[0]));
    config.use_invert_color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_app.checks[1]));
    config.use_flip_gray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_app.checks[2]));
    config.use_flip_color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_app.checks[3]));
    config.use_blur_gray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_app.checks[4]));
    config.use_blur_color = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(g_app.checks[5]));
    config.threads = gtk_get_int_from_entry(g_app.threads_entry, 18);
    config.kernel_gray = gtk_get_int_from_entry(g_app.kernel_gray_entry, 27);
    config.kernel_color = gtk_get_int_from_entry(g_app.kernel_color_entry, 27);
    normalize_kernel(&config.kernel_gray);
    normalize_kernel(&config.kernel_color);

    if (!config.use_invert_gray && !config.use_invert_color && !config.use_flip_gray &&
        !config.use_flip_color && !config.use_blur_gray && !config.use_blur_color) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_app.window), GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                                                   "Select at least one transformation or click All.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    make_output_directory(g_app.output_directory, sizeof(g_app.output_directory));
    char output_text[MAX_PATH + 32];
    snprintf(output_text, sizeof(output_text), "Output directory: %s", g_app.output_directory);
    gtk_label_set_text(GTK_LABEL(g_app.output_label), output_text);

    RunStats stats = run_transformations(&g_app.selection, &config, g_app.output_directory);
    gtk_update_runtime_label(stats.elapsed_seconds);

    char summary[5120];  /* Buffer large enough for MAX_PATH + format string overhead */
    snprintf(summary, sizeof(summary),
             "Processing completed in %.4f seconds.\n"
             "Generated files: %d/%d\n"
             "Skipped input files: %d\n"
             "Output folder: %s",
             stats.elapsed_seconds,
             stats.generated_outputs,
             stats.expected_outputs,
             stats.skipped_inputs,
             g_app.output_directory);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_app.window), GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", summary);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void on_about_clicked(GtkButton *button, gpointer user_data) {
    (void)button; (void)user_data;
    const char *about_text = "TC3003\n"
        "Tecnol\u00f3gico de Monterrey\n"
        "Campus Puebla\n"
        "Mayo 2026\n\n"
        "- Alejandro Santana Moreno (A01733717)\n"
        "- Francisco Antonio Lopez Ricardez (A01737275)\n"
        "- Humberto P\u00e9rez Galindo (A01732526)\n"
        "- Yahel Alejandro Jim\u00e9nez Fern\u00e1ndez (A01736980)";

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(g_app.window), GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", about_text);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static GtkWidget *make_section_label(const char *text) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    return label;
}

static void build_gtk_ui(GtkApplication *app, gpointer user_data) {
    (void)user_data;

    memset(&g_app, 0, sizeof(g_app));
    make_output_directory(g_app.output_directory, sizeof(g_app.output_directory));

    g_app.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(g_app.window), "Image Processing");
    gtk_window_set_default_size(GTK_WINDOW(g_app.window), 1020, 560);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(g_app.window), root);
    gtk_container_set_border_width(GTK_CONTAINER(root), 14);

    GtkWidget *title = gtk_label_new("BMP Image Processing");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(root), title, FALSE, FALSE, 0);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(root), content, TRUE, TRUE, 0);

    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(content), left, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(left), make_section_label("Selected BMP files (max 10):"), FALSE, FALSE, 0);
    g_app.file_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(g_app.file_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(g_app.file_view), FALSE);
    gtk_widget_set_size_request(g_app.file_view, 420, 230);
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), g_app.file_view);
    gtk_box_pack_start(GTK_BOX(left), scrolled, TRUE, TRUE, 0);

    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(left), button_row, FALSE, FALSE, 0);
    GtkWidget *add_button = gtk_button_new_with_label("Add images...");
    GtkWidget *clear_button = gtk_button_new_with_label("Clear");
    GtkWidget *all_button = gtk_button_new_with_label("All");
    gtk_box_pack_start(GTK_BOX(button_row), add_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), clear_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_row), all_button, FALSE, FALSE, 0);

    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(content), right, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(right), make_section_label("Transformations"), FALSE, FALSE, 0);

    const char *labels[6] = {
        "1 - Vertical grayscale",
        "2 - Vertical color",
        "3 - Horizontal grayscale",
        "4 - Horizontal color",
        "5 - Grayscale blur",
        "6 - Color blur"
    };

    for (int i = 0; i < 6; i++) {
        g_app.checks[i] = gtk_check_button_new_with_label(labels[i]);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g_app.checks[i]), TRUE);
        gtk_box_pack_start(GTK_BOX(right), g_app.checks[i], FALSE, FALSE, 0);
    }

    GtkWidget *kernel_row = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(kernel_row), 8);
    gtk_grid_set_column_spacing(GTK_GRID(kernel_row), 10);
    gtk_box_pack_start(GTK_BOX(right), kernel_row, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(kernel_row), make_section_label("Kernel:"), 0, 0, 1, 1);
    g_app.kernel_gray_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_app.kernel_gray_entry), "27");
    gtk_grid_attach(GTK_GRID(kernel_row), g_app.kernel_gray_entry, 1, 0, 1, 1);

    gtk_grid_attach(GTK_GRID(kernel_row), make_section_label("Kernel:"), 0, 1, 1, 1);
    g_app.kernel_color_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_app.kernel_color_entry), "27");
    gtk_grid_attach(GTK_GRID(kernel_row), g_app.kernel_color_entry, 1, 1, 1, 1);

    gtk_grid_attach(GTK_GRID(kernel_row), make_section_label("OpenMP threads:"), 0, 2, 1, 1);
    g_app.threads_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(g_app.threads_entry), "18");
    gtk_grid_attach(GTK_GRID(kernel_row), g_app.threads_entry, 1, 2, 1, 1);

    GtkWidget *execute_button = gtk_button_new_with_label("Execute");
    gtk_box_pack_start(GTK_BOX(right), execute_button, FALSE, FALSE, 8);
    GtkWidget *about_button = gtk_button_new_with_label("About");
    gtk_box_pack_start(GTK_BOX(right), about_button, FALSE, FALSE, 0);

    g_app.output_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(g_app.output_label), 0.0f);
    char output_text[MAX_PATH + 32];
    snprintf(output_text, sizeof(output_text), "Output directory: %s", g_app.output_directory);
    gtk_label_set_text(GTK_LABEL(g_app.output_label), output_text);
    gtk_box_pack_start(GTK_BOX(root), g_app.output_label, FALSE, FALSE, 0);

    g_app.runtime_label = gtk_label_new("Execution time: not run yet");
    gtk_label_set_xalign(GTK_LABEL(g_app.runtime_label), 0.0f);
    gtk_box_pack_start(GTK_BOX(root), g_app.runtime_label, FALSE, FALSE, 0);

    /* Try to load the logo (src/logo.bmp) and show it on the right side */
    {
        char exe_dir[MAX_PATH];
        get_executable_directory(exe_dir, sizeof(exe_dir));
        char logo_path[MAX_PATH + 32];  /* Extra space for /src/logo.bmp suffix */
        snprintf(logo_path, sizeof(logo_path), "%s/src/logo.bmp", exe_dir);
        if (g_file_test(logo_path, G_FILE_TEST_EXISTS)) {
            GdkPixbuf *pix = gdk_pixbuf_new_from_file_at_scale(logo_path, LOGO_CANVAS_WIDTH, LOGO_CANVAS_HEIGHT, TRUE, NULL);
            if (pix) {
                GtkWidget *image = gtk_image_new_from_pixbuf(pix);
                gtk_widget_set_margin_end(image, LOGO_RIGHT_MARGIN);
                gtk_widget_set_margin_bottom(image, LOGO_BOTTOM_MARGIN);
                gtk_widget_set_size_request(image, LOGO_CANVAS_WIDTH, LOGO_CANVAS_HEIGHT);
                gtk_box_pack_end(GTK_BOX(content), image, FALSE, FALSE, 0);
                g_object_unref(pix);
            }
        }
    }

    gtk_refresh_file_view();

    g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_files_clicked), NULL);
    g_signal_connect(clear_button, "clicked", G_CALLBACK(on_clear_clicked), NULL);
    g_signal_connect(all_button, "clicked", G_CALLBACK(on_all_clicked), NULL);
    g_signal_connect(execute_button, "clicked", G_CALLBACK(on_execute_clicked), NULL);
    g_signal_connect(about_button, "clicked", G_CALLBACK(on_about_clicked), NULL);

    gtk_widget_show_all(g_app.window);
}

int main(int argc, char **argv) {
    GtkApplication *application = gtk_application_new("com.example.imageprocessing", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(application, "activate", G_CALLBACK(build_gtk_ui), NULL);
    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}

#endif
