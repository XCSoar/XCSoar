// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Path.hpp"

#include <cstring>

extern "C" {
#include "tap.h"
}

int main()
{
  plan_tests(58);

  /* IsValidFilename() — valid cases */
  ok1(Path("file.txt").IsValidFilename());
  ok1(Path("data.xcm").IsValidFilename());
  ok1(Path(".hidden").IsValidFilename());
  ok1(Path("a").IsValidFilename());
  ok1(Path("file-with-dashes_and_underscores").IsValidFilename());

  /* IsValidFilename() — invalid cases */
  ok1(!Path(nullptr).IsValidFilename());
  ok1(!Path("").IsValidFilename());
  ok1(!Path(".").IsValidFilename());
  ok1(!Path("..").IsValidFilename());
  ok1(!Path("sub/file.txt").IsValidFilename());
  ok1(!Path("../escape").IsValidFilename());

  /* IsBase() */
  ok1(Path("plain").IsBase());
  ok1(Path("file.txt").IsBase());
  ok1(!Path("a/b").IsBase());
  ok1(!Path("/").IsBase());
  ok1(!Path("/absolute").IsBase());

  /* IsAbsolute() */
  ok1(Path("/").IsAbsolute());
  ok1(Path("/home/user").IsAbsolute());
  ok1(!Path("relative").IsAbsolute());
  ok1(!Path("a/b").IsAbsolute());

  /* GetBase() */
  ok1(Path("file.txt").GetBase() == Path("file.txt"));
  ok1(Path("/home/file.txt").GetBase() == Path("file.txt"));
  ok1(Path("a/b/c").GetBase() == Path("c"));
  ok1(Path("/").GetBase() == nullptr);

  /* GetParent() */
  ok1(Path("file.txt").GetParent() == Path("."));
  ok1(Path("/home/file.txt").GetParent() == Path("/home"));
  ok1(Path("/file.txt").GetParent() == Path("."));
  ok1(Path("a/b/c").GetParent() == Path("a/b"));

  /* RelativeTo() */
  ok1(Path("/home/user/file.txt").RelativeTo(Path("/home/user"))
      == Path("file.txt"));
  ok1(Path("/home/user/sub/file.txt").RelativeTo(Path("/home/user"))
      == Path("sub/file.txt"));
  ok1(Path("/other/file.txt").RelativeTo(Path("/home")) == nullptr);
  ok1(Path("/home").RelativeTo(Path("/home")) == nullptr);

  /* operator== / operator!= */
  ok1(Path("abc") == Path("abc"));
  ok1(Path("abc") != Path("def"));
  ok1(Path(nullptr) == Path(nullptr));
  ok1(Path(nullptr) != Path("abc"));
  ok1(Path("abc") != Path(nullptr));
  ok1(!(Path(nullptr) == Path("abc")));

  /* empty() */
  ok1(Path("").empty());
  ok1(!Path("a").empty());

  /* operator+ */
  ok1(Path("file") + ".txt" == Path("file.txt"));
  ok1(Path("/home/user") + "/sub" == Path("/home/user/sub"));

  /* EndsWithIgnoreCase() */
  ok1(Path("track.igc").EndsWithIgnoreCase(".igc"));
  ok1(Path("TRACK.IGC").EndsWithIgnoreCase(".igc"));
  ok1(Path("track.IGC").EndsWithIgnoreCase(".igc"));
  ok1(!Path("track.cup").EndsWithIgnoreCase(".igc"));
  ok1(!Path("igc").EndsWithIgnoreCase(".igc"));

  /* GetSuffix() */
  ok(strcmp(Path("file.txt").GetSuffix(), ".txt") == 0,
     "GetSuffix .txt");
  ok(strcmp(Path("archive.tar.gz").GetSuffix(), ".gz") == 0,
     "GetSuffix .tar.gz");
  ok1(Path("no_suffix").GetSuffix() == nullptr);
  ok1(Path(".hidden").GetSuffix() == nullptr);

  /* WithSuffix() */
  ok1(Path("file.txt").WithSuffix(".igc") == Path("file.igc"));
  ok1(Path("file").WithSuffix(".igc") == Path("file.igc"));
  ok1(Path("/home/file.txt").WithSuffix(".cup")
      == Path("/home/file.cup"));

  /* ToUTF8() */
  ok1(Path("hello").ToUTF8() == "hello");
  ok1(Path(nullptr).ToUTF8().empty());

  /* AllocatedPath::Build() */
  ok1(AllocatedPath::Build(Path("/home"), Path("file"))
      == Path("/home/file"));
  ok1(AllocatedPath::Build(Path("a"), Path("b"))
      == Path("a/b"));

  return exit_status();
}
