// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifndef KOBO
#error This header is only for Kobo builds
#endif

#include <span>

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
  CLARA_BW,
  CLARA_COLOUR,
  NIA,
  LIBRA2,
  LIBRA_H2O,
};

KoboModel
DetectKoboModel() noexcept;

[[gnu::pure]]
KoboModel
ParseKoboModel(std::span<const char> data) noexcept;

constexpr bool
IsKoboMediaTek(KoboModel model) noexcept
{
  return model == KoboModel::CLARA_BW ||
    model == KoboModel::CLARA_COLOUR;
}

bool
IsKoboMediaTek() noexcept;

[[gnu::const]]
const char *
GetKoboWifiInterface(KoboModel model) noexcept;

const char *
GetKoboWifiInterface() noexcept;
