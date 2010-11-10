#ifndef ZZIP_UTIL_H
#define ZZIP_UTIL_H

#include <zzip/lib.h>

#include <fcntl.h>

/**
 * Opens a file inside a ZIP archive in read-only binary mode.
 *
 * @param dir optionally a ZZIP_DIR object
 * @param path the path inside the archive
 */
static inline ZZIP_FILE *
zzip_open_rb(ZZIP_DIR *dir, const char *path)
{
  int mode = O_RDONLY;
#ifdef O_NOCTTY
  mode |= O_NOCTTY;
#endif
#ifdef O_BINARY
  mode |= O_BINARY;
#endif

  return dir != NULL
    ? zzip_file_open(dir, path, mode)
    : zzip_open(path, mode);
}

#endif
