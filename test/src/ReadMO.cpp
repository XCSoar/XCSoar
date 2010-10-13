#include "MOLoader.hpp"

#include <stdio.h>

#ifdef _UNICODE
#include <windows.h>
#include <syslimits.h>
#endif

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s FILE.mo STRING\n", argv[0]);
    return 1;
  }

#ifdef _UNICODE
  TCHAR path[PATH_MAX];
  int length = ::MultiByteToWideChar(CP_ACP, 0, argv[1], -1, path, PATH_MAX);
  if (length == 0)
    return 2;
#else
  const char *path = argv[1];
#endif

  const char *original = argv[2];

  MOLoader mo(path);
  if (mo.error()) {
    fprintf(stderr, "Failed to load %s\n", path);
    return 2;
  }

  const char *translated = mo.get().lookup(original);
  if (translated == NULL) {
    fprintf(stderr, "No such string\n");
    return 3;
  }

  puts(translated);
  return 0;
}
