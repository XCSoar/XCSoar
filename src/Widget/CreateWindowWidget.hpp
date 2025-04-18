// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

#include <functional>
#include <memory>

class WindowStyle;

/**
 * A class that calls a function object to create the Window in
 * Prepare() and deletes the Window in Unprepare().
 */
class CreateWindowWidget final : public WindowWidget {
  typedef std::function<std::unique_ptr<Window>(ContainerWindow &parent,
                                                const PixelRect &rc,
                                                WindowStyle style)> CreateFunction;

  CreateFunction create;

public:
  explicit CreateWindowWidget(CreateFunction &&_create) noexcept
    :create(std::move(_create)) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
