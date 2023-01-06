
#include <stdio.h>
#include <unistd.h>

#include <GL/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdlib.h>

#include "colors.h"
#include "egl.h"

static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE , 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE  , 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE
};

static const EGLint pbufferAttribs[] = {
    EGL_WIDTH , BUFFER_WIDTH ,
    EGL_HEIGHT, BUFFER_HEIGHT,
    EGL_NONE,
};

static const EGLint contextAttribs[] = {
  EGL_CONTEXT_CLIENT_VERSION, 2,
  EGL_NONE,
};

static EGLDisplay display;
static EGLint major, minor;
static EGLint numConfigs;
static EGLConfig config;
static EGLSurface surface;
static EGLContext context;

static void check_err();

#define ATTR(attr)                                      \
   eglGetConfigAttrib(display, configs[i], attr, &val); \
   fprintf(stderr, "%*s: %d\n", 24, #attr, val);

// https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglChooseConfig.xhtml
// https://www.khronos.org/files/egl-1-4-quick-reference-card.pdf
static void egl_print_available_configs() {
    EGLint ccnt, n;
    eglGetConfigs(display, NULL, 0, &ccnt);
    fprintf(stderr, "EGL has %d configs total\n", ccnt);
    EGLConfig* configs = calloc(ccnt, sizeof *configs);
    eglChooseConfig(display, configAttribs, configs, ccnt, &n);
    for (int i = 0; i < n; i++) {
        fprintf(stderr, "Config id: %u\n", configs[i]);
        EGLint val;
        ATTR(EGL_BUFFER_SIZE);
        ATTR(EGL_RED_SIZE);
        ATTR(EGL_GREEN_SIZE);
        ATTR(EGL_BLUE_SIZE);
        ATTR(EGL_ALPHA_SIZE);
        ATTR(EGL_RENDERABLE_TYPE);
        ATTR(EGL_SURFACE_TYPE);
        // just choose the first one
        // egl_conf = configs[i];
        // break;
    }
}

#define ERR_CASE(err_id, text) case err_id : fprintf(stderr, "%s\n%s\n", KRED #err_id KRST, text); break;
static void print_err(GLint err_id) {
    switch(err_id) {
        ERR_CASE(EGL_SUCCESS            , "The last function succeeded without error.")
        ERR_CASE(EGL_NOT_INITIALIZED    , "EGL is not initialized, or could not be initialized, for the specified EGL display connection.")
        ERR_CASE(EGL_BAD_ACCESS         , "EGL cannot access a requested resource (for example a context is bound in another thread).")
        ERR_CASE(EGL_BAD_ALLOC          , "EGL failed to allocate resources for the requested operation.")
        ERR_CASE(EGL_BAD_ATTRIBUTE      , "An unrecognized attribute or attribute value was passed in the attribute list.")
        ERR_CASE(EGL_BAD_CONTEXT        , "An EGLContext argument does not name a valid EGL rendering context.")
        ERR_CASE(EGL_BAD_CONFIG         , "An EGLConfig argument does not name a valid EGL frame buffer configuration.")
        ERR_CASE(EGL_BAD_CURRENT_SURFACE, "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.")
        ERR_CASE(EGL_BAD_DISPLAY        , "An EGLDisplay argument does not name a valid EGL display connection.")
        ERR_CASE(EGL_BAD_SURFACE        , "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL rendering.")
        ERR_CASE(EGL_BAD_MATCH          , "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).")
        ERR_CASE(EGL_BAD_PARAMETER      , "One or more argument values are invalid.")
        ERR_CASE(EGL_BAD_NATIVE_PIXMAP  , "A NativePixmapType argument does not refer to a valid native pixmap.")
        ERR_CASE(EGL_BAD_NATIVE_WINDOW  , "A NativeWindowType argument does not refer to a valid native window.")
        ERR_CASE(EGL_CONTEXT_LOST       , "A power management event has occurred. The application must destroy all contexts and reinitialise OpenGL ES state and objects to continue rendering.")
    }
}
#undef ERR_CASE

#define DEBUG
#ifdef DEBUG
#define ERR(res, cmd, ...) {               \
    res = cmd(__VA_ARGS__);                \
    EGLint err_id = eglGetError();         \
    fprintf(stderr, #cmd" - ");            \
    if(err_id != EGL_SUCCESS) {            \
        print_err(err_id);                 \
        exit(-1);                          \
    } else {                               \
        fprintf(stderr, KGRN "OK\n" KRST); \
    }                                      \
}
#else
#define ERR(res, cmd, ...) {                    \
        res = cmd(__VA_ARGS__);                 \
        EGLint err_id = eglGetError();          \
        if(err_id != EGL_SUCCESS) exit(-1);    \
}
#endif

