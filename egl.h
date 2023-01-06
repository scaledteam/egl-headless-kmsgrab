#pragma once

#define BUFFER_WIDTH   1920
#define BUFFER_HEIGHT  1080

#include <EGL/egl.h>

int egl_init();
void egl_swap();

void egl_close();

EGLImageKHR
create_dmabuf_egl_image(unsigned int width,
			unsigned int height, uint32_t drm_format,
			uint32_t n_planes, const int *fds,
			const uint32_t *strides, const uint32_t *offsets,
			const uint64_t modifier);

void egl_destroy_image(EGLImageKHR eimg);
