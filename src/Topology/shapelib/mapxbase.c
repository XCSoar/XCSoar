/*
** This code is entirely based on the previous work of Frank Warmerdam. It is
** essentially shapelib 1.1.5. However, there were enough changes that it was
** incorporated into the MapServer source to avoid confusion. See the README
** for licence details.
*/
/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "mapprimitive.h"
#include "mapshape.h"
#include "maperror.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/
static void *
SfRealloc(void *pMem, size_t nNewSize)
{
  return realloc(pMem, nNewSize);
}

/************************************************************************/
/*                              msDBFOpen()                             */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/
DBFHandle msDBFOpen( const char * pszFilename, const char * pszAccess )

{
    DBFHandle		psDBF;
    uchar		*pabyBuf;
    int			nFields, nRecords, nHeadLen, nRecLen, iField;
    char	        *pszDBFFilename;

    /* -------------------------------------------------------------------- */
    /*      We only allow the access strings "rb" and "r+".                 */
    /* -------------------------------------------------------------------- */
    if( strcmp(pszAccess,"r") != 0 && strcmp(pszAccess,"r+") != 0
        && strcmp(pszAccess,"rb") != 0 && strcmp(pszAccess,"r+b") != 0 )
        return( NULL );

    /* -------------------------------------------------------------------- */
    /*	Ensure the extension is converted to dbf or DBF if it is 	    */
    /*	currently .shp or .shx.						    */
    /* -------------------------------------------------------------------- */
    pszDBFFilename = (char *) malloc(strlen(pszFilename)+1);
    strcpy( pszDBFFilename, pszFilename );

    if( strcmp(pszFilename+strlen(pszFilename)-4,".shp")
	|| strcmp(pszFilename+strlen(pszFilename)-4,".shx") )
    {
        strcpy( pszDBFFilename+strlen(pszDBFFilename)-4, ".dbf");
    }
    else if( strcmp(pszFilename+strlen(pszFilename)-4,".SHP")
	     || strcmp(pszFilename+strlen(pszFilename)-4,".SHX") )
    {
        strcpy( pszDBFFilename+strlen(pszDBFFilename)-4, ".DBF");
    }

    /* -------------------------------------------------------------------- */
    /*      Open the file.                                                  */
    /* -------------------------------------------------------------------- */
    psDBF = (DBFHandle) calloc( 1, sizeof(DBFInfo) );
    psDBF->zfp = zzip_fopen( pszDBFFilename, pszAccess );
    if( psDBF->zfp == NULL )
        return( NULL );

    psDBF->nCurrentRecord = -1;

    psDBF->pszStringField = NULL;
    psDBF->nStringFieldLen = 0;

    free( pszDBFFilename );

    /* -------------------------------------------------------------------- */
    /*  Read Table Header info                                              */
    /* -------------------------------------------------------------------- */
    pabyBuf = (uchar *) malloc(500);
    zzip_fread( pabyBuf, 32, 1, psDBF->zfp );

    psDBF->nRecords = nRecords =
     pabyBuf[4] + pabyBuf[5]*256 + pabyBuf[6]*256*256 + pabyBuf[7]*256*256*256;

    psDBF->nHeaderLength = nHeadLen = pabyBuf[8] + pabyBuf[9]*256;
    psDBF->nRecordLength = nRecLen = pabyBuf[10] + pabyBuf[11]*256;

    psDBF->nFields = nFields = (nHeadLen - 32) / 32;

    psDBF->pszCurrentRecord = (char *) malloc(nRecLen);

    /* -------------------------------------------------------------------- */
    /*  Read in Field Definitions                                           */
    /* -------------------------------------------------------------------- */
    pabyBuf = (uchar *) SfRealloc(pabyBuf,nHeadLen);
    psDBF->pszHeader = (char *) pabyBuf;

    zzip_seek( psDBF->zfp, 32, 0 );
    zzip_fread( pabyBuf, nHeadLen, 1, psDBF->zfp );

    psDBF->panFieldOffset = (int *) malloc(sizeof(int) * nFields);
    psDBF->panFieldSize = (int *) malloc(sizeof(int) * nFields);
    psDBF->panFieldDecimals = (int *) malloc(sizeof(int) * nFields);
    psDBF->pachFieldType = (char *) malloc(sizeof(char) * nFields);

    for( iField = 0; iField < nFields; iField++ )
    {
	uchar		*pabyFInfo;

	pabyFInfo = pabyBuf+iField*32;

	if( pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F' )
	{
	    psDBF->panFieldSize[iField] = pabyFInfo[16];
	    psDBF->panFieldDecimals[iField] = pabyFInfo[17];
	}
	else
	{
	    psDBF->panFieldSize[iField] = pabyFInfo[16] + pabyFInfo[17]*256;
	    psDBF->panFieldDecimals[iField] = 0;
	}

	psDBF->pachFieldType[iField] = (char) pabyFInfo[11];
	if( iField == 0 )
	    psDBF->panFieldOffset[iField] = 1;
	else
	    psDBF->panFieldOffset[iField] =
	      psDBF->panFieldOffset[iField-1] + psDBF->panFieldSize[iField-1];
    }

    return( psDBF );
}

/************************************************************************/
/*                              msDBFClose()                            */
/************************************************************************/

void  msDBFClose(DBFHandle psDBF)
{
    /* -------------------------------------------------------------------- */
    /*      Close, and free resources.                                      */
    /* -------------------------------------------------------------------- */
    if (psDBF->zfp) {
      zzip_fclose( psDBF->zfp );
      psDBF->zfp = 0;
    }

    if( psDBF->panFieldOffset != NULL )
    {
        free( psDBF->panFieldOffset );
        free( psDBF->panFieldSize );
        free( psDBF->panFieldDecimals );
        free( psDBF->pachFieldType );
    }

    free( psDBF->pszHeader );
    free( psDBF->pszCurrentRecord );

    free(psDBF->pszStringField);

    free( psDBF );
}

/************************************************************************/
/*                          msDBFReadAttribute()                        */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/
static char *msDBFReadAttribute(DBFHandle psDBF, int hEntity, int iField )

{
    int	       	nRecordOffset, i;
    const char *pabyRec;
    char	*pReturnField = NULL;

    /* -------------------------------------------------------------------- */
    /*	Is the request valid?                  				    */
    /* -------------------------------------------------------------------- */
    if( iField < 0 || iField >= psDBF->nFields )
    {
        msSetError(MS_DBFERR, "Invalid field index %d.", "msDBFGetItemIndex()",iField );
        return( NULL );
    }

    if( hEntity < 0 || hEntity >= psDBF->nRecords )
    {
        msSetError(MS_DBFERR, "Invalid record number %d.", "msDBFGetItemIndex()",hEntity );
        return( NULL );
    }

    /* -------------------------------------------------------------------- */
    /*	Have we read the record?					    */
    /* -------------------------------------------------------------------- */
    if( psDBF->nCurrentRecord != hEntity )
    {
	nRecordOffset = psDBF->nRecordLength * hEntity + psDBF->nHeaderLength;

	zzip_seek( psDBF->zfp, nRecordOffset, 0 );
	zzip_fread( psDBF->pszCurrentRecord, psDBF->nRecordLength, 1, psDBF->zfp );

	psDBF->nCurrentRecord = hEntity;
    }

    pabyRec = (const char *) psDBF->pszCurrentRecord;

    /* -------------------------------------------------------------------- */
    /*	Ensure our field buffer is large enough to hold this buffer.	    */
    /* -------------------------------------------------------------------- */
    if( psDBF->panFieldSize[iField]+1 > psDBF->nStringFieldLen )
    {
	psDBF->nStringFieldLen = psDBF->panFieldSize[iField]*2 + 10;
	psDBF->pszStringField = (char *) SfRealloc(psDBF->pszStringField,psDBF->nStringFieldLen);
    }

    /* -------------------------------------------------------------------- */
    /*	Extract the requested field.					    */
    /* -------------------------------------------------------------------- */
    strncpy( psDBF->pszStringField, pabyRec+psDBF->panFieldOffset[iField], psDBF->panFieldSize[iField] );
    psDBF->pszStringField[psDBF->panFieldSize[iField]] = '\0';

    /*
    ** Trim trailing blanks (SDL Modification)
    */
    for(i=strlen(psDBF->pszStringField)-1;i>=0;i--) {
      if(psDBF->pszStringField[i] != ' ') {
	psDBF->pszStringField[i+1] = '\0';
	break;
      }
    }

    if(i == -1) psDBF->pszStringField[0] = '\0'; // whole string is blank (SDL fix)

    /*
    ** Trim/skip leading blanks (SDL/DM Modification - only on numeric types)
    */
    if( psDBF->pachFieldType[iField] == 'N' || psDBF->pachFieldType[iField] == 'F' || psDBF->pachFieldType[iField] == 'D' ) {
        for(i=0;i<(int)strlen(psDBF->pszStringField);i++) {
            if(psDBF->pszStringField[i] != ' ')
                break;
        }
        pReturnField = psDBF->pszStringField+i;
    }
    else
        pReturnField = psDBF->pszStringField;

    return( pReturnField );
}

/************************************************************************/
/*                        msDBFReadIntAttribute()                       */
/*                                                                      */
/*      Read an integer attribute.                                      */
/************************************************************************/
int	msDBFReadIntegerAttribute( DBFHandle psDBF, int iRecord, int iField )

{
  return(atoi(msDBFReadAttribute( psDBF, iRecord, iField )));
}

/************************************************************************/
/*                        msDBFReadDoubleAttribute()                    */
/*                                                                      */
/*      Read a double attribute.                                        */
/************************************************************************/
double	msDBFReadDoubleAttribute( DBFHandle psDBF, int iRecord, int iField )
{
  return(atof(msDBFReadAttribute( psDBF, iRecord, iField )));
}

/************************************************************************/
/*                        msDBFReadStringAttribute()                      */
/*                                                                      */
/*      Read a string attribute.                                        */
/************************************************************************/
const char *msDBFReadStringAttribute( DBFHandle psDBF, int iRecord, int iField )
{
  return( msDBFReadAttribute( psDBF, iRecord, iField ) );
}

/************************************************************************/
/*                          msDBFGetFieldCount()                        */
/*                                                                      */
/*      Return the number of fields in this table.                      */
/************************************************************************/
int	msDBFGetFieldCount( DBFHandle psDBF )
{
  return( psDBF->nFields );
}

/************************************************************************/
/*                         msDBFGetRecordCount()                        */
/*                                                                      */
/*      Return the number of records in this table.                     */
/************************************************************************/
int	msDBFGetRecordCount( DBFHandle psDBF )
{
  return( psDBF->nRecords );
}

/************************************************************************/
/*                          msDBFGetFieldInfo()                         */
/*                                                                      */
/*      Return any requested information about the field.               */
/************************************************************************/
DBFFieldType msDBFGetFieldInfo( DBFHandle psDBF, int iField, char * pszFieldName, int * pnWidth, int * pnDecimals )
{
  if( iField < 0 || iField >= psDBF->nFields )
    return( FTInvalid );

  if( pnWidth != NULL )
    *pnWidth = psDBF->panFieldSize[iField];

  if( pnDecimals != NULL )
    *pnDecimals = psDBF->panFieldDecimals[iField];

  if( pszFieldName != NULL )
    {
      int	i;

      strncpy( pszFieldName, (char *) psDBF->pszHeader+iField*32, 11 );
      pszFieldName[11] = '\0';
      for( i = 10; i > 0 && pszFieldName[i] == ' '; i-- )
	pszFieldName[i] = '\0';
    }

  if( psDBF->pachFieldType[iField] == 'N'
      || psDBF->pachFieldType[iField] == 'F'
      || psDBF->pachFieldType[iField] == 'D' )
    {
      if( psDBF->panFieldDecimals[iField] > 0 )
	return( FTDouble );
      else
	return( FTInteger );
    }
  else
    {
      return( FTString );
    }
}

static int
m_strcasecmp(const char *s1, const char*s2)
{
  unsigned int i;

  for (i = 0; s1[i] != 0 && s2[i] != 0; i++) {
    unsigned char x1 = tolower(s1[i]);
    unsigned char x2 = tolower(s2[i]);
    if (x1 > x2) {
      return 1;
    } else if (x1 < x2) {
      return -1;
    }
  }
  if ((0 == s1[i]) && (0 == s2[i])) {
    return 0;
  }
  if (s1[i] != 0) {
    return 1;
  }
  return -1;
}

/*
** Which column number in the .DBF file does the item correspond to
*/
int msDBFGetItemIndex(DBFHandle dbffile, char *name)
{
  int i;
  DBFFieldType dbfField;
  int fWidth,fnDecimals; /* field width and number of decimals */
  char fName[32]; /* field name */

  if(!name) {
    msSetError(MS_MISCERR, "NULL item name passed.", "msGetItemIndex()");
    return(-1);
  }

  /* does name exist as a field? */
  for(i=0;i<msDBFGetFieldCount(dbffile);i++) {
    dbfField = msDBFGetFieldInfo(dbffile,i,fName,&fWidth,&fnDecimals);
    if(m_strcasecmp(name,fName) == 0) /* found it */
      return(i);
  }

  msSetError(MS_DBFERR, "Item '%s' not found.", "msDBFGetItemIndex()",name);
  return(-1); /* item not found */
}

/*
** Load item names into a character array
*/
char **msDBFGetItems(DBFHandle dbffile)
{
  char **items;
  int i, nFields;
  char fName[32];

  if((nFields = msDBFGetFieldCount(dbffile)) == 0) {
    msSetError(MS_DBFERR, "File contains no data.", "msGetDBFItems()");
    return(NULL);
  }

  if((items = (char **)malloc(sizeof(char *)*nFields)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msGetDBFItems()");
    return(NULL);
  }

  for(i=0;i<nFields;i++) {
    msDBFGetFieldInfo(dbffile, i, fName, NULL, NULL);
    items[i] = strdup(fName);
  }

  return(items);
}

/*
** Load item values into a character array
*/
char **msDBFGetValues(DBFHandle dbffile, int record)
{
  char **values;
  int i, nFields;

  if((nFields = msDBFGetFieldCount(dbffile)) == 0) {
    msSetError(MS_DBFERR, "File contains no data.", "msGetDBFValues()");
    return(NULL);
  }

  if((values = (char **)malloc(sizeof(char *)*nFields)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msGetAllDBFValues()");
    return(NULL);
  }

  for(i=0;i<nFields;i++)
    values[i] = strdup(msDBFReadStringAttribute(dbffile, record, i));

  return(values);
}

int *msDBFGetItemIndexes(DBFHandle dbffile, char **items, int numitems)
{
  int *itemindexes=NULL, i;

  if(numitems == 0) return(NULL);

  itemindexes = (int *)malloc(sizeof(int)*numitems);
  if(!itemindexes) {
    msSetError(MS_MEMERR, NULL, "msGetItemIndexes()");
    return(NULL);
  }

  for(i=0;i<numitems;i++) {
    itemindexes[i] = msDBFGetItemIndex(dbffile, items[i]);
    if(itemindexes[i] == -1) {
      free(itemindexes);
      return(NULL); // item not found
    }
  }

  return(itemindexes);
}

char **msDBFGetValueList(DBFHandle dbffile, int record, int *itemindexes, int numitems)
{
  const char *value;
  char **values=NULL;
  int i;

  if(numitems == 0) return(NULL);

  if((values = (char **)malloc(sizeof(char *)*numitems)) == NULL) {
    msSetError(MS_MEMERR, NULL, "msGetSomeDBFValues()");
    return(NULL);
  }

  for(i=0;i<numitems;i++) {
    value = msDBFReadStringAttribute(dbffile, record, itemindexes[i]);
    if (value == NULL)
      return NULL; /* Error already reported by msDBFReadStringAttribute() */
    values[i] = strdup(value);
  }

  return(values);
}
