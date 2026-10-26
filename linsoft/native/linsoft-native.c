#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <gio/gdesktopappinfo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <limits.h>
#include <locale.h>
#include <png.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TITLE 32
#define PANEL 52
#define MAX_CLIENTS 128
#define FILE_W 850
#define FILE_H 580
#define WORKSPACES 4
#define MAX_TABS 8
#define MAX_APPS 256
#define FILE_ROWS_Y 130

typedef struct {
    Window win, frame;
    Window edges[4], transient;
    int x, y, w, h, ox, oy, ow, oh;
    bool used, hidden, maximized, snapped;
    unsigned long workspace;
} Client;

typedef enum {
    ICON_LOGO, ICON_FOLDER, ICON_TERMINAL, ICON_BROWSER, ICON_DOCUMENT,
    ICON_SHEET, ICON_SETTINGS, ICON_APPS, ICON_SOUND, ICON_RESTART,
    ICON_POWER, ICON_FILE, ICON_IMAGE, ICON_AUDIO, ICON_VIDEO, ICON_ARCHIVE,
    ICON_HOME, ICON_UP, ICON_NEW_FOLDER, ICON_NETWORK, ICON_BATTERY, ICON_USB,
    ICON_COUNT
} Icon;

typedef struct {
    char *name;
    bool directory;
    Icon icon;
    bool marked;
} Entry;

typedef struct {
    char path[PATH_MAX], filter[256];
    char history[64][PATH_MAX];
    int history_count, history_position, scroll;
    bool show_hidden;
} FileTab;

static Display *dpy;
static int screen, sw, sh;
static Window root, desktop, panel, menu, files, support, context;
static XIM input_method;
static XIC files_input, menu_input;
static GC gc;
static XftFont *font, *large;
static XftColor ink, muted, accent;
static unsigned long bg, surface, raised, line, blue;
static unsigned long icon_cyan, icon_light, icon_green, icon_gold, icon_purple;
static Atom wm_delete, wm_protocols, active_atom, state_atom, max_h, max_v;
static Atom clients_atom, supported_atom, command_atom, utf8, name_atom, wm_state;
static Client clients[MAX_CLIENTS], *focused;
static bool running = true, startup_error, menu_open;
static char cwd[PATH_MAX], message[512];
static char **clipboard;
static size_t clipboard_count, clipboard_capacity;
static bool clipboard_move, file_job_move, file_job_cancellable, file_job_cancel_requested, show_hidden;
static char history[64][PATH_MAX];
static int history_count, history_position = -1;
static bool history_navigation;
static pid_t last_spawn, file_job;
static Entry *entries;
static size_t entry_count, entry_capacity;
static int selected = -1, scroll;
static char edit[PATH_MAX];
static int editing;
static bool edit_all_selected;
static FileTab tabs[MAX_TABS];
static int tab_count = 1, active_tab;
static char file_filter[256], menu_filter[256];
static GList *applications;
static GAppInfo *menu_apps[MAX_APPS];
static int menu_matches[MAX_APPS + 10], menu_count;
static unsigned long current_workspace;
static int file_drag_index = -1, file_drag_x, file_drag_y;
static bool file_drag_active;
static Pixmap preview;
static char preview_path[PATH_MAX];
static unsigned long last_click;
static int last_index = -1;
static Client *dragged;
static int drag_x, drag_y, start_x, start_y, start_w, start_h;
static bool resizing;
static int resize_edges;
static Window title_click_window;
static Time title_click_time;
static bool context_open;
static int context_kind, context_selected, menu_selected;
static int desktop_icon_size = 48, pinned_launcher = 1;
static Pixmap wallpaper;
static char wallpaper_path[PATH_MAX];
static volatile sig_atomic_t stopping;
static void draw_desktop(void);
static void draw_files(void);
static void files_key(XKeyEvent *event);
static void preferences(void);
static void draw_context(void);
static void draw_panel(void);
static void draw_menu(void);
static void status(const char *operation);
static void context_show(bool file_menu, int x, int y);
static void workspace_switch(unsigned long workspace);
static void workspace_move(Client *c, unsigned long workspace);
static void menu_search(void);
static void tab_switch(int index);

static Atom atom(const char *name) { return XInternAtom(dpy, name, False); }

static unsigned long color(const char *name) {
    XColor value, exact;
    if (!XAllocNamedColor(dpy, DefaultColormap(dpy, screen), name, &value, &exact)) {
        fprintf(stderr, "Linsoft Native: cannot allocate color %s\n", name);
        exit(1);
    }
    return value.pixel;
}

static unsigned long channel_pixel(unsigned char channel, unsigned long mask) {
    if (!mask) return 0;
    int shift = 0;
    while (!(mask & 1)) { mask >>= 1; ++shift; }
    return ((unsigned long)channel * mask / 255) << shift;
}

static bool wallpaper_dimensions_valid(png_uint_32 width, png_uint_32 height) {
    return width && height && (uint64_t)width * height <= 24000000;
}

static unsigned char *wallpaper_read(const char *path, png_image *image) {
    image->version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(image, path)) {
        snprintf(message, sizeof(message), "PNG pozadie: %.400s", image->message);
        fprintf(stderr, "Linsoft Native: %s\n", message);
        png_image_free(image); return NULL;
    }
    if (!wallpaper_dimensions_valid(image->width, image->height)) {
        snprintf(message, sizeof(message), "PNG pozadie je prilis velke (limit 24 megapixelov).");
        fprintf(stderr, "Linsoft Native: %s\n", message);
        png_image_free(image); return NULL;
    }
    image->format = PNG_FORMAT_RGB;
    unsigned char *pixels = malloc(PNG_IMAGE_SIZE(*image));
    if (!pixels) { status("Pamat pozadia"); png_image_free(image); return NULL; }
    if (!png_image_finish_read(image, NULL, pixels, 0, NULL)) {
        snprintf(message, sizeof(message), "PNG pozadie: %.400s", image->message);
        fprintf(stderr, "Linsoft Native: %s\n", message);
        free(pixels); png_image_free(image); return NULL;
    }
    return pixels;
}

static Pixmap png_pixmap(const char *path, int width, int height) {
    png_image image = {.version = PNG_IMAGE_VERSION};
    unsigned char *pixels = wallpaper_read(path, &image);
    if (!pixels) return None;
    XImage *output = XCreateImage(dpy, DefaultVisual(dpy, screen),
                                 (unsigned int)DefaultDepth(dpy, screen), ZPixmap, 0, NULL,
                                 (unsigned int)width, (unsigned int)height, 32, 0);
    if (!output) {
        errno = ENOMEM; status("Obraz pozadia");
        free(pixels); png_image_free(&image); return None;
    }
    output->data = calloc((size_t)height, (size_t)output->bytes_per_line);
    if (!output->data) {
        status("Pamat pozadia"); XDestroyImage(output);
        free(pixels); png_image_free(&image); return None;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t source_x = (size_t)x * image.width / (unsigned int)width;
            size_t source_y = (size_t)y * image.height / (unsigned int)height;
            unsigned char *p = pixels + (source_y * image.width + source_x) * 3;
            unsigned long pixel = channel_pixel(p[0], output->red_mask) |
                                  channel_pixel(p[1], output->green_mask) |
                                  channel_pixel(p[2], output->blue_mask);
            XPutPixel(output, x, y, pixel);
        }
    }
    Pixmap next = XCreatePixmap(dpy, root, (unsigned int)width, (unsigned int)height,
                               (unsigned int)DefaultDepth(dpy, screen));
    XPutImage(dpy, next, gc, output, 0, 0, 0, 0, (unsigned int)width, (unsigned int)height);
    XDestroyImage(output);
    free(pixels); png_image_free(&image);
    return next;
}

static Pixmap image_preview(const char *path) {
    int width, height;
    if (!gdk_pixbuf_get_file_info(path, &width, &height) || width <= 0 || height <= 0 ||
        (uint64_t)width * (unsigned int)height > 24000000) {
        snprintf(message, sizeof(message), "Náhľad: nepodporovaný obrázok alebo viac ako 24 megapixelov.");
        fprintf(stderr, "Linsoft Native: %s\n", message); return None;
    }
    GError *error = NULL;
    GdkPixbuf *image = gdk_pixbuf_new_from_file_at_scale(path, 200, 150, TRUE, &error);
    if (!image) {
        snprintf(message, sizeof(message), "Náhľad: %.400s", error ? error->message : "chyba dekodéra");
        fprintf(stderr, "Linsoft Native: %s\n", message);
        g_clear_error(&error); return None;
    }
    XImage *output = XCreateImage(dpy, DefaultVisual(dpy, screen),
                                  (unsigned int)DefaultDepth(dpy, screen), ZPixmap, 0, NULL, 200, 150, 32, 0);
    if (!output) {
        errno = ENOMEM; status("Obraz nahladu"); g_object_unref(image); return None;
    }
    output->data = calloc(150, (size_t)output->bytes_per_line);
    if (!output->data) {
        status("Pamat nahladu"); XDestroyImage(output); g_object_unref(image); return None;
    }
    int image_w = gdk_pixbuf_get_width(image), image_h = gdk_pixbuf_get_height(image);
    int channels = gdk_pixbuf_get_n_channels(image), stride = gdk_pixbuf_get_rowstride(image);
    const guchar *pixels = gdk_pixbuf_read_pixels(image);
    for (int y = 0; y < 150; ++y) {
        for (int x = 0; x < 200; ++x) {
            int ix = x - (200 - image_w) / 2, iy = y - (150 - image_h) / 2;
            unsigned long pixel = surface;
            if (ix >= 0 && ix < image_w && iy >= 0 && iy < image_h) {
                const guchar *p = pixels + iy * stride + ix * channels;
                pixel = channel_pixel(p[0], output->red_mask) |
                        channel_pixel(p[1], output->green_mask) | channel_pixel(p[2], output->blue_mask);
            }
            XPutPixel(output, x, y, pixel);
        }
    }
    Pixmap result = XCreatePixmap(dpy, root, 200, 150, (unsigned int)DefaultDepth(dpy, screen));
    XPutImage(dpy, result, gc, output, 0, 0, 0, 0, 200, 150);
    XDestroyImage(output); g_object_unref(image);
    return result;
}

static void wallpaper_load(void) {
    if (!*wallpaper_path) {
        if (wallpaper) XFreePixmap(dpy, wallpaper);
        wallpaper = None;
        return;
    }
    Pixmap next = png_pixmap(wallpaper_path, sw, sh);
    if (next) {
        if (wallpaper) XFreePixmap(dpy, wallpaper);
        wallpaper = next;
    }
}

static void preferences_load(void) {
    const char *home = getenv("HOME");
    const char *xdg = getenv("XDG_CONFIG_HOME");
    char path[PATH_MAX];
    int length = xdg && *xdg ? snprintf(path, sizeof(path), "%s/linsoft-native/desktop.conf", xdg) :
                 home ? snprintf(path, sizeof(path), "%s/.config/linsoft-native/desktop.conf", home) : -1;
    if (length < 0 || length >= (int)sizeof(path)) {
        errno = ENAMETOOLONG; status("Konfiguracia plochy"); return;
    }
    FILE *file = fopen(path, "r");
    if (!file) {
        if (errno != ENOENT) status("Citanie konfiguracie plochy");
        return;
    }
    int theme, accent_index, size, pin;
    char background[PATH_MAX], extra[2];
    bool valid = fscanf(file, "%d\n%d\n%d\n%d\n", &theme, &accent_index, &size, &pin) == 4 &&
                 fgets(background, sizeof(background), file) && !fgets(extra, sizeof(extra), file) &&
                 theme >= 0 && theme <= 2 && accent_index >= 0 && accent_index <= 2 &&
                 (size == 32 || size == 48 || size == 64) && pin >= 1 && pin <= 4;
    if (ferror(file)) valid = false;
    fclose(file);
    if (!valid) {
        snprintf(message, sizeof(message), "Neplatná konfigurácia plochy; opravte ju v Nastaveniach.");
        fprintf(stderr, "Linsoft Native: %s\n", message); return;
    }
    background[strcspn(background, "\n")] = '\0';
    if (strcmp(background, "-") && background[0] != '/') {
        snprintf(message, sizeof(message), "Pozadie vyzaduje absolutnu cestu PNG.");
        fprintf(stderr, "Linsoft Native: %s\n", message); return;
    }
    desktop_icon_size = size; pinned_launcher = pin;
    snprintf(wallpaper_path, sizeof(wallpaper_path), "%s", strcmp(background, "-") ? background : "");
    static const char *const backgrounds[] = {"#0b1425", "#18202a", "#16251f"};
    static const char *const accents[] = {"#54d4de", "#69dfb0", "#bca0f5"};
    bg = color(backgrounds[theme]); icon_cyan = color(accents[accent_index]);
    XftColorFree(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen), &accent);
    if (!XftColorAllocName(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen),
                           accents[accent_index], &accent)) {
        fprintf(stderr, "Linsoft Native: cannot allocate configured accent\n"); exit(1);
    }
    wallpaper_load();
}

static void rect(Window w, int x, int y, unsigned int width, unsigned int height,
                 unsigned long pixel) {
    XSetForeground(dpy, gc, pixel);
    XFillRectangle(dpy, w, gc, x, y, width, height);
}

static void text(Window w, int x, int y, const char *value, XftColor *fg, bool big) {
    XftDraw *draw = XftDrawCreate(dpy, w, DefaultVisual(dpy, screen),
                                DefaultColormap(dpy, screen));
    if (!draw) {
        fprintf(stderr, "Linsoft Native: XftDrawCreate failed\n");
        exit(1);
    }
    XRectangle clip = {0, 0, (unsigned short)sw, (unsigned short)sh};
    XftDrawSetClipRectangles(draw, 0, 0, &clip, 1);
    XftDrawStringUtf8(draw, fg, big ? large : font, x, y,
                     (const FcChar8 *)value, (int)strlen(value));
    XftDrawDestroy(draw);
}

static void icon_line(Drawable w, int x, int y, int size, int x1, int y1, int x2, int y2) {
    XDrawLine(dpy, w, gc, x + x1 * size / 32, y + y1 * size / 32,
              x + x2 * size / 32, y + y2 * size / 32);
}

static void icon_box(Drawable w, int x, int y, int size, int left, int top, int width, int height) {
    XDrawRectangle(dpy, w, gc, x + left * size / 32, y + top * size / 32,
                   (unsigned int)(width * size / 32), (unsigned int)(height * size / 32));
}

static void icon_arc(Drawable w, int x, int y, int size, int left, int top,
                     int width, int height, int start, int sweep) {
    XDrawArc(dpy, w, gc, x + left * size / 32, y + top * size / 32,
             (unsigned int)(width * size / 32), (unsigned int)(height * size / 32),
             start * 64, sweep * 64);
}

static void draw_icon(Drawable w, int x, int y, int size, Icon icon) {
    unsigned long tint = icon_cyan;
    if (icon == ICON_DOCUMENT || icon == ICON_FILE || icon == ICON_HOME) tint = icon_light;
    else if (icon == ICON_SHEET || icon == ICON_AUDIO || icon == ICON_SOUND) tint = icon_green;
    else if (icon == ICON_ARCHIVE || icon == ICON_APPS) tint = icon_gold;
    else if (icon == ICON_IMAGE || icon == ICON_VIDEO) tint = icon_purple;
    else if (icon == ICON_BATTERY) tint = icon_green;
    else if (icon == ICON_USB) tint = icon_gold;
    XSetForeground(dpy, gc, tint);
    XSetLineAttributes(dpy, gc, (unsigned int)(size >= 32 ? size / 14 : 2),
                       LineSolid, CapRound, JoinRound);
    switch (icon) {
        case ICON_LOGO:
            icon_line(w, x, y, size, 7, 5, 7, 25);
            icon_line(w, x, y, size, 7, 25, 24, 25);
            icon_line(w, x, y, size, 14, 5, 14, 18);
            icon_line(w, x, y, size, 14, 18, 25, 18);
            icon_line(w, x, y, size, 22, 5, 27, 10);
            break;
        case ICON_FOLDER: case ICON_NEW_FOLDER:
            icon_line(w, x, y, size, 4, 11, 4, 6);
            icon_line(w, x, y, size, 4, 6, 13, 6);
            icon_line(w, x, y, size, 13, 6, 17, 10);
            icon_line(w, x, y, size, 17, 10, 28, 10);
            icon_box(w, x, y, size, 4, 13, 24, 13);
            if (icon == ICON_NEW_FOLDER) {
                icon_line(w, x, y, size, 16, 16, 16, 23);
                icon_line(w, x, y, size, 12, 20, 20, 20);
            }
            break;
        case ICON_TERMINAL:
            icon_box(w, x, y, size, 3, 5, 26, 22);
            icon_line(w, x, y, size, 8, 11, 13, 16);
            icon_line(w, x, y, size, 13, 16, 8, 21);
            icon_line(w, x, y, size, 17, 21, 23, 21);
            break;
        case ICON_BROWSER:
            icon_arc(w, x, y, size, 4, 4, 24, 24, 0, 360);
            icon_arc(w, x, y, size, 11, 4, 10, 24, 0, 360);
            icon_line(w, x, y, size, 5, 12, 27, 12);
            icon_line(w, x, y, size, 5, 20, 27, 20);
            break;
        case ICON_NETWORK:
            icon_line(w, x, y, size, 8, 8, 16, 16);
            icon_line(w, x, y, size, 24, 8, 16, 16);
            icon_line(w, x, y, size, 16, 16, 16, 26);
            icon_arc(w, x, y, size, 3, 3, 10, 10, 0, 360);
            icon_arc(w, x, y, size, 19, 3, 10, 10, 0, 360);
            icon_arc(w, x, y, size, 11, 19, 10, 10, 0, 360);
            break;
        case ICON_BATTERY:
            icon_box(w, x, y, size, 3, 9, 25, 15);
            icon_box(w, x, y, size, 28, 13, 2, 7);
            icon_line(w, x, y, size, 8, 13, 8, 20);
            icon_line(w, x, y, size, 8, 20, 13, 20);
            icon_line(w, x, y, size, 13, 20, 13, 13);
            icon_line(w, x, y, size, 13, 13, 18, 13);
            break;
        case ICON_USB:
            icon_line(w, x, y, size, 16, 26, 16, 6);
            icon_line(w, x, y, size, 16, 13, 7, 13);
            icon_line(w, x, y, size, 7, 13, 7, 7);
            icon_line(w, x, y, size, 16, 18, 25, 18);
            icon_line(w, x, y, size, 25, 18, 25, 24);
            icon_line(w, x, y, size, 16, 9, 12, 5);
            icon_line(w, x, y, size, 16, 9, 20, 5);
            icon_arc(w, x, y, size, 4, 4, 6, 6, 0, 360);
            icon_box(w, x, y, size, 22, 22, 6, 6);
            break;
        case ICON_DOCUMENT: case ICON_SHEET: case ICON_FILE: case ICON_ARCHIVE:
            icon_line(w, x, y, size, 7, 4, 20, 4);
            icon_line(w, x, y, size, 20, 4, 26, 10);
            icon_line(w, x, y, size, 26, 10, 26, 28);
            icon_line(w, x, y, size, 26, 28, 7, 28);
            icon_line(w, x, y, size, 7, 28, 7, 4);
            icon_line(w, x, y, size, 20, 4, 20, 11);
            icon_line(w, x, y, size, 20, 11, 26, 11);
            if (icon == ICON_DOCUMENT) {
                for (int row = 15; row <= 23; row += 4)
                    icon_line(w, x, y, size, 11, row, row == 23 ? 18 : 22, row);
            } else if (icon == ICON_SHEET) {
                icon_box(w, x, y, size, 11, 15, 11, 9);
                icon_line(w, x, y, size, 16, 15, 16, 24);
                icon_line(w, x, y, size, 11, 19, 22, 19);
            } else if (icon == ICON_ARCHIVE) {
                for (int row = 9; row <= 22; row += 4)
                    icon_line(w, x, y, size, 13, row, 16, row);
                icon_box(w, x, y, size, 13, 23, 4, 3);
            }
            break;
        case ICON_SETTINGS:
            for (int column = 8; column <= 24; column += 8) {
                int knob = column == 16 ? 21 : 11;
                icon_line(w, x, y, size, column, 5, column, 27);
                XSetForeground(dpy, gc, icon_light);
                icon_box(w, x, y, size, column - 3, knob - 3, 6, 6);
                XSetForeground(dpy, gc, tint);
            }
            break;
        case ICON_APPS:
            for (int row = 5; row <= 19; row += 14)
                for (int column = 5; column <= 19; column += 14)
                    icon_box(w, x, y, size, column, row, 8, 8);
            break;
        case ICON_SOUND:
            icon_box(w, x, y, size, 4, 13, 6, 7);
            icon_line(w, x, y, size, 10, 13, 17, 7);
            icon_line(w, x, y, size, 17, 7, 17, 26);
            icon_line(w, x, y, size, 17, 26, 10, 20);
            icon_arc(w, x, y, size, 15, 9, 10, 16, -65, 130);
            icon_arc(w, x, y, size, 13, 5, 17, 24, -60, 120);
            break;
        case ICON_RESTART:
            icon_arc(w, x, y, size, 5, 5, 22, 22, 50, 285);
            icon_line(w, x, y, size, 24, 4, 24, 11);
            icon_line(w, x, y, size, 24, 11, 17, 11);
            break;
        case ICON_POWER:
            icon_arc(w, x, y, size, 5, 6, 22, 22, 135, 270);
            icon_line(w, x, y, size, 16, 3, 16, 16);
            break;
        case ICON_IMAGE:
            icon_box(w, x, y, size, 4, 5, 24, 22);
            icon_arc(w, x, y, size, 9, 9, 4, 4, 0, 360);
            icon_line(w, x, y, size, 5, 24, 13, 17);
            icon_line(w, x, y, size, 13, 17, 18, 21);
            icon_line(w, x, y, size, 18, 21, 23, 16);
            icon_line(w, x, y, size, 23, 16, 28, 21);
            break;
        case ICON_AUDIO:
            icon_line(w, x, y, size, 12, 23, 12, 8);
            icon_line(w, x, y, size, 12, 8, 25, 5);
            icon_line(w, x, y, size, 25, 5, 25, 21);
            icon_line(w, x, y, size, 12, 12, 25, 9);
            icon_arc(w, x, y, size, 5, 21, 7, 6, 0, 360);
            icon_arc(w, x, y, size, 18, 19, 7, 6, 0, 360);
            break;
        case ICON_VIDEO:
            icon_box(w, x, y, size, 4, 6, 24, 20);
            icon_line(w, x, y, size, 13, 11, 22, 16);
            icon_line(w, x, y, size, 22, 16, 13, 21);
            icon_line(w, x, y, size, 13, 21, 13, 11);
            break;
        case ICON_HOME:
            icon_line(w, x, y, size, 3, 15, 16, 4);
            icon_line(w, x, y, size, 16, 4, 29, 15);
            icon_line(w, x, y, size, 7, 13, 7, 27);
            icon_line(w, x, y, size, 7, 27, 25, 27);
            icon_line(w, x, y, size, 25, 27, 25, 13);
            icon_box(w, x, y, size, 13, 18, 6, 9);
            break;
        case ICON_UP:
            icon_line(w, x, y, size, 16, 27, 16, 5);
            icon_line(w, x, y, size, 16, 5, 6, 15);
            icon_line(w, x, y, size, 16, 5, 26, 15);
            break;
        case ICON_COUNT: break;
    }
    XSetLineAttributes(dpy, gc, 0, LineSolid, CapButt, JoinMiter);
}

static bool extension_is(const char *extension, const char *const values[], size_t count) {
    for (size_t i = 0; i < count; ++i)
        if (!strcasecmp(extension, values[i])) return true;
    return false;
}

static Icon file_icon(const char *name, bool directory) {
    if (directory) return ICON_FOLDER;
    const char *extension = strrchr(name, '.');
    if (!extension) return ICON_FILE;
    static const char *const documents[] = {".txt", ".md", ".pdf", ".odt", ".doc", ".docx", ".rtf"};
    static const char *const sheets[] = {".ods", ".xls", ".xlsx", ".csv"};
    static const char *const images[] = {".png", ".jpg", ".jpeg", ".svg", ".gif", ".webp", ".bmp"};
    static const char *const audio[] = {".mp3", ".wav", ".flac", ".ogg", ".m4a"};
    static const char *const video[] = {".mp4", ".mkv", ".webm", ".avi", ".mov"};
    static const char *const archives[] = {".zip", ".tar", ".gz", ".xz", ".bz2", ".7z", ".deb"};
    if (extension_is(extension, documents, sizeof(documents) / sizeof(documents[0]))) return ICON_DOCUMENT;
    if (extension_is(extension, sheets, sizeof(sheets) / sizeof(sheets[0]))) return ICON_SHEET;
    if (extension_is(extension, images, sizeof(images) / sizeof(images[0]))) return ICON_IMAGE;
    if (extension_is(extension, audio, sizeof(audio) / sizeof(audio[0]))) return ICON_AUDIO;
    if (extension_is(extension, video, sizeof(video) / sizeof(video[0]))) return ICON_VIDEO;
    if (extension_is(extension, archives, sizeof(archives) / sizeof(archives[0]))) return ICON_ARCHIVE;
    return ICON_FILE;
}

static Icon window_icon(Window window) {
    if (window == files) return ICON_FOLDER;
    XClassHint hint = {0};
    Icon icon = ICON_APPS;
    if (XGetClassHint(dpy, window, &hint)) {
        const char *name = hint.res_name ? hint.res_name : "";
        const char *class = hint.res_class ? hint.res_class : "";
        if (strcasestr(class, "xterm")) icon = ICON_TERMINAL;
        else if (strcasestr(class, "firefox")) icon = ICON_BROWSER;
        else if (strcasestr(name, "calc")) icon = ICON_SHEET;
        else if (strcasestr(name, "writer")) icon = ICON_DOCUMENT;
        else if (strcasestr(class, "libreoffice")) icon = ICON_DOCUMENT;
        else if (strcasestr(class, "pavucontrol")) icon = ICON_SOUND;
        else if (strcasestr(class, "zenity") || strcasestr(class, "settings")) icon = ICON_SETTINGS;
        if (hint.res_name) XFree(hint.res_name);
        if (hint.res_class) XFree(hint.res_class);
    }
    return icon;
}

static void status(const char *operation) {
    snprintf(message, sizeof(message), "%s: %s", operation, strerror(errno));
    fprintf(stderr, "Linsoft Native: %s\n", message);
}

static bool spawn(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        status("Spustenie aplikacie");
        draw_desktop(); draw_files();
        return false;
    }
    if (pid == 0) {
        close(ConnectionNumber(dpy));
        if (setsid() < 0) { perror("Linsoft setsid"); _exit(126); }
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    last_spawn = pid;
    return true;
}

static Client *client(Window w) {
    if (!w) return NULL;
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].used) {
            if (clients[i].win == w || clients[i].frame == w) return &clients[i];
            for (int edge = 0; edge < 4; ++edge)
                if (clients[i].edges[edge] == w) return &clients[i];
        }
    return NULL;
}

static void update_clients(void) {
    Window list[MAX_CLIENTS];
    int count = 0;
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].used) list[count++] = clients[i].win;
    XChangeProperty(dpy, root, clients_atom, XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)list, count);
}

static void get_title(Client *c, char *out, size_t length) {
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *value = NULL;
    if (XGetWindowProperty(dpy, c->win, name_atom, 0, 256, False, utf8,
                           &type, &format, &count, &remaining, &value) == Success &&
        value && format == 8 && count > 0) {
        snprintf(out, length, "%.*s", (int)count, (char *)value);
        XFree(value);
        return;
    }
    if (value) XFree(value);
    XTextProperty title = {0};
    if (XGetWMName(dpy, c->win, &title) && title.value) {
        char **list = NULL;
        int items = 0;
        int result = Xutf8TextPropertyToTextList(dpy, &title, &list, &items);
        if (result >= Success && list && items > 0) {
            snprintf(out, length, "%s", list[0]);
            XFreeStringList(list);
            XFree(title.value);
            return;
        }
        if (list) XFreeStringList(list);
        XFree(title.value);
        fprintf(stderr, "Linsoft Native: cannot decode window title (status %d)\n", result);
    }
    snprintf(out, length, "Aplikácia");
}

static void shorten(char *value, size_t limit) {
    if (strlen(value) <= limit) return;
    while (limit && ((unsigned char)value[limit] & 0xc0) == 0x80) --limit;
    strcpy(value + limit, "...");
}

static void draw_frame(Client *c) {
    char title[128];
    get_title(c, title, sizeof(title));
    rect(c->frame, 0, 0, (unsigned int)c->w, TITLE, c == focused ? raised : surface);
    rect(c->frame, 0, TITLE - 2, (unsigned int)c->w, 2, c == focused ? blue : line);
    shorten(title, 45);
    draw_icon(c->frame, 8, 6, 20, window_icon(c->win));
    text(c->frame, 36, 22, title, &ink, false);
    rect(c->frame, c->w - 102, 0, 102, TITLE - 2, surface);
    text(c->frame, c->w - 90, 21, "_", &muted, false);
    text(c->frame, c->w - 57, 22, "[]", &muted, false);
    text(c->frame, c->w - 23, 22, "x", &ink, false);
}

static void draw_panel(void) {
    bool compact = sw < 1800;
    int task_limit = sw - (compact ? 550 : 600);
    int task_width = compact ? 148 : 174;
    int task_step = compact ? 156 : 182;
    rect(panel, 0, 0, (unsigned int)sw, PANEL, surface);
    rect(panel, 0, 0, (unsigned int)sw, 2, line);
    rect(panel, 10, 8, 102, 36, blue);
    draw_icon(panel, 17, 14, 24, ICON_LOGO);
    text(panel, 48, 32, "ŠTART", &ink, false);
    draw_icon(panel, 124, 15, 24, ICON_FOLDER);
    text(panel, 153, 32, "Súbory", &ink, false);
    const Icon pins[] = {ICON_FOLDER, ICON_TERMINAL, ICON_BROWSER, ICON_DOCUMENT, ICON_SHEET};
    const char *const labels[] = {"Súbory", "Terminál", "Internet", "Writer", "Calc"};
    draw_icon(panel, 212, 15, 24, pins[pinned_launcher]);
    text(panel, 241, 32, labels[pinned_launcher], &ink, false);
    int x = 330;
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        Client *c = &clients[i];
        if (!c->used || c->workspace != current_workspace || x + task_step > task_limit) continue;
        char title[128];
        get_title(c, title, sizeof(title));
        shorten(title, compact ? 12 : 15);
        rect(panel, x, 8, (unsigned int)task_width, 36, c == focused && !c->hidden ? raised : bg);
        draw_icon(panel, x + 6, 16, 22, window_icon(c->win));
        text(panel, x + 34, 32, title, c->hidden ? &muted : &ink, false);
        x += task_step;
    }
    time_t now = time(NULL);
    struct tm local;
    char clock[64];
    if (localtime_r(&now, &local)) {
        strftime(clock, sizeof(clock), "%d.%m. %H:%M", &local);
        text(panel, sw - 120, 32, clock, &ink, false);
    }
    char volume[16] = "?", network[16] = "?", battery[16] = "?", usb_status[16] = "?";
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *value = NULL;
    if (XGetWindowProperty(dpy, root, atom("_LINSOFT_NATIVE_STATUS"), 0, 128, False, AnyPropertyType,
                          &type, &format, &count, &remaining, &value) == Success && value && format == 8) {
        if (sscanf((char *)value, "%15s %15s %15s %15s", volume, network, battery, usb_status) != 4)
            fprintf(stderr, "Linsoft Native: invalid system panel status\n");
    }
    if (value) XFree(value);
    int status_left = sw - (compact ? 545 : 582);
    draw_icon(panel, status_left, 4, 19, ICON_SOUND);
    text(panel, status_left - 4, 45, volume, &muted, false);
    draw_icon(panel, status_left + 55, 4, 19, ICON_NETWORK);
    text(panel, status_left + 51, 45, network, &muted, false);
    draw_icon(panel, status_left + 110, 4, 19, ICON_BATTERY);
    text(panel, status_left + 106, 45, battery, &muted, false);
    draw_icon(panel, status_left + 165, 4, 19, ICON_USB);
    text(panel, status_left + 161, 45, usb_status, &muted, false);
    draw_icon(panel, sw - (compact ? 330 : 350), 15, 20, ICON_DOCUMENT);
    draw_icon(panel, sw - (compact ? 296 : 318), 15, 24, ICON_SETTINGS);
    int workspace_left = sw - (compact ? 264 : 280);
    int workspace_width = compact ? 24 : 28;
    for (int i = 0; i < WORKSPACES; ++i) {
        char number[2] = {(char)('1' + i), '\0'};
        int button_x = workspace_left + i * workspace_width;
        if ((unsigned long)i == current_workspace)
            rect(panel, button_x, 8, (unsigned int)(workspace_width - 1), 36, raised);
        text(panel, button_x + (compact ? 9 : 10), 32, number, &accent, false);
    }
}

static void wm_state_set(Client *c, long state) {
    long data[] = {state, None};
    XChangeProperty(dpy, c->win, wm_state, wm_state, 32, PropModeReplace,
                    (unsigned char *)data, 2);
}

static void workarea_update(void) {
    long areas[WORKSPACES * 4];
    for (int i = 0; i < WORKSPACES; ++i) {
        areas[i * 4] = 0; areas[i * 4 + 1] = 0;
        areas[i * 4 + 2] = sw; areas[i * 4 + 3] = sh - PANEL;
    }
    XChangeProperty(dpy, root, atom("_NET_WORKAREA"), XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)areas, WORKSPACES * 4);
}

static bool window_has_atom(Window win, Atom property, Atom expected) {
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *value = NULL;
    bool found = false;
    if (XGetWindowProperty(dpy, win, property, 0, 64, False, XA_ATOM,
                          &type, &format, &count, &remaining, &value) == Success &&
        value && type == XA_ATOM && format == 32) {
        for (unsigned long i = 0; i < count; ++i)
            if (((Atom *)value)[i] == expected) found = true;
    }
    if (value) XFree(value);
    return found;
}

static void focus(Client *c) {
    if (!c || !c->used) return;
    if (c->workspace != current_workspace) workspace_switch(c->workspace);
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        Client *child = &clients[i];
        if (!child->used || child->transient != c->win || child->hidden) continue;
        if (window_has_atom(child->win, state_atom, atom("_NET_WM_STATE_MODAL"))) {
            c = child; break;
        }
    }
    Client *previous = focused;
    c->hidden = false;
    XMapWindow(dpy, c->frame);
    XRaiseWindow(dpy, c->frame);
    XRaiseWindow(dpy, panel);
    if (menu_open) XRaiseWindow(dpy, menu);
    XWMHints *hints = XGetWMHints(dpy, c->win);
    if (!hints || !(hints->flags & InputHint) || hints->input)
        XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
    if (hints) XFree(hints);
    if (files_input) {
        if (c->win == files) XSetICFocus(files_input);
        else XUnsetICFocus(files_input);
    }
    XChangeProperty(dpy, root, active_atom, XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&c->win, 1);
    wm_state_set(c, NormalState);
    Atom take_focus = atom("WM_TAKE_FOCUS");
    Atom *protocols = NULL;
    int count = 0;
    if (XGetWMProtocols(dpy, c->win, &protocols, &count)) {
        for (int i = 0; i < count; ++i) {
            if (protocols[i] == take_focus) {
                XEvent event = {0};
                event.xclient.type = ClientMessage;
                event.xclient.window = c->win;
                event.xclient.message_type = wm_protocols;
                event.xclient.format = 32;
                event.xclient.data.l[0] = (long)take_focus;
                event.xclient.data.l[1] = CurrentTime;
                XSendEvent(dpy, c->win, False, NoEventMask, &event);
            }
        }
        XFree(protocols);
    }
    focused = c;
    if (previous && previous->used) draw_frame(previous);
    draw_frame(c);
    draw_panel();
}

static void workspace_switch(unsigned long workspace) {
    if (workspace >= WORKSPACES || workspace == current_workspace) return;
    current_workspace = workspace;
    focused = NULL;
    Window none = None;
    XChangeProperty(dpy, root, active_atom, XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&none, 1);
    XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    XChangeProperty(dpy, root, atom("_NET_CURRENT_DESKTOP"), XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&current_workspace, 1);
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        Client *c = &clients[i];
        if (!c->used) continue;
        if (c->workspace == current_workspace && !c->hidden) XMapWindow(dpy, c->frame);
        else XUnmapWindow(dpy, c->frame);
    }
    for (size_t i = MAX_CLIENTS; i > 0; --i) {
        Client *c = &clients[i - 1];
        if (c->used && c->workspace == current_workspace && !c->hidden) { focus(c); break; }
    }
    draw_panel();
}

static void workspace_move(Client *c, unsigned long workspace) {
    if (!c || workspace >= WORKSPACES || c->workspace == workspace) return;
    c->workspace = workspace;
    XChangeProperty(dpy, c->win, atom("_NET_WM_DESKTOP"), XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)&workspace, 1);
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].used && clients[i].transient == c->win) workspace_move(&clients[i], workspace);
    if (workspace != current_workspace) {
        XUnmapWindow(dpy, c->frame);
        if (focused == c) {
            focused = NULL;
            Window none = None;
            XChangeProperty(dpy, root, active_atom, XA_WINDOW, 32, PropModeReplace,
                            (unsigned char *)&none, 1);
            XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        }
    } else if (!c->hidden) XMapWindow(dpy, c->frame);
    draw_panel();
}

static void geometry(Client *c) {
    if (!c->maximized && !c->snapped) {
        XSizeHints hints = {0};
        long supplied;
        if (XGetWMNormalHints(dpy, c->win, &hints, &supplied)) {
            if (hints.flags & PMinSize) {
                if (c->w < hints.min_width) c->w = hints.min_width;
                if (c->h < hints.min_height) c->h = hints.min_height;
            }
            if (hints.flags & PMaxSize) {
                if (hints.max_width > 0 && c->w > hints.max_width) c->w = hints.max_width;
                if (hints.max_height > 0 && c->h > hints.max_height) c->h = hints.max_height;
            }
            if (hints.flags & PResizeInc) {
                int base_w = hints.flags & PBaseSize ? hints.base_width :
                             hints.flags & PMinSize ? hints.min_width : 0;
                int base_h = hints.flags & PBaseSize ? hints.base_height :
                             hints.flags & PMinSize ? hints.min_height : 0;
                if (hints.width_inc > 0 && c->w > base_w)
                    c->w -= (c->w - base_w) % hints.width_inc;
                if (hints.height_inc > 0 && c->h > base_h)
                    c->h -= (c->h - base_h) % hints.height_inc;
            }
        }
    }
    if (c->w < 100) c->w = 100;
    if (c->h < 60) c->h = 60;
    if (c == dragged && resizing) {
        if (resize_edges & 1) c->x = start_x + start_w - c->w;
        if (resize_edges & 4) c->y = start_y + start_h - c->h;
    }
    XMoveResizeWindow(dpy, c->frame, c->x, c->y, (unsigned int)c->w,
                      (unsigned int)(c->h + TITLE));
    XMoveResizeWindow(dpy, c->win, 0, TITLE, (unsigned int)c->w, (unsigned int)c->h);
    if (c->edges[0]) {
        XMoveResizeWindow(dpy, c->edges[0], 0, TITLE, 5, (unsigned int)c->h);
        XMoveResizeWindow(dpy, c->edges[1], c->w - 5, TITLE, 5, (unsigned int)c->h);
        XMoveResizeWindow(dpy, c->edges[2], 0, 0, (unsigned int)(c->w > 104 ? c->w - 103 : 1), 4);
        XMoveResizeWindow(dpy, c->edges[3], 0, c->h + TITLE - 5, (unsigned int)c->w, 5);
    }
    XEvent event = {0};
    event.xconfigure.type = ConfigureNotify;
    event.xconfigure.display = dpy;
    event.xconfigure.event = c->win;
    event.xconfigure.window = c->win;
    event.xconfigure.x = c->x;
    event.xconfigure.y = c->y + TITLE;
    event.xconfigure.width = c->w;
    event.xconfigure.height = c->h;
    XSendEvent(dpy, c->win, False, StructureNotifyMask, &event);
    draw_frame(c);
}

static void frame_extents(Window window) {
    long extents[] = {1, 1, TITLE + 1, 1};
    if (window_has_atom(window, atom("_NET_WM_WINDOW_TYPE"), atom("_NET_WM_WINDOW_TYPE_NOTIFICATION")))
        memset(extents, 0, sizeof(extents));
    XChangeProperty(dpy, window, atom("_NET_FRAME_EXTENTS"), XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *)extents, 4);
}

static void maximize_state(Client *c) {
    Atom type;
    int format;
    unsigned long count, remaining;
    unsigned char *value = NULL;
    Atom states[258];
    int kept = 0;
    if (XGetWindowProperty(dpy, c->win, state_atom, 0, 256, False, XA_ATOM,
                          &type, &format, &count, &remaining, &value) == Success &&
        value && type == XA_ATOM && format == 32) {
        for (unsigned long i = 0; i < count; ++i) {
            Atom state = ((Atom *)value)[i];
            if (state != max_h && state != max_v) states[kept++] = state;
        }
    }
    if (value) XFree(value);
    if (c->maximized) { states[kept++] = max_h; states[kept++] = max_v; }
    XChangeProperty(dpy, c->win, state_atom, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)states, kept);
}

static void maximize(Client *c) {
    if (!c) return;
    if (!c->maximized) {
        if (!c->snapped) {
            c->ox = c->x; c->oy = c->y; c->ow = c->w; c->oh = c->h;
        }
        c->x = 0; c->y = 0; c->w = sw; c->h = sh - PANEL - TITLE;
    } else {
        c->x = c->ox; c->y = c->oy; c->w = c->ow; c->h = c->oh;
    }
    c->maximized = !c->maximized;
    c->snapped = false;
    maximize_state(c);
    geometry(c);
}

static void snap(Client *c, bool right) {
    if (!c) return;
    if (!c->snapped && !c->maximized) {
        c->ox = c->x; c->oy = c->y; c->ow = c->w; c->oh = c->h;
    }
    c->maximized = false; c->snapped = true;
    maximize_state(c);
    c->x = right ? sw / 2 : 0; c->y = 0;
    c->w = right ? sw - sw / 2 : sw / 2; c->h = sh - PANEL - TITLE;
    geometry(c);
}

static void minimize(Client *c) {
    if (!c) return;
    c->hidden = true;
    XUnmapWindow(dpy, c->frame);
    wm_state_set(c, IconicState);
    if (focused == c) {
        focused = NULL;
        Window none = None;
        XChangeProperty(dpy, root, active_atom, XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *)&none, 1);
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    }
    draw_panel();
}

static void close_client(Client *c) {
    if (!c) return;
    if (c->win == files) { minimize(c); return; }
    Atom *protocols = NULL;
    int count = 0;
    bool supported = false;
    if (XGetWMProtocols(dpy, c->win, &protocols, &count)) {
        for (int i = 0; i < count; ++i) supported |= protocols[i] == wm_delete;
        XFree(protocols);
    }
    if (supported) {
        XEvent event = {0};
        event.xclient.type = ClientMessage;
        event.xclient.window = c->win;
        event.xclient.message_type = wm_protocols;
        event.xclient.format = 32;
        event.xclient.data.l[0] = (long)wm_delete;
        event.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, c->win, False, NoEventMask, &event);
    } else {
        XKillClient(dpy, c->win);
    }
}

static void manage(Window win) {
    if (client(win)) { focus(client(win)); return; }
    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, win, &attr) || attr.override_redirect) return;
    if (window_has_atom(win, atom("_NET_WM_WINDOW_TYPE"), atom("_NET_WM_WINDOW_TYPE_NOTIFICATION"))) {
        frame_extents(win);
        XMapRaised(dpy, win);
        return;
    }
    Client *c = NULL;
    size_t index = 0;
    for (; index < MAX_CLIENTS; ++index)
        if (!clients[index].used) { c = &clients[index]; break; }
    if (!c) {
        fprintf(stderr, "Linsoft Native: window limit reached\n");
        XMapWindow(dpy, win);
        return;
    }
    *c = (Client){.win = win, .used = true, .workspace = current_workspace};
    c->w = attr.width < 200 ? 200 : attr.width;
    c->h = attr.height < 100 ? 100 : attr.height;
    if (c->w > sw) c->w = sw;
    if (c->h > sh - PANEL - TITLE) c->h = sh - PANEL - TITLE;
    c->x = (sw - c->w) / 2 + (int)(index % 4) * 18;
    c->y = (sh - PANEL - c->h - TITLE) / 2 + (int)(index % 4) * 18;
    XGetTransientForHint(dpy, win, &c->transient);
    Client *parent = client(c->transient);
    if (parent) {
        c->workspace = parent->workspace;
        c->x = parent->x + (parent->w - c->w) / 2;
        c->y = parent->y + (parent->h - c->h) / 2;
    }
    XChangeProperty(dpy, win, atom("_NET_WM_DESKTOP"), XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&c->workspace, 1);
    if (c->x + c->w > sw) c->x = sw - c->w;
    if (c->y + c->h + TITLE > sh - PANEL) c->y = sh - PANEL - c->h - TITLE;
    c->frame = XCreateSimpleWindow(dpy, root, c->x, c->y, (unsigned int)c->w,
                                   (unsigned int)(c->h + TITLE), 1, line, surface);
    XSelectInput(dpy, c->frame, ExposureMask | ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask | SubstructureNotifyMask | SubstructureRedirectMask);
    XSelectInput(dpy, win, PropertyChangeMask | StructureNotifyMask |
                 (win == files ? ExposureMask | KeyPressMask | ButtonPressMask |
                                  ButtonReleaseMask | PointerMotionMask : 0));
    XSetWindowBorderWidth(dpy, win, 0);
    if (win != files) XAddToSaveSet(dpy, win);
    XReparentWindow(dpy, win, c->frame, 0, TITLE);
    XMapWindow(dpy, win);
    XMapWindow(dpy, c->frame);
    const unsigned int cursors[] = {XC_left_side, XC_right_side, XC_top_side, XC_bottom_side};
    for (int edge = 0; edge < 4; ++edge) {
        XSetWindowAttributes attrs = {.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask};
        c->edges[edge] = XCreateWindow(dpy, c->frame, 0, 0, 5, 5, 0, 0, InputOnly,
                                      CopyFromParent, CWEventMask, &attrs);
        Cursor cursor = XCreateFontCursor(dpy, cursors[edge]);
        XDefineCursor(dpy, c->edges[edge], cursor);
        XFreeCursor(dpy, cursor);
        XMapRaised(dpy, c->edges[edge]);
    }
    frame_extents(win);
    geometry(c);
    if (win != files)
        XGrabButton(dpy, Button1, AnyModifier, win, False, ButtonPressMask,
                    GrabModeSync, GrabModeAsync, None, None);
    update_clients();
    focus(c);
}

static void unmanage(Client *c, bool destroyed) {
    if (!c) return;
    bool restore_focus = focused == c;
    if (focused == c) {
        focused = NULL;
        Window none = None;
        XChangeProperty(dpy, root, active_atom, XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *)&none, 1);
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    }
    if (dragged == c) dragged = NULL;
    if (!destroyed) {
        if (c->win != files) XRemoveFromSaveSet(dpy, c->win);
        XReparentWindow(dpy, c->win, root, c->x, c->y + TITLE);
    }
    XDestroyWindow(dpy, c->frame);
    c->used = false;
    if (destroyed && c->win == files) {
        if (files_input) XDestroyIC(files_input);
        files_input = NULL;
        files = None;
    }
    update_clients();
    draw_panel();
    if (restore_focus) {
        Client *parent = client(c->transient);
        if (parent && !parent->hidden && parent->workspace == current_workspace) focus(parent);
        else {
            for (size_t i = MAX_CLIENTS; i > 0; --i) {
                if (clients[i - 1].used && !clients[i - 1].hidden &&
                    clients[i - 1].workspace == current_workspace) {
                    focus(&clients[i - 1]); break;
                }
            }
        }
    }
}

static int entry_compare(const void *a, const void *b) {
    const Entry *aa = a, *bb = b;
    if (aa->directory != bb->directory) return aa->directory ? -1 : 1;
    return strcoll(aa->name, bb->name);
}

static bool join_path(char *out, size_t length, const char *dir, const char *name) {
    int count = snprintf(out, length, "%s%s%s", dir, strcmp(dir, "/") ? "/" : "", name);
    if (count < 0 || (size_t)count >= length) {
        errno = ENAMETOOLONG; status("Cesta"); return false;
    }
    return true;
}

static bool matches(const char *value, const char *query) {
    if (!*query) return true;
    char *valid_value = g_utf8_make_valid(value, -1);
    char *valid_query = g_utf8_make_valid(query, -1);
    char *folded_value = g_utf8_casefold(valid_value, -1), *folded_query = g_utf8_casefold(valid_query, -1);
    bool found = strstr(folded_value, folded_query) != NULL;
    g_free(folded_value); g_free(folded_query);
    g_free(valid_value); g_free(valid_query);
    return found;
}

static void *reserve_array(void *array, size_t *capacity, size_t needed, size_t item_size) {
    if (needed <= *capacity) return array;
    if (!item_size || needed > SIZE_MAX / item_size) { errno = EOVERFLOW; return NULL; }
    size_t grown = *capacity ? *capacity : 64;
    while (grown < needed) {
        if (grown > SIZE_MAX / 2) { grown = needed; break; }
        grown *= 2;
    }
    if (grown > SIZE_MAX / item_size) { errno = EOVERFLOW; return NULL; }
    void *resized = realloc(array, grown * item_size);
    if (!resized) return NULL;
    *capacity = grown;
    return resized;
}

static bool load_directory(const char *path) {
    char canonical[PATH_MAX];
    if (!realpath(path, canonical)) { status("Cesta"); return false; }
    path = canonical;
    DIR *dir = opendir(path);
    if (!dir) { status("Otvorenie priecinka"); return false; }
    char next[PATH_MAX];
    if (snprintf(next, sizeof(next), "%s", path) >= (int)sizeof(next)) {
        closedir(dir); errno = ENAMETOOLONG; status("Cesta"); return false;
    }
    for (size_t i = 0; i < entry_count; ++i) free(entries[i].name);
    if (preview) XFreePixmap(dpy, preview);
    preview = None; *preview_path = '\0';
    entry_count = 0; selected = -1; scroll = 0;
    snprintf(cwd, sizeof(cwd), "%s", next);
    if (!history_navigation && (history_position < 0 || strcmp(history[history_position], cwd))) {
        if (history_position == 63) {
            memmove(history, history + 1, 63 * sizeof(history[0]));
            --history_position;
        }
        ++history_position;
        snprintf(history[history_position], sizeof(history[0]), "%s", cwd);
        history_count = history_position + 1;
    }
    struct dirent *item;
    errno = 0;
    while ((item = readdir(dir))) {
        if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, "..")) continue;
        if (!show_hidden && item->d_name[0] == '.') continue;
        if (!matches(item->d_name, file_filter)) continue;
        if (entry_count >= INT_MAX) {
            snprintf(message, sizeof(message), "Priecinok prekrocil podporovany pocet poloziek.");
            break;
        }
        char full[PATH_MAX];
        if (!join_path(full, sizeof(full), cwd, item->d_name)) break;
        struct stat st;
        bool directory = stat(full, &st) == 0 && S_ISDIR(st.st_mode);
        char *name = strdup(item->d_name);
        if (!name) { status("Pamat"); break; }
        Entry *resized = reserve_array(entries, &entry_capacity, entry_count + 1, sizeof(*entries));
        if (!resized) {
            free(name);
            status("Pamat");
            break;
        }
        entries = resized;
        entries[entry_count++] = (Entry){name, directory, file_icon(name, directory), false};
        errno = 0;
    }
    if (!item && errno) status("Citanie priecinka");
    closedir(dir);
    if (entry_count > 1) qsort(entries, entry_count, sizeof(entries[0]), entry_compare);
    return true;
}

static void tab_save(void) {
    FileTab *tab = &tabs[active_tab];
    snprintf(tab->path, sizeof(tab->path), "%s", cwd);
    snprintf(tab->filter, sizeof(tab->filter), "%s", file_filter);
    memcpy(tab->history, history, sizeof(history));
    tab->history_count = history_count; tab->history_position = history_position;
    tab->show_hidden = show_hidden; tab->scroll = scroll;
}

static void tab_switch(int index) {
    if (index < 0 || index >= tab_count) return;
    tab_save();
    active_tab = index;
    FileTab *tab = &tabs[index];
    memcpy(history, tab->history, sizeof(history));
    history_count = tab->history_count; history_position = tab->history_position;
    show_hidden = tab->show_hidden;
    snprintf(file_filter, sizeof(file_filter), "%s", tab->filter);
    history_navigation = true;
    load_directory(tab->path);
    history_navigation = false;
    scroll = tab->scroll;
    editing = 0; edit_all_selected = false;
    draw_files();
}

static void tab_new(void) {
    if (tab_count == MAX_TABS) {
        snprintf(message, sizeof(message), "Najviac %d kariet.", MAX_TABS);
        draw_files(); return;
    }
    tab_save();
    tabs[tab_count] = tabs[active_tab];
    ++tab_count;
    tab_switch(tab_count - 1);
}

static void tab_close(void) {
    if (tab_count == 1) { close_client(client(files)); return; }
    int closing = active_tab;
    tab_save();
    for (int i = closing; i + 1 < tab_count; ++i) tabs[i] = tabs[i + 1];
    --tab_count;
    active_tab = closing < tab_count ? closing : tab_count - 1;
    FileTab saved = tabs[active_tab];
    *cwd = '\0';
    tabs[active_tab] = saved;
    memcpy(history, saved.history, sizeof(history));
    history_count = saved.history_count; history_position = saved.history_position;
    show_hidden = saved.show_hidden;
    snprintf(file_filter, sizeof(file_filter), "%s", saved.filter);
    history_navigation = true; load_directory(saved.path); history_navigation = false;
    scroll = saved.scroll; editing = 0; edit_all_selected = false;
    draw_files();
}

static void history_step(int direction) {
    int next = history_position + direction;
    if (next < 0 || next >= history_count) return;
    history_navigation = true;
    if (load_directory(history[next])) history_position = next;
    history_navigation = false;
    draw_files();
}

static void mark_selection(int index, unsigned int modifiers) {
    if (index < 0 || index >= (int)entry_count) return;
    if ((modifiers & ShiftMask) && selected >= 0) {
        int first = selected < index ? selected : index;
        int last = selected > index ? selected : index;
        for (int i = first; i <= last; ++i) entries[i].marked = true;
    } else if (modifiers & ControlMask) {
        entries[index].marked = !entries[index].marked;
    } else {
        for (size_t i = 0; i < entry_count; ++i) entries[i].marked = false;
        entries[index].marked = true;
    }
    selected = index;
}

static void clipboard_set(bool move) {
    for (size_t i = 0; i < clipboard_count; ++i) free(clipboard[i]);
    clipboard_count = 0; clipboard_move = move;
    for (size_t i = 0; i < entry_count; ++i) {
        if (!entries[i].marked) continue;
        char path[PATH_MAX];
        if (!join_path(path, sizeof(path), cwd, entries[i].name)) goto failed;
        char *copy = strdup(path);
        if (!copy) { status("Pamat"); goto failed; }
        char **resized = reserve_array(clipboard, &clipboard_capacity,
                                       clipboard_count + 1, sizeof(*clipboard));
        if (!resized) {
            free(copy);
            status("Pamat");
            goto failed;
        }
        clipboard = resized;
        clipboard[clipboard_count++] = copy;
    }
    snprintf(message, sizeof(message), "%zu vybranych: Ctrl+V %s.", clipboard_count,
             move ? "presunie" : "skopiruje");
    return;
failed:
    for (size_t i = 0; i < clipboard_count; ++i) free(clipboard[i]);
    clipboard_count = 0;
}

static void file_operation_at(bool trash, const char *destination, const char *action) {
    if (file_job) {
        snprintf(message, sizeof(message), "Pockajte na dokoncenie predchadzajucej operacie.");
        return;
    }
    size_t selected_count = 0;
    if (trash) {
        for (size_t i = 0; i < entry_count; ++i)
            if (entries[i].marked) ++selected_count;
    } else selected_count = clipboard_count;
    if (selected_count > SIZE_MAX / sizeof(char *) - 4) {
        errno = EOVERFLOW;
        status("Operacia suborov");
        return;
    }
    char **args = calloc(selected_count + 4, sizeof(*args));
    if (!args) { status("Pamat"); return; }
    size_t count = 0;
    args[count++] = "linsoft-native-file-action";
    args[count++] = (char *)(action ? action : trash ? "trash" : clipboard_move ? "move" : "copy");
    args[count++] = (char *)destination;
    bool valid = true;
    if (trash) {
        for (size_t i = 0; i < entry_count; ++i) {
            if (!entries[i].marked) continue;
            char path[PATH_MAX];
            if (!join_path(path, sizeof(path), cwd, entries[i].name)) { valid = false; break; }
            char *copy = strdup(path);
            if (!copy) { status("Pamat"); valid = false; break; }
            args[count++] = copy;
        }
    } else {
        for (size_t i = 0; i < clipboard_count; ++i) args[count++] = clipboard[i];
    }
    args[count] = NULL;
    if (valid && count > 3 && spawn(args)) {
        file_job = last_spawn;
        file_job_move = !trash && clipboard_move;
        file_job_cancellable = !trash;
        file_job_cancel_requested = false;
        snprintf(message, sizeof(message), file_job_cancellable
                 ? "Operácia prebieha: %zu položiek. Esc = zrušiť."
                 : "Operácia prebieha: %zu položiek.", count - 3);
    }
    if (trash)
        for (size_t i = 3; i < count; ++i) free(args[i]);
    free(args);
}

static void file_operation(bool trash) { file_operation_at(trash, cwd, NULL); }

static void draw_files(void) {
    if (!files) return;
    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, files, &attr)) return;
    int w = attr.width, h = attr.height;
    rect(files, 0, 0, (unsigned int)w, (unsigned int)h, bg);
    rect(files, 0, 0, (unsigned int)w, 90, surface);
    draw_icon(files, 15, 7, 24, ICON_FOLDER);
    text(files, 48, 26, "Linsoft / Súbory", &accent, false);
    draw_icon(files, 15, 39, 22, ICON_HOME);
    text(files, 42, 56, "Domov", &ink, false);
    draw_icon(files, 102, 39, 22, ICON_UP);
    text(files, 129, 56, "Vyššie", &ink, false);
    draw_icon(files, 192, 39, 22, ICON_NEW_FOLDER);
    text(files, 219, 56, "Nový priečinok", &ink, false);
    draw_icon(files, 342, 39, 22, ICON_RESTART);
    text(files, 369, 56, "Obnoviť", &ink, false);
    text(files, 463, 56, "Späť", &ink, false);
    text(files, 520, 56, "Ďalej", &ink, false);
    text(files, 585, 56, "Kôš", &ink, false);
    text(files, 640, 56, show_hidden ? "Skryté: áno" : "Skryté: nie", &muted, false);
    text(files, 745, 56, "Hľadať", &accent, false);
    char path[110];
    snprintf(path, sizeof(path), "%.*s", 105, cwd);
    text(files, 18, 80, path, &muted, false);
    int tab_width = (w - 55) / tab_count;
    if (tab_width > 160) tab_width = 160;
    for (int i = 0; i < tab_count; ++i) {
        char title[40];
        const char *path_value = i == active_tab ? cwd : tabs[i].path;
        const char *name = strrchr(path_value, '/');
        snprintf(title, sizeof(title), "%d %s", i + 1, name && name[1] ? name + 1 : "/");
        shorten(title, 15);
        rect(files, 5 + i * tab_width, 96, (unsigned int)(tab_width - 3), 29,
             i == active_tab ? raised : surface);
        text(files, 12 + i * tab_width, 117, title, &ink, false);
    }
    text(files, w - 42, 117, "+", &accent, false);
    int visible = (h - 202) / 29;
    if (visible < 1) visible = 1;
    int max_scroll = (int)entry_count - visible;
    if (max_scroll < 0) max_scroll = 0;
    if (scroll > max_scroll) scroll = max_scroll;
    if (scroll < 0) scroll = 0;
    for (int row = 0; row < visible && row + scroll < (int)entry_count; ++row) {
        int index = row + scroll;
        Entry *item = &entries[index];
        if (entries[index].marked) rect(files, 10, FILE_ROWS_Y + row * 29, (unsigned int)(w - 20), 28, raised);
        draw_icon(files, 20, FILE_ROWS_Y + 2 + row * 29, 24, item->icon);
        char label[256];
        snprintf(label, sizeof(label), "%s", item->name);
        shorten(label, 65);
        text(files, 65, FILE_ROWS_Y + 21 + row * 29, label, &ink, false);
    }
    if (selected >= 0 && entries[selected].icon == ICON_IMAGE && w >= 650 && h >= 380) {
        char path_value[PATH_MAX];
        if (join_path(path_value, sizeof(path_value), cwd, entries[selected].name)) {
            if (strcmp(preview_path, path_value)) {
                if (preview) XFreePixmap(dpy, preview);
                preview = image_preview(path_value);
                snprintf(preview_path, sizeof(preview_path), "%s", path_value);
            }
            if (preview) {
                rect(files, w - 220, FILE_ROWS_Y, 215, 185, surface);
                text(files, w - 210, FILE_ROWS_Y + 22, "Náhľad obrázka", &muted, false);
                XCopyArea(dpy, preview, files, gc, 0, 0, 200, 150, w - 210, FILE_ROWS_Y + 30);
            }
        }
    }
    rect(files, 0, h - 65, (unsigned int)w, 65, surface);
    if (editing) {
        if (edit_all_selected) rect(files, 10, h - 36, (unsigned int)(w - 20), 29, raised);
        text(files, 15, h - 43, editing == 4 ? "Hľadať názov v priečinku: Enter, prázdne zruší filter" :
             editing == 3 ? "Cesta: Enter otvorí, Esc zruší" :
             editing == 1 ? "Nový priečinok: Enter potvrdí, Esc zruší" :
             "Premenovať: Enter potvrdí, Esc zruší", &accent, false);
        text(files, 15, h - 18, edit, &ink, false);
    } else {
        text(files, 15, h - 43, "Ctrl+T/W karty | Ctrl+F hľadať | Ctrl+L cesta | Ctrl+C/X/V | Delete kôš", &muted, false);
        char info[512];
        if (*message) snprintf(info, sizeof(info), "%s", message);
        else snprintf(info, sizeof(info), "%zu položiek | filter: %.200s | F5 obnoví", entry_count,
                      *file_filter ? file_filter : "-");
        text(files, 15, h - 18, info, &ink, false);
    }
}

static void open_files(void) {
    if (!files) {
        files = XCreateSimpleWindow(dpy, root, 100, 100, FILE_W, FILE_H, 0, line, bg);
        const char files_title[] = "Linsoft Súbory";
        XStoreName(dpy, files, "Linsoft Subory");
        XChangeProperty(dpy, files, name_atom, utf8, 8, PropModeReplace,
                        (const unsigned char *)files_title, (int)strlen(files_title));
        XClassHint files_class = {.res_name = "linsoft-files", .res_class = "LinsoftFiles"};
        XSetClassHint(dpy, files, &files_class);
        XSelectInput(dpy, files, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask |
                     ButtonReleaseMask | PointerMotionMask);
        if (input_method) {
            files_input = XCreateIC(input_method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                                   XNClientWindow, files, XNFocusWindow, files, NULL);
            if (!files_input) {
                fprintf(stderr, "Linsoft Native: UTF-8 input context unavailable\n");
                snprintf(message, sizeof(message), "UTF-8 vstup nie je dostupny.");
            } else XSetICFocus(files_input);
        }
        const char *home = getenv("HOME");
        if (*cwd) load_directory(cwd);
        else load_directory(home ? home : "/home/linsoft");
        manage(files);
    } else {
        focus(client(files));
    }
    draw_files();
}

static void open_selected(void) {
    if (selected < 0 || selected >= (int)entry_count) return;
    char path[PATH_MAX];
    if (!join_path(path, sizeof(path), cwd, entries[selected].name)) return;
    if (entries[selected].directory) load_directory(path);
    else {
        char *args[] = {"xdg-open", path, NULL};
        spawn(args);
    }
    draw_files();
}

static void parent_directory(void) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s", cwd);
    char *slash = strrchr(path, '/');
    if (slash && slash != path) *slash = '\0';
    else strcpy(path, "/");
    load_directory(path);
    draw_files();
}

static void edit_commit(void) {
    edit_all_selected = false;
    if (editing == 4) {
        if (strlen(edit) >= sizeof(file_filter)) {
            snprintf(message, sizeof(message), "Hladany nazov je prilis dlhy.");
        } else {
            snprintf(file_filter, sizeof(file_filter), "%s", edit);
            *message = '\0'; load_directory(cwd);
        }
        editing = 0; draw_files(); return;
    }
    if (editing == 3) {
        if (*edit) load_directory(edit);
        else         snprintf(message, sizeof(message), "Cesta nesmie byť prázdna.");
        editing = 0; draw_files(); return;
    }
    if (strlen(edit) > NAME_MAX) {
        snprintf(message, sizeof(message), "Nazov je prilis dlhy (najviac %d bajtov).", NAME_MAX);
        editing = 0;
        draw_files(); return;
    }
    if (!*edit || strchr(edit, '/') || !strcmp(edit, ".") || !strcmp(edit, "..")) {
        snprintf(message, sizeof(message), "Neplatny nazov priecinka alebo suboru.");
        editing = 0; draw_files(); return;
    }
    char path[PATH_MAX];
    if (!join_path(path, sizeof(path), cwd, edit)) { editing = 0; draw_files(); return; }
    if (editing == 1) {
        if (mkdir(path, 0755) < 0) status("Vytvorenie priecinka");
        else         snprintf(message, sizeof(message), "Priečinok vytvorený.");
    } else if (selected >= 0 && selected < (int)entry_count) {
        char old[PATH_MAX];
        struct stat st;
        if (lstat(path, &st) == 0) {
            snprintf(message, sizeof(message), "Cielovy nazov uz existuje; nic nebolo prepisane.");
        } else if (errno != ENOENT) {
            status("Kontrola ciela");
        } else if (join_path(old, sizeof(old), cwd, entries[selected].name)) {
            if (renameat2(AT_FDCWD, old, AT_FDCWD, path, RENAME_NOREPLACE) < 0) status("Premenovanie");
            else snprintf(message, sizeof(message), "Polozka premenovana.");
        }
    }
    editing = 0;
    load_directory(cwd);
    draw_files();
}

static void files_key(XKeyEvent *event) {
    char buffer[512];
    KeySym key = NoSymbol;
    int count;
    if (files_input && !event->send_event) {
        Status result;
        count = Xutf8LookupString(files_input, event, buffer, sizeof(buffer), &key, &result);
        if (result == XBufferOverflow) {
            snprintf(message, sizeof(message), "Vstup je príliš dlhý.");
            draw_files(); return;
        }
    } else count = XLookupString(event, buffer, sizeof(buffer), &key, NULL);
    if (!editing && (key == XK_Menu || (key == XK_F10 && (event->state & ShiftMask)))) {
        Client *c = client(files);
        context_show(true, c ? c->x + 50 : 50, c ? c->y + 100 : 100);
        return;
    }
    if (editing) {
        if ((event->state & ControlMask) && (key == XK_a || key == XK_A)) edit_all_selected = true;
        else if (key == XK_Escape) { editing = 0; edit_all_selected = false; }
        else if (key == XK_Return) { edit_commit(); return; }
        else if (key == XK_BackSpace) {
            if (edit_all_selected) { *edit = '\0'; edit_all_selected = false; }
            size_t length = strlen(edit);
            if (length) {
                do { --length; } while (length && ((unsigned char)edit[length] & 0xc0) == 0x80);
                edit[length] = '\0';
            }
        } else if (count > 0 && !(event->state & ControlMask) &&
                   (edit_all_selected ? 0 : strlen(edit)) + (size_t)count < sizeof(edit)) {
            if ((unsigned char)buffer[0] >= 32) {
                if (edit_all_selected) { *edit = '\0'; edit_all_selected = false; }
                size_t length = strlen(edit);
                memcpy(edit + length, buffer, (size_t)count);
                edit[length + (size_t)count] = '\0';
            }
        }
        draw_files(); return;
    }
    if ((event->state & ControlMask) && (key == XK_l || key == XK_L)) {
        editing = 3; edit_all_selected = true; snprintf(edit, sizeof(edit), "%s", cwd);
    } else if ((event->state & ControlMask) && key == XK_t) {
        tab_new();
    } else if ((event->state & ControlMask) && key == XK_w) {
        tab_close(); return;
    } else if ((event->state & ControlMask) && (key == XK_Tab || key == XK_ISO_Left_Tab)) {
        tab_switch((active_tab + (event->state & ShiftMask ? tab_count - 1 : 1)) % tab_count);
    } else if ((event->state & ControlMask) && key == XK_f) {
        editing = 4; edit_all_selected = true; snprintf(edit, sizeof(edit), "%s", file_filter);
    } else if ((event->state & ControlMask) && (key == XK_h || key == XK_H)) {
        show_hidden = !show_hidden; load_directory(cwd);
    } else if (key == XK_Escape && file_job && file_job_cancellable &&
               !file_job_cancel_requested) {
        if (kill(file_job, SIGTERM) == 0) {
            file_job_cancel_requested = true;
            snprintf(message, sizeof(message), "Zrusenie operacie; obnovujem povodne polozky.");
        } else if (errno != ESRCH) status("Zrusenie operacie");
    } else if ((event->state & ControlMask) && key == XK_a) {
        for (size_t i = 0; i < entry_count; ++i) entries[i].marked = true;
        if (entry_count && selected < 0) selected = 0;
    } else if ((event->state & Mod1Mask) && key == XK_Left) history_step(-1);
    else if ((event->state & Mod1Mask) && key == XK_Right) history_step(1);
    else if ((event->state & ControlMask) && (key == XK_N || key == XK_n)) {
        editing = 1; *edit = '\0';
    } else if (key == XK_F2 && selected >= 0) {
        editing = 2;
        snprintf(edit, sizeof(edit), "%s", entries[selected].name);
    } else if (key == XK_F5) load_directory(cwd);
    else if (key == XK_BackSpace) parent_directory();
    else if (key == XK_Return) { open_selected(); return; }
    else if (key == XK_Down && selected + 1 < (int)entry_count) {
        mark_selection(selected + 1, event->state);
        XWindowAttributes attr;
        XGetWindowAttributes(dpy, files, &attr);
        int visible = (attr.height - 202) / 29;
        if (selected >= scroll + visible) ++scroll;
    } else if (key == XK_Up && selected > 0) {
        mark_selection(selected - 1, event->state); if (selected < scroll) scroll = selected;
    } else if ((event->state & ControlMask) && key == XK_c && selected >= 0) {
        clipboard_set(false);
    } else if ((event->state & ControlMask) && key == XK_x && selected >= 0) {
        clipboard_set(true);
    } else if ((event->state & ControlMask) && key == XK_v && clipboard_count) {
        file_operation(false);
    } else if (key == XK_Delete && selected >= 0) {
        file_operation(true);
    }
    draw_files();
}

static const char *menu_items[] = {
    "Súbory", "Terminál", "Internet", "Písanie / Writer", "Tabuľky / Calc",
    "Nastavenia", "Aplikácie a aktualizácie", "Zvuk", "Reštartovať", "Vypnúť"
};

static void menu_search(void) {
    menu_count = 0;
    for (int i = 0; i < 10; ++i)
        if (matches(menu_items[i], menu_filter)) menu_matches[menu_count++] = i;
    if (*menu_filter) {
        for (int i = 0; i < MAX_APPS && menu_apps[i]; ++i) {
            if (matches(g_app_info_get_display_name(menu_apps[i]), menu_filter) ||
                matches(g_app_info_get_executable(menu_apps[i]), menu_filter))
                menu_matches[menu_count++] = i + 10;
        }
    }
    menu_selected = 0;
}

static void launch(int index) {
    if (index >= 10 && index < MAX_APPS + 10 && menu_apps[index - 10]) {
        const char *path = g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(menu_apps[index - 10]));
        if (path) {
            char *args[] = {"gio", "launch", (char *)path, NULL};
            spawn(args);
        } else {
            snprintf(message, sizeof(message), "Aplikácia nemá spustiteľný desktopový súbor.");
            fprintf(stderr, "Linsoft Native: %s\n", message); draw_desktop();
        }
        return;
    }
    char *terminal[] = {"xterm", "-fa", "DejaVu Sans Mono", "-fs", "11",
                        "-bg", "#101c2f", "-fg", "#e4edf8", NULL};
    char *browser[] = {"linsoft-browser", NULL};
    char *writer[] = {"libreoffice", "--writer", NULL};
    char *calc[] = {"libreoffice", "--calc", NULL};
    char *settings[] = {"linsoft-settings", NULL};
    char *apps[] = {"linsoft-app-center", NULL};
    char *sound[] = {"linsoft-settings", "sound", NULL};
    char *restart[] = {"linsoft-session-action", "restart", NULL};
    char *poweroff[] = {"linsoft-session-action", "poweroff", NULL};
    switch (index) {
        case 0: open_files(); break;
        case 1: spawn(terminal); break;
        case 2: spawn(browser); break;
        case 3: spawn(writer); break;
        case 4: spawn(calc); break;
        case 5: spawn(settings); break;
        case 6: spawn(apps); break;
        case 7: spawn(sound); break;
        case 8: spawn(restart); break;
        case 9: spawn(poweroff); break;
        default: break;
    }
}

static void draw_menu(void) {
    static const Icon icons[] = {
        ICON_FOLDER, ICON_TERMINAL, ICON_BROWSER, ICON_DOCUMENT, ICON_SHEET,
        ICON_SETTINGS, ICON_APPS, ICON_SOUND, ICON_RESTART, ICON_POWER
    };
    rect(menu, 0, 0, 330, 466, surface);
    draw_icon(menu, 18, 9, 36, ICON_LOGO);
    text(menu, 66, 35, "Linsoft", &accent, true);
    char query[280];
    snprintf(query, sizeof(query), "Hľadať: %s", *menu_filter ? menu_filter : "napíšte názov aplikácie...");
    shorten(query, 32);
    text(menu, 20, 61, query, &muted, false);
    int first = menu_selected >= 10 ? menu_selected - 9 : 0;
    for (int row = 0; row < 10 && first + row < menu_count; ++row) {
        int index = first + row, app = menu_matches[index];
        if (index == menu_selected) rect(menu, 8, 75 + row * 36, 314, 34, raised);
        draw_icon(menu, 20, 79 + row * 36, 26, app < 10 ? icons[app] : ICON_APPS);
        char label[256];
        snprintf(label, sizeof(label), "%s", app < 10 ? menu_items[app] :
                 g_app_info_get_display_name(menu_apps[app - 10]));
        shorten(label, 30);
        text(menu, 60, 99 + row * 36, label, &ink, false);
    }
    if (!menu_count) text(menu, 20, 110, "Žiadna aplikácia. Backspace upraví hľadanie.", &muted, false);
}

static void toggle_menu(void) {
    menu_open = !menu_open;
    if (menu_open) {
        g_list_free_full(applications, g_object_unref);
        applications = g_app_info_get_all();
        memset(menu_apps, 0, sizeof(menu_apps));
        int count = 0;
        for (GList *item = applications; item && count < MAX_APPS - 1; item = item->next) {
            GAppInfo *app = item->data;
            if (G_IS_DESKTOP_APP_INFO(app) && g_app_info_should_show(app) &&
                g_app_info_get_executable(app)) menu_apps[count++] = app;
        }
        *menu_filter = '\0'; menu_search();
        XMapRaised(dpy, menu); draw_menu();
        if (menu_input) XSetICFocus(menu_input);
        XGrabKeyboard(dpy, menu, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    } else {
        if (menu_input) XUnsetICFocus(menu_input);
        XUnmapWindow(dpy, menu); XUngrabKeyboard(dpy, CurrentTime);
    }
}

static void draw_desktop(void) {
    rect(desktop, 0, 0, (unsigned int)sw, (unsigned int)sh, bg);
    if (wallpaper) XCopyArea(dpy, wallpaper, desktop, gc, 0, 0, (unsigned int)sw, (unsigned int)sh, 0, 0);
    else {
        int center_x = sw * 3 / 4, center_y = sh / 2;
        XSetForeground(dpy, gc, line);
        XSetLineAttributes(dpy, gc, 2, LineSolid, CapRound, JoinRound);
        for (int i = 0; i < 4; ++i) {
            int radius = sh * 7 / 20 + i * 26;
            XDrawArc(dpy, desktop, gc, center_x - radius, center_y - radius,
                     (unsigned int)(radius * 2), (unsigned int)(radius * 2), 0, 360 * 64);
        }
        XSetLineAttributes(dpy, gc, 0, LineSolid, CapButt, JoinMiter);
    }
    draw_icon(desktop, 58, 62, 48, ICON_LOGO);
    text(desktop, 125, 100, "Linsoft", &accent, true);
    text(desktop, 63, 134, "LINSOFT DESKTOP PROFESSIONAL", &ink, false);
    text(desktop, 63, 163, "Jedna plocha. Vlastné okná, panel aj súbory.", &muted, false);
    const Icon icons[] = {
        ICON_FOLDER, ICON_TERMINAL, ICON_BROWSER, ICON_DOCUMENT, ICON_SHEET,
        ICON_SETTINGS, ICON_APPS
    };
    const char *const labels[] = {
        "Súbory", "Terminál", "Internet", "Writer", "Calc",
        "Nastavenia", "Aplikácie"
    };
    for (int i = 0; i < 7; ++i) {
        int y = 184 + i * 88;
        draw_icon(desktop, 60, y, desktop_icon_size, icons[i]);
        text(desktop, 140, y + desktop_icon_size + 15, labels[i], &ink, true);
    }
    text(desktop, 63, sh - 100, "Super+E súbory  |  Super+T terminál  |  Alt+Tab okná", &muted, false);
    if (*message) text(desktop, 63, sh - 73, message, &accent, false);
}

static void preferences(void) {
    char *args[] = {"linsoft-native-preferences", NULL};
    spawn(args);
}

static const char *const file_context[] = {
    "Otvoriť", "Kopírovať", "Vystrihnúť", "Vložiť", "Premenovať",
    "Nový priečinok", "Do koša", "Obnoviť z koša", "Skryté súbory", "Zadať cestu"
};
static const char *const desktop_context[] = {
    "Súbory", "Terminál", "Nastavenia plochy", "Systémové ovládanie"
};

static void draw_context(void) {
    rect(context, 0, 0, 280, (unsigned int)(context_kind ? 10 * 32 : 4 * 32), surface);
    int count = context_kind ? 10 : 4;
    for (int i = 0; i < count; ++i) {
        if (i == context_selected) rect(context, 2, i * 32, 276, 31, raised);
        text(context, 14, i * 32 + 22, context_kind ? file_context[i] : desktop_context[i], &ink, false);
    }
}

static void context_hide(void) {
    context_open = false;
    XUnmapWindow(dpy, context);
    XUngrabKeyboard(dpy, CurrentTime);
}

static void context_show(bool file_menu, int x, int y) {
    if (menu_open) toggle_menu();
    context_kind = file_menu; context_selected = 0;
    int height = file_menu ? 320 : 128;
    if (x > sw - 280) x = sw - 280;
    if (y > sh - PANEL - height) y = sh - PANEL - height;
    XMoveResizeWindow(dpy, context, x, y, 280, (unsigned int)height);
    context_open = true; XMapRaised(dpy, context); draw_context();
    XGrabKeyboard(dpy, context, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

static void context_action(int index) {
    context_hide();
    if (!context_kind) {
        if (index < 2) launch(index);
        else if (index == 2) preferences();
        else {
            char *args[] = {"linsoft-native-system", NULL}; spawn(args);
        }
        return;
    }
    if (index == 0) open_selected();
    else if (index == 1) clipboard_set(false);
    else if (index == 2) clipboard_set(true);
    else if (index == 3) file_operation(false);
    else if (index == 4 && selected >= 0) {
        editing = 2; snprintf(edit, sizeof(edit), "%s", entries[selected].name);
    } else if (index == 5) { editing = 1; *edit = '\0'; }
    else if (index == 6) file_operation(true);
    else if (index == 7) {
        char *args[] = {"linsoft-native-trash", NULL}; spawn(args);
    } else if (index == 8) { show_hidden = !show_hidden; load_directory(cwd); }
    else if (index == 9) { editing = 3; snprintf(edit, sizeof(edit), "%s", cwd); }
    draw_files();
}

static void button(XButtonEvent *e) {
    if (e->window == context) {
        int index = e->y / 32;
        if (index >= 0 && index < (context_kind ? 10 : 4)) context_action(index);
        return;
    }
    if (context_open) context_hide();
    if (e->window == menu) {
        int row = (e->y - 75) / 36;
        int index = row + (menu_selected >= 10 ? menu_selected - 9 : 0);
        if (e->y >= 75 && row < 10 && index < menu_count) {
            int app = menu_matches[index]; toggle_menu(); launch(app);
        }
        return;
    }
    if (menu_open) toggle_menu();
    if (e->window == panel) {
        bool compact = sw < 1800;
        int task_limit = sw - (compact ? 550 : 600);
        int task_step = compact ? 156 : 182;
        int task_width = compact ? 148 : 174;
        int workspace_left = sw - (compact ? 264 : 280);
        int preferences_left = sw - (compact ? 300 : 318);
        int center_left = sw - (compact ? 332 : 350);
        if (e->x < 120) toggle_menu();
        else if (e->x < 205) launch(0);
        else if (e->x < 320) launch(pinned_launcher);
        else if (e->x >= sw - 168) {
            char *args[] = {"linsoft-native-center", "calendar", NULL}; spawn(args);
        }
        else if (e->x >= task_limit && e->x < center_left) {
            char *args[] = {"linsoft-native-system", NULL}; spawn(args);
        }
        else if (e->x >= workspace_left && e->x < sw - 168) {
            workspace_switch((unsigned long)(e->x - workspace_left) / (compact ? 24 : 28));
        }
        else if (e->x >= preferences_left && e->x < workspace_left) {
            preferences();
        }
        else if (e->x >= center_left && e->x < preferences_left) {
                char *args[] = {"linsoft-native-center", NULL}; spawn(args);
        }
        else {
            int x = 330;
            for (size_t i = 0; i < MAX_CLIENTS; ++i) {
                Client *c = &clients[i];
                if (!c->used || c->workspace != current_workspace || x + task_step > task_limit) continue;
                if (e->x >= x && e->x < x + task_width) {
                    if (focused == c && !c->hidden) minimize(c); else focus(c);
                    break;
                }
                x += task_step;
            }
        }
        return;
    }
    if (e->window == desktop) {
        if (e->button == Button3) { context_show(false, e->x_root, e->y_root); return; }
        if (e->x < 350 && e->y >= 174 && e->y < 174 + 7 * 88)
            launch((e->y - 174) / 88);
        return;
    }
    Client *c = client(e->window);
    if (c) focus(c);
    if (c && e->button == Button1 && !c->maximized) {
        for (int edge = 0; edge < 4; ++edge) {
            if (e->window != c->edges[edge]) continue;
            dragged = c; resizing = true; resize_edges = 1 << edge;
            c->snapped = false;
            drag_x = e->x_root; drag_y = e->y_root;
            start_x = c->x; start_y = c->y; start_w = c->w; start_h = c->h;
            return;
        }
    }
    if (c && e->window == c->frame) {
        if (e->y < TITLE && e->x > c->w - 34) close_client(c);
        else if (e->y < TITLE && e->x > c->w - 68) maximize(c);
        else if (e->y < TITLE && e->x > c->w - 102) minimize(c);
        else if (e->y < TITLE || (e->state & Mod1Mask)) {
            if (e->button == Button1 && e->y < TITLE) {
                if (title_click_window == c->win && e->time - title_click_time < 400) {
                    title_click_window = None; maximize(c); return;
                }
                title_click_window = c->win; title_click_time = e->time;
            }
            if (!c->maximized) {
                dragged = c; resizing = e->button == Button3;
                resize_edges = resizing ? 2 | 8 : 0;
                c->snapped = false;
                drag_x = e->x_root; drag_y = e->y_root;
                start_x = c->x; start_y = c->y; start_w = c->w; start_h = c->h;
            }
        }
    }
    if (c && e->window == c->win && c->win != files)
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
    if (e->window == files) {
        if (e->button == Button4) { scroll -= 3; draw_files(); return; }
        if (e->button == Button5) { scroll += 3; draw_files(); return; }
        if (e->button == Button3 && e->y < 98) {
            context_show(true, e->x_root, e->y_root);
            return;
        }
        if (e->y < 65 && e->y >= 35) {
            if (e->x < 95) { const char *home = getenv("HOME"); load_directory(home ? home : "/home/linsoft"); }
            else if (e->x < 185) parent_directory();
            else if (e->x < 335) { editing = 1; *edit = '\0'; }
            else if (e->x < 450) load_directory(cwd);
            else if (e->x < 510) history_step(-1);
            else if (e->x < 575) history_step(1);
            else if (e->x < 630) {
                char *args[] = {"linsoft-native-trash", NULL}; spawn(args);
            } else if (e->x < 730) { show_hidden = !show_hidden; load_directory(cwd); }
            else { editing = 4; edit_all_selected = true; snprintf(edit, sizeof(edit), "%s", file_filter); }
        } else if (e->y >= 65 && e->y < 90) {
            editing = 3; snprintf(edit, sizeof(edit), "%s", cwd);
        } else if (e->y >= 96 && e->y < FILE_ROWS_Y) {
            XWindowAttributes attr; XGetWindowAttributes(dpy, files, &attr);
            int tab_width = (attr.width - 55) / tab_count;
            if (tab_width > 160) tab_width = 160;
            if (e->x >= attr.width - 55) tab_new();
            else if (e->x >= 5) tab_switch((e->x - 5) / tab_width);
        } else if (e->y >= FILE_ROWS_Y) {
            XWindowAttributes attr;
            XGetWindowAttributes(dpy, files, &attr);
            int index = (e->y - FILE_ROWS_Y) / 29 + scroll;
            if (e->y < attr.height - 65 && index < (int)entry_count) {
                if ((e->button != Button3 && e->button != Button1) || !entries[index].marked ||
                    (e->state & (ControlMask | ShiftMask)))
                    mark_selection(index, e->state);
                else selected = index;
                if (e->button == Button1 && !(e->state & (ControlMask | ShiftMask)) &&
                    index == last_index && e->time - last_click < 400) open_selected();
                last_index = index; last_click = e->time;
                if (e->button == Button1) {
                    file_drag_index = index; file_drag_x = e->x; file_drag_y = e->y;
                    file_drag_active = false;
                }
            }
            if (e->button == Button3) context_show(true, e->x_root, e->y_root);
        }
        draw_files();
    }
}

static void file_drop(XButtonEvent *event) {
    if (file_drag_index < 0 || !file_drag_active || event->window != files) goto done;
    char destination[PATH_MAX] = "";
    if (event->y >= FILE_ROWS_Y) {
        int target = (event->y - FILE_ROWS_Y) / 29 + scroll;
        XWindowAttributes attr; XGetWindowAttributes(dpy, files, &attr);
        if (event->y < attr.height - 65 && target >= 0 && target < (int)entry_count &&
            target != file_drag_index && entries[target].directory)
            join_path(destination, sizeof(destination), cwd, entries[target].name);
    } else if (event->y >= 96) {
        XWindowAttributes attr; XGetWindowAttributes(dpy, files, &attr);
        int width = (attr.width - 55) / tab_count;
        if (width > 160) width = 160;
        int target = (event->x - 5) / width;
        if (event->x >= 5 && target >= 0 && target < tab_count && target != active_tab)
            snprintf(destination, sizeof(destination), "%s", tabs[target].path);
    } else if (event->y >= 35 && event->y < 65 && event->x >= 95 && event->x < 185) {
        snprintf(destination, sizeof(destination), "%s", cwd);
        char *slash = strrchr(destination, '/');
        if (slash && slash != destination) *slash = '\0';
        else strcpy(destination, "/");
    }
    if (*destination) {
        clipboard_set(false);
        if (clipboard_count) file_operation_at(false, destination, "drop");
    } else snprintf(message, sizeof(message), "Pusť na priečinok, inú kartu alebo tlačidlo Vyššie.");
    draw_files();
done:
    file_drag_index = -1; file_drag_active = false;
}

static int xerror(Display *display, XErrorEvent *error) {
    if (error->error_code == BadAccess && error->request_code == 2) startup_error = true;
    else if (error->error_code != BadWindow) {
        char buffer[256];
        XGetErrorText(display, error->error_code, buffer, sizeof(buffer));
        fprintf(stderr, "Linsoft Native X11: %s (request %u, window %lu)\n",
                buffer, error->request_code, error->resourceid);
    }
    return 0;
}

static void stop_signal(int signal_number) { (void)signal_number; stopping = 1; }

static Window shell_window(int x, int y, unsigned int w, unsigned int h) {
    XSetWindowAttributes attrs = {.override_redirect = True, .background_pixel = bg,
        .event_mask = ExposureMask | ButtonPressMask | KeyPressMask};
    return XCreateWindow(dpy, root, x, y, w, h, 0, CopyFromParent, InputOutput,
                          CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    if (argc == 3 && !strcmp(argv[1], "--check-wallpaper")) {
        png_image image = {.version = PNG_IMAGE_VERSION};
        unsigned char *pixels = wallpaper_read(argv[2], &image);
        if (!pixels) return 1;
        free(pixels); png_image_free(&image);
        return 0;
    }
    dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "Linsoft Native: cannot open X display\n"); return 1; }
    screen = DefaultScreen(dpy); root = RootWindow(dpy, screen);
    command_atom = atom("_LINSOFT_NATIVE_COMMAND");
    bool directory_request = argc == 3 && !strcmp(argv[1], "--files");
    if (directory_request || (argc == 2 && (!strcmp(argv[1], "--quit") || !strcmp(argv[1], "--files") || !strcmp(argv[1], "--reload")))) {
        char directory[PATH_MAX];
        if (directory_request) {
            struct stat info;
            if (!realpath(argv[2], directory) || stat(directory, &info) != 0 || !S_ISDIR(info.st_mode)) {
                fprintf(stderr, "Linsoft Native: invalid directory: %s\n", argv[2]);
                XCloseDisplay(dpy);
                return 1;
            }
        }
        Atom type;
        int format;
        unsigned long count, remaining;
        unsigned char *value = NULL;
        Atom check = atom("_NET_SUPPORTING_WM_CHECK");
        bool available = false;
        if (XGetWindowProperty(dpy, root, check, 0, 1, False, XA_WINDOW,
                              &type, &format, &count, &remaining, &value) == Success &&
            type == XA_WINDOW && format == 32 && count == 1 && value) {
            char *owner = NULL;
            if (XFetchName(dpy, *(Window *)value, &owner) && owner) {
                available = !strcmp(owner, "Linsoft Native");
                XFree(owner);
            }
        }
        if (value) XFree(value);
        if (!available) {
            fprintf(stderr, "Linsoft Native: native desktop is not running\n");
            XCloseDisplay(dpy);
            return 1;
        }
        XEvent event = {0};
        event.xclient.type = ClientMessage;
        event.xclient.window = root;
        event.xclient.message_type = command_atom;
        event.xclient.format = 32;
        event.xclient.data.l[0] = !strcmp(argv[1], "--quit") ? 1 : !strcmp(argv[1], "--files") ? 2 : 3;
        Window request = None;
        if (directory_request) {
            request = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0, 0, 0);
            XChangeProperty(dpy, request, atom("_LINSOFT_NATIVE_DIRECTORY"), XA_STRING, 8,
                            PropModeReplace, (unsigned char *)directory, (int)strlen(directory));
            event.xclient.window = request;
            event.xclient.data.l[0] = 4;
        }
        XSendEvent(dpy, root, False, SubstructureNotifyMask, &event);
        XSync(dpy, False);
        if (directory_request) {
            int result = 1;
            gint64 deadline = g_get_monotonic_time() + 10 * G_TIME_SPAN_SECOND;
            bool received = false;
            while (!received && g_get_monotonic_time() < deadline) {
                while (XPending(dpy)) {
                    XEvent reply;
                    XNextEvent(dpy, &reply);
                    if (reply.type == ClientMessage && reply.xclient.window == request &&
                        reply.xclient.message_type == command_atom && reply.xclient.format == 32) {
                        result = reply.xclient.data.l[0] == 0 ? 0 : 1;
                        received = true;
                    }
                }
                if (!received) {
                    struct pollfd connection = {.fd = ConnectionNumber(dpy), .events = POLLIN};
                    if (poll(&connection, 1, 100) < 0 && errno != EINTR) break;
                }
            }
            if (result) fprintf(stderr, "Linsoft Native: directory request failed or timed out: %s\n", directory);
            XDestroyWindow(dpy, request);
            XCloseDisplay(dpy);
            return result;
        }
        XCloseDisplay(dpy);
        return 0;
    }
    if (argc != 1) {
        fprintf(stderr, "Usage: linsoft-native [--files [DIRECTORY]|--quit|--reload|--check-wallpaper PNG]\n");
        return 2;
    }
    XSetErrorHandler(xerror);
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask |
                 KeyPressMask | StructureNotifyMask);
    XSync(dpy, False);
    if (startup_error) { fprintf(stderr, "Linsoft Native: another window manager is running\n"); return 1; }
    signal(SIGTERM, stop_signal); signal(SIGINT, stop_signal);
    sw = DisplayWidth(dpy, screen); sh = DisplayHeight(dpy, screen);
    bg = color("#0b1425"); surface = color("#132238"); raised = color("#243e5a");
    line = color("#284863"); blue = color("#126b87");
    icon_cyan = color("#54d4de"); icon_light = color("#b8d9ff");
    icon_green = color("#69dfb0"); icon_gold = color("#f0c56e"); icon_purple = color("#bca0f5");
    Colormap cmap = DefaultColormap(dpy, screen);
    Visual *visual = DefaultVisual(dpy, screen);
    if (!XftColorAllocName(dpy, visual, cmap, "#e4edf8", &ink) ||
        !XftColorAllocName(dpy, visual, cmap, "#94adca", &muted) ||
        !XftColorAllocName(dpy, visual, cmap, "#54d4de", &accent)) {
        fprintf(stderr, "Linsoft Native: cannot allocate text colors\n"); return 1;
    }
    font = XftFontOpenName(dpy, screen, "DejaVu Sans:size=10");
    large = XftFontOpenName(dpy, screen, "DejaVu Sans:size=23");
    if (!font || !large) { fprintf(stderr, "Linsoft Native: cannot load fonts\n"); return 1; }
    gc = XCreateGC(dpy, root, 0, NULL);
    XSetLocaleModifiers("");
    input_method = XOpenIM(dpy, NULL, NULL, NULL);
    if (!input_method) {
        fprintf(stderr, "Linsoft Native: cannot open X input method; basic input only\n");
        snprintf(message, sizeof(message), "Vstupná metóda UTF-8 nie je dostupná.");
    }
    preferences_load();
    wm_protocols = atom("WM_PROTOCOLS"); wm_delete = atom("WM_DELETE_WINDOW");
    active_atom = atom("_NET_ACTIVE_WINDOW"); clients_atom = atom("_NET_CLIENT_LIST");
    state_atom = atom("_NET_WM_STATE"); max_h = atom("_NET_WM_STATE_MAXIMIZED_HORZ");
    max_v = atom("_NET_WM_STATE_MAXIMIZED_VERT"); wm_state = atom("WM_STATE");
    utf8 = atom("UTF8_STRING"); name_atom = atom("_NET_WM_NAME");
    supported_atom = atom("_NET_SUPPORTED");
    support = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0, 0, 0);
    Atom check = atom("_NET_SUPPORTING_WM_CHECK");
    XChangeProperty(dpy, root, check, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&support, 1);
    XChangeProperty(dpy, support, check, XA_WINDOW, 32, PropModeReplace, (unsigned char *)&support, 1);
    XStoreName(dpy, support, "Linsoft Native");
    XChangeProperty(dpy, support, name_atom, utf8, 8, PropModeReplace,
                    (const unsigned char *)"Linsoft Native", 14);
    Atom supported[] = {active_atom, clients_atom, state_atom, max_h, max_v,
                        atom("_NET_CLOSE_WINDOW"), check, name_atom, atom("_NET_WORKAREA"),
                        atom("_NET_FRAME_EXTENTS"), atom("_NET_REQUEST_FRAME_EXTENTS"),
                        atom("_NET_NUMBER_OF_DESKTOPS"), atom("_NET_CURRENT_DESKTOP"),
                        atom("_NET_WM_DESKTOP"), atom("_NET_DESKTOP_NAMES"),
                        atom("_NET_WM_WINDOW_TYPE"), atom("_NET_WM_WINDOW_TYPE_NOTIFICATION")};
    XChangeProperty(dpy, root, supported_atom, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)supported, (int)(sizeof(supported) / sizeof(supported[0])));
    unsigned long desktop_count = WORKSPACES;
    XChangeProperty(dpy, root, atom("_NET_NUMBER_OF_DESKTOPS"), XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&desktop_count, 1);
    XChangeProperty(dpy, root, atom("_NET_CURRENT_DESKTOP"), XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&current_workspace, 1);
    const char desktop_names[] = "Plocha 1\0Plocha 2\0Plocha 3\0Plocha 4";
    XChangeProperty(dpy, root, atom("_NET_DESKTOP_NAMES"), utf8, 8, PropModeReplace,
                    (const unsigned char *)desktop_names, sizeof(desktop_names));
    workarea_update();
    desktop = shell_window(0, 0, (unsigned int)sw, (unsigned int)sh);
    panel = shell_window(0, sh - PANEL, (unsigned int)sw, PANEL);
    menu = shell_window(10, sh - PANEL - 476, 330, 466);
    context = shell_window(0, 0, 280, 320);
    XStoreName(dpy, desktop, "Linsoft Native Desktop");
    XStoreName(dpy, panel, "Linsoft Native Panel");
    XStoreName(dpy, menu, "Linsoft Start");
    XStoreName(dpy, context, "Linsoft Context");
    if (input_method)
        menu_input = XCreateIC(input_method, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                              XNClientWindow, menu, XNFocusWindow, menu, NULL);
    if (!menu_input) fprintf(stderr, "Linsoft Native: Start UTF-8 input context unavailable\n");
    XMapWindow(dpy, desktop); XLowerWindow(dpy, desktop); XMapRaised(dpy, panel);
    KeySym keys[] = {XK_t, XK_e, XK_i, XK_p, XK_space, XK_Up, XK_Down, XK_Left, XK_Right,
                     XK_F4, XK_Tab, XK_1, XK_2, XK_3, XK_4};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        unsigned int modifier = keys[i] == XK_F4 || keys[i] == XK_Tab ? Mod1Mask : Mod4Mask;
        for (int mask = 0; mask < 4; ++mask)
            XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i]), modifier |
                     (mask & 1 ? LockMask : 0) | (mask & 2 ? Mod2Mask : 0),
                     root, True, GrabModeAsync, GrabModeAsync);
        if (keys[i] >= XK_1 && keys[i] <= XK_4)
            for (int mask = 0; mask < 4; ++mask)
                XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i]), Mod4Mask | ShiftMask |
                         (mask & 1 ? LockMask : 0) | (mask & 2 ? Mod2Mask : 0),
                         root, True, GrabModeAsync, GrabModeAsync);
    }
    Window ignored_root, ignored_parent, *children = NULL;
    unsigned int count = 0;
    if (XQueryTree(dpy, root, &ignored_root, &ignored_parent, &children, &count)) {
        for (unsigned int i = 0; i < count; ++i) {
            XWindowAttributes attr;
            if (XGetWindowAttributes(dpy, children[i], &attr) &&
                attr.map_state == IsViewable && !attr.override_redirect) manage(children[i]);
        }
        XFree(children);
    }
    draw_desktop(); draw_panel(); update_clients();
    time_t last_minute = time(NULL) / 60;
    fprintf(stderr, "LINSOFT_NATIVE_READY: original window manager, panel and files; uid=%ld\n", (long)getuid());
    while (running && !stopping) {
        while (XPending(dpy)) {
            XEvent event;
            XNextEvent(dpy, &event);
            if (XFilterEvent(&event, None)) continue;
            Client *c = client(event.xany.window);
            switch (event.type) {
                case MappingNotify:
                    XRefreshKeyboardMapping(&event.xmapping);
                    break;
                case MapRequest: manage(event.xmaprequest.window); break;
                case DestroyNotify:
                    c = client(event.xdestroywindow.window);
                    if (c && event.xdestroywindow.window == c->win) unmanage(c, true);
                    break;
                case UnmapNotify:
                    c = client(event.xunmap.window);
                    if (c && event.xunmap.window == c->win &&
                        event.xunmap.event == c->frame) unmanage(c, false);
                    break;
                case ConfigureRequest: {
                    XConfigureRequestEvent *r = &event.xconfigurerequest;
                    c = client(r->window);
                    if (c && !c->maximized) {
                        if (r->value_mask & CWWidth) c->w = r->width > 100 ? r->width : 100;
                        if (r->value_mask & CWHeight) c->h = r->height > 60 ? r->height : 60;
                        geometry(c);
                    } else if (!c) {
                        XWindowChanges changes = {.x = r->x, .y = r->y, .width = r->width,
                            .height = r->height, .border_width = r->border_width,
                            .sibling = r->above, .stack_mode = r->detail};
                        XConfigureWindow(dpy, r->window, (unsigned int)r->value_mask, &changes);
                    }
                    break;
                }
                case ConfigureNotify:
                    if (event.xconfigure.window == root &&
                        (event.xconfigure.width != sw || event.xconfigure.height != sh)) {
                        sw = event.xconfigure.width; sh = event.xconfigure.height;
                        XMoveResizeWindow(dpy, desktop, 0, 0, (unsigned int)sw, (unsigned int)sh);
                        XMoveResizeWindow(dpy, panel, 0, sh - PANEL, (unsigned int)sw, PANEL);
                        XMoveWindow(dpy, menu, 10, sh - PANEL - 476);
                        workarea_update();
                        for (size_t i = 0; i < MAX_CLIENTS; ++i) {
                            Client *item = &clients[i];
                            if (!item->used) continue;
                            if (item->maximized) {
                                item->x = 0; item->y = 0;
                                item->w = sw; item->h = sh - PANEL - TITLE;
                            } else if (item->snapped) {
                                bool right = item->x > 0;
                                item->x = right ? sw / 2 : 0; item->y = 0;
                                item->w = right ? sw - sw / 2 : sw / 2;
                                item->h = sh - PANEL - TITLE;
                            } else {
                                if (item->w > sw) item->w = sw;
                                if (item->h > sh - PANEL - TITLE) item->h = sh - PANEL - TITLE;
                                if (item->x + item->w > sw) item->x = sw - item->w;
                                if (item->y + item->h + TITLE > sh - PANEL)
                                    item->y = sh - PANEL - item->h - TITLE;
                            }
                            geometry(item);
                        }
                        draw_desktop(); draw_panel();
                        if (*wallpaper_path) { wallpaper_load(); draw_desktop(); }
                    }
                    break;
                case Expose:
                    if (event.xexpose.count) break;
                    if (event.xany.window == desktop) draw_desktop();
                    else if (event.xany.window == panel) draw_panel();
                    else if (event.xany.window == menu) draw_menu();
                    else if (event.xany.window == files) draw_files();
                    else if (event.xany.window == context) draw_context();
                    else if (c && event.xany.window == c->frame) draw_frame(c);
                    break;
                case PropertyNotify:
                    if (c) { draw_frame(c); draw_panel(); }
                    else if (event.xproperty.window == root &&
                             event.xproperty.atom == atom("_LINSOFT_NATIVE_STATUS")) draw_panel();
                    break;
                case ButtonPress: button(&event.xbutton); break;
                case ButtonRelease:
                    if (event.xbutton.button == Button1) file_drop(&event.xbutton);
                    if (dragged && !resizing) {
                        if (event.xbutton.x_root < 8) snap(dragged, false);
                        else if (event.xbutton.x_root > sw - 8) snap(dragged, true);
                        else if (event.xbutton.y_root < 8) maximize(dragged);
                    }
                    dragged = NULL;
                    break;
                case MotionNotify:
                    if (file_drag_index >= 0 && event.xmotion.window == files &&
                        (abs(event.xmotion.x - file_drag_x) > 8 || abs(event.xmotion.y - file_drag_y) > 8))
                        file_drag_active = true;
                    if (dragged) {
                        int dx = event.xmotion.x_root - drag_x, dy = event.xmotion.y_root - drag_y;
                        if (resizing) {
                            int width = start_w + (resize_edges & 1 ? -dx : resize_edges & 2 ? dx : 0);
                            int height = start_h + (resize_edges & 4 ? -dy : resize_edges & 8 ? dy : 0);
                            dragged->w = width > 220 ? width : 220;
                            dragged->h = height > 150 ? height : 150;
                            if (resize_edges & 1) dragged->x = start_x + start_w - dragged->w;
                            if (resize_edges & 4) dragged->y = start_y + start_h - dragged->h;
                        } else {
                            dragged->x = start_x + dx; dragged->y = start_y + dy;
                            if (dragged->y < 0) dragged->y = 0;
                            if (dragged->y > sh - PANEL - TITLE) dragged->y = sh - PANEL - TITLE;
                        }
                        geometry(dragged);
                    }
                    break;
                case KeyPress: {
                    KeySym key = XLookupKeysym(&event.xkey, 0);
                    if (context_open || menu_open) {
                        char typed[256];
                        int typed_count = 0;
                        if (menu_open) {
                            if (menu_input && !event.xkey.send_event) {
                                Status result;
                                typed_count = Xutf8LookupString(menu_input, &event.xkey, typed,
                                                                sizeof(typed), &key, &result);
                                if (result == XBufferOverflow) typed_count = 0;
                            } else typed_count = XLookupString(&event.xkey, typed, sizeof(typed), &key, NULL);
                        }
                        int *choice = context_open ? &context_selected : &menu_selected;
                        int count = context_open ? (context_kind ? 10 : 4) : menu_count;
                        if (key == XK_Escape) {
                            if (context_open) context_hide(); else toggle_menu();
                        } else if ((key == XK_Up || key == XK_Down) && count) {
                            *choice = (*choice + (key == XK_Up ? count - 1 : 1)) % count;
                            if (context_open) draw_context(); else draw_menu();
                        } else if (key == XK_Return && count) {
                            int action = *choice;
                            if (context_open) context_action(action);
                            else { action = menu_matches[action]; toggle_menu(); launch(action); }
                        } else if (menu_open && key == XK_BackSpace) {
                            size_t length = strlen(menu_filter);
                            if (length) {
                                do { --length; } while (length && ((unsigned char)menu_filter[length] & 0xc0) == 0x80);
                                menu_filter[length] = '\0';
                            }
                            menu_search(); draw_menu();
                        } else if (menu_open && typed_count > 0 && (unsigned char)typed[0] >= 32 &&
                                   !(event.xkey.state & (ControlMask | Mod1Mask | Mod4Mask)) &&
                                   strlen(menu_filter) + (size_t)typed_count < sizeof(menu_filter)) {
                            size_t length = strlen(menu_filter);
                            memcpy(menu_filter + length, typed, (size_t)typed_count);
                            menu_filter[length + (size_t)typed_count] = '\0';
                            menu_search(); draw_menu();
                        }
                        break;
                    }
                    if (event.xkey.state & Mod4Mask) {
                        if (key >= XK_1 && key <= XK_4) {
                            if (event.xkey.state & ShiftMask) workspace_move(focused, key - XK_1);
                            else workspace_switch(key - XK_1);
                        }
                        else if (key == XK_t) launch(1);
                        else if (key == XK_e) launch(0);
                        else if (key == XK_i) launch(5);
                        else if (key == XK_p) preferences();
                        else if (key == XK_space) toggle_menu();
                        else if (key == XK_Up) maximize(focused);
                        else if (key == XK_Down) minimize(focused);
                        else if (key == XK_Left) snap(focused, false);
                        else if (key == XK_Right) snap(focused, true);
                    } else if ((event.xkey.state & Mod1Mask) && key == XK_F4) close_client(focused);
                    else if ((event.xkey.state & Mod1Mask) && key == XK_Tab) {
                        size_t start = focused ? (size_t)(focused - clients) + 1 : 0;
                        for (size_t i = 0; i < MAX_CLIENTS; ++i) {
                            Client *next = &clients[(start + i) % MAX_CLIENTS];
                            if (next->used && next->workspace == current_workspace) { focus(next); break; }
                        }
                    } else if (event.xkey.window == files) files_key(&event.xkey);
                    break;
                }
                case ClientMessage:
                    c = client(event.xclient.window);
                    if (event.xclient.message_type == atom("_NET_CURRENT_DESKTOP")) {
                        workspace_switch((unsigned long)event.xclient.data.l[0]);
                    } else if (c && event.xclient.message_type == atom("_NET_WM_DESKTOP")) {
                        workspace_move(c, (unsigned long)event.xclient.data.l[0]);
                    } else if (event.xclient.message_type == atom("_NET_REQUEST_FRAME_EXTENTS")) {
                        frame_extents(event.xclient.window);
                    } else if (event.xclient.message_type == command_atom) {
                        if (event.xclient.data.l[0] == 1) running = false;
                        else if (event.xclient.data.l[0] == 2) open_files();
                        else if (event.xclient.data.l[0] == 3) {
                            preferences_load(); draw_desktop(); draw_panel(); draw_files(); draw_menu();
                            for (size_t i = 0; i < MAX_CLIENTS; ++i)
                                if (clients[i].used) draw_frame(&clients[i]);
                        } else if (event.xclient.data.l[0] == 4) {
                            Atom type;
                            int format;
                            unsigned long count, remaining;
                            unsigned char *directory = NULL;
                            bool loaded = false;
                            if (XGetWindowProperty(dpy, event.xclient.window, atom("_LINSOFT_NATIVE_DIRECTORY"),
                                                  0, PATH_MAX / 4, True, XA_STRING,
                                                  &type, &format, &count, &remaining, &directory) == Success &&
                                directory && type == XA_STRING && format == 8 && count > 0 &&
                                count < PATH_MAX && !remaining && directory[0] == '/' &&
                                !memchr(directory, '\0', count)) {
                                open_files();
                                *file_filter = '\0';
                                loaded = load_directory((char *)directory);
                                draw_files();
                            }
                            if (directory) XFree(directory);
                            if (!loaded) fprintf(stderr, "Linsoft Native: cannot open requested directory\n");
                            XEvent reply = {0};
                            reply.xclient.type = ClientMessage;
                            reply.xclient.window = event.xclient.window;
                            reply.xclient.message_type = command_atom;
                            reply.xclient.format = 32;
                            reply.xclient.data.l[0] = loaded ? 0 : 1;
                            XSendEvent(dpy, reply.xclient.window, False, NoEventMask, &reply);
                            XFlush(dpy);
                        }
                    } else if (event.xclient.message_type == active_atom) focus(c);
                    else if (event.xclient.message_type == atom("_NET_CLOSE_WINDOW")) close_client(c);
                    else if (event.xclient.message_type == atom("WM_CHANGE_STATE") &&
                             event.xclient.data.l[0] == IconicState) minimize(c);
                    else if (c && event.xclient.message_type == state_atom &&
                             ((Atom)event.xclient.data.l[1] == max_h ||
                              (Atom)event.xclient.data.l[1] == max_v)) {
                        long action = event.xclient.data.l[0];
                        if (action == 2 || (action == 1 && !c->maximized) ||
                            (action == 0 && c->maximized)) maximize(c);
                    }
                    break;
                default: break;
            }
        }
        int child_status;
        pid_t child;
        while ((child = waitpid(-1, &child_status, WNOHANG)) > 0) {
            bool completed_file_job = child == file_job;
            if (completed_file_job) {
                file_job = 0;
                file_job_cancellable = false;
                file_job_cancel_requested = false;
                if (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) {
                    snprintf(message, sizeof(message), "Operacia dokoncena.");
                    if (file_job_move) {
                        for (size_t i = 0; i < clipboard_count; ++i) free(clipboard[i]);
                        clipboard_count = 0;
                    }
                } else if (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 2) {
                    snprintf(message, sizeof(message), "Operacia zrusena.");
                }
                if (*cwd) load_directory(cwd);
                draw_files();
            }
            if (!WIFEXITED(child_status) ||
                (WEXITSTATUS(child_status) != 0 &&
                 !(completed_file_job && WEXITSTATUS(child_status) == 2))) {
                snprintf(message, sizeof(message), "Operacia zlyhala (proces %ld, stav %d). Pozrite zaznam relacie.",
                         (long)child, child_status);
                fprintf(stderr, "Linsoft Native: %s\n", message);
                draw_desktop();
                draw_files();
            }
        }
        time_t minute = time(NULL) / 60;
        if (minute != last_minute) { last_minute = minute; draw_panel(); }
        XFlush(dpy);
        struct timespec pause = {.tv_sec = 0, .tv_nsec = 30000000};
        nanosleep(&pause, NULL);
    }
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i].used) {
            XReparentWindow(dpy, clients[i].win, root, clients[i].x, clients[i].y + TITLE);
            if (clients[i].win != files) XRemoveFromSaveSet(dpy, clients[i].win);
        }
    for (size_t i = 0; i < entry_count; ++i) free(entries[i].name);
    for (size_t i = 0; i < clipboard_count; ++i) free(clipboard[i]);
    free(entries);
    free(clipboard);
    if (files_input) XDestroyIC(files_input);
    if (menu_input) XDestroyIC(menu_input);
    g_list_free_full(applications, g_object_unref);
    if (preview) XFreePixmap(dpy, preview);
    if (input_method) XCloseIM(input_method);
    if (wallpaper) XFreePixmap(dpy, wallpaper);
    XDeleteProperty(dpy, root, active_atom);
    XDeleteProperty(dpy, root, clients_atom);
    XDeleteProperty(dpy, root, atom("_NET_SUPPORTING_WM_CHECK"));
    XftFontClose(dpy, font); XftFontClose(dpy, large);
    XftColorFree(dpy, visual, cmap, &ink); XftColorFree(dpy, visual, cmap, &muted);
    XftColorFree(dpy, visual, cmap, &accent);
    XFreeGC(dpy, gc);
    XCloseDisplay(dpy);
    return 0;
}
