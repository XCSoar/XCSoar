// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL

#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "ui/dim/Size.hpp"

#include <memory>

class Canvas;
class GLFrameBuffer;
class GLTexture;
class TopographyStore;
class TopographyRenderer;
struct TopographyLook;

namespace ForwardViewTopographyTexture {

static constexpr unsigned WIDTH = 2048;
static constexpr unsigned HEIGHT = 1024;

/** Corridor-aligned UV for a point in the terrain mesh anchor frame. */
struct UV {
  float u, v;
};

class Cache {
  const TopographyLook &look;
  TopographyStore *store = nullptr;

  std::unique_ptr<TopographyRenderer> renderer;
  std::unique_ptr<GLTexture> texture;
  std::unique_ptr<GLFrameBuffer> frame_buffer;

  bool valid = false;
  GeoPoint anchor = GeoPoint::Invalid();
  Angle track = Angle::Zero();
  double range = 0.;
  float aspect = 0.f;
  unsigned store_serial = 0;

public:
  explicit Cache(const TopographyLook &_look) noexcept;
  ~Cache() noexcept;

  void SetStore(TopographyStore *_store) noexcept;

  void Invalidate() noexcept;

  /**
   * Rasterise vector topography into the corridor texture when stale.
   * @param rebuilt set to true when the texture was re-rendered
   * @return true when a texture is available for sampling
   */
  bool Ensure(GeoPoint anchor, Angle track, double range, float aspect,
              unsigned store_serial, bool *rebuilt=nullptr) noexcept;

  [[nodiscard]]
  bool IsValid() const noexcept {
    return valid && texture != nullptr;
  }

  void BindTexture() const noexcept;

  UV ComputeUV(double x, double y) const noexcept;
};

} // namespace ForwardViewTopographyTexture

#endif
