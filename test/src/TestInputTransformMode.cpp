// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/event/poll/libinput/InputTransform.hpp"
#include "TestUtil.hpp"

int
main()
{
  plan_tests(6);

  const PixelSize screen_size{1800, 2880};
  const PixelPoint raw{900, 1440};

  const PixelPoint system_mapped =
    UI::TransformInputPointForMode(raw, screen_size,
                                   DisplayOrientation::PORTRAIT,
                                   UI::InputTransformMode::SYSTEM_ROTATED);
  ok1(system_mapped == raw);

  const PixelSize system_size =
    UI::GetLogicalInputSizeForMode(screen_size,
                                   DisplayOrientation::PORTRAIT,
                                   UI::InputTransformMode::SYSTEM_ROTATED);
  ok1(system_size == screen_size);

  const PixelPoint xcsoar_portrait =
    UI::TransformInputPointForMode(raw, screen_size,
                                   DisplayOrientation::PORTRAIT,
                                   UI::InputTransformMode::XCSOAR_ROTATED);
  ok1(xcsoar_portrait == PixelPoint(1440, 900));

  const PixelSize xcsoar_portrait_size =
    UI::GetLogicalInputSizeForMode(screen_size,
                                   DisplayOrientation::PORTRAIT,
                                   UI::InputTransformMode::XCSOAR_ROTATED);
  ok1(xcsoar_portrait_size == PixelSize(2880, 1800));

  const PixelPoint xcsoar_reverse_landscape =
    UI::TransformInputPointForMode(raw, screen_size,
                                   DisplayOrientation::REVERSE_LANDSCAPE,
                                   UI::InputTransformMode::XCSOAR_ROTATED);
  ok1(xcsoar_reverse_landscape == PixelPoint(900, 1440));

  const PixelPoint system_reverse_landscape =
    UI::TransformInputPointForMode(raw, screen_size,
                                   DisplayOrientation::REVERSE_LANDSCAPE,
                                   UI::InputTransformMode::SYSTEM_ROTATED);
  ok1(system_reverse_landscape == raw);

  return exit_status();
}
