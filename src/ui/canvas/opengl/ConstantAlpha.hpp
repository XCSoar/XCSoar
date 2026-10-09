// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Prepare for drawing a texture with a given constant alpha value.
 */
class ScopeTextureConstantAlpha {
  const bool enabled;

public:
  ScopeTextureConstantAlpha(bool use_texture_alpha, float alpha);
  ~ScopeTextureConstantAlpha();

  ScopeTextureConstantAlpha(const ScopeTextureConstantAlpha &&) = delete;
  ScopeTextureConstantAlpha &operator=(const ScopeTextureConstantAlpha &&) = delete;
};

/**
 * Prepare for drawing a texture in "multiply" mode, with a given
 * opacity: result = dst * (1 - alpha * (1 - src)). A white texel
 * leaves the map unchanged, a black texel darkens it fully (at
 * alpha=1).
 */
class ScopeTextureMultiplyAlpha {
public:
  explicit ScopeTextureMultiplyAlpha(float alpha);
  ~ScopeTextureMultiplyAlpha();

  ScopeTextureMultiplyAlpha(const ScopeTextureMultiplyAlpha &&) = delete;
  ScopeTextureMultiplyAlpha &operator=(const ScopeTextureMultiplyAlpha &&) = delete;
};
