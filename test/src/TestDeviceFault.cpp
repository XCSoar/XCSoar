// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Fault.hpp"
#include "TestUtil.hpp"

int
main()
{
  plan_tests(8);

  ok1(ClassifyDeviceFault(true, true, true, false, false,
                          PortState::FAILED, false) ==
      DeviceFaultKind::NONE);
  ok1(ClassifyDeviceFault(false, false, true, false, false,
                          PortState::FAILED, false) ==
      DeviceFaultKind::NONE);
  ok1(ClassifyDeviceFault(false, true, false, false, false,
                          PortState::FAILED, false) ==
      DeviceFaultKind::NONE);
  ok1(ClassifyDeviceFault(false, true, true, true, false,
                          PortState::FAILED, false) ==
      DeviceFaultKind::NONE);
  ok1(ClassifyDeviceFault(false, true, true, false, true,
                          PortState::READY, false) ==
      DeviceFaultKind::NONE);
  ok1(ClassifyDeviceFault(false, true, true, false, false,
                          PortState::FAILED, false) ==
      DeviceFaultKind::ERROR);
  ok1(ClassifyDeviceFault(false, true, true, false, false,
                          PortState::LIMBO, false) ==
      DeviceFaultKind::NONE);
  ok1(ClassifyDeviceFault(false, true, true, false, false,
                          PortState::READY, false) ==
      DeviceFaultKind::NO_DATA);

  return exit_status();
}
