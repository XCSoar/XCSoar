// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Regions.hpp"
#include "util/StaticString.hxx"

struct SkySightSettings {
  StaticString<64> email;
  StaticString<64> password;
  StaticString<32> region;

  constexpr bool IsDefined() const noexcept {
    return !email.empty() && !password.empty();
  }

  void SetDefaults() noexcept {
    email.clear();
    password.clear();
    region = GetDefaultSkySightRegion().id;
  }
};