// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

enum class SSHStatus {
  ENABLED,
  DISABLED,
  TEMPORARY,
};

[[gnu::pure]]
SSHStatus
OpenvarioGetSSHStatus();

bool
OpenvarioEnableSSH(bool temporary);

bool
OpenvarioDisableSSH();
