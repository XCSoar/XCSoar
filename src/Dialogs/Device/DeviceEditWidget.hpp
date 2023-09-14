// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Device/Config.hpp"

#include <cassert>

class DeviceEditWidget : public RowFormWidget, DataFieldListener {
public:
  struct Listener {
    virtual void OnModified(DeviceEditWidget &widget) noexcept = 0;
  };

private:
  DeviceConfig config;

  Listener *listener = nullptr;

public:
  DeviceEditWidget(const DeviceConfig &_config) noexcept;

  void SetListener(Listener *_listener) noexcept {
    assert(listener == nullptr);
    assert(_listener != nullptr);

    listener = _listener;
  }

  const DeviceConfig &GetConfig() const noexcept {
    return config;
  }

  /**
   * Fill new values into the form.
   */
  void SetConfig(const DeviceConfig &config) noexcept;

  void UpdateVisibilities() noexcept;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};
