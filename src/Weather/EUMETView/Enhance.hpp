// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <chrono>
#include <cstdint>

class UncompressedImage;

namespace EUMETView {

/**
 * A luminance histogram, accumulated over the tiles of a block.
 *
 * The imagery arrives as radiance, not albedo: EUMETView applies no
 * contrast enhancement of its own, so a tile straight from the server
 * is dark and flat, and how dark depends on the sun.  The cure is to
 * stretch it, and the stretch has to know how bright the scene
 * actually is -- which is what this counts.
 */
class ToneHistogram {
  std::array<uint32_t, 256> bins{};
  uint64_t total = 0;

public:
  void Clear() noexcept {
    bins = {};
    total = 0;
  }

  [[nodiscard]]
  bool IsEmpty() const noexcept {
    return total == 0;
  }

  [[nodiscard]]
  uint64_t GetCount() const noexcept {
    return total;
  }

  /**
   * Count the pixels of one tile.  Grey, RGB and RGBA are all
   * accepted; colour is reduced to luminance.
   */
  void Add(const UncompressedImage &image) noexcept;

  /**
   * @return the brightness below which @p permille of the samples
   * lie, or 0 when empty
   */
  [[gnu::pure]]
  uint8_t Percentile(unsigned permille) const noexcept;
};

/**
 * The range of the input that is mapped onto the full output range.
 */
struct ToneWindow {
  uint8_t low = 0, high = 255;

  [[nodiscard]]
  constexpr bool IsValid() const noexcept {
    return high > low;
  }

  [[nodiscard]]
  constexpr bool operator==(const ToneWindow &) const noexcept = default;
};

/**
 * The window this histogram calls for: the 2nd and 98th percentile.
 *
 * Not the extremes, because a handful of outlying pixels would then
 * decide the contrast of the whole picture.
 *
 * @return an invalid window if the histogram cannot support one
 */
[[gnu::pure]]
ToneWindow MakeToneWindow(const ToneHistogram &histogram) noexcept;

/**
 * Mix a freshly measured window into the one carried over from the
 * previous refresh, so the picture does not jump from frame to frame.
 *
 * The weight follows the elapsed time rather than the number of
 * frames, because the imagery is fetched irregularly: after a long
 * gap the old window says nothing about the present scene and is
 * discarded, while frames in quick succession are smoothed.  One
 * formula covers a page left open, a page opened twice an hour, and a
 * single fetch, with no special cases.
 *
 * @param previous the window in use so far; an invalid one makes
 * @p fresh the result
 */
[[gnu::pure]]
ToneWindow BlendToneWindow(ToneWindow previous, ToneWindow fresh,
                           std::chrono::steady_clock::duration elapsed) noexcept;

/**
 * How far apart two windows are, in brightness steps.  Used to decide
 * whether a finished block is worth redrawing.
 */
[[gnu::pure]]
unsigned ToneWindowDistance(ToneWindow a, ToneWindow b) noexcept;

/**
 * Stretch @p image onto @p window, then sharpen it.
 *
 * The sharpening is the one thing here that adds no information: it
 * amplifies the scale of individual cloud cells so that the gaps
 * between them read at a glance, at the cost of a little noise.
 *
 * @return the enhanced image, or an invalid image if the input is not
 * one of the supported formats
 */
[[nodiscard]]
UncompressedImage Enhance(const UncompressedImage &image,
                          ToneWindow window) noexcept;

} // namespace EUMETView
