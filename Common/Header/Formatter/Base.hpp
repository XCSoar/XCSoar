#ifndef XCSOAR_FORMATTER_BASE_HPP
#define XCSOAR_FORMATTER_BASE_HPP

#include "Sizes.h"

#include <tchar.h>

class InfoBoxFormatter {
 public:
  InfoBoxFormatter(const TCHAR *theformat);

  virtual const TCHAR *Render(int *color);
  virtual const TCHAR *RenderTitle(int *color); // VENTA3
  void RenderInvalid(int *color);
  bool Valid;
  double Value;
  TCHAR Format[FORMAT_SIZE+1];
  TCHAR Text[100];
  TCHAR CommentText[100];

  virtual void AssignValue(int i);
  const TCHAR *GetCommentText();
  bool isValid();
};

#endif
