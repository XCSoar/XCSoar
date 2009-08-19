#ifndef XCSOAR_SCREEN_ANIMATION_HPP
#define XCSOAR_SCREEN_ANIMATION_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern bool EnableAnimation;

void SetSourceRectangle(RECT fromRect);

////////////////////////////////////////////////////////////////////////////
//
// FUNCTION:    DrawWireRects
//
// DESCRIPTION: Creates exploding wire rectanges
//
// INPUTS:  LPRECT lprcFrom      Source Rectangle
//          LPRECT lprcTo        Destination Rectangle
//          UINT nMilliSecSpeed  Speed in millisecs for animation
//
// RETURN:    None
// NOTES:    None
//
//  Maintenance Log
//  Author      Date    Version     Notes
//  NT Almond   011199  1.0         Origin
//  CJ Maunder  010899  1.1         Modified rectangle transition code
//
/////////////////////////////////////////////////////////////////////////
RECT WINAPI DrawWireRects(LPRECT lprcTo, UINT nMilliSecSpeed);

#endif
