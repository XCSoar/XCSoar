// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/GroupedListWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Device/Config.hpp"

#include <cassert>

class DeviceEditWidget : public GroupedListWidget {
public:
  struct Listener {
    virtual void OnModified(DeviceEditWidget &widget) noexcept = 0;
  };

private:
  DeviceConfig config;

  /** the values this page was opened with, so Save() can ignore a
      row which the chosen port does not use */
  DeviceConfig baseline;

  /** the port scanner edits this field; the list only shows it */
  DataFieldEnum port_df;

  Listener *listener = nullptr;

  unsigned passthrough_item = 0;
  unsigned sync_from_item = 0;
  unsigned sync_to_item = 0;
  unsigned send_position_item = 0;
  unsigned k6bt_item = 0;

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
  void Fill() noexcept;
  void Notify() noexcept;

  void PickPort() noexcept;
  void PickDriver(bool second) noexcept;
};
