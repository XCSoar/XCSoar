// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Ramp.hpp"
#include "ui/canvas/Color.hpp"
#include "TestUtil.hpp"

int main()
{
  const ColorRamp ramp[] = {
    {   0, { 0xff, 0x80, 0x00 }},
    {1000, { 0x00, 0x40, 0xcc }},
  };
  const ColorRamp ramp2[] = {
    {-1000, { 0x00, 0x00, 0xff }},
    {   -1, { 0x00, 0xff, 0xff }},
    {    0, { 0xff, 0xff, 0x00 }},
    { 1000, { 0xff, 0x00, 0x00 }},
  };
  RGB8Color color;

  plan_tests(39);

  // Test lower limit
  color = ColorRampLookup(0, ramp, 2);
  ok1(color.Red() == 0xff);
  ok1(color.Green() == 0x80);
  ok1(color.Blue() == 0x00);

  // Test below lower limit
  color = ColorRampLookup(-100, ramp, 2);
  ok1(color.Red() == 0xff);
  ok1(color.Green() == 0x80);
  ok1(color.Blue() == 0x00);

  // Test upper limit
  color = ColorRampLookup(1000, ramp, 2);
  ok1(color.Red() == 0x00);
  ok1(color.Green() == 0x40);
  ok1(color.Blue() == 0xcc);

  // Test above upper limit
  color = ColorRampLookup(1500, ramp, 2);
  ok1(color.Red() == 0x00);
  ok1(color.Green() == 0x40);
  ok1(color.Blue() == 0xcc);

  // Test middle
  color = ColorRampLookup(500, ramp, 2);
  ok1(color.Red() == 0x7f);
  ok1(color.Green() == 0x60);
  ok1(color.Blue() == 0x66);



  // Test lower limit
  color = ColorRampLookup(-1000, ramp2, 4);
  ok1(color.Red() == 0x00);
  ok1(color.Green() == 0x00);
  ok1(color.Blue() == 0xff);

  // Test below lower limit
  color = ColorRampLookup(-2000, ramp2, 4);
  ok1(color.Red() == 0x00);
  ok1(color.Green() == 0x00);
  ok1(color.Blue() == 0xff);

  // Test upper limit
  color = ColorRampLookup(1000, ramp2, 4);
  ok1(color.Red() == 0xff);
  ok1(color.Green() == 0x00);
  ok1(color.Blue() == 0x00);

  // Test above upper limit
  color = ColorRampLookup(2000, ramp2, 4);
  ok1(color.Red() == 0xff);
  ok1(color.Green() == 0x00);
  ok1(color.Blue() == 0x00);

  // Test interpolation point 1
  color = ColorRampLookup(0, ramp2, 4);
  ok1(color.Red() == 0xff);
  ok1(color.Green() == 0xff);
  ok1(color.Blue() == 0x00);

  // Test interpolation point 2
  color = ColorRampLookup(-1, ramp2, 4);
  ok1(color.Red() == 0x00);
  ok1(color.Green() == 0xff);
  ok1(color.Blue() == 0xff);

  // Test intermediate point 1
  color = ColorRampLookup(500, ramp2, 4);
  ok1(color.Red() == 0xff);
  ok1(color.Green() == 0x7f);
  ok1(color.Blue() == 0x00);

  // Test intermediate point 2
  color = ColorRampLookup(-500, ramp2, 4);
  ok1(color.Red() == 0x00);
  ok1(color.Green() == 0x7f);
  ok1(color.Blue() == 0xff);

  return exit_status();
}
