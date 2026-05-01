#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
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

static void safe_copy(char *destination, size_t destination_size, const char *source) {
    if (destination_size == 0) {
        return;
    }
    strncpy(destination, source ? source : "", destination_size - 1);
    destination[destination_size - 1] = '\0';
}

static void normalize_kernel(int *kernel) {
    if (*kernel < 1) {
        *kernel = 1;
    }
    if ((*kernel % 2) == 0) {
        (*kernel)++;
    }
}

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
#endif
}

static void make_output_directory(char *output_directory, size_t output_directory_size) {
    char executable_directory[MAX_PATH];
    get_executable_directory(executable_directory, sizeof(executable_directory));

#if defined(_WIN32)
    SetCurrentDirectoryA(executable_directory);
    snprintf(output_directory, output_directory_size, "%s\\%s", executable_directory, k_output_folder_name);
    CreateDirectoryA(k_output_folder_name, NULL);
#endif
}

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

static RunStats run_transformations(const ImageSelection *selection, const TransformConfig *config, const char *output_directory) {
    RunStats stats;
    memset(&stats, 0, sizeof(stats));

    omp_set_num_threads(config->threads > 0 ? config->threads : 18);

    double start_time = omp_get_wtime();

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

        if (config->use_invert_gray) {
            snprintf(output_name, sizeof(output_name), "%s_inv_gray", safe_stem);
            inv_img(output_name, selection->files[i]);
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_invert_color) {
            snprintf(output_name, sizeof(output_name), "%s_inv_color", safe_stem);
            inv_img_color(output_name, selection->files[i]);
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_flip_gray) {
            snprintf(output_name, sizeof(output_name), "%s_flip_gray", safe_stem);
            inv_img_grey_horizontal(output_name, selection->files[i]);
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_flip_color) {
            snprintf(output_name, sizeof(output_name), "%s_flip_color", safe_stem);
            inv_img_color_horizontal(output_name, selection->files[i]);
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_blur_gray) {
            snprintf(output_name, sizeof(output_name), "%s_blur_gray", safe_stem);
            desenfoque(selection->files[i], output_name, config->kernel_gray);
            update_stats_for_output(&stats, output_directory, output_name);
        }

        if (config->use_blur_color) {
            snprintf(output_name, sizeof(output_name), "%s_blur_color", safe_stem);
            desenfoque_color(selection->files[i], output_name, config->kernel_color);
            update_stats_for_output(&stats, output_directory, output_name);
        }
    }

    stats.elapsed_seconds = omp_get_wtime() - start_time;
    return stats;
}

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
        char full_path[MAX_PATH];
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

static void execute_processing(HWND hwnd) {
    if (g_selection.count == 0) {
        MessageBoxA(hwnd, "Please add at least one BMP file.", "Missing input", MB_ICONWARNING | MB_OK);
        return;
    }

    TransformConfig config = {0};
    config.use_invert_gray = is_checked(g_checks[0]);
    config.use_invert_color = is_checked(g_checks[1]);
    config.use_flip_gray = is_checked(g_checks[2]);
    config.use_flip_color = is_checked(g_checks[3]);
    config.use_blur_gray = is_checked(g_checks[4]);
    config.use_blur_color = is_checked(g_checks[5]);
    config.threads = get_int_from_edit(g_edit_threads, 18);
    config.kernel_gray = get_int_from_edit(g_edit_kernel_gray, 27);
    config.kernel_color = get_int_from_edit(g_edit_kernel_color, 27);
    set_even_or_odd_kernel_text(&config.kernel_gray);
    set_even_or_odd_kernel_text(&config.kernel_color);

    if (!config.use_invert_gray && !config.use_invert_color && !config.use_flip_gray &&
        !config.use_flip_color && !config.use_blur_gray && !config.use_blur_color) {
        MessageBoxA(hwnd, "Select at least one transformation or click All.", "Missing option", MB_ICONWARNING | MB_OK);
        return;
    }

    // Keep relative output paths stable even after file chooser interactions.
    SetCurrentDirectoryA(g_executable_directory);
    make_output_directory(g_output_directory, sizeof(g_output_directory));
    SetWindowTextA(g_output_dir_label, g_output_directory);

    RunStats stats = run_transformations(&g_selection, &config, g_output_directory);
    update_runtime_label(stats.elapsed_seconds);

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

    CreateWindowA("STATIC", "Gray kernel:", WS_CHILD | WS_VISIBLE,
                  760, 165, 90, 20, hwnd, NULL, NULL, NULL);
    g_edit_kernel_gray = CreateWindowA("EDIT", "27", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                       850, 162, 70, 24, hwnd, (HMENU)ID_EDIT_KERNEL_1, NULL, NULL);

    CreateWindowA("STATIC", "Color kernel:", WS_CHILD | WS_VISIBLE,
                  760, 195, 90, 20, hwnd, NULL, NULL, NULL);
    g_edit_kernel_color = CreateWindowA("EDIT", "27", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                        850, 192, 70, 24, hwnd, (HMENU)ID_EDIT_KERNEL_2, NULL, NULL);

    CreateWindowA("STATIC", "OpenMP threads:", WS_CHILD | WS_VISIBLE,
                  500, 240, 110, 20, hwnd, NULL, NULL, NULL);
    g_edit_threads = CreateWindowA("EDIT", "18", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                                   615, 237, 70, 24, hwnd, (HMENU)ID_EDIT_THREADS, NULL, NULL);

    CreateWindowA("BUTTON", "Execute", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                  500, 285, 130, 35, hwnd, (HMENU)ID_BTN_EXEC, NULL, NULL);

    CreateWindowA("STATIC", "Output directory:", WS_CHILD | WS_VISIBLE,
                  20, 395, 110, 20, hwnd, NULL, NULL, NULL);
    g_output_dir_label = CreateWindowA("STATIC", g_output_directory, WS_CHILD | WS_VISIBLE,
                                       130, 395, 620, 20, hwnd, NULL, NULL, NULL);

    g_runtime_label = CreateWindowA("STATIC", "Execution time: not run yet", WS_CHILD | WS_VISIBLE,
                                    20, 425, 260, 20, hwnd, NULL, NULL, NULL);

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
                default:
                    return 0;
            }

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
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}

#endif