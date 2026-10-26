#define _DEFAULT_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_mode.h>

static volatile sig_atomic_t stopping;
static void stop(int signal_number) { (void)signal_number; stopping = 1; }

static uint32_t component(unsigned value, struct fb_bitfield field)
{
    return (value * ((1U << field.length) - 1) / 255) << field.offset;
}

static void pixel(unsigned char *memory, const struct fb_fix_screeninfo *fixed,
                  const struct fb_var_screeninfo *screen, int x, int y,
                  unsigned red, unsigned green, unsigned blue)
{
    if (x < 0 || y < 0 || x >= (int)screen->xres || y >= (int)screen->yres) return;
    uint32_t colour = component(red, screen->red) | component(green, screen->green) |
                      component(blue, screen->blue);
    size_t offset = (size_t)(y + screen->yoffset) * fixed->line_length +
                    (size_t)(x + screen->xoffset) * 4;
    *(uint32_t *)(memory + offset) = colour;
}

int main(void)
{
    FILE *asset = fopen("/usr/share/linsoft-boot/splash.ppm", "rb");
    unsigned width, height, maximum;
    if (!asset || fscanf(asset, "P6\n%u %u\n%u", &width, &height, &maximum) != 3 ||
        width != 960 || height != 540 || maximum != 255 || fgetc(asset) != '\n') {
        fprintf(stderr, "Linsoft splash: invalid/missing artwork.\n");
        return 1;
    }
    unsigned char *art = malloc((size_t)width * height * 3);
    if (!art || fread(art, 3, (size_t)width * height, asset) != (size_t)width * height) {
        fprintf(stderr, "Linsoft splash: cannot read artwork.\n");
        return 1;
    }
    fclose(asset);
    signal(SIGTERM, stop);
    signal(SIGINT, stop);
    int fb = -1;
    for (int attempt = 0; attempt < 150 && !stopping; ++attempt) {
        fb = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
        if (fb >= 0) break;
        usleep(100000);
    }
    if (fb < 0) { perror("Linsoft splash: DRM device"); return 1; }
    drmModeRes *resources = drmModeGetResources(fb);
    drmModeConnector *connector = NULL;
    if (resources) {
        for (int i = 0; i < resources->count_connectors; ++i) {
            connector = drmModeGetConnector(fb, resources->connectors[i]);
            if (connector && connector->connection == DRM_MODE_CONNECTED && connector->count_modes) break;
            if (connector) drmModeFreeConnector(connector);
            connector = NULL;
        }
    }
    drmModeEncoder *encoder = connector ? drmModeGetEncoder(fb, connector->encoder_id) : NULL;
    if (!resources || !connector || !encoder || !encoder->crtc_id) {
        fprintf(stderr, "Linsoft splash: no connected DRM display.\n");
        return 1;
    }
    drmModeModeInfo mode = connector->modes[0];
    struct drm_mode_create_dumb buffer = { .width = mode.hdisplay, .height = mode.vdisplay, .bpp = 32 };
    uint32_t framebuffer;
    struct drm_mode_map_dumb mapping = {0};
    if (ioctl(fb, DRM_IOCTL_MODE_CREATE_DUMB, &buffer) ||
        drmModeAddFB(fb, buffer.width, buffer.height, 24, 32, buffer.pitch, buffer.handle, &framebuffer)) {
        perror("Linsoft splash: create scanout"); return 1;
    }
    mapping.handle = buffer.handle;
    if (ioctl(fb, DRM_IOCTL_MODE_MAP_DUMB, &mapping)) { perror("Linsoft splash: map"); return 1; }
    struct fb_fix_screeninfo fixed = { .line_length = buffer.pitch, .smem_len = buffer.size };
    struct fb_var_screeninfo screen = { .xres = buffer.width, .yres = buffer.height,
        .red = {.offset = 16, .length = 8}, .green = {.offset = 8, .length = 8},
        .blue = {.offset = 0, .length = 8} };
    unsigned char *memory = mmap(NULL, buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, mapping.offset);
    int tty = open("/dev/tty7", O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (memory == MAP_FAILED || tty < 0) {
        perror("Linsoft splash: framebuffer/console");
        return 1;
    }
    struct termios original, raw;
    if (tcgetattr(tty, &original)) { perror("Linsoft splash: terminal"); return 1; }
    raw = original;
    cfmakeraw(&raw);
    if (tcsetattr(tty, TCSANOW, &raw) || ioctl(tty, KDSETMODE, KD_GRAPHICS) ||
        ioctl(tty, VT_ACTIVATE, 7) || ioctl(tty, VT_WAITACTIVE, 7)) {
        perror("Linsoft splash: activate console");
        tcsetattr(tty, TCSANOW, &original);
        ioctl(tty, KDSETMODE, KD_TEXT);
        return 1;
    }
    double scale = fmin(1.0, fmin(screen.xres / 960.0, screen.yres / 540.0));
    int aw = (int)(width * scale), ah = (int)(height * scale);
    int left = ((int)screen.xres - aw) / 2, top = ((int)screen.yres - ah) / 2;
    for (unsigned y = 0; y < screen.yres; ++y)
        for (unsigned x = 0; x < screen.xres; ++x)
            pixel(memory, &fixed, &screen, (int)x, (int)y, 9, 16, 32);
    for (int y = 0; y < ah; ++y) {
        for (int x = 0; x < aw; ++x) {
            size_t index = ((size_t)(y / scale) * width + (unsigned)(x / scale)) * 3;
            pixel(memory, &fixed, &screen, x + left, y + top, art[index], art[index+1], art[index+2]);
        }
    }
    free(art);
    if (drmModeSetCrtc(fb, encoder->crtc_id, framebuffer, 0, 0, &connector->connector_id, 1, &mode)) {
        perror("Linsoft splash: display logo");
        tcsetattr(tty, TCSANOW, &original);
        ioctl(tty, KDSETMODE, KD_TEXT);
        ioctl(tty, VT_ACTIVATE, 1);
        return 1;
    }
    fprintf(stderr, "LINSOFT_SPLASH_READY\n");
    time_t started = time(NULL);
    unsigned frame = 0;
    int fallback = 0;
    while (!stopping) {
        struct vt_stat state;
        if (ioctl(tty, VT_GETSTATE, &state)) { perror("Linsoft splash: console state"); fallback = 1; break; }
        if (state.v_active != 7) break;
        unsigned char key;
        ssize_t count = read(tty, &key, 1);
        if (count == 1 && key == 27) {
            fprintf(stderr, "LINSOFT_SPLASH_SKIPPED\n");
            fallback = 1; break;
        }
        if (count < 0 && errno != EAGAIN && errno != EINTR) {
            perror("Linsoft splash: keyboard"); fallback = 1; break;
        }
        if (time(NULL) - started > 120) {
            fprintf(stderr, "Linsoft splash: startup timeout; releasing the display.\n");
            fallback = 1; break;
        }
        for (int dot = 0; dot < 12; ++dot) {
            double angle = dot * 6.28318530718 / 12;
            int cx = (int)screen.xres / 2 + (int)(cos(angle) * 19 * scale);
            int cy = top + (int)(383 * scale) + (int)(sin(angle) * 19 * scale);
            unsigned distance = (dot + 12 - (frame / 2) % 12) % 12;
            unsigned light = 45 + (11 - distance) * 18;
            for (int dy = -3; dy <= 3; ++dy)
                for (int dx = -3; dx <= 3; ++dx)
                    if (dx*dx + dy*dy <= 9)
                        pixel(memory, &fixed, &screen, cx+dx, cy+dy, light/2, light*4/5, light);
        }
        if (drmModeDirtyFB(fb, framebuffer, NULL, 0) && errno != ENOSYS && errno != EOPNOTSUPP) {
            perror("Linsoft splash: refresh"); fallback = 1; break;
        }
        ++frame;
        usleep(50000);
    }
    tcsetattr(tty, TCSANOW, &original);
    ioctl(tty, KDSETMODE, KD_TEXT);
    if (fallback) ioctl(tty, VT_ACTIVATE, 1);
    munmap(memory, fixed.smem_len);
    close(fb);
    close(tty);
    drmModeFreeEncoder(encoder);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    return 0;
}
