// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Input/InputConfig.hpp"
#include "TestUtil.hpp"
#include "ui/event/KeyCode.hpp"

#include <tchar.h>

int main()
{
  plan_tests(37);

  InputConfig config;
  config.SetDefaults();

  /* Test arrow key detection - arrow keys should NOT be normalized */
  /* Use the platform-specific KeyCode constants */
  const unsigned KEY_UP_CODE = KEY_UP;
  const unsigned KEY_DOWN_CODE = KEY_DOWN;
  const unsigned KEY_LEFT_CODE = KEY_LEFT;
  const unsigned KEY_RIGHT_CODE = KEY_RIGHT;

  /* Set up test bindings for arrow keys */
  config.SetKeyEvent(0, KEY_UP_CODE, 100);
  config.SetKeyEvent(0, KEY_DOWN_CODE, 101);
  config.SetKeyEvent(0, KEY_LEFT_CODE, 102);
  config.SetKeyEvent(0, KEY_RIGHT_CODE, 103);

  /* Arrow keys should be found as-is (not normalized) */
  ok1(config.GetKeyEvent(0, KEY_UP_CODE) == 100);
  ok1(config.GetKeyEvent(0, KEY_DOWN_CODE) == 101);
  ok1(config.GetKeyEvent(0, KEY_LEFT_CODE) == 102);
  ok1(config.GetKeyEvent(0, KEY_RIGHT_CODE) == 103);

  /* Test lowercase letter normalization */
  config.SetKeyEvent(0, 'A', 200);
  config.SetKeyEvent(0, 'T', 201);

  /* Lowercase letters should be normalized to uppercase for lookup */
  ok1(config.GetKeyEvent(0, 'a') == 200);
  ok1(config.GetKeyEvent(0, 'A') == 200);
  ok1(config.GetKeyEvent(0, 't') == 201);
  ok1(config.GetKeyEvent(0, 'T') == 201);

  /* Test that arrow keys are NOT normalized. On Linux/Wayland, arrow key
     codes (KEY_UP=103, KEY_DOWN=108, KEY_LEFT=105, KEY_RIGHT=106) conflict
     with ASCII 'g', 'l', 'i', 'j', so arrow keys must be detected before
     normalization. On other platforms, arrow keys don't conflict but the
     logic should still work correctly */
  ok1(config.GetKeyEvent(0, KEY_UP_CODE) == 100);
  ok1(config.GetKeyEvent(0, KEY_DOWN_CODE) == 101);
  ok1(config.GetKeyEvent(0, KEY_LEFT_CODE) == 102);
  ok1(config.GetKeyEvent(0, KEY_RIGHT_CODE) == 103);

  /* Test SetKeyEvent normalization - should normalize to uppercase */
  config.SetKeyEvent(0, 'z', 300);
  ok1(config.GetKeyEvent(0, 'z') == 300);
  ok1(config.GetKeyEvent(0, 'Z') == 300);

  /* Test that non-letter keys are not affected */
  config.SetKeyEvent(0, '0', 400);
  config.SetKeyEvent(0, '9', 401);
  config.SetKeyEvent(0, '!', 402);
  config.SetKeyEvent(0, ' ', 403);

  ok1(config.GetKeyEvent(0, '0') == 400);
  ok1(config.GetKeyEvent(0, '9') == 401);
  ok1(config.GetKeyEvent(0, '!') == 402);
  ok1(config.GetKeyEvent(0, ' ') == 403);

  /* Test mode-specific lookups */
  config.SetKeyEvent(1, 'A', 500);
  ok1(config.GetKeyEvent(1, 'a') == 500);
  ok1(config.GetKeyEvent(1, 'A') == 500);
  ok1(config.GetKeyEvent(0, 'a') == 200); /* Should fall back to mode 0 */

  /* Test commonly used keys from default.xci */
  /* Function keys F1-F4 are heavily used (F1=QuickMenu, F2=Analysis, etc.) */
  config.SetKeyEvent(0, KEY_F1, 600);
  config.SetKeyEvent(0, KEY_F2, 601);
  config.SetKeyEvent(0, KEY_F3, 602);
  config.SetKeyEvent(0, KEY_F4, 603);
  ok1(config.GetKeyEvent(0, KEY_F1) == 600);
  ok1(config.GetKeyEvent(0, KEY_F2) == 601);
  ok1(config.GetKeyEvent(0, KEY_F3) == 602);
  ok1(config.GetKeyEvent(0, KEY_F4) == 603);

  /* APP keys (APP1-APP4) are used for mode switching in default.xci */
  config.SetKeyEvent(0, KEY_APP1, 700);
  config.SetKeyEvent(0, KEY_APP2, 701);
  config.SetKeyEvent(0, KEY_APP3, 702);
  config.SetKeyEvent(0, KEY_APP4, 703);
  ok1(config.GetKeyEvent(0, KEY_APP1) == 700);
  ok1(config.GetKeyEvent(0, KEY_APP2) == 701);
  ok1(config.GetKeyEvent(0, KEY_APP3) == 702);
  ok1(config.GetKeyEvent(0, KEY_APP4) == 703);

  /* Special keys: RETURN, ESCAPE, MENU are frequently used */
  config.SetKeyEvent(0, KEY_RETURN, 800);
  config.SetKeyEvent(0, KEY_ESCAPE, 801);
  config.SetKeyEvent(0, KEY_MENU, 802);
  ok1(config.GetKeyEvent(0, KEY_RETURN) == 800);
  ok1(config.GetKeyEvent(0, KEY_ESCAPE) == 801);
  ok1(config.GetKeyEvent(0, KEY_MENU) == 802);

  /* Numeric keys 6-9 are heavily used in default.xci */
  config.SetKeyEvent(0, '6', 900);
  config.SetKeyEvent(0, '7', 901);
  config.SetKeyEvent(0, '8', 902);
  ok1(config.GetKeyEvent(0, '6') == 900);
  ok1(config.GetKeyEvent(0, '7') == 901);
  ok1(config.GetKeyEvent(0, '8') == 902);

  /* Test that non-existent bindings return 0 */
  ok1(config.GetKeyEvent(0, 'X') == 0);
  ok1(config.GetKeyEvent(0, 999) == 0);

  return exit_status();
}
