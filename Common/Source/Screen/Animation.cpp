#include "Screen/Animation.hpp"

#if (WINDOWSPC<1)
#define GdiFlush() do { } while (0)
#endif

bool EnableAnimation = false;

static RECT AnimationRectangle = {0,0,0,0};

void SetSourceRectangle(RECT fromRect) {
  AnimationRectangle = fromRect;
}

RECT WINAPI DrawWireRects(LPRECT lprcTo, UINT nMilliSecSpeed)
{
  if (!EnableAnimation)
    return AnimationRectangle;

  LPRECT lprcFrom = &AnimationRectangle;
  const int nNumSteps = 10;

  GdiFlush();
  Sleep(10);  // Let the desktop window sort itself out

  // if hwnd is null - "you have the CON".
  HDC hDC = ::GetDC(NULL);

  // Pen size, urmmm not too thick
  HPEN hPen = ::CreatePen(PS_SOLID, 2, RGB(0,0,0));

  int nMode = ::SetROP2(hDC, R2_NOT);
  HPEN hOldPen = (HPEN) ::SelectObject(hDC, hPen);

  for (int i = 0; i < nNumSteps; i++)
    {
      double dFraction = (double) i / (double) nNumSteps;

      RECT transition;
      transition.left   = lprcFrom->left +
	(int)((lprcTo->left - lprcFrom->left) * dFraction);
      transition.right  = lprcFrom->right +
	(int)((lprcTo->right - lprcFrom->right) * dFraction);
      transition.top    = lprcFrom->top +
	(int)((lprcTo->top - lprcFrom->top) * dFraction);
      transition.bottom = lprcFrom->bottom +
	(int)((lprcTo->bottom - lprcFrom->bottom) * dFraction);

      POINT pt[5];
      pt[0].x = transition.left; pt[0].y= transition.top;
      pt[1].x = transition.right; pt[1].y= transition.top;
      pt[2].x = transition.right; pt[2].y= transition.bottom;
      pt[3].x = transition.left; pt[3].y= transition.bottom;
      pt[4].x = transition.left; pt[4].y= transition.top;

      // We use Polyline because we can determine our own pen size
      // Draw Sides
      ::Polyline(hDC,pt,5);

      GdiFlush();

      Sleep(nMilliSecSpeed);

      // UnDraw Sides
      ::Polyline(hDC,pt,5);

      GdiFlush();
    }

  ::SetROP2(hDC, nMode);
  ::SelectObject(hDC, hOldPen);
  ::DeleteObject(hPen);
  ::ReleaseDC(NULL,hDC);
  return AnimationRectangle;
}
