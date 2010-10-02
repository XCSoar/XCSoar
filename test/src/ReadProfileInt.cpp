#include "Profile/Profile.hpp"

#include <stdio.h>
#include <windows.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s NAME\n", argv[0]);
    return 1;
  }

#ifdef _UNICODE
  TCHAR name[1024];
  int length = ::MultiByteToWideChar(CP_ACP, 0, argv[1], -1, name, 1024);
  if (length == 0)
    return 2;
#else
  const char *name = argv[1];
#endif

  int value;
  if (Profile::Get(name, value)) {
    printf("%d\n", value);
    return 0;
  } else {
    fputs("No such value\n", stderr);
    return 2;
  }
}
