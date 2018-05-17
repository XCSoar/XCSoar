/***********************************************************************
**
**   grecord.h
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

#ifndef GRECORD_H
#define GRECORD_H

#include <stdint.h>
#include <stdio.h>

class GRECORD {
 private:
  char grecord[80];
  int  tricnt;
  int  gcnt;
  uint8_t ba[3];
  FILE *ausgabe;
  void init(void);                  // Initialisieren der Werte

 public:
  GRECORD(FILE *ausgabedatei);
  void update(uint8_t b);
  void finish(void);

};


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
print_g_record(FILE *datei, const uint8_t *puffer, size_t puflen);

#endif
