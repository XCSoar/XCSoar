// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigChooser.hpp"
#include "ui/canvas/opengl/Globals.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "LogFile.hpp"

#ifdef MESA_KMS
#include "ui/canvas/egl/GBM.hpp"
#endif

#include <algorithm>
#include <array>
#include <optional>
#include <span>

namespace EGL {

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

#if defined(ANDROID) || (defined(USE_EGL) && defined(USE_X11))

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
               int want_depth, int want_stencil,
               int want_samples) noexcept
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
  int samples = AttribDistance(display, config, EGL_SAMPLES, want_samples);

  return distance + r + g + b + a + d + s + samples;
}

[[gnu::pure]]
static EGLConfig
FindClosestConfig(EGLDisplay display,
                  std::span<const EGLConfig> configs,
                  int want_r, int want_g, int want_b,
                  int want_a,
                  int want_depth, int want_stencil,
                  int want_samples) noexcept
{
  EGLConfig closestConfig = nullptr;
  int closestDistance = 10000;

  for (EGLConfig config : configs) {
    int distance = ConfigDistance(display, config,
                                  want_r, want_g, want_b, want_a,
                                  want_depth, want_stencil, want_samples);
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

/**
 * Ask EGL for a list of configurations and pick the one which suits
 * XCSoar best. Throws on error.
 *
 * @param antialiasing_samples the requested number of MSAA samples;
 * 0 disables antialiasing
 * @return the chosen config or std::nullopt if there is no
 * configuration matching the requested antialiasing
 */
static std::optional<EGLConfig>
TryChooseConfig(EGLDisplay display, unsigned antialiasing_samples)
{
  static constexpr EGLint base_attributes[] = {
#ifdef ANDROID
    /* EGL_STENCIL_SIZE not listed here because we have a fallback for
       configurations without stencil (but we prefer native stencil)
       (maybe we can just require a stencil and get rid of the
       complicated and slow fallback code eventually?) */

    EGL_RED_SIZE, 4,
    EGL_GREEN_SIZE, 4,
    EGL_BLUE_SIZE, 4,
    EGL_ALPHA_SIZE, 4,

#else //  !ANDROID

    EGL_STENCIL_SIZE, 1,
#ifdef MESA_KMS
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
#ifndef RASPBERRY_PI /* the Raspberry Pi 4 doesn't have an alpha channel */
    EGL_ALPHA_SIZE, 1,
#endif
#endif // MESA_KMS

#endif // !ANDROID

    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

  /* the base attributes plus an optional multisampling request;
     eglChooseConfig() sorts configurations without multisampling
     first, so we need to ask for it explicitly instead of picking a
     multisample configuration from the result list */
  std::array<EGLint, std::size(base_attributes) + 4> attributes;
  auto *a = std::copy_n(base_attributes, std::size(base_attributes) - 1,
                        attributes.data());
  if (antialiasing_samples > 0) {
    *a++ = EGL_SAMPLE_BUFFERS;
    *a++ = 1;
    *a++ = EGL_SAMPLES;
    *a++ = static_cast<EGLint>(antialiasing_samples);
  }
  *a = EGL_NONE;

  std::array<EGLConfig, 64> configs;
  EGLint num_configs;
  if (!eglChooseConfig(display, attributes.data(),
                       configs.data(), configs.size(),
                       &num_configs))
    throw FmtRuntimeError("eglChooseConfig() failed: {:#x}", eglGetError());

  if (num_configs == 0)
    return std::nullopt;

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

  if (i < 0 && antialiasing_samples > 0)
    /* none of the multisample configurations has a usable native
       visual; let the caller retry without antialiasing */
    return std::nullopt;

  return i >= 0 ? configs[i] : configs[0];
#elif defined(ANDROID) || (defined(USE_EGL) && defined(USE_X11))
  const auto closest_config =
    FindClosestConfig(display, {configs.data(), std::size_t(num_configs)},
                      8, 8, 8, 0, 0, 1, antialiasing_samples);
  if (closest_config == nullptr)
    return std::nullopt;

  return closest_config;
#else
  /* eglChooseConfig() has sorted the configurations for us, and the
     multisampling request (if any) was part of the attribute list */
  return configs[0];
#endif
}

EGLConfig
ChooseConfig(EGLDisplay display, unsigned requested_samples)
{
  /* probe first so the request can step down to the next lower
     level this display actually offers, instead of jumping straight
     from the request to "disabled" */
  const unsigned mask = ProbeAntialiasingSamples(display);
  OpenGL::SetAvailableAntialiasingSamples(mask);

  const unsigned samples =
    OpenGL::SelectAntialiasingSamples(requested_samples, mask);

  if (requested_samples > 0 && samples != requested_samples) {
    if (samples > 0)
      LogFormat("Requested %ux anti-aliasing not available, using %ux",
                requested_samples, samples);
    else
      LogFormat("Requested %ux anti-aliasing not available, disabling",
                requested_samples);
  }

  if (samples > 0) {
    if (const auto config = TryChooseConfig(display, samples))
      return *config;

    /* the probe said this level exists; if the driver still refuses
       it, fall through to the "disabled" configuration below */
    LogFormat("Failed to obtain the %ux anti-aliasing configuration "
              "reported available, disabling", samples);
  }

  const auto config = TryChooseConfig(display, 0);
  if (!config)
    throw std::runtime_error("eglChooseConfig() failed");

  return *config;
}

unsigned
ProbeAntialiasingSamples(EGLDisplay display) noexcept
{
  /* bit 0 marks the mask as valid; "no antialiasing" always works */
  unsigned mask = 1;

  for (const unsigned n : OpenGL::ANTIALIASING_SAMPLE_COUNTS) {
    try {
      /* this asks exactly the question the user cares about: would
         ChooseConfig() find a usable configuration for this level?
         EGL_SAMPLES matches "at least", though, so a request for 8
         may return a 16x configuration; only an exact match means
         this level exists (a larger one sets its own bit in its own
         iteration) */
      const auto config = TryChooseConfig(display, n);
      if (config &&
          unsigned(GetConfigAttrib(display, *config, EGL_SAMPLES, 0)) == n)
        mask |= 1u << n;
    } catch (...) {
      /* eglChooseConfig() failed; treat this level as unavailable */
    }
  }

  return mask;
}

} // namespace EGL
