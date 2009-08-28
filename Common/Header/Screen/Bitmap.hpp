#ifndef XCSOAR_SCREEN_BITMAP_HPP
#define XCSOAR_SCREEN_BITMAP_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <tchar.h>

class Bitmap {
protected:
  HBITMAP bitmap;

public:
  Bitmap():bitmap(NULL) {}
  ~Bitmap();

public:
  bool defined() const {
    return bitmap != NULL;
  }

  void load(const TCHAR *name);

  void load(WORD id) {
    load(MAKEINTRESOURCE(id));
  }

  void create(const BITMAPINFO *pbmi, VOID **ppvBits);

  void reset();

  const SIZE get_size() const;

  HBITMAP native() const {
    return bitmap;
  }
};

#endif
