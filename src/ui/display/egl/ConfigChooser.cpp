// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigChooser.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "ui/opengl/Features.hpp"

#ifdef MESA_KMS
#include "ui/canvas/egl/GBM.hpp"
#endif

#include <array>
#include <span>

namespace EGL {

#ifdef ANDROID

[[gnu::pure]]
static int
GetConfigAttrib(EGLDisplay display, EGLConfig config,
                int attribute, int default_value) noexcept
{
  int value;
  return eglGetConfigAttrib(display, config, attribute, &value)
    ? value
    : default_value;
}

[[gnu::pure]]
static int
AttribDistance(EGLDisplay display, EGLConfig config,
               int attribute, int want) noexcept
{
  int value = GetConfigAttrib(display, config, attribute, 0);
  int distance = std::abs(value - want);
  if (want > 0 && value == 0)
    /* big penalty if this attribute if zero, but XCSoar prefers it
       to be non-zero */
    distance += 100;
  else if (want == 0 && value > 0)
    /* small penalty if this attribute is non-zero, but XCSoar
       prefers it to be zero */
    distance += 10;
  return distance;
}

[[gnu::pure]]
static int
ConfigDistance(EGLDisplay display, EGLConfig config,
               int want_r, int want_g, int want_b, int want_a,
               int want_depth, int want_stencil) noexcept
{
  int distance = 0;

  int caveat = GetConfigAttrib(display, config,
                               EGL_CONFIG_CAVEAT, EGL_NONE);
  if (caveat != EGL_NONE)
    /* large penalty for unaccelerated software renderer configs */
    distance += 1000;

  int r = AttribDistance(display, config, EGL_RED_SIZE, want_r);
  int g = AttribDistance(display, config, EGL_GREEN_SIZE, want_g);
  int b = AttribDistance(display, config, EGL_BLUE_SIZE, want_b);
  int a = AttribDistance(display, config, EGL_ALPHA_SIZE, want_a);
  int d = AttribDistance(display, config, EGL_DEPTH_SIZE, want_depth);
  int s = AttribDistance(display, config, EGL_STENCIL_SIZE, want_stencil);
  int ms = AttribDistance(display, config, EGL_SAMPLES, OPENGL_MSAA_SAMPLES);
  int mb = AttribDistance(display, config, EGL_SAMPLE_BUFFERS,
                          OPENGL_MSAA_SAMPLES > 0 ? 1 : 0);

  return distance + r + g + b + a + d + s + ms + mb;
}

[[gnu::pure]]
static EGLConfig
FindClosestConfig(EGLDisplay display,
                  std::span<const EGLConfig> configs,
                  int want_r, int want_g, int want_b,
                  int want_a,
                  int want_depth, int want_stencil) noexcept
{
  EGLConfig closestConfig = nullptr;
  int closestDistance = 1000;

  for (EGLConfig config : configs) {
    int distance = ConfigDistance(display, config,
                                  want_r, want_g, want_b, want_a,
                                  want_depth, want_stencil);
    if (distance < closestDistance) {
      closestDistance = distance;
      closestConfig = config;
    }
  }

  return closestConfig;
}

#endif

#ifdef MESA_KMS

/**
 * Find an EGLConfig with the specified attribute value.
 *
 * @return an index into the #configs parameter or -1 if no matching
 * EGLConfig was found
 */
static EGLint
FindConfigWithAttribute(EGLDisplay display,
                        const EGLConfig *configs, EGLint num_configs,
                        EGLint attribute, EGLint expected_value) noexcept
{
  for (EGLint i = 0; i < num_configs; ++i) {
    EGLint value;
    if (eglGetConfigAttrib(display, configs[i], attribute, &value) &&
        value == expected_value)
      return i;
  }

  return -1;
}

#endif /* MESA_KMS */

static bool
QueryConfigs(EGLDisplay display, const EGLint *attributes,
             std::array<EGLConfig, 64> &configs, EGLint &num_configs)
{
  if (!eglChooseConfig(display, attributes, configs.data(), configs.size(),
                       &num_configs))
    throw FmtRuntimeError("eglChooseConfig() failed: {:#x}", eglGetError());

  return num_configs > 0;
}

EGLConfig
ChooseConfig(EGLDisplay display)
{
  std::array<EGLConfig, 64> configs;
  EGLint num_configs = 0;

#ifdef ANDROID
  static constexpr EGLint attributes[] = {
    EGL_RED_SIZE, 4,
    EGL_GREEN_SIZE, 4,
    EGL_BLUE_SIZE, 4,
    EGL_ALPHA_SIZE, 4,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  if (!QueryConfigs(display, attributes, configs, num_configs))
    throw std::runtime_error("eglChooseConfig() failed");

#elif defined(MESA_KMS)

#if OPENGL_MSAA_SAMPLES > 0
  static constexpr EGLint attributes_msaa[] = {
    EGL_STENCIL_SIZE, 1,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
#ifndef RASPBERRY_PI
    EGL_ALPHA_SIZE, 1,
#endif
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SAMPLE_BUFFERS, 1,
    EGL_SAMPLES, OPENGL_MSAA_SAMPLES,
    EGL_NONE
  };
#endif

  static constexpr EGLint attributes[] = {
    EGL_STENCIL_SIZE, 1,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
#ifndef RASPBERRY_PI
    EGL_ALPHA_SIZE, 1,
#endif
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

#if OPENGL_MSAA_SAMPLES > 0
  if (!QueryConfigs(display, attributes_msaa, configs, num_configs))
#endif
    if (!QueryConfigs(display, attributes, configs, num_configs))
      throw std::runtime_error("eglChooseConfig() failed");

#else /* !ANDROID && !MESA_KMS */

#if OPENGL_MSAA_SAMPLES > 0
  static constexpr EGLint attributes_msaa[] = {
    EGL_STENCIL_SIZE, 1,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SAMPLE_BUFFERS, 1,
    EGL_SAMPLES, OPENGL_MSAA_SAMPLES,
    EGL_NONE
  };
#endif

  static constexpr EGLint attributes[] = {
    EGL_STENCIL_SIZE, 1,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

#if OPENGL_MSAA_SAMPLES > 0
  if (!QueryConfigs(display, attributes_msaa, configs, num_configs))
#endif
    if (!QueryConfigs(display, attributes, configs, num_configs))
      throw std::runtime_error("eglChooseConfig() failed");

#endif /* platform attribute lists */

#ifdef MESA_KMS
  /* On some GBM targets, such as the Raspberry Pi 4,
     eglChooseConfig() gives us an EGLConfig which will later fail
     eglCreateWindowSurface() with EGL_BAD_MATCH.  Only the EGLConfig
     which has the matching EGL_NATIVE_VISUAL_ID will work. */
  EGLint i = FindConfigWithAttribute(display, configs.data(), num_configs,
                                     EGL_NATIVE_VISUAL_ID,
                                     XCSOAR_GBM_FORMAT);
  if (i < 0)
    i = FindConfigWithAttribute(display, configs.data(), num_configs,
                                EGL_NATIVE_VISUAL_ID,
                                XCSOAR_GBM_FORMAT_FALLBACK);
  return i >= 0 ? configs[i] : configs[0];
#elif defined(ANDROID)
  const auto closest_config =
    FindClosestConfig(display, {configs.data(), std::size_t(num_configs)},
                      8, 8, 8, 0, 0, 1);
  if (closest_config == nullptr)
    throw std::runtime_error("eglChooseConfig() failed");

  return closest_config;
#else
  return configs[0];
#endif
}

} // namespace EGL
