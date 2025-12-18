// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/MessagingFile.hpp"
#include "FLARM/MessagingDatabase.hpp"
#include "FLARM/MessagingRecord.hpp"
#include "FLARM/Id.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/FileReader.hxx"
#include "io/BufferedReader.hxx"
#include "TestUtil.hpp"

#include <atomic>
#include <optional>
#include <string>
#include <utility>

#include "thread/Thread.hpp"

template<typename F>
class LambdaThread : public Thread {
  F fn;

public:
  explicit LambdaThread(F &&_fn) noexcept
    :fn(std::forward<F>(_fn)) {}

  void Run() noexcept override {
    fn();
  }
};

static std::optional<MessagingRecord>
FindMessagingRecord(const FlarmMessagingDatabase &db, const char *hex)
{
  FlarmId id = FlarmId::Parse(hex, nullptr);
  return db.FindRecordById(id);
}

static void
UpdateMessagingRecord(FlarmMessagingDatabase &db, const MessagingRecord &base,
                      const char *registration = nullptr,
                      const char *pilot = nullptr,
                      const char *plane_type = nullptr,
                      const char *callsign = nullptr)
{
  MessagingRecord record = base;
  if (registration != nullptr)
    record.registration = registration;
  if (pilot != nullptr)
    record.pilot = pilot;
  if (plane_type != nullptr)
    record.plane_type = plane_type;
  if (callsign != nullptr)
    record.callsign = callsign;
  db.Update(record);
}

static void
TestFlarmMessagingIO()
{
  // Prepare a small database with one record
  FlarmMessagingDatabase db;
  MessagingRecord rec;
  rec.id = FlarmId::Parse("DDAED5", nullptr);
  rec.pilot = "Orville Wright";
  rec.plane_type = "Cessna 172";
  rec.registration = "HB-SIA";
  rec.callsign = "ZM";
  db.Insert(rec);

  // Write to a test file
  Path out_path(_T("xcsoar-messaging-test.csv"));
  FileOutputStream fos(out_path);
  BufferedOutputStream bos(fos);
  SaveFlarmMessagingFile(bos, db);
  bos.Flush();
  fos.Commit();

  // Read back into a new database
  FlarmMessagingDatabase db2;
  FileReader fr(out_path);
  BufferedReader br(fr);
  LoadFlarmMessagingFile(br, db2);

  // Verify the record persisted correctly
  auto mr = FindMessagingRecord(db2, "DDAED5");
  if (ok1(mr.has_value())) {
    struct FieldCheck { const char *value; const char *expected; };
    const FieldCheck fields[] = {
      { mr->registration.c_str(), "HB-SIA" },
      { mr->callsign.c_str(), "ZM" },
      { mr->pilot.c_str(), "Orville Wright" },
      { mr->plane_type.c_str(), "Cessna 172" },
    };
    for (const FieldCheck &field : fields)
      ok1(StringIsEqual(field.value, field.expected));
  } else {
    skip(4, 0, "messaging record not persisted");
  }

  // Clean up test file
  File::Delete(out_path);
}

static void
TestFlarmMessagingFile()
{
  // Load from test data file with edge cases
  FlarmMessagingDatabase db;
  FileReader fr(Path(_T("test/data/flarmmessaging/flarm-messaging.csv")));
  BufferedReader br(fr);
  unsigned count = LoadFlarmMessagingFile(br, db);

  ok1(count == 4);

  // Verify first record
  ok1(FindMessagingRecord(db, "DDAED5").has_value());

  // Verify second record with special characters
  auto mr = FindMessagingRecord(db, "DEADFF");
  ok1(mr.has_value());
  if (mr.has_value())
    ok1(StringIsEqual(mr->registration.c_str(), "D-1234"));
  else
    skip(1, 0, "second record not found");

  // Verify third record with missing field
  ok1(FindMessagingRecord(db, "AABBCC").has_value());

  // Verify fourth record with whitespace
  ok1(FindMessagingRecord(db, "112233").has_value());
}