/*
  https://jan.newmarch.name/Wayland/EGL/

  EGL has a display that it writes on. The display is built on a native display,
  and is obtained by the call eglGetDisplay. The EGL platform is then
  initialised using eglInitialize.

  Typically an EGL display will support a number of configurations. For example,
  a pixel may be 16 bits (5 red, 5 blue and 6 green), 24 bits (8 red, 8 green
  and 8 blue) or 32 bits (8 extra bits for alpha transparency). An application
  will specify certain parameters such as the minimum size of a red pixel, and
  can then access the array of matching configurations using eglChooseConfig.
  The attributes of a configuration can be queried using eglGetConfigAttrib. One
  configuration should be chosen before proceeding.

  Each configuration will support one or more client APIs such as OpenGL. The
  API is usually requested through the configuration attribute
  EGL_RENDERABLE_TYPE which should have a value such as EGL_OPENGL_ES2_BIT.

  In addition to a configuration, each application needs one or more contexts.
  Each context defines a level of the API that will be used for rendering.
  Examples typically use a level of 2, and a context is created using
  eglCreateContext.
*/

int egl_init() {

    EGLBoolean res;

    // 1. Initialize EGL
    ERR(display, eglGetDisplay, EGL_DEFAULT_DISPLAY);
    ERR(res, eglInitialize, display, &major, &minor);
    fprintf(stderr, "version %d.%d\n", major, minor);
    egl_print_available_configs();

    // 2. Select an appropriate configuration
    ERR(res, eglChooseConfig, display, configAttribs, &config, 1, &numConfigs);

    // 3. Create a surface
    ERR(surface, eglCreatePbufferSurface, display, config, pbufferAttribs);

    // 4. Bind the API
    ERR(res, eglBindAPI, EGL_OPENGL_API);

    // 5. Create a context and make it current
    ERR(context, eglCreateContext, display, config, EGL_NO_CONTEXT, contextAttribs);
    ERR(res, eglMakeCurrent, display, surface, surface, context);
    return 0;
}

void egl_swap() {
	glFlush();
    eglSwapBuffers(display, surface);
}

void egl_close() {
    // 6. Terminate EGL when finished
    eglTerminate(display);
}

static void check_err() {
   EGLint err = eglGetError();
   if(err!=0x3000) {
       fprintf(stderr, "Error %h\n", err);
   } else {
       fprintf(stderr, "OK\n");
   }
}

EGLImageKHR
create_dmabuf_egl_image(unsigned int width,
			unsigned int height, uint32_t drm_format,
			uint32_t n_planes, const int *fds,
			const uint32_t *strides, const uint32_t *offsets,
			const uint64_t modifier)
{
	EGLAttrib attribs[47];
	int atti = 0;

	/* This requires the Mesa commit in
	 * Mesa 10.3 (08264e5dad4df448e7718e782ad9077902089a07) or
	 * Mesa 10.2.7 (55d28925e6109a4afd61f109e845a8a51bd17652).
	 * Otherwise Mesa closes the fd behind our back and re-importing
	 * will fail.
	 * https://bugs.freedesktop.org/show_bug.cgi?id=76188
	 * */

	attribs[atti++] = EGL_WIDTH;
	attribs[atti++] = width;
	attribs[atti++] = EGL_HEIGHT;
	attribs[atti++] = height;
	attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
	attribs[atti++] = drm_format;

	if (n_planes > 0) {
		attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
		attribs[atti++] = fds[0];
		attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
		attribs[atti++] = offsets[0];
		attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
		attribs[atti++] = strides[0];
		if (modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
			attribs[atti++] = modifier & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
			attribs[atti++] = modifier >> 32;
		}
	}

	if (n_planes > 1) {
		attribs[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
		attribs[atti++] = fds[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
		attribs[atti++] = offsets[1];
		attribs[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
		attribs[atti++] = strides[1];
		if (modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT;
			attribs[atti++] = modifier & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT;
			attribs[atti++] = modifier >> 32;
		}
	}

	if (n_planes > 2) {
		attribs[atti++] = EGL_DMA_BUF_PLANE2_FD_EXT;
		attribs[atti++] = fds[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
		attribs[atti++] = offsets[2];
		attribs[atti++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
		attribs[atti++] = strides[2];
		if (modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT;
			attribs[atti++] = modifier & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT;
			attribs[atti++] = modifier >> 32;
		}
	}

	if (n_planes > 3) {
		attribs[atti++] = EGL_DMA_BUF_PLANE3_FD_EXT;
		attribs[atti++] = fds[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_OFFSET_EXT;
		attribs[atti++] = offsets[3];
		attribs[atti++] = EGL_DMA_BUF_PLANE3_PITCH_EXT;
		attribs[atti++] = strides[3];
		if (modifier) {
			attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT;
			attribs[atti++] = modifier & 0xFFFFFFFF;
			attribs[atti++] = EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT;
			attribs[atti++] = modifier >> 32;
		}
	}

	attribs[atti++] = EGL_NONE;

	return eglCreateImage(display, EGL_NO_CONTEXT,
			      EGL_LINUX_DMA_BUF_EXT, 0, attribs);
}

void egl_destroy_image(EGLImageKHR eimg) {
	eglDestroyImage(display, eimg);
}
