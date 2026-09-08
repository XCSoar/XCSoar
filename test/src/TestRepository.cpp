// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Repository/Parser.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Freshness.hpp"
#include "io/LineReader.hpp"
#include "TestUtil.hpp"

#include <cstring>
#include <chrono>

/**
 * A simple in-memory NLineReader for testing.
 * Splits input on newlines, returning writable lines.
 */
class StringLineReader : public NLineReader {
  char *p;

public:
  explicit StringLineReader(char *buffer) noexcept
    :p(buffer) {}

  char *ReadLine() override {
    if (p == nullptr || *p == '\0')
      return nullptr;

    char *line = p;
    char *nl = strchr(p, '\n');
    if (nl != nullptr) {
      *nl = '\0';
      p = nl + 1;
    } else {
      p = nullptr;
    }
    return line;
  }
};

static void
TestEmpty()
{
  char input[] = "";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));
  ok1(repo.FindByName("anything") == nullptr);
}

static void
TestCommentsAndBlanks()
{
  char input[] =
    "# This is a comment\n"
    "\n"
    "  # indented comment\n"
    "  \n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));
  ok1(repo.FindByName("anything") == nullptr);
}

static void
TestSingleFile()
{
  char input[] =
    "name = test.xcw\n"
    "uri = http://example.com/test.xcw\n"
    "description = Test waypoints\n"
    "area = EU\n"
    "type = waypoint\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("test.xcw");
  ok1(f != nullptr);
  ok1(f->name == "test.xcw");
  ok1(f->uri == "http://example.com/test.xcw");
  ok1(f->description == "Test waypoints");
  ok(strcmp(f->GetArea(), "EU") == 0,
     "area matches");
  ok1(f->type == FileType::WAYPOINT);
  ok1(!f->HasHash());
}

static void
TestMultipleFiles()
{
  char input[] =
    "name = airspace.txt\n"
    "uri = http://example.com/airspace.txt\n"
    "type = airspace\n"
    "\n"
    "name = map.xcm\n"
    "uri = http://example.com/map.xcm\n"
    "type = map\n"
    "\n"
    "name = flarm.fln\n"
    "uri = http://example.com/flarm.fln\n"
    "type = flarmnet\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *a = repo.FindByName("airspace.txt");
  ok1(a != nullptr);
  ok1(a->type == FileType::AIRSPACE);

  const auto *m = repo.FindByName("map.xcm");
  ok1(m != nullptr);
  ok1(m->type == FileType::MAP);

  const auto *f = repo.FindByName("flarm.fln");
  ok1(f != nullptr);
  ok1(f->type == FileType::FLARMNET);
}

static void
TestAllTypes()
{
  char input[] =
    "name = a.sua\n"
    "uri = http://x/a\n"
    "type = airspace\n"
    "\n"
    "name = b.cup\n"
    "uri = http://x/b\n"
    "type = waypoint\n"
    "\n"
    "name = c.txt\n"
    "uri = http://x/c\n"
    "type = waypoint-details\n"
    "\n"
    "name = d.xcm\n"
    "uri = http://x/d\n"
    "type = map\n"
    "\n"
    "name = e.fln\n"
    "uri = http://x/e\n"
    "type = flarmnet\n"
    "\n"
    "name = f.rasp\n"
    "uri = http://x/f\n"
    "type = rasp\n"
    "\n"
    "name = g.xci\n"
    "uri = http://x/g\n"
    "type = xci\n"
    "\n"
    "name = h.tsk\n"
    "uri = http://x/h\n"
    "type = task\n"
    "\n"
    "name = i.xcl\n"
    "uri = http://x/i\n"
    "type = checklist\n"
    "\n"
    "name = xcsoar-UNIX\n"
    "uri = https://xcsoar.org/download\n"
    "description = Stable release\n"
    "type = software-update\n"
    "target = UNIX\n"
    "version = 2026.1\n"
    "channel = stable\n"
    "offer-id = 2026.1\n"
    "source = XCSoar\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  ok1(repo.FindByName("a.sua")->type == FileType::AIRSPACE);
  ok1(repo.FindByName("b.cup")->type == FileType::WAYPOINT);
  ok1(repo.FindByName("c.txt")->type == FileType::WAYPOINTDETAILS);
  ok1(repo.FindByName("d.xcm")->type == FileType::MAP);
  ok1(repo.FindByName("e.fln")->type == FileType::FLARMNET);
  ok1(repo.FindByName("f.rasp")->type == FileType::RASP);
  ok1(repo.FindByName("g.xci")->type == FileType::XCI);
  ok1(repo.FindByName("h.tsk")->type == FileType::TASK);
  ok1(repo.FindByName("i.xcl")->type == FileType::CHECKLIST);
  const auto *update = repo.FindByName("xcsoar-UNIX");
  ok1(update != nullptr);
  ok1(update->type == FileType::SOFTWARE_UPDATE);
  ok1(update->software_update.has_value());
  ok1(update->software_update->target == "UNIX");
  ok1(update->software_update->version == "2026.1");
  ok1(update->software_update->channel == "stable");
  ok1(update->software_update->offer_id == "2026.1");
  ok1(update->software_update->source == "XCSoar");
}

