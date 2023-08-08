// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/FlarmNetDatabase.hpp"
#include "FLARM/FlarmNetReader.hpp"
#include "FLARM/FlarmNetRecord.hpp"
#include "FLARM/Id.hpp"
#include "system/Path.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(15);

  FlarmNetDatabase db;
  int count = FlarmNetReader::LoadFile(Path(_T("test/data/flarmnet/data.fln")),
                                       db);
  ok1(count == 6);

  FlarmId id = FlarmId::Parse("DDA85C", NULL);

  const FlarmNetRecord *record = db.FindRecordById(id);
  ok1(record != NULL);

  ok1(StringIsEqual(record->id, _T("DDA85C")));
  ok1(StringIsEqual(record->pilot, _T("Tobias Bieniek")));
  ok1(StringIsEqual(record->airfield, _T("AACHEN")));
  ok1(StringIsEqual(record->plane_type, _T("Hornet")));
  ok1(StringIsEqual(record->registration, _T("D-4449")));
  ok1(StringIsEqual(record->callsign, _T("TH")));
  ok1(StringIsEqual(record->frequency, _T("130.625")));

  const FlarmNetRecord *array[3];
  ok1(db.FindRecordsByCallSign(_T("TH"), array, 3) == 2);

  bool found4449 = false, found5799 = false;
  for (unsigned i = 0; i < 2; i++) {
    record = array[i];
    if (StringIsEqual(record->registration, _T("D-4449")))
      found4449 = true;
    if (StringIsEqual(record->registration, _T("D-5799")))
      found5799 = true;
  }
  ok1(found4449);
  ok1(found5799);

  FlarmId ids[3];
  ok1(db.FindIdsByCallSign(_T("TH"), ids, 3) == 2);

  id = FlarmId::Parse("DDA85C", NULL);
  FlarmId id2 = FlarmId::Parse("DDA896", NULL);
  bool foundDDA85C = false, foundDDA896 = false;
  for (unsigned i = 0; i < 2; i++) {
    if (ids[i] == id)
      foundDDA85C = true;
    if (ids[i] == id2)
      foundDDA896 = true;
  }
  ok1(foundDDA85C);
  ok1(foundDDA896);

  return exit_status();
}
