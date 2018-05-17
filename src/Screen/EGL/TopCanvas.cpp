/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/OpenGL/Init.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/Features.hpp"

#include <stdio.h>
#include <stdlib.h>

#ifdef MESA_KMS
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

constexpr const char * DEFAULT_DRI_DEVICE = "/dev/dri/card0";

struct drm_fb {
  struct gbm_bo *bo;
  uint32_t fb_id;
  int dri_fd;
};
#endif

/**
 * Returns the EGL API to bind to using eglBindAPI().
 */
static constexpr EGLenum
GetBindAPI()
{
  return HaveGLES()
    ? EGL_OPENGL_ES_API
    : EGL_OPENGL_API;
}

/**
 * Returns the requested renderable type for EGL_RENDERABLE_TYPE.
 */
static constexpr EGLint
GetRenderableType()
{
  return HaveGLES()
    ? (HaveGLES2() ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT)
    : EGL_OPENGL_BIT;
}

#if !defined(USE_X11) && !defined(USE_WAYLAND)

void
TopCanvas::Create(PixelSize new_size,
                  bool full_screen, bool resizable)
{
#ifdef USE_TTY
  InitialiseTTY();
#endif

#ifdef USE_VIDEOCORE
  vc_display = vc_dispmanx_display_open(0);
  vc_update = vc_dispmanx_update_start(0);

  VC_RECT_T dst_rect;
  dst_rect.x = dst_rect.y = 0;
  dst_rect.width = new_size.cx;
  dst_rect.height = new_size.cy;

  VC_RECT_T src_rect = dst_rect;
  src_rect.x = src_rect.y = 0;
  src_rect.width = new_size.cx << 16;
  src_rect.height = new_size.cy << 16;

  vc_element = vc_dispmanx_element_add(vc_update, vc_display,
                                       0, &dst_rect, 0, &src_rect,
                                       DISPMANX_PROTECTION_NONE,
                                       0, 0,
                                       DISPMANX_NO_ROTATE);
  vc_dispmanx_update_submit_sync(vc_update);

  vc_window.element = vc_element;
  vc_window.width = new_size.cx;
  vc_window.height = new_size.cy;

  const EGLNativeDisplayType native_display = EGL_DEFAULT_DISPLAY;
  const EGLNativeWindowType native_window = &vc_window;
#elif defined(HAVE_MALI)
  const EGLNativeDisplayType native_display = EGL_DEFAULT_DISPLAY;
  mali_native_window.width = new_size.cx;
  mali_native_window.height = new_size.cy;
  struct mali_native_window *native_window = &mali_native_window;
#elif defined(MESA_KMS)
  current_bo = nullptr;
  connector = nullptr;
  encoder = nullptr;
  saved_crtc = nullptr;

  const char* dri_device = getenv("DRI_DEVICE");
  if (nullptr == dri_device)
    dri_device = DEFAULT_DRI_DEVICE;
  printf("Using DRI device %s (use environment variable "
           "DRI_DEVICE to override)\n",
         dri_device);
  dri_fd = open(dri_device, O_RDWR);
  if (dri_fd == -1) {
    fprintf(stderr, "Could not open DRI device %s: %s\n", dri_device,
            strerror(errno));
    exit(EXIT_FAILURE);
  }
  native_display = gbm_create_device(dri_fd);
  if (native_display == nullptr) {
    fprintf(stderr, "Could not create GBM device\n");
    exit(EXIT_FAILURE);
  }

  evctx = { 0 };
  evctx.version = DRM_EVENT_CONTEXT_VERSION;
  evctx.page_flip_handler = [](int fd, unsigned int frame, unsigned int sec,
                               unsigned int usec, void *flip_finishedPtr) {
    *reinterpret_cast<bool*>(flip_finishedPtr) = true;
  };

  drmModeRes *resources = drmModeGetResources(dri_fd);
  if (resources == nullptr) {
    fprintf(stderr, "drmModeGetResources() failed\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0;
       (i < resources->count_connectors) && (connector == nullptr);
       ++i) {
    connector = drmModeGetConnector(dri_fd, resources->connectors[i]);
    if (nullptr != connector) {
      if ((connector->connection != DRM_MODE_CONNECTED) ||
          (connector->count_modes <= 0)) {
        drmModeFreeConnector(connector);
        connector = nullptr;
      }
    }
  }

  if (nullptr == connector) {
    fprintf(stderr, "No usable DRM connector found\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0;
       (i < resources->count_encoders) && (encoder == nullptr);
       i++) {
    encoder = drmModeGetEncoder(dri_fd, resources->encoders[i]);
    if (encoder != nullptr) {
      if (encoder->encoder_id != connector->encoder_id) {
        drmModeFreeEncoder(encoder);
        encoder = nullptr;
      }
    }
  }

  if (encoder == nullptr) {
    fprintf(stderr, "No usable DRM encoder found\n");
    exit(EXIT_FAILURE);
  }

  mode = connector->modes[0];

  native_window = gbm_surface_create(native_display, mode.hdisplay,
                                     mode.vdisplay,
                                     GBM_FORMAT_XRGB8888,
                                     GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  if (native_window == nullptr) {
    fprintf(stderr, "Could not create GBM surface\n");
    exit(EXIT_FAILURE);
  }
#endif

  CreateEGL(native_display, native_window);
}

#endif

void
TopCanvas::CreateEGL(EGLNativeDisplayType native_display,
                     EGLNativeWindowType native_window)
{
  display = eglGetDisplay(native_display);
  if (display == EGL_NO_DISPLAY) {
    fprintf(stderr, "eglGetDisplay(EGL_DEFAULT_DISPLAY) failed\n");
    exit(EXIT_FAILURE);
  }

  if (!eglInitialize(display, nullptr, nullptr)) {
    fprintf(stderr, "eglInitialize() failed\n");
    exit(EXIT_FAILURE);
  }

  if (!eglBindAPI(GetBindAPI())) {
    fprintf(stderr, "eglBindAPI() failed\n");
    exit(EXIT_FAILURE);
  }

  static constexpr EGLint attributes[] = {
    EGL_STENCIL_SIZE, 1,
#ifdef MESA_KMS
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_ALPHA_SIZE, 1,
#endif
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, GetRenderableType(),
    EGL_NONE
  };

  EGLint num_configs;
  EGLConfig chosen_config = 0;
  eglChooseConfig(display, attributes, &chosen_config, 1, &num_configs);
  if (num_configs == 0) {
    fprintf(stderr, "eglChooseConfig() failed\n");
    exit(EXIT_FAILURE);
  }

  surface = eglCreateWindowSurface(display, chosen_config,
                                   native_window, nullptr);
  if (surface == nullptr) {
    fprintf(stderr, "eglCreateWindowSurface() failed\n");
    exit(EXIT_FAILURE);
  }

  const PixelSize effective_size = GetNativeSize();
  if (effective_size.cx <= 0 || effective_size.cy <= 0) {
    fprintf(stderr, "eglQuerySurface() failed\n");
    exit(EXIT_FAILURE);
  }

#ifdef HAVE_GLES2
  static constexpr EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
#else
  const EGLint *context_attributes = nullptr;
#endif

  context = eglCreateContext(display, chosen_config,
                             EGL_NO_CONTEXT, context_attributes);

  if (!eglMakeCurrent(display, surface, surface, context)) {
    fprintf(stderr, "eglMakeCurrent() failed\n");
    exit(EXIT_FAILURE);
  }

  OpenGL::SetupContext();
  SetupViewport(effective_size);
}

void
TopCanvas::Destroy()
{
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(display, surface);
  eglDestroyContext(display, context);
  eglTerminate(display);

#ifdef USE_TTY
  DeinitialiseTTY();
#endif

#ifdef MESA_KMS
  if (nullptr != saved_crtc)
    drmModeSetCrtc(dri_fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                   saved_crtc->x, saved_crtc->y, &connector->connector_id, 1,
                   &saved_crtc->mode);
  gbm_surface_destroy(native_window);
  gbm_device_destroy(native_display);
  close(dri_fd);
#endif
}

PixelSize
TopCanvas::GetNativeSize() const
{
  GLint w, h;
  if (!eglQuerySurface(display, surface, EGL_WIDTH, &w) ||
      !eglQuerySurface(display, surface, EGL_HEIGHT, &h) ||
      w <= 0 || h <= 0)
    return PixelSize(0, 0);

  return PixelSize(w, h);
}

void
TopCanvas::Flip()
{
  if (!eglSwapBuffers(display, surface)) {
    fprintf(stderr, "eglSwapBuffers() failed: 0x%x\n", eglGetError());
    exit(EXIT_FAILURE);
  }

#ifdef MESA_KMS
  gbm_bo *new_bo = gbm_surface_lock_front_buffer(native_window);

  drm_fb *fb = (drm_fb*) gbm_bo_get_user_data(new_bo);
  if (!fb) {
    fb = new drm_fb;
    fb->bo = new_bo;
    fb->dri_fd = dri_fd;

    int ret = drmModeAddFB(dri_fd, gbm_bo_get_width(new_bo),
                           gbm_bo_get_height(new_bo), 24, 32,
                           gbm_bo_get_stride(new_bo),
                           gbm_bo_get_handle(new_bo).u32, &fb->fb_id);
    if (ret != 0) {
      fprintf(stderr, "drmModeAddFB() failed: %d\n", ret);
      exit(EXIT_FAILURE);
    }
  }

  gbm_bo_set_user_data(new_bo,
                       fb,
                       [](struct gbm_bo *bo, void *data) {
    struct drm_fb *fb = (struct drm_fb*) data;
    if (fb->fb_id)
      drmModeRmFB(fb->dri_fd, fb->fb_id);

    delete fb;
  });

  if (nullptr == current_bo) {
    saved_crtc = drmModeGetCrtc(dri_fd, encoder->crtc_id);
    drmModeSetCrtc(dri_fd, encoder->crtc_id, fb->fb_id, 0, 0,
                   &connector->connector_id, 1, &mode);
  }

  bool flip_finished = false;
  int page_flip_ret = drmModePageFlip(dri_fd, encoder->crtc_id, fb->fb_id,
                                      DRM_MODE_PAGE_FLIP_EVENT,
                                      &flip_finished);
  if (0 != page_flip_ret) {
    fprintf(stderr, "drmModePageFlip() failed: %d\n", page_flip_ret);
    exit(EXIT_FAILURE);
  }
  while (!flip_finished) {
    int handle_event_ret = drmHandleEvent(dri_fd, &evctx);
    if (0 != handle_event_ret) {
      fprintf(stderr, "drmHandleEvent() failed: %d\n", handle_event_ret);
      exit(EXIT_FAILURE);
    }
  }

  if (nullptr != current_bo)
    gbm_surface_release_buffer(native_window, current_bo);

  current_bo = new_bo;
#endif
}
