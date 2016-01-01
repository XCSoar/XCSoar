/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "OS/FileUtil.hpp"
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
  Visit(Path path, Path filename)
  {
    if (filename == Path(_T("a.txt"))) {
      ok(true, "a.txt");
    } else if (filename == Path(_T("b.txt"))) {
      ok(true, "b.txt");
    } else if (filename == Path(_T("c.tx"))) {
      ok(!filtered, "c.tx");
    } else if (filename == Path(_T("d.txt"))) {
      ok(recursive, "d.txt");
    } else {
      ok(false, "unexpected file");
    }
  }
};

int main(int argc, char **argv)
{
  plan_tests(17);

  ok1(Directory::Exists(Path(_T("test/data/file_visitor_test"))));
  ok1(File::Exists(Path(_T("test/data/file_visitor_test/a.txt"))));
  ok1(File::Exists(Path(_T("test/data/file_visitor_test/b.txt"))));
  ok1(File::Exists(Path(_T("test/data/file_visitor_test/c.tx"))));
  ok1(File::Exists(Path(_T("test/data/file_visitor_test/subfolder/d.txt"))));

  TestingFileVisitor fv1(false, false);
  Directory::VisitFiles(Path(_T("test/data/file_visitor_test")), fv1, false);

  TestingFileVisitor fv2(true, false);
  Directory::VisitFiles(Path(_T("test/data/file_visitor_test")), fv2, true);

  TestingFileVisitor fv3(false, true);
  Directory::VisitSpecificFiles(Path(_T("test/data/file_visitor_test")),
                                _T("*.txt"), fv3, false);

  TestingFileVisitor fv4(true, true);
  Directory::VisitSpecificFiles(Path(_T("test/data/file_visitor_test")),
                                _T("*.txt"), fv4, true);

  return exit_status();
}
