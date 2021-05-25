/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Topography/TopographyFile.hpp"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "util/ConstBuffer.hxx"

#include <string>
#include <forward_list>

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

struct TopoInput {
  std::string name;

  std::forward_list<std::string> paths;

  TopoInput(ConstBuffer<uint8_t> input) {
    char buffer[1024];
    sprintf(buffer, "/tmp/FuzzTopographyFile-%d", gettid());
    name = buffer;

    /* split the input buffer into separate files */
    SplitWrite(input, ".shp");
    SplitWrite(input, ".shx");
    SplitWrite(input, ".dbf");
    SplitWrite(input, ".qix");
  }

  ~TopoInput() noexcept {
    for (const auto &i : paths)
      unlink(i.c_str());
  }

  TopoInput(const TopoInput &) = delete;
  TopoInput &operator=(const TopoInput &) = delete;

  void SplitWrite(ConstBuffer<uint8_t> &input, const char *suffix) {
    const uint8_t *p = (const uint8_t *)
      memmem(input.data, input.size, "deadbeef", 8);
    ConstBuffer<uint8_t> segment = input;
    if (p != nullptr) {
      segment.SetEnd(p);
      input.MoveFront(p + 8);
    }

    paths.emplace_front(name);
    paths.front() += suffix;

    auto fd = OpenWriteOnly(paths.front().c_str(), O_CREAT|O_EXCL);
    if (!segment.empty())
      fd.Write(segment.data, segment.size);
  }
};

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  try {
    TopoInput input({data, size});
    TopographyFile file(nullptr, input.name.c_str(),
                        1, 1, 1,
                        Color{});
  } catch (...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
