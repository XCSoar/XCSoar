/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "NMEA/Validity.hpp"
#include "TestUtil.hpp"

#include <stdio.h>

static Validity
invalid()
{
  Validity v;
  v.Clear();
  return v;
}

int main(int argc, char **argv)
{
  plan_tests(11);

  Validity v;
  v.Clear();
  ok1(!v.IsValid());
  v.Update(fixed(100));
  ok1(v.IsValid());
  v.Expire(fixed(101), fixed(5));
  ok1(v.IsValid());
  v.Expire(fixed(105), fixed(5));
  ok1(v.IsValid());
  v.Expire(fixed(106), fixed(5));
  ok1(!v.IsValid());

  v.Update(fixed(100));
  ok1(v.Modified(Validity(fixed(99))));
  ok1(!v.Modified(Validity(fixed(100))));
  ok1(!v.Modified(Validity(fixed(101))));
  ok1(!v.Complement(Validity(fixed(1))));

  v.Clear();
  ok1(!v.Complement(invalid()));
  ok1(v.Complement(Validity(fixed(1))));

  return exit_status();
}
