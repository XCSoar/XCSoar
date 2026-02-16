// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/FileUtil.hpp"
#include "TestUtil.hpp"

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
  plan_tests(17);

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

  return exit_status();
}
