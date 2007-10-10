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

#include "vlapityp.h"
#include <stdio.h>


class GRECORD {
 private:
  char grecord[80];
  int  tricnt;
  int  gcnt;
  byte ba[3];
  FILE *ausgabe;
  void init(void);                  // Initialisieren der Werte

 public:
  GRECORD(FILE *ausgabedatei);
  void update(byte b);
  void final (void);

};


/*
DATA-GCS:
  - Binärblock beim Logger anfordern und im Speicher ablegen
* - Binärblock ins IGC-Format konvertieren
* - IGC-Datei abspeichern
  - Binärblock im radix-64-Format als G-Records an IGC-Datei anhängen

VALI-GCS:
  - IGC-Datei laden und ohne die nicht vom Logger stammenden Datensätze
    und Whitespaces in temp1.igc abspeichern
  - G-Records aus IGC-Datei laden von radix-64 in Binärblock umwandeln
* - Binärblock ins IGC-Format konvertieren
*   und speichern in Datei temp2.igc
  - Sicherheitscheck:
    Dateien temp1 und temp2 vergleichen
    Signatur überprüfen

* kann für DATA- und VALI-Programm genutzt werden



Benötigte Funktionen: (D=für DATA, V=für VALI, P=schon programmiert)
DV P
x  x - Verzeichnis der Flüge auslesen
x  x - Binärblock(Flug) vom Logger lesen
xx   - Binärblock ins IGC-Format konvertieren dabei IGC-Datei abspeichern
x    - Dateiname nach IGC-Vorschrift generieren
xx   - Datei kopieren
 x   - Signatur in Binärblock überprüfen
x  x - Binärblock in GR64 konvertieren und anhängen
 x   - GR64 laden, in Binärblock umwandeln und im Speicher ablegen
     - IGC-Datei laden und alle nicht vom Logger stammenden Datensätze
       ausfiltern, die Datei dann wieder als temp-Datei abspeichern

*/


/*
Den Inhalt des Puffers *puffer mit der Länge puflen radix-64 kodieren
und je 72 Datenworte als G-Datensatz an datei *dateiname anhängen
*/
//void append_g_record(char *dateiname, byte huge *puffer, unsigned long puflen);

void print_g_record(FILE *datei, lpb puffer, int32 puflen);


/*
Aus Datei *dateiname die G-Records extrahieren (nur als zusammenhängender
Block), von radix-64 in Binär umwandeln und in *puffer speichern.
Pufferlänge puflen ist angegeben, um ein Überschreiben nicht zum Puffer
gehörender Bereiche zu verhindern
*/
int get_g_record(char *dateiname, lpb puffer, unsigned long puflen);


// Eine IGC-Datei von allen Zeilen befreien, die vom Pilot oder OO legal zur
// Datei hinzugefügt worden sein könnten
// Speichern der "cleanen" Datei
void clean_igcfile(char *quelldateiname, char *zieldateiname);


#endif
