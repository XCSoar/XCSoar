#include "Screen/Font.hpp"

bool
Font::set(const LOGFONT *lplf)
{
  reset();

  font = ::CreateFontIndirect(lplf);
  if (font == NULL)
    return false;

  if (GetObjectType(font) != OBJ_FONT) {
    reset();
    return false;
  }

  return true;
}

void
Font::reset()
{
  if (font != NULL) {
    ::DeleteObject(font);
    font = NULL;
  }
}
