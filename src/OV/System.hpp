// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

enum class SSHStatus {
  ENABLED,
  DISABLED,
  TEMPORARY,
};

uint_least8_t
OpenvarioGetBrightness() noexcept;

void
OpenvarioSetBrightness(uint_least8_t value) noexcept;

[[gnu::pure]]
SSHStatus
OpenvarioGetSSHStatus();

bool
OpenvarioEnableSSH(bool temporary);

bool
OpenvarioDisableSSH();
