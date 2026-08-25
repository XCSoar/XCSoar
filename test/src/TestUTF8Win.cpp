// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/UTF8Win32.hpp"
#include "system/FileUtil.hpp"
#include "system/OpenPathFile.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "util/Macros.hpp"
#include "util/SpanCast.hxx"
#include "util/UTF8.hpp"
#include "util/StringAPI.hxx"

#include <span>
#include <string>
#include <string_view>

#include <windows.h>

extern "C" {
#include "tap.h"
}

/**
 * UTF-8 path samples exercised on Windows CI (Wine). Covers scripts
 * that use different legacy ANSI code pages (CP1251, GBK, CP932, …).
 */
struct ScriptSample {
  const char *utf8;
  std::size_t wide_chars;
};

static constexpr ScriptSample SCRIPT_SAMPLES[] = {
  /* Russian Линар (#2824 reporter path) */
  { "\xd0\x9b\xd0\xb8\xd0\xbd\xd0\xb0\xd1\x80", 5 },
  /* Ukrainian Київ */
  { "\xd0\x9a\xd0\xb8\xd1\x97\xd0\xb2", 4 },
  /* Greek Αθήνα */
  { "\xce\x91\xce\xb8\xce\xae\xce\xbd\xce\xb1", 5 },
  /* Hebrew עברית */
  { "\xd7\xa2\xd7\x91\xd7\xa8\xd7\x99\xd7\xaa", 5 },
  /* Arabic العربية */
  { "\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9", 7 },
  /* Telugu తెలుగు */
  { "\xe0\xb0\xa4\xe0\xb1\x86\xe0\xb0\xb2\xe0\xb1\x81\xe0\xb0\x97\xe0\xb1\x81", 6 },
  /* Vietnamese Việt */
  { "Vi\xe1\xbb\x87t", 4 },
  /* Chinese 北京 */
  { "\xe5\x8c\x97\xe4\xba\xac", 2 },
  /* Japanese 東京 */
  { "\xe6\x9d\xb1\xe4\xba\xac", 2 },
  /* Korean 한국 */
  { "\xed\x95\x9c\xea\xb5\xad", 2 },
  /* Western European café */
  { "caf\xc3\xa9", 4 },
  /* Supplementary-plane emoji (UTF-16 surrogate pair) */
  { "\xf0\x9f\x98\x80", 2 },
};

class FindUtf8Igc final : public File::Visitor {
public:
  Path expected_name;
  bool found = false;
  bool name_utf8 = false;
  bool path_utf8 = false;
  bool exists = false;

  explicit FindUtf8Igc(Path expected_name) noexcept
    :expected_name(expected_name) {}

  void Visit(Path path, Path filename) override {
    if (!StringIsEqual(filename.c_str(), expected_name.c_str()))
      return;

    found = true;
    name_utf8 = ValidateUTF8(filename.c_str());
    path_utf8 = ValidateUTF8(path.c_str());
    exists = File::Exists(path);
  }
};

/**
 * Create an empty temp directory (UTF-8 Path from GetTempPathW).
 */
static AllocatedPath
MakeTempDir() noexcept
{
  wchar_t temp[MAX_PATH];
  const DWORD n = GetTempPathW(MAX_PATH, temp);
  if (n == 0 || n >= MAX_PATH)
    return nullptr;

  std::wstring path(temp, n);
  path += L"xcsoar-utf8-";
  path += std::to_wstring(GetCurrentProcessId());

  if (!CreateDirectoryW(path.c_str(), nullptr) &&
      GetLastError() != ERROR_ALREADY_EXISTS)
    return nullptr;

  return AllocatedPath{WideToUTF8(path).c_str()};
}

static void
TestUtf8FileRoundTrip() noexcept
{
  const AllocatedPath dir = MakeTempDir();
  if (dir == nullptr) {
    skip(6, 0, "temp directory failed");
    return;
  }

  /* café.igc — non-ASCII name that used to break list/Exists/open */
  static constexpr const char *const utf8_name = "caf\xc3\xa9.igc";
  const AllocatedPath file = AllocatedPath::Build(dir, Path(utf8_name));

  static constexpr std::string_view payload = "AIGC UTF-8 path test\n";
  try {
    FileOutputStream out(file, FileOutputStream::Mode::CREATE);
    out.Write(AsBytes(payload));
    out.Commit();
  } catch (...) {
    skip(6, 0, "FileOutputStream failed");
    Directory::Remove(dir);
    return;
  }

  ok1(File::Exists(file));

  FindUtf8Igc visitor{Path(utf8_name)};
  Directory::VisitSpecificFiles(dir, "*.igc", visitor, false);
  ok1(visitor.found);
  ok1(visitor.name_utf8);
  ok1(visitor.path_utf8);
  ok1(visitor.exists);

  try {
    FileReader reader(file);
    char buffer[64]{};
    const std::size_t n =
      reader.Read(std::as_writable_bytes(std::span{buffer}));
    ok1(n == payload.size() &&
        std::string_view(buffer, n) == payload);
  } catch (...) {
    ok1(false);
  }

  File::Delete(file);
  Directory::Remove(dir);
}

