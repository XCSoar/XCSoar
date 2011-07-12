/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef WINGDI_H
#define WINGDI_H

#define LF_FACESIZE 256

enum {
  FW_MEDIUM = 500,
  FW_BOLD = 700,
};

typedef struct tagLOGFONT {
  unsigned lfHeight;
  unsigned lfWidth;
  unsigned char lfEscapement;
  unsigned char lfOrientation;
  unsigned short lfWeight;
  bool lfItalic;
  bool lfUnderline;
  bool lfStrikeOut;
  unsigned char lfCharSet;
  unsigned char lfOutPrecision;
  unsigned char lfClipPrecision;
  unsigned char lfQuality;
  unsigned char lfPitchAndFamily;
	char lfFaceName[LF_FACESIZE];
} LOGFONT;

#endif