static void
TestUnknownType()
{
  char input[] =
    "name = test.dat\n"
    "uri = http://x/test.dat\n"
    "type = bogus\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("test.dat");
  ok1(f != nullptr);
  ok1(f->type == FileType::UNKNOWN);
}

static void
TestUpdateDate()
{
  char input[] =
    "name = test.xcw\n"
    "uri = http://x/test.xcw\n"
    "update = 2024-06-15\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("test.xcw");
  ok1(f != nullptr);
  ok1(f->update_date.IsPlausible());
  ok1(f->update_date.year == 2024);
  ok1(f->update_date.month == 6);
  ok1(f->update_date.day == 15);
}

static void
TestInvalidDate()
{
  char input[] =
    "name = test.xcw\n"
    "uri = http://x/test.xcw\n"
    "update = not-a-date\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("test.xcw");
  ok1(f != nullptr);
  /* invalid date format leaves default invalid date */
  ok1(!f->update_date.IsPlausible());
}

static void
TestSha256()
{
  char input[] =
    "name = test.xcw\n"
    "uri = http://x/test.xcw\n"
    "sha256 = "
    "abcdef0123456789abcdef0123456789"
    "abcdef0123456789abcdef0123456789\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("test.xcw");
  ok1(f != nullptr);
  ok1(f->HasHash());
  ok1(f->sha256_hash[0] == std::byte{0xab});
  ok1(f->sha256_hash[1] == std::byte{0xcd});
  ok1(f->sha256_hash[2] == std::byte{0xef});
}

static void
TestInvalidSha256()
{
  char input[] =
    "name = test.xcw\n"
    "uri = http://x/test.xcw\n"
    "sha256 = tooshort\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("test.xcw");
  ok1(f != nullptr);
  /* invalid hash stays zeroed */
  ok1(!f->HasHash());
}

static void
TestMissingUri()
{
  /* a file with name but no uri is not valid and should be rejected */
  char input[] =
    "name = test.xcw\n"
    "description = No URI\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(!ParseFileRepository(repo, reader));
}

static void
TestMalformedLine()
{
  /* a line without '=' is malformed and should cause parse failure */
  char input[] =
    "name = test.xcw\n"
    "this line has no equals sign\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(!ParseFileRepository(repo, reader));
}

static void
TestInvalidFilename()
{
  /* filenames with path separators should be rejected */
  char input[] =
    "name = ../etc/passwd\n"
    "uri = http://x/evil\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(!ParseFileRepository(repo, reader));
}

static void
TestInvalidFilenameSlash()
{
  char input[] =
    "name = sub/file.xcw\n"
    "uri = http://x/sub/file.xcw\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(!ParseFileRepository(repo, reader));
}