/**
 * UTF8ToWide / WideToUTF8 for each script sample.
 * 3 assertions per sample.
 */
static void
TestScriptConversions() noexcept
{
  for (const auto &sample : SCRIPT_SAMPLES) {
    const std::string_view utf8{sample.utf8};
    ok1(ValidateUTF8(utf8));
    ok1(UTF8ToWide(utf8).size() == sample.wide_chars);
    ok1(WideToUTF8(UTF8ToWide(utf8)) == sample.utf8);
  }
}

/**
 * Directory Create/Exists/IsWritable/Remove for each script.
 * Also writes a small file via OpenPathFile under that directory.
 * 8 assertions per sample.
 */
static void
TestScriptDirectories() noexcept
{
  constexpr unsigned PER_SAMPLE = 8;
  constexpr unsigned TOTAL = PER_SAMPLE * ARRAY_SIZE(SCRIPT_SAMPLES);

  const AllocatedPath dir = MakeTempDir();
  if (dir == nullptr) {
    skip(TOTAL, 0, "temp directory failed");
    return;
  }

  static constexpr std::string_view PAYLOAD = "utf8\n";

  for (const auto &sample : SCRIPT_SAMPLES) {
    const AllocatedPath sub =
      AllocatedPath::Build(dir, Path(sample.utf8));

    Directory::Create(sub);
    ok1(Directory::Exists(sub));
    ok1(ValidateUTF8(sub.c_str()));
    ok1(Directory::IsWritable(sub));

    const AllocatedPath file =
      AllocatedPath::Build(sub, Path("t.txt"));
    FILE *out = OpenPathFile(file, "wb");
    ok1(out != nullptr);
    if (out != nullptr) {
      const auto n = fwrite(PAYLOAD.data(), 1, PAYLOAD.size(), out);
      fclose(out);
      ok1(n == PAYLOAD.size());
    } else {
      ok1(false);
    }

    ok1(File::Exists(file));
    File::Delete(file);

    ok1(Directory::Remove(sub));
    ok1(!Directory::Exists(sub));
  }

  Directory::Remove(dir);
}

/**
 * Bytes that are valid CP1251 for "Ли" must not be accepted as UTF-8.
 * (Do not call UTF8ToWide here — debug builds assert on invalid input.)
 */
static void
TestRejectAcpBytes() noexcept
{
  static constexpr char cp1251_li[] = { '\xcb', '\xe8', '\0' };
  ok1(!ValidateUTF8(cp1251_li));
}

int
main()
{
  constexpr unsigned N_SCRIPTS = ARRAY_SIZE(SCRIPT_SAMPLES);
  plan_tests(12 + 6 + 3 * N_SCRIPTS + 8 * N_SCRIPTS + 1);

  /* Returned size() must equal character count (no null in size) */
  ok1(UTF8ToWide(std::string_view("")).size() == 0);
  ok1(UTF8ToWide(std::string_view("H")).size() == 1);
  ok1(UTF8ToWide(std::string_view("Hi")).size() == 2);
  ok1(UTF8ToWide(std::string_view("Hello")).size() == 5);

  /* Multi-byte UTF-8: one wide char per Unicode code point */
  ok1(UTF8ToWide(std::string_view("\xc3\xbc")).size() == 1);   /* U+00FC */
  ok1(UTF8ToWide(std::string_view("caf\xc3\xa9")).size() == 4); /* café */

  /* c_str() remains null-terminated */
  std::wstring w = UTF8ToWide(std::string_view("x"));
  ok1(w.size() == 1);
  ok1(w.c_str()[0] == L'x' && w.c_str()[1] == L'\0');

  /* Round-trip through WideToUTF8 */
  ok1(WideToUTF8(UTF8ToWide(std::string_view("caf\xc3\xa9"))) ==
      "caf\xc3\xa9");
  ok1(WideToUTF8(UTF8ToWide(std::string_view("G\xc3\xa9rard"))) ==
      "G\xc3\xa9rard");
  ok1(WideToUTF8(std::wstring_view()).empty());
  ok1(WideToUTF8((const wchar_t *)nullptr).empty());

  TestUtf8FileRoundTrip();
  TestScriptConversions();
  TestScriptDirectories();
  TestRejectAcpBytes();

  return exit_status();
}
