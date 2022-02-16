/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "ui/display/Display.hpp"
#include "ui/dim/Size.hpp"
#include "system/Error.hxx"
#include "util/RuntimeError.hxx"
#include "LogFile.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"

#include <android/native_window_jni.h>
#endif

#include <stdio.h>

#ifdef MESA_KMS
#include "GBM.hpp"
#include "DrmFrameBuffer.hpp"
#endif

#ifdef ANDROID

TopCanvas::TopCanvas(UI::Display &_display)
  :display(_display)
{
  /* no surface yet; it will be created later by AcquireSurface(); but
   * we need to make the context "current" */
  display.MakeCurrent(EGL_NO_SURFACE);
}

#elif defined(MESA_KMS)

TopCanvas::TopCanvas(UI::Display &_display)
  :display(_display),
   gbm_surface(display.GetGbmDevice(),
               display.GetMode().hdisplay,
               display.GetMode().vdisplay,
               XCSOAR_GBM_FORMAT,
               GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING)
{
  evctx = { 0 };
  evctx.version = DRM_EVENT_CONTEXT_VERSION;
  evctx.page_flip_handler = [](int fd, unsigned int frame, unsigned int sec,
                               unsigned int usec, void *flip_finishedPtr) {
    *reinterpret_cast<bool*>(flip_finishedPtr) = true;
  };

  CreateSurface(gbm_surface);
}

#endif // MESA_KMS

void
TopCanvas::CreateSurface(EGLNativeWindowType native_window)
{
  surface = display.CreateWindowSurface(native_window);

  const PixelSize effective_size = GetNativeSize();
  if (effective_size.width == 0 || effective_size.height == 0)
    throw std::runtime_error("eglQuerySurface() failed");

  display.MakeCurrent(surface);

  SetupViewport(effective_size);
}

TopCanvas::~TopCanvas() noexcept
{
  ReleaseSurface();

#ifdef MESA_KMS
  if (current_bo != nullptr)
    gbm_surface_release_buffer(gbm_surface, current_bo);

  if (nullptr != saved_crtc)
    display.ModeSetCrtc(saved_crtc->crtc_id, saved_crtc->buffer_id,
                        saved_crtc->x, saved_crtc->y,
                        &saved_crtc->mode);
#endif
}

PixelSize
TopCanvas::GetNativeSize() const noexcept
{
  auto w = display.QuerySurface(surface, EGL_WIDTH);
  auto h = display.QuerySurface(surface, EGL_HEIGHT);

  return w && h && *w > 0 && *h > 0
    ? PixelSize{*w, *h}
    : PixelSize{};
}

#ifdef ANDROID

bool
TopCanvas::AcquireSurface()
{
  const auto env = Java::GetEnv();
  const auto android_surface = native_view->GetSurface(env);
  if (!android_surface)
    /* failed - retry later */
    return false;

  ANativeWindow *native_window =
    ANativeWindow_fromSurface(env, android_surface.Get());
  CreateSurface(native_window);

  return true;
}

#endif // ANDROID

void
TopCanvas::ReleaseSurface() noexcept
{
  if (surface == EGL_NO_SURFACE)
    return;

  display.MakeCurrent(EGL_NO_SURFACE);
  display.DestroySurface(surface);
  surface = EGL_NO_SURFACE;
}

void
TopCanvas::Flip()
{
  if (!display.SwapBuffers(surface)) {
#ifdef ANDROID
    LogFormat("eglSwapBuffers() failed: 0x%x", eglGetError());
#else
    fprintf(stderr, "eglSwapBuffers() failed: 0x%x\n", eglGetError());
    exit(EXIT_FAILURE);
#endif
  }

#ifdef MESA_KMS
  const FileDescriptor dri_fd = display.GetDriFD();

  gbm_bo *new_bo = gbm_surface_lock_front_buffer(gbm_surface);

  auto *fb = (EGL::DrmFrameBuffer *)gbm_bo_get_user_data(new_bo);
  if (!fb) {
    fb = new EGL::DrmFrameBuffer(dri_fd, gbm_bo_get_width(new_bo),
                                 gbm_bo_get_height(new_bo), 24, 32,
                                 gbm_bo_get_stride(new_bo),
                                 gbm_bo_get_handle(new_bo).u32);

    gbm_bo_set_user_data(new_bo, fb, [](struct gbm_bo *bo, void *data) {
      auto *fb = (EGL::DrmFrameBuffer *)data;
      delete fb;
    });
  }

  if (nullptr == current_bo) {
    saved_crtc = display.ModeGetCrtc();
    display.ModeSetCrtc(fb->GetId(), 0, 0);
  } else {

    bool flip_finished = false;
    int page_flip_ret = display.ModePageFlip(fb->GetId(),
                                             DRM_MODE_PAGE_FLIP_EVENT,
                                             &flip_finished);
    if (0 != page_flip_ret) {
      fprintf(stderr, "drmModePageFlip() failed: %d\n", page_flip_ret);
      exit(EXIT_FAILURE);
    }
    while (!flip_finished) {
      int handle_event_ret = drmHandleEvent(dri_fd.Get(), &evctx);
      if (0 != handle_event_ret) {
        fprintf(stderr, "drmHandleEvent() failed: %d\n", handle_event_ret);
        exit(EXIT_FAILURE);
      }
    }

    gbm_surface_release_buffer(gbm_surface, current_bo);
  }

  current_bo = new_bo;
#endif
}
