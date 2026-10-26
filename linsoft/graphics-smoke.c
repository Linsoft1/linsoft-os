#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    EGLDisplay display = eglGetPlatformDisplay(
        EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL);
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int status = EXIT_FAILURE;
    EGLint major, minor, count;
    EGLConfig config;
    const EGLint attributes[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    const EGLint surface_attributes[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE
    };
    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };

    if (display == EGL_NO_DISPLAY ||
        !eglInitialize(display, &major, &minor)) {
        fprintf(stderr, "Cannot initialize surfaceless EGL: 0x%x\n",
                eglGetError());
        return EXIT_FAILURE;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API) ||
        !eglChooseConfig(display, attributes, &config, 1, &count) ||
        count != 1) {
        fprintf(stderr, "Cannot select an EGL ES2 configuration: 0x%x\n",
                eglGetError());
        goto cleanup;
    }
    surface = eglCreatePbufferSurface(display, config, surface_attributes);
    context = eglCreateContext(display, config, EGL_NO_CONTEXT,
                               context_attributes);
    if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT ||
        !eglMakeCurrent(display, surface, surface, context)) {
        fprintf(stderr, "Cannot create a current EGL ES2 context: 0x%x\n",
                eglGetError());
        goto cleanup;
    }

    GLubyte pixel[4] = {0};
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR || pixel[0] != 255 || pixel[1] != 0 ||
        pixel[2] != 0 || pixel[3] != 255) {
        fprintf(stderr, "EGL render/readback failed: GL=0x%x RGBA=%u,%u,%u,%u\n",
                error, pixel[0], pixel[1], pixel[2], pixel[3]);
        goto cleanup;
    }
    const GLubyte *renderer = glGetString(GL_RENDERER);
    if (!renderer) {
        fprintf(stderr, "OpenGL ES did not report a renderer.\n");
        goto cleanup;
    }
    printf("PASS: EGL %d.%d, renderer %s, RGBA=255,0,0,255\n",
           major, minor, renderer);
    status = EXIT_SUCCESS;

cleanup:
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (context != EGL_NO_CONTEXT)
        eglDestroyContext(display, context);
    if (surface != EGL_NO_SURFACE)
        eglDestroySurface(display, surface);
    eglTerminate(display);
    return status;
}
