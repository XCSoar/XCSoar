/*! \file
 * \brief Small Windows GDI helper functions
 */

#ifndef XCSOAR_SCREEN_UTIL_HPP
#define XCSOAR_SCREEN_UTIL_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum {
  MAXCLIPPOLYGON = 5000,
};

void ClipPolygon(HDC hdc, const POINT *m_ptin, unsigned int inLength,
                 RECT rc, bool fill);

void ClipPolyline(HDC hdc, POINT* pt, const int npoints, const RECT rc);

void ClipLine(const HDC& hdc, const POINT &ptStart,
              const POINT &ptEnd, const RECT rc);

int  Circle(HDC hdc, long x, long y, int radius, RECT rc, bool clip=false,
            bool fill=true);
int Segment(HDC hdc, long x, long y, int radius, RECT rc,
	    double start,
	    double end,
            bool horizon= false);
// VENTA3 DrawArc
int DrawArc(HDC hdc, long x, long y, int radius, RECT rc,
	    double start,
	    double end);

int
GetTextWidth(HDC hDC, const TCHAR *text);
void
ExtTextOutClip(HDC hDC, int x, int y, const TCHAR *text, int width);

void PolygonRotateShift(POINT* poly, int n, int x, int y,
                        double angle);

#endif