static void
TestFlarmMessagingCycle()
{
  FlarmMessagingDatabase db;
  FlarmId id = FlarmId::Parse("ABCDEF", nullptr);
  MessagingRecord base;
  base.id = id;

  // First cycle: all fields present
  UpdateMessagingRecord(db, base, "REG");
  UpdateMessagingRecord(db, base, nullptr, "P1");
  UpdateMessagingRecord(db, base, nullptr, nullptr, "T1");
  UpdateMessagingRecord(db, base, nullptr, nullptr, nullptr, "CS1");

  auto mr = db.FindRecordById(id);
  if (!ok1(mr.has_value()))
    return;
  ok1(StringIsEqual(mr->callsign.c_str(), "CS1"));

  // Second cycle: callsign missing
  UpdateMessagingRecord(db, base, "REG");
  UpdateMessagingRecord(db, base, nullptr, "P1");
  UpdateMessagingRecord(db, base, nullptr, nullptr, "T1");

  // Third cycle start (boundary): registration repeats, should clear missing callsign
  UpdateMessagingRecord(db, base, "REG");

  // Re-fetch pointer after updates to ensure validity
  mr = db.FindRecordById(id);
  if (!ok1(mr.has_value()))
    return;

  ok1(mr->callsign.empty());
  ok1(StringIsEqual(mr->registration.c_str(), "REG"));
  ok1(StringIsEqual(mr->pilot.c_str(), "P1"));
  ok1(StringIsEqual(mr->plane_type.c_str(), "T1"));

  // Scenario: single field arrives and may not repeat for a while.
  // We retain it until a boundary occurs, and repeating the same field later
  // does not drop it.
  {
    FlarmId id2 = FlarmId::Parse("FFEEDD", nullptr);
    MessagingRecord base2;
    base2.id = id2;

    // Only one field arrives and never repeats
    UpdateMessagingRecord(db, base2, nullptr, "PilotX");

    auto mr2 = db.FindRecordById(id2);
    if (!ok1(mr2.has_value()))
      return;

    ok1(StringIsEqual(mr2->pilot.c_str(), "PilotX"));
    ok1(mr2->registration.empty());
    ok1(mr2->plane_type.empty());
    ok1(mr2->callsign.empty());

    // Later: it returns and we see the same field again; boundary starts,
    // but nothing is lost (others were empty, pilot remains).
    UpdateMessagingRecord(db, base2, nullptr, "PilotX");

    mr2 = db.FindRecordById(id2);
    ok1(mr2.has_value());
    ok1(StringIsEqual(mr2->pilot.c_str(), "PilotX"));
  }
}

static void
TestFlarmMessagingThreadSafety()
{
  // Scenario 1: Concurrent reads and writes
  {
    FlarmMessagingDatabase db;
    MessagingRecord base;
    base.id = FlarmId::Parse("CCBBAA", nullptr);

    std::atomic<bool> writer_done{false};

    LambdaThread writer([&]{
      for (int i = 0; i < 500; ++i) {
        MessagingRecord r = base;
        r.pilot = std::string("Pilot") + std::to_string(i);
        db.Update(r);
      }
      writer_done.store(true, std::memory_order_relaxed);
    });

    LambdaThread reader([&]{
      while (!writer_done.load(std::memory_order_relaxed)) {
        std::as_const(db).FindRecordById(base.id);
      }
    });

    writer.Start();
    reader.Start();
    writer.Join();
    reader.Join();

    const auto final = std::as_const(db).FindRecordById(base.id);
    ok1(final.has_value());
    if (final.has_value()) {
      ok1(StringIsEqual(final->pilot.c_str(), "Pilot499"));
      ok1(final->registration.empty());
    } else {
      skip(2, 0, "scenario 1: concurrent reads/writes - final record missing");
    }
  }

  // Scenario 2: Concurrent updates and file saves
  {
    FlarmMessagingDatabase db;
    MessagingRecord base;
    base.id = FlarmId::Parse("DADADA", nullptr);

    std::atomic<bool> updates_done{false};
    Path out_path(_T("xcsoar-messaging-concurrent-save.csv"));

    LambdaThread updater([&]{
      for (int i = 0; i < 200; ++i) {
        MessagingRecord r = base;
        r.registration = std::string("REG") + std::to_string(i);
        db.Update(r);
      }
      updates_done.store(true, std::memory_order_relaxed);
    });

    LambdaThread saver([&]{
      while (!updates_done.load(std::memory_order_relaxed)) {
        FileOutputStream fos(out_path);
        BufferedOutputStream bos(fos);
        SaveFlarmMessagingFile(bos, db);
        bos.Flush();
        fos.Commit();
      }
    });

    updater.Start();
    saver.Start();
    updater.Join();
    updates_done.store(true, std::memory_order_relaxed);
    saver.Join();

    // Final save after all updates
    {
      FileOutputStream fos(out_path);
      BufferedOutputStream bos(fos);
      SaveFlarmMessagingFile(bos, db);
      bos.Flush();
      fos.Commit();
    }

    // Load back and check we see the last update (ensures no deadlock/corruption)
    FlarmMessagingDatabase db2;
    FileReader fr(out_path);
    BufferedReader br(fr);
    LoadFlarmMessagingFile(br, db2);
    File::Delete(out_path);

    auto mr = db2.FindRecordById(base.id);
    ok1(mr.has_value());
    if (mr.has_value()) {
      ok1(StringIsEqual(mr->registration.c_str(), "REG199"));
      ok1(mr->pilot.empty());
    } else {
      skip(2, 0, "scenario 2: concurrent update/save - record missing");
    }
  }
}

int main()
{
  plan_tests(31);

  TestFlarmMessagingIO();
  TestFlarmMessagingFile();
  TestFlarmMessagingCycle();
  TestFlarmMessagingThreadSafety();

  return exit_status();
}
