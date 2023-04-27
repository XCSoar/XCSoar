// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "System.hpp"
#include "system/Process.hpp"

#include <unistd.h>
#include <sys/stat.h>

SSHStatus
OpenvarioGetSSHStatus()
{
  if (Run("/bin/systemctl", "--quiet", "is-enabled", "dropbear.socket")) {
    return SSHStatus::ENABLED;
  } else if (Run("/bin/systemctl", "--quiet", "is-active", "dropbear.socket")) {
    return SSHStatus::TEMPORARY;
  } else {
    return SSHStatus::DISABLED;
  }
}

bool
OpenvarioEnableSSH(bool temporary)
{
  if (temporary) {
    return Run("/bin/systemctl", "disable", "dropbear.socket") && 
      Run("/bin/systemctl", "start", "dropbear.socket");
  }

  return Run("/bin/systemctl", "enable", "--now", "dropbear.socket");
}

bool
OpenvarioDisableSSH()
{
  return Run("/bin/systemctl", "disable", "--now", "dropbear.socket");
}
