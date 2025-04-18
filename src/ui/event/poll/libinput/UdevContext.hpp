// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

static constexpr const char *UDEV_DEFAULT_SEAT = "seat0";

struct udev;

/**
 * Helper class for initialisation and (thread safe) access to the udev context.
 */
class UdevContext {
  struct udev *ud;

  explicit UdevContext(struct udev *_ud) noexcept:ud(_ud) {}

public:
  UdevContext() noexcept:ud(nullptr) {}
  UdevContext(const UdevContext &) noexcept;

  UdevContext(UdevContext &&other) noexcept
    :ud(other.ud) {
    other.ud = nullptr;
  }

  UdevContext &operator=(const UdevContext &) noexcept;
  ~UdevContext() noexcept;

  struct udev *Get() noexcept {
    return ud;
  }

  static UdevContext NewRef() noexcept;
};
