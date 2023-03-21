// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <glm/fwd.hpp>

struct GeoPoint;
class WindowProjection;

[[gnu::pure]]
glm::mat4
ToGLM(const WindowProjection &projection, const GeoPoint &reference) noexcept;
