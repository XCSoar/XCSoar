// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/FileUtil.hpp"
#include "TestUtil.hpp"

#ifdef __linux__
#include <dirent.h>
#include <stdexcept>

static unsigned
CountFileDescriptors()
{
  DIR *dir = opendir("/proc/self/fd");
  if (dir == nullptr)
    return 0;

  unsigned count = 0;
  while (readdir(dir) != nullptr)
    ++count;
  closedir(dir);
  return count;
}

static void
TestVisitorException()
{
  const auto before = CountFileDescriptors();
  if (before == 0) {
    skip(2, 0, "/proc/self/fd unavailable");
    return;
  }

  struct ThrowingVisitor final : File::Visitor {
    void Visit(Path, Path) override {
      throw std::runtime_error{"test visitor failure"};
    }
  } visitor;

  unsigned caught = 0;
  for (unsigned i = 0; i < 5; ++i) {
    try {
      Directory::VisitFiles(Path{"test/data/file_visitor_test"}, visitor);
    } catch (const std::runtime_error &) {
      ++caught;
    }
  }
  ok1(caught == 5);
  ok1(CountFileDescriptors() == before);
}
#endif

class TestingFileVisitor: public File::Visitor
{
private:
  bool recursive;
  bool filtered;

public:
  TestingFileVisitor(bool _recursive, bool _filtered) :
    recursive(_recursive), filtered(_filtered) {}

  void
  Visit([[maybe_unused]] Path path, Path filename)
  {
    if (filename == Path("a.txt")) {
      ok(true, "a.txt");
    } else if (filename == Path("b.txt")) {
      ok(true, "b.txt");
    } else if (filename == Path("c.tx")) {
      ok(!filtered, "c.tx");
    } else if (filename == Path("d.txt")) {
      ok(recursive, "d.txt");
    } else {
      ok(false, "unexpected file");
    }
  }
};

int main()
{
  plan_tests(17
#ifdef __linux__
             + 2
#endif
             );

  ok1(Directory::Exists(Path("test/data/file_visitor_test")));
  ok1(File::Exists(Path("test/data/file_visitor_test/a.txt")));
  ok1(File::Exists(Path("test/data/file_visitor_test/b.txt")));
  ok1(File::Exists(Path("test/data/file_visitor_test/c.tx")));
  ok1(File::Exists(Path("test/data/file_visitor_test/subfolder/d.txt")));

  TestingFileVisitor fv1(false, false);
  Directory::VisitFiles(Path("test/data/file_visitor_test"), fv1, false);

  TestingFileVisitor fv2(true, false);
  Directory::VisitFiles(Path("test/data/file_visitor_test"), fv2, true);

  TestingFileVisitor fv3(false, true);
  Directory::VisitSpecificFiles(Path("test/data/file_visitor_test"),
                                "*.txt", fv3, false);

  TestingFileVisitor fv4(true, true);
  Directory::VisitSpecificFiles(Path("test/data/file_visitor_test"),
                                "*.txt", fv4, true);

#ifdef __linux__
  TestVisitorException();
#endif

  return exit_status();
}
