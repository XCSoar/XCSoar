// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Topography/TopographyFile.hpp"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"

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

  TopoInput(std::span<const uint8_t> input) {
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

  void SplitWrite(std::span<const uint8_t> &input, const char *suffix) {
    const uint8_t *p = (const uint8_t *)
      memmem(input.data(), input.size(), "deadbeef", 8);
    auto segment = input;
    if (p != nullptr) {
      segment = segment.first(p - segment.data());
      input = input.subspan(p + 8 - input.data());
    }

    paths.emplace_front(name);
    paths.front() += suffix;

    auto fd = OpenWriteOnly(paths.front().c_str(), O_CREAT|O_EXCL);
    if (!segment.empty())
      (void)fd.Write(segment.data(), segment.size());
  }
};

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  try {
    TopoInput input({data, size});
    TopographyFile file(nullptr, input.name.c_str(),
                        1, 1, 1,
                        {});
  } catch (...) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