static void
TestWhitespaceHandling()
{
  /* parser should strip whitespace around name and value */
  char input[] =
    "  name  =  spaced.xcw  \n"
    "  uri  =  http://x/spaced.xcw  \n"
    "  area  =  US  \n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  const auto *f = repo.FindByName("spaced.xcw");
  ok1(f != nullptr);
  ok1(f->uri == "http://x/spaced.xcw");
  ok(strcmp(f->GetArea(), "US") == 0,
     "area whitespace stripped");
}

static void
TestFindByName()
{
  char input[] =
    "name = first.xcw\n"
    "uri = http://x/first\n"
    "\n"
    "name = second.xcm\n"
    "uri = http://x/second\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  ok1(repo.FindByName("first.xcw") != nullptr);
  ok1(repo.FindByName("second.xcm") != nullptr);
  ok1(repo.FindByName("nonexistent") == nullptr);
  ok1(repo.FindByName("FIRST.XCW") == nullptr);
}

static void
TestFieldsBeforeName()
{
  /* fields appearing before any name= should be ignored */
  char input[] =
    "uri = http://x/orphan\n"
    "type = airspace\n"
    "\n"
    "name = real.xcw\n"
    "uri = http://x/real.xcw\n";
  StringLineReader reader(input);
  FileRepository repo;

  ok1(ParseFileRepository(repo, reader));

  ok1(repo.FindByName("real.xcw") != nullptr);
  /* the orphan uri should not create a file */
  unsigned count = 0;
  for ([[maybe_unused]] const auto &f : repo)
    ++count;
  ok1(count == 1);
}

static void
TestAvailableFile()
{
  AvailableFile f;
  f.Clear();

  ok1(f.IsEmpty());
  ok1(!f.IsValid());
  ok1(!f.HasHash());

  f.name = "test.xcw";
  ok1(!f.IsEmpty());
  ok1(!f.IsValid()); /* still no uri */

  f.uri = "http://x/test.xcw";
  ok1(f.IsValid());

  f.sha256_hash[15] = std::byte{0x42};
  ok1(f.HasHash());

  f.Clear();
  ok1(f.IsEmpty());
  ok1(!f.HasHash());
  ok1(f.type == FileType::UNKNOWN);
}

static void
TestRefreshFreshness()
{
  using namespace std::chrono;
  const system_clock::time_point now{seconds{200000}};
  ok1(Repository::IsRefreshDue({}, now));
  ok1(!Repository::IsRefreshDue(now - hours{23}, now));
  ok1(Repository::IsRefreshDue(now - hours{24}, now));
  ok1(Repository::IsRefreshDue(now + seconds{1}, now));
}

int main()
{
  plan_tests(
    2 +   // TestEmpty
    2 +   // TestCommentsAndBlanks
    8 +   // TestSingleFile
    7 +   // TestMultipleFiles
    18 +  // TestAllTypes
    3 +   // TestUnknownType
    6 +   // TestUpdateDate
    3 +   // TestInvalidDate
    6 +   // TestSha256
    3 +   // TestInvalidSha256
    1 +   // TestMissingUri
    1 +   // TestMalformedLine
    1 +   // TestInvalidFilename
    1 +   // TestInvalidFilenameSlash
    4 +   // TestWhitespaceHandling
    5 +   // TestFindByName
    3 +   // TestFieldsBeforeName
    10 +  // TestAvailableFile
    4     // TestRefreshFreshness
  );

  TestEmpty();
  TestCommentsAndBlanks();
  TestSingleFile();
  TestMultipleFiles();
  TestAllTypes();
  TestUnknownType();
  TestUpdateDate();
  TestInvalidDate();
  TestSha256();
  TestInvalidSha256();
  TestMissingUri();
  TestMalformedLine();
  TestInvalidFilename();
  TestInvalidFilenameSlash();
  TestWhitespaceHandling();
  TestFindByName();
  TestFieldsBeforeName();
  TestAvailableFile();
  TestRefreshFreshness();

  return exit_status();
}
