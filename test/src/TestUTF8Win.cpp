// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/UTF8Win32.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
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
 * Create an empty temp directory with an ASCII path (so teardown via
 * Directory::Remove remains reliable).
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

int
main()
{
  plan_tests(18);

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

  return exit_status();
}
