/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "FLARM/FLARMNet.hpp"
#include "FLARM/FlarmId.hpp"
#include "TestUtil.hpp"

int main(int argc, char **argv)
{
  plan_tests(9);

  FLARMNetDatabase db;
  int count = db.LoadFile(_T("test/data/flarmnet/data.fln"));
  ok1(count == 6);

  FlarmId id;
  id.parse("DDA85C", NULL);

  const FLARMNetRecord *record = db.Find(id);
  ok1(record != NULL);

  ok1(_tcscmp(record->id, _T("DDA85C")) == 0);
  ok1(_tcscmp(record->name, _T("Tobias Bieniek")) == 0);
  ok1(_tcscmp(record->airfield, _T("AACHEN")) == 0);
  ok1(_tcscmp(record->type, _T("Hornet")) == 0);
  ok1(_tcscmp(record->reg, _T("D-4449")) == 0);
  ok1(_tcscmp(record->cn, _T("TH")) == 0);
  ok1(_tcscmp(record->freq, _T("130.625")) == 0);

  return exit_status();
}
