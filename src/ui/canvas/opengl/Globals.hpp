// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * A collection of global variables for the OpenGL backend.  Global
 * variables are not good style, but since there can be only once
 * OpenGL context at a time, this is good enough for XCSoar.
 *
 */

#pragma once

#include "ui/opengl/Features.hpp"
#include "ui/opengl/System.hpp"

#include <glm/fwd.hpp>

#include <cstdint>

#ifdef SOFTWARE_ROTATE_DISPLAY
enum class DisplayOrientation : uint8_t;
#endif

struct UnsignedPoint2D;
struct PixelPoint;

namespace OpenGL {

/**
 * Is the extension ARB_texture_non_power_of_two present?  If yes,
 * then textures can have any size, not just power of two.
 */
extern bool texture_non_power_of_two;

/**
 * Fallback when GL_MAX_TEXTURE_SIZE is missing or non-positive
 * (common GLES2 floor, e.g. VC4).
 */
inline constexpr unsigned DEFAULT_MAX_TEXTURE_SIZE = 2048;

/**
 * GL_MAX_TEXTURE_SIZE from the driver (e.g. 2048 on VC4).
 */
extern unsigned max_texture_size;

/**
 * Is glMapBufferOES() available?  May be implemented by the extension
 * GL_OES_mapbuffer.
 */
extern bool mapbuffer;

/**
 * Which depth+stencil internalFormat is supported by the
 * Renderbuffer?
 */
extern GLenum render_buffer_depth_stencil;

/**
 * Which stencil internalFormat is supported by the Renderbuffer?
 */
extern GLenum render_buffer_stencil;

/**
 * The MSAA sample counts XCSoar offers in its user interface.
 */
inline constexpr unsigned ANTIALIASING_SAMPLE_COUNTS[] = { 2, 4, 8, 16 };

/**
 * Bit mask of the MSAA sample counts this display can actually
 * provide: bit n is set if a window configuration with n samples
 * exists. Bit 0 is set whenever the mask has been probed at all, so
 * a value of zero means "unknown", i.e. the platform cannot
 * enumerate its configurations (libSDL).
 */
extern unsigned available_antialiasing_samples;

/**
 * Publish the result of probing the display for usable MSAA sample
 * counts.
 */
void
SetAvailableAntialiasingSamples(unsigned mask) noexcept;

/**
 * The number of MSAA samples the profile asks for (0 = disabled), as
 * opposed to the number any particular buffer really gets.  Set by
 * ScreenGlobalInit() once, before the first window is painted.
 */
extern unsigned requested_antialiasing_samples;

/**
 * The number of MSAA samples the window surface really has (0 =
 * disabled) as opposed to the number requested in the profile.
 * Only changes on startup, no protection necessary.
 */
extern unsigned antialiasing_samples;

/**
 * Publish the number of MSAA samples of the window surface. Called
 * by the platform code as soon as the surface configuration is
 * known, and before the first window is painted.
 */
void
SetAntialiasingSamples(unsigned samples) noexcept;

/**
 * How this GL implementation can render a framebuffer object with
 * multisampling.
 */
enum class FboAntialiasingMode : uint8_t {
  /** not at all */
  NONE,

  /**
   * GL_EXT_multisampled_render_to_texture: the texture is attached
   * with a sample count and resolved implicitly when the framebuffer
   * is unbound.  Nearly free on tiled GPUs, which is where this
   * extension is found.
   */
  IMPLICIT,

  /**
   * A multisampled colour renderbuffer, resolved into the texture
   * with glBlitFramebuffer().  What desktop GPUs offer.
   */
  BLIT,
};

extern FboAntialiasingMode fbo_antialiasing_mode;

/**
 * The largest number of MSAA samples a framebuffer object can be
 * rendered with, or 0 if this GL implementation cannot multisample an
 * FBO at all (in which case #fbo_antialiasing_mode is NONE).  Unlike
 * #antialiasing_samples, which is a property of the window surface,
 * this is a property of the GL context.
 *
 * The two are deliberately independent: the window surface
 * antialiases everything painted directly to it (the gauges, the
 * dialogs, the form controls), while this one antialiases everything
 * painted through a #BufferCanvas (the map, the InfoBoxes, the
 * renderer caches).  Each uses as many samples as it can, so they may
 * differ.
 */
extern unsigned fbo_antialiasing_samples;

/**
 * Publish the framebuffer object multisample capability.  Called by
 * OpenGL::SetupContext() once the context exists.
 */
void
SetFboAntialiasing(FboAntialiasingMode mode, unsigned samples) noexcept;

/**
 * The dimensions of the OpenGL window in pixels.
 */
extern UnsignedPoint2D window_size;

/**
 * The dimensions of the OpenGL viewport in pixels.
 */
extern UnsignedPoint2D viewport_size;

#ifdef SOFTWARE_ROTATE_DISPLAY
extern DisplayOrientation display_orientation;
#endif

/**
 * The current SubCanvas translation in pixels.
 */
extern PixelPoint translate;

extern glm::mat4 projection_matrix;

/**
 * Maximum map scale in meters for zoom-out, to work around
 * GPU driver bugs.  0 means no GPU-imposed limit.
 */
extern unsigned max_map_scale;

} // namespace OpenGL
