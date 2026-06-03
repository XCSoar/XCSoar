// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL

#include "Look/Colors.hpp"
#include "ui/canvas/opengl/Buffer.hpp"
#include "ui/opengl/System.hpp"

#include <glm/glm.hpp>

struct Color;

namespace ForwardViewTerrainShader {

struct MeshVertex {
  float x;
  float y_geo;
  float msl;
  /** 0 = land, 1 = water, 2 = special (flat white). */
  float material;
  float base_r;
  float base_g;
  float base_b;
  float nx;
  float ny;
  float nz;
  /** Negative illum cap when cast in shadow at mesh build; 0 = none. */
  float shadow_cap;
  /** Corridor topography texture coordinates. */
  float topo_u;
  float topo_v;
};

struct DrawParams {
  glm::mat4 projection;
  glm::mat4 modelview;
  glm::vec2 scroll;
  float ref_alt;
  float vertical_ref;
  float eye_z;
  glm::vec3 sun_dir;
  bool sun_active;
  bool topo_active;
  /** 1 = full elevation ramp; lower when @p topo_active. */
  float terrain_ramp_weight = 1.f;
};

/** Returns false when shader compile/link fails. */
bool EnsureInitialised() noexcept;

void UploadMesh(const MeshVertex *vertices, unsigned triangle_count) noexcept;

void Draw(const DrawParams &params) noexcept;

} // namespace ForwardViewTerrainShader

#endif
