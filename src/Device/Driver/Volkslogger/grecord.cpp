/***********************************************************************
**
**   grecord.cpp
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include "grecord.h"

#include <stdio.h>
#include <string.h>

// base-64 functions
//
static const char *
byte_bas64(const uint8_t *b)
{
 static constexpr char base64tab[] =
   "0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ`abcdefghijklmnopqrstuvwxyz";
 static char bas64ar[5];
  bas64ar[0] = base64tab[   b[0] >> 2                           ];
  bas64ar[1] = base64tab[ ((b[0] & 0x03) << 4) | (b[1] >> 4)    ];
  bas64ar[2] = base64tab[ ((b[1] & 0x0f) << 2) | (b[2] >> 6)    ];
  bas64ar[3] = base64tab[   b[2] & 0x3f                         ];
  bas64ar[4] = 0;
  return bas64ar;
}

// g-record functions
//
GRECORD::GRECORD(FILE *ausgabedatei) {
  strcpy(grecord, "");
  tricnt = 0;
  gcnt   = 0;
  memset(ba,0xff,3);
  ausgabe = ausgabedatei;
}

void
GRECORD::update(uint8_t b)
{
  ba[tricnt++] = b;
  if (tricnt == 3) {
  tricnt = 0;
  strcat(grecord,byte_bas64(ba));
  memset(ba,0xff,3);
  gcnt++;
    if (gcnt == 18) {
      gcnt = 0;
      fprintf(ausgabe,"G%s\n",grecord);
      strcpy(grecord, "");
    }
  }
}

void GRECORD::finish(void) {
  if (tricnt || gcnt) {
    strcat(grecord,byte_bas64(ba));
    fprintf(ausgabe,"G%s\n",grecord);
  }
}

/*
DATA-GCS:
  - Bin�rblock beim Logger anfordern und im Speicher ablegen
* - Bin�rblock ins IGC-Format konvertieren
* - IGC-Datei abspeichern
  - Bin�rblock im radix-64-Format als G-Records an IGC-Datei anh�ngen

VALI-GCS:
  - IGC-Datei laden und ohne die nicht vom Logger stammenden Datens�tze
    und Whitespaces in temp1.igc abspeichern
  - G-Records aus IGC-Datei laden von radix-64 in Bin�rblock umwandeln
* - Bin�rblock ins IGC-Format konvertieren
*   und speichern in Datei temp2.igc
  - Sicherheitscheck:
    Dateien temp1 und temp2 vergleichen
    Signatur �berpr�fen

* kann f�r DATA- und VALI-Programm genutzt werden



Ben�tigte Funktionen: (D=f�r DATA, V=f�r VALI, P=schon programmiert)
DV P
x  x - Verzeichnis der Fl�ge auslesen
x  x - Bin�rblock(Flug) vom Logger lesen
xx   - Bin�rblock ins IGC-Format konvertieren dabei IGC-Datei abspeichern
x    - Dateiname nach IGC-Vorschrift generieren
xx   - Datei kopieren
 x   - Signatur in Bin�rblock �berpr�fen
x  x - Bin�rblock in GR64 konvertieren und anh�ngen
 x   - GR64 laden, in Bin�rblock umwandeln und im Speicher ablegen
     - IGC-Datei laden und alle nicht vom Logger stammenden Datens�tze
       ausfiltern, die Datei dann wieder als temp-Datei abspeichern

*/

void
print_g_record(FILE *datei, const uint8_t *puffer, size_t puflen)
{
  GRECORD g1(datei);
  for (size_t i = 0; i < puflen; ++i)
    g1.update(puffer[i]);
  g1.finish();
}
