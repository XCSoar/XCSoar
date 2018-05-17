/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

int
main(int argc, char **argv)
{
  plan_tests(12);

  uint8_t test1[] = { 0x42, 0x00 };
  ok1(ReadUnalignedLE16((const uint16_t *)(const void *)test1) == 0x0042);
  ok1(ReadUnalignedBE16((const uint16_t *)(const void *)test1) == 0x4200);

  uint8_t test2[] = { 0x00, 0x42 };
  ok1(ReadUnalignedLE16((const uint16_t *)(const void *)test2) == 0x4200);
  ok1(ReadUnalignedBE16((const uint16_t *)(const void *)test2) == 0x0042);

  uint8_t test3[] = { 0x37, 0x13 };
  ok1(ReadUnalignedLE16((const uint16_t *)(const void *)test3) == 0x1337);
  ok1(ReadUnalignedBE16((const uint16_t *)(const void *)test3) == 0x3713);

  WriteUnalignedLE16((uint16_t *)(void *)test2, 0x0042);
  ok1(test2[0] == 0x42 && test2[1] == 0x00);

  WriteUnalignedLE16((uint16_t *)(void *)test3, 0x4200);
  ok1(test3[0] == 0x00 && test3[1] == 0x42);

  WriteUnalignedLE16((uint16_t *)(void *)test1, 0x1337);
  ok1(test1[0] == 0x37 && test1[1] == 0x13);

  WriteUnalignedBE16((uint16_t *)(void *)test2, 0x4200);
  ok1(test2[0] == 0x42 && test2[1] == 0x00);

  WriteUnalignedBE16((uint16_t *)(void *)test3, 0x0042);
  ok1(test3[0] == 0x00 && test3[1] == 0x42);

  WriteUnalignedBE16((uint16_t *)(void *)test1, 0x3713);
  ok1(test1[0] == 0x37 && test1[1] == 0x13);

  return exit_status();
}
