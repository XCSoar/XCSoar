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
