#define main desktop_main
#include "linsoft-native.c"
#undef main
#include <assert.h>
#include <stdint.h>

static void wait_active(Window expected) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        Atom type;
        int format;
        unsigned long count, after;
        unsigned char *value = NULL;
        Window active = None;
        if (XGetWindowProperty(dpy, root, atom("_NET_ACTIVE_WINDOW"), 0, 1, False,
                              XA_WINDOW, &type, &format, &count, &after, &value) == Success &&
            value && type == XA_WINDOW && format == 32 && count == 1)
            active = *(Window *)value;
        if (value) XFree(value);
        if (active == expected) return;
        usleep(10000);
    }
    assert(!"window focus did not reach expected client");
}

int main(int argc, char **argv) {
    assert(wallpaper_dimensions_valid(6000, 4000));
    assert(!wallpaper_dimensions_valid(6001, 4000));
    assert(!wallpaper_dimensions_valid(0, 4000));
    assert(matches("ŽLTÝ priečinok", "žltý"));
    assert(!matches("Writer", "Calc"));
    void *resizable = NULL;
    size_t capacity = 0;
    resizable = reserve_array(resizable, &capacity, 8193, sizeof(uint32_t));
    assert(resizable);
    assert(capacity >= 8193);
    assert(!reserve_array(resizable, &capacity, SIZE_MAX, sizeof(uint64_t)) && errno == EOVERFLOW);
    free(resizable);
    if (argc == 4 && !strcmp(argv[1], "--directory-count")) {
        dpy = XOpenDisplay(NULL);
        assert(dpy);
        char *end = NULL;
        unsigned long long expected = strtoull(argv[3], &end, 10);
        assert(end && !*end && expected <= SIZE_MAX);
        assert(load_directory(argv[2]));
        assert(entry_count == (size_t)expected);
        for (size_t i = 0; i < entry_count; ++i) free(entries[i].name);
        free(entries);
        XCloseDisplay(dpy);
        puts("PASS: directory enumeration grows beyond the former fixed limit");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--notification")) {
        dpy = XOpenDisplay(NULL);
        assert(dpy);
        screen = DefaultScreen(dpy); root = RootWindow(dpy, screen);
        Window parent = XCreateSimpleWindow(dpy, root, 200, 200, 600, 400, 0, 0, 0);
        XStoreName(dpy, parent, "Native Notification Focus");
        XMapWindow(dpy, parent); XFlush(dpy);
        wait_active(parent);
        Window notice = XCreateSimpleWindow(dpy, root, 900, 40, 300, 100, 0, 0, 0);
        Atom type = atom("_NET_WM_WINDOW_TYPE_NOTIFICATION");
        XChangeProperty(dpy, notice, atom("_NET_WM_WINDOW_TYPE"), XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)&type, 1);
        XMapWindow(dpy, notice); XFlush(dpy);
        XWindowAttributes attr;
        for (int attempt = 0; attempt < 200; ++attempt) {
            assert(XGetWindowAttributes(dpy, notice, &attr));
            if (attr.map_state == IsViewable) break;
            usleep(10000);
        }
        assert(attr.map_state == IsViewable);
        Window returned_root, returned_parent, *children;
        unsigned int count;
        assert(XQueryTree(dpy, notice, &returned_root, &returned_parent, &children, &count));
        if (children) XFree(children);
        assert(returned_parent == root);
        assert(attr.x == 900 && attr.y == 40 && attr.width == 300 && attr.height == 100);
        Atom actual_type;
        int format;
        unsigned long length, remaining;
        unsigned char *extents = NULL;
        assert(XGetWindowProperty(dpy, notice, atom("_NET_FRAME_EXTENTS"), 0, 4, False,
                                  XA_CARDINAL, &actual_type, &format, &length, &remaining, &extents) == Success);
        assert(extents && actual_type == XA_CARDINAL && format == 32 && length == 4);
        for (unsigned long i = 0; i < length; ++i) assert(((unsigned long *)extents)[i] == 0);
        XFree(extents);
        wait_active(parent);
        XDestroyWindow(dpy, notice); XDestroyWindow(dpy, parent);
        XSync(dpy, False); XCloseDisplay(dpy);
        puts("PASS: notifications keep position/size, remain undecorated and never take application focus");
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--preview-pixel")) {
        dpy = XOpenDisplay(NULL);
        assert(dpy);
        screen = DefaultScreen(dpy);
        Window window = strtoul(argv[2], NULL, 10);
        XWindowAttributes attr; assert(XGetWindowAttributes(dpy, window, &attr));
        XImage *image = XGetImage(dpy, window, attr.width - 110, FILE_ROWS_Y + 90,
                                  1, 1, AllPlanes, ZPixmap);
        unsigned long pixel = image ? XGetPixel(image, 0, 0) : 0;
        Visual *visual = DefaultVisual(dpy, screen);
        assert(image);
        assert(abs((int)((pixel & visual->red_mask) * 255 / visual->red_mask) - 38) <= 3);
        assert(abs((int)((pixel & visual->green_mask) * 255 / visual->green_mask) - 68) <= 3);
        assert(abs((int)((pixel & visual->blue_mask) * 255 / visual->blue_mask) - 90) <= 3);
        XDestroyImage(image); XCloseDisplay(dpy);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--modal")) {
        dpy = XOpenDisplay(NULL);
        assert(dpy);
        screen = DefaultScreen(dpy); root = RootWindow(dpy, screen);
        Window parent = XCreateSimpleWindow(dpy, root, 200, 200, 600, 400, 0, 0, 0);
        XStoreName(dpy, parent, "Native Modal Parent");
        XMapWindow(dpy, parent); XFlush(dpy);
        wait_active(parent);
        Window child = XCreateSimpleWindow(dpy, root, 0, 0, 260, 160, 0, 0, 0);
        XStoreName(dpy, child, "Native Modal Child");
        XSetTransientForHint(dpy, child, parent);
        Atom modal = atom("_NET_WM_STATE_MODAL");
        XChangeProperty(dpy, child, atom("_NET_WM_STATE"), XA_ATOM, 32, PropModeReplace,
                        (unsigned char *)&modal, 1);
        XMapWindow(dpy, child); XFlush(dpy);
        wait_active(child);
        Window ignored;
        int px, py, cx, cy;
        XTranslateCoordinates(dpy, parent, root, 0, 0, &px, &py, &ignored);
        XTranslateCoordinates(dpy, child, root, 0, 0, &cx, &cy, &ignored);
        assert(abs(cx - (px + 170)) <= 2 && abs(cy - (py + 120)) <= 2);
        XEvent request = {0};
        request.xclient.type = ClientMessage;
        request.xclient.window = parent;
        request.xclient.message_type = atom("_NET_ACTIVE_WINDOW");
        request.xclient.format = 32;
        XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &request);
        XFlush(dpy); usleep(150000);
        wait_active(child);
        request.xclient.window = child;
        request.xclient.message_type = atom("_NET_WM_STATE");
        request.xclient.data.l[0] = 1;
        request.xclient.data.l[1] = (long)atom("_NET_WM_STATE_MAXIMIZED_HORZ");
        request.xclient.data.l[2] = (long)atom("_NET_WM_STATE_MAXIMIZED_VERT");
        for (int maximized = 1; maximized >= 0; --maximized) {
            request.xclient.data.l[0] = maximized;
            XSendEvent(dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &request);
            XFlush(dpy); usleep(150000);
            Atom type;
            int format;
            unsigned long count, after;
            unsigned char *value = NULL;
            assert(XGetWindowProperty(dpy, child, atom("_NET_WM_STATE"), 0, 64, False,
                                      XA_ATOM, &type, &format, &count, &after, &value) == Success);
            bool found = false;
            for (unsigned long i = 0; i < count; ++i) if (((Atom *)value)[i] == modal) found = true;
            assert(found && count == (maximized ? 3UL : 1UL));
            XFree(value);
        }
        XDestroyWindow(dpy, child); XFlush(dpy);
        wait_active(parent);
        XDestroyWindow(dpy, parent); XSync(dpy, False);
        XCloseDisplay(dpy);
        puts("PASS: real transient/modal focus, centering, parent focus restoration and retained EWMH state");
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--write-wallpaper")) {
        png_image image = {.version = PNG_IMAGE_VERSION, .width = 2, .height = 2,
                           .format = PNG_FORMAT_RGB};
        const unsigned char pixels[] = {38, 68, 90, 38, 68, 90, 38, 68, 90, 38, 68, 90};
        assert(png_image_write_to_file(&image, argv[2], 0, pixels, 0, NULL));
        png_image_free(&image);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--pixel")) {
        dpy = XOpenDisplay(NULL);
        assert(dpy);
        screen = DefaultScreen(dpy);
        root = RootWindow(dpy, screen);
        XImage *image = XGetImage(dpy, root, 1100, 600, 1, 1, AllPlanes, ZPixmap);
        assert(image && XGetPixel(image, 0, 0) == color(argv[2]));
        XDestroyImage(image);
        XCloseDisplay(dpy);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--write-jpeg")) {
        GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, 64, 64);
        assert(pixbuf);
        int stride = gdk_pixbuf_get_rowstride(pixbuf);
        guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                guchar *p = pixels + y * stride + x * 3;
                p[0] = 38; p[1] = 68; p[2] = 90;
            }
        GError *error = NULL;
        assert(gdk_pixbuf_save(pixbuf, argv[2], "jpeg", &error, "quality", "92", NULL));
        g_clear_error(&error);
        g_object_unref(pixbuf);
        return 0;
    }
    assert(argc == 1);
    assert(file_icon("folder.txt", true) == ICON_FOLDER);
    assert(file_icon("notes.TXT", false) == ICON_DOCUMENT);
    assert(file_icon("sheet.ods", false) == ICON_SHEET);
    assert(file_icon("photo.PNG", false) == ICON_IMAGE);
    assert(file_icon("song.flac", false) == ICON_AUDIO);
    assert(file_icon("clip.webm", false) == ICON_VIDEO);
    assert(file_icon("backup.tar.xz", false) == ICON_ARCHIVE);
    assert(file_icon("installer.deb", false) == ICON_ARCHIVE);
    assert(file_icon("README", false) == ICON_FILE);
    assert(file_icon("unknown.xyz", false) == ICON_FILE);

    dpy = XOpenDisplay(NULL);
    assert(dpy);
    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    gc = XCreateGC(dpy, root, 0, NULL);
    bg = color("#0b1425");
    icon_cyan = color("#54d4de"); icon_light = color("#b8d9ff");
    icon_green = color("#69dfb0"); icon_gold = color("#f0c56e"); icon_purple = color("#bca0f5");
    XSetErrorHandler(xerror);
    Window fixture = XCreateSimpleWindow(dpy, root, 0, 0, 32, 32, 0, 0, bg);
    files = fixture;
    assert(window_icon(fixture) == ICON_FOLDER);
    files = None;
    char *names[] = {"xterm", "navigator", "libreoffice-calc", "libreoffice-writer",
                     "pavucontrol", "zenity", "unknown"};
    char *classes[] = {"XTerm", "firefox-esr", "libreoffice", "libreoffice",
                       "Pavucontrol", "Zenity", "Unknown"};
    const Icon expected[] = {ICON_TERMINAL, ICON_BROWSER, ICON_SHEET, ICON_DOCUMENT,
                             ICON_SOUND, ICON_SETTINGS, ICON_APPS};
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        XClassHint hint = {.res_name = names[i], .res_class = classes[i]};
        XSetClassHint(dpy, fixture, &hint);
        assert(window_icon(fixture) == expected[i]);
    }
    Pixmap canvas = XCreatePixmap(dpy, root, 128, 128, (unsigned int)DefaultDepth(dpy, screen));
    const int sizes[] = {16, 20, 22, 24, 26, 36, 48, 96};
    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); ++s) {
        uint64_t hashes[ICON_COUNT];
        for (int icon = 0; icon < ICON_COUNT; ++icon) {
            rect(canvas, 0, 0, 128, 128, bg);
            draw_icon(canvas, 8, 8, sizes[s], (Icon)icon);
            XImage *image = XGetImage(dpy, canvas, 0, 0, 128, 128, AllPlanes, ZPixmap);
            assert(image);
            size_t colored = 0;
            uint64_t hash = UINT64_C(14695981039346656037);
            for (int y = 0; y < 128; ++y) {
                for (int x = 0; x < 128; ++x) {
                    unsigned long pixel = XGetPixel(image, x, y);
                    if (pixel != bg) {
                        assert(x >= 8 && x < 8 + sizes[s]);
                        assert(y >= 8 && y < 8 + sizes[s]);
                        assert(pixel == icon_cyan || pixel == icon_light || pixel == icon_green ||
                               pixel == icon_gold || pixel == icon_purple);
                        ++colored;
                    }
                    hash = (hash ^ pixel) * UINT64_C(1099511628211);
                }
            }
            assert(colored >= 12);
            for (int previous = 0; previous < icon; ++previous) assert(hash != hashes[previous]);
            hashes[icon] = hash;
            XDestroyImage(image);
        }
    }
    XFreePixmap(dpy, canvas);
    XDestroyWindow(dpy, fixture);
    XFreeGC(dpy, gc);
    XCloseDisplay(dpy);
    puts("PASS: 22 distinct original icons at 8 sizes, bounded pixels, palette, file types and WM_CLASS");
    return 0;
}
