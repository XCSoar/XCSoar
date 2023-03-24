// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef KOBO
#error This header is only for Kobo builds
#endif

enum class KoboModel {
  UNKNOWN,
  MINI,
  TOUCH,
  TOUCH2,
  AURA,
  AURA2,
  GLO,
  GLO_HD,
  CLARA_HD,
  CLARA_2E,
  NIA,
  LIBRA2,
  LIBRA_H2O,
};

[[gnu::const]]
KoboModel
DetectKoboModel() noexcept;

[[gnu::const]]
const char *
GetKoboWifiInterface() noexcept;
