/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"



//------------------------------------------------------------------------

#include <tools/debug.hxx>

#include "dociter.hxx"
#include "cell.hxx"
#include "document.hxx"

#include "exp_op.hxx"

#if ENABLE_LOTUS123_EXPORT
const USHORT ExportWK1::WK1MAXCOL = 255;
const USHORT ExportWK1::WK1MAXROW = 8191;

BYTE ExportWK1::GenFormByte( const ScPatternAttr& /*aAttr*/ )
{
	return 0xFF;
}


inline void ExportWK1::Bof()
{	// (0x00)
	aOut << ( USHORT ) 0x00 << ( USHORT ) 2 << ( USHORT ) 0x0406;   // Version 1-2-3/2, Symhony/1.1
}


inline void ExportWK1::Eof()
{	// (0x01)
	aOut << ( USHORT ) 0x01 << ( USHORT ) 0;
}


inline void ExportWK1::Calcmode()
{	// (0x02)
	// Calculationmode = automatic
	aOut << ( USHORT ) 0x02 << ( USHORT ) 1 << ( BYTE ) 0xFF;
}


inline void ExportWK1::Calcorder()
{	// (0x03)
	// order = natural
	aOut << ( USHORT ) 0x03 << ( USHORT ) 1 << ( BYTE ) 0x00;
}


inline void ExportWK1::Split()
{	// (0x04)
	// not split
	aOut << ( USHORT ) 0x04 << ( USHORT ) 1 << ( BYTE ) 0x00;
}


inline void ExportWK1::Sync()
{	// (0x05)
	// not synchronized
	aOut << ( USHORT ) 0x05 << ( USHORT ) 1 << ( BYTE ) 0x00;
}


inline void ExportWK1::Dimensions()
{	// (0x06)
	SCCOL nEndCol;
    SCROW nEndRow;
	aOut << ( USHORT ) 0x06 << ( USHORT ) 8 << ( USHORT ) 0 << ( USHORT ) 0;        // Starting Col/Row
	pD->GetPrintArea( 0, nEndCol, nEndRow );
#if SC_ROWLIMIT_MORE_THAN_64K
#error row limit 64k
#endif
    USHORT nCol = static_cast<USHORT>(nEndCol);
    USHORT nRow = static_cast<USHORT>(nEndRow);
	DBG_ASSERT( nCol <= WK1MAXCOL, "ExportWK1::Dimensions(): Col > WK1MAXCOL" );
	DBG_ASSERT( nRow <= WK1MAXROW, "ExportWK1::Dimensions(): Row > WK1MAXROW" );
	aOut << nCol << nRow;   // Ending Col/Row
}


inline void ExportWK1::Window1()
{	// (0x07)
	aOut << ( USHORT ) 0x07 << ( USHORT ) 32
		<< ( USHORT ) 0 << ( USHORT ) 0 // Cursor Col/Row
		<< ( BYTE ) 0xFF                // Format: protected, special, default
		<< ( BYTE ) 0                   // Dummy
		<< ( USHORT ) 9                 // Default column width
		<< ( USHORT ) 8 << ( USHORT ) 14// Number of cols/rows on screen
		<< ( USHORT ) 0 << ( USHORT ) 0 // Left col / top row
										// Rest aus Doku-Beispiel
		<< ( USHORT ) 0 << ( USHORT ) 0 // Number of title cols / rows
		<< ( USHORT ) 0 << ( USHORT ) 0 // Left title col / top title row
		<< ( USHORT ) 0x0004 << ( USHORT ) 0x0004// Top-left col / row
		<< ( USHORT ) 0x0048            // Number of cols in window
		<< ( USHORT ) 0x00;             // Dummy
}


inline void ExportWK1::Colw()
{	// (0x08)
	// ACHTUNG: muss nach Window1 und vor hidden cols record kommen!
	USHORT	nWidth;
	BYTE	nWidthSpaces;
	for( USHORT nCol = 0 ; nCol < 256 ; nCol++ )
	{
		nWidth = pD->GetColWidth( static_cast<SCCOL>(nCol), 0 );
		nWidthSpaces = ( BYTE ) ( nWidth / TWIPS_PER_CHAR );
		aOut << ( USHORT ) 0x08 << ( USHORT ) 3 << nCol << nWidthSpaces;
	}
}


void ExportWK1::Blank( const USHORT nCol, const USHORT nRow, const ScPatternAttr& aAttr )
{	// (0x0C)
	// PREC:    nCol <= WK1MAXCOL, nRow <= WK1MAXROW
	DBG_ASSERT( nCol <= WK1MAXCOL, "ExportWK1::Blank(): Col > WK1MAXCOL" );
	DBG_ASSERT( nRow <= WK1MAXROW, "ExportWK1::Blank(): Row > WK1MAXROW" );

	aOut << ( USHORT ) 0x0C << ( USHORT ) 5 << GenFormByte( aAttr ) << nCol << nRow;
}


void ExportWK1::Number( const USHORT nCol, const USHORT nRow, const double fWert, const ScPatternAttr &aAttr )
{	// (0x0E)
	// PREC:    nCol <= WK1MAXCOL, nRow <= WK1MAXROW
	DBG_ASSERT( nCol <= WK1MAXCOL, "ExportWK1::Number(): Col > WK1MAXCOL" );
	DBG_ASSERT( nRow <= WK1MAXROW, "ExportWK1::Number(): Row > WK1MAXROW" );

	aOut << ( USHORT ) 0x0E << ( USHORT ) 13 << GenFormByte( aAttr ) << nCol << nRow << fWert;
}


void ExportWK1::Label( const USHORT nCol, const USHORT nRow, const String& rStr, const ScPatternAttr& aAttr )
{	// (0x0F)
	// PREC:    nCol <= WK1MAXCOL, nRow <= WK1MAXROW
	DBG_ASSERT( nCol <= WK1MAXCOL, "ExportWK1::Label(): Col > WK1MAXCOL" );
	DBG_ASSERT( nRow <= WK1MAXROW, "ExportWK1::Label(): Row > WK1MAXROW" );

	ByteString	aStr( rStr, eZielChar );

	USHORT		nLaenge = 7;         // Anzahl Bytes vor String+Nullbyte am Ende + Alignment-Char

	xub_StrLen	nAnz = aStr.Len();


	if( nAnz > 240 )            // max. 240 Zeichen im String
		nAnz = 240;

    nLaenge = nLaenge + ( USHORT ) nAnz;            // + Stringlaenge

	aOut << ( USHORT ) 0x0F << nLaenge << GenFormByte( aAttr ) << nCol << nRow << ( sal_Char ) '\'';
					// ACHTUNG: ZUNAECHST NUR LEFT ALIGNMENT

	aOut.Write( aStr.GetBuffer(), nAnz );

	aOut << ( BYTE ) 0x00;      // ...und Nullterminator anhaengen
}


void ExportWK1::Formula( const USHORT nCol, const USHORT nRow, const ScFormulaCell* pFC, const ScPatternAttr& aAttr )
{	// (0x10)
	// PREC:    nCol <= WK1MAXCOL, nRow <= WK1MAXROW
	DBG_ASSERT( nCol <= WK1MAXCOL, "ExportWK1::Formula(): Col > WK1MAXCOL" );
	DBG_ASSERT( nRow <= WK1MAXROW, "ExportWK1::Formula(): Row > WK1MAXROW" );

	USHORT	nLaenge = 15;    // Bytes bis Formel
	double	fErgebnis;

	// zunaechst nur Dummy-Angaben (Formel := Ergebnis der Berechnung )
	nLaenge += 9+1;

	fErgebnis = ( ( ScFormulaCell* ) pFC )->GetValue();

	aOut << ( USHORT ) 0x10 << ( USHORT ) nLaenge
		<< GenFormByte( aAttr ) << nCol << nRow
		<< fErgebnis
		<< ( USHORT ) 9+1               // Dummy-Formula-Size
		<< ( BYTE ) 0x00                // constant, floating point
		<< fErgebnis
		<< ( BYTE ) 0x03;               // return
}


inline void ExportWK1::Protect()
{	// (0x24)
	//Global protection off
	aOut << ( USHORT ) 0x24 << ( USHORT ) 1 << ( BYTE ) 0x00;
}


inline void ExportWK1::Footer()
{	// (0x25)
	// zunaechst nur leerer C-String
	aOut << ( USHORT ) 0x25 << ( USHORT ) 242 << ( sal_Char ) '\'';   // linksbuendiger leerer String
	for( short nLauf = 0 ; nLauf < 241 ; nLauf++ )
		aOut << ( BYTE ) 0x00;
}


inline void ExportWK1::Header()
{	// (0x26)
	// zunaechst nur leerer C-String
	aOut << ( USHORT ) 0x26 << ( USHORT ) 242 << ( sal_Char ) '\'';   // linksbuendiger leerer String
	for( short nLauf = 0 ; nLauf < 241 ; nLauf++ )
		aOut << ( BYTE ) 0x00;
}


inline void ExportWK1::Margins()
{	// (0x28)
	aOut << ( USHORT ) 0x28 << ( USHORT ) 10
		<< ( USHORT ) 4 << ( USHORT ) 76    // Left/right margin
		<< ( USHORT ) 66                    // Page length
		<< ( USHORT ) 2 << ( USHORT ) 2;    // Top/Bottom margin
}


inline void ExportWK1::Labelfmt()
{	// (0x29)
	// Global label alignment = left
	aOut << ( USHORT ) 0x29 << ( USHORT ) 1 << ( BYTE ) 0x27;
}


inline void ExportWK1::Calccount()
{	// (0x2F)
	// Iteration count = 16 (oder so aehnlich)
	aOut << ( USHORT ) 0x2F << ( USHORT ) 1 << ( BYTE ) 16;
}


inline void ExportWK1::Cursorw12()
{	// (0x31)
	// Cursor location in window 1
	aOut << ( USHORT ) 0x31 << ( USHORT ) 1 << ( BYTE ) 1;
}


void ExportWK1::WKString( const USHORT /*nCol*/, const USHORT /*nRow*/, const ScFormulaCell* /*pFC*/, const ScPatternAttr& /*aAttr*/ )
{	// (0x33)
	// PREC:    nCol <= WK1MAXCOL, nRow <= WK1MAXROW
/*  DBG_ASSERT( nCol <= WK1MAXCOL, "ExportWK1::Label(): Col > WK1MAXCOL" );
	DBG_ASSERT( nRow <= WK1MAXROW, "ExportWK1::Label(): Row > WK1MAXROW" );

	String aStr;
	( ( ScFormulaCell * ) pFC )->GetString( aStr );     // Formeltext zunaechst so belassen

	USHORT nLaenge = 6;         // Anzahl Bytes vor String+Nullbyte am Ende

	USHORT nAnz = aStr.Len();

	if( nAnz > 240 )            // max. 240 Zeichen im String
		nAnz = 240;

	nLaenge += nAnz;            // + Stringlaenge

	aOut << ( USHORT ) 0x33 << nLaenge
		<< GenFormByte( aAttr ) << nCol << nRow;

	// Zeichenweise String ausgeben
	for( USHORT nLauf = 0 ; nLauf < nAnz ; nLauf++ )
		aOut << aStr[ nLauf ];

	aOut << ( BYTE ) 0x00;      // ...und Nullterminator anhaengen
*/
	}


inline void ExportWK1::Snrange()
{	// (0x47)
	//aOut << ( USHORT ) 0x47 << ( USHORT ) x
/*  ScRangeName *pRanges = pD->GetRangeName();
	ScRangeData *pData;
	String aName;

	USHORT nAnz = pRanges->GetCount();

	for( USHORT nLauf = 0 ; nLauf < nAnz ; nLauf++ )
		{
		pData = pRanges[ nLauf ];

		}
*/
}


inline void ExportWK1::Hidcol()
{	// (0x64)
	sal_uInt32 nHide = 0x00000000;   // ...niemand ist versteckt

	aOut << ( USHORT ) 0x64 << ( USHORT ) 32;

	for( int nLauf = 0 ; nLauf < 8 ; nLauf++ )
		aOut << nHide;          // 8 * 32 Bits = 256
}


inline void ExportWK1::Cpi()
{	// (0x96)
	//aOut << ( USHORT ) 0x96 << ( USHORT ) x;
}


FltError ExportWK1::Write()
{
	Bof();
	//Dimensions();
	//Cpi();
	//Calccount();
	//Calcmode();
	//Calcorder();
	//Split();
	//Sync();
	//Window1();
	Colw();
	//Hidcol();
	//Cursorw12();
	//Snrange();
	//Protect();
	//Footer();
	//Header();
	//Margins();
	//Labelfmt();

	// Zellen-Bemachung
	ScDocumentIterator		aIter( pD, 0, 0 );
	ScBaseCell*				pCell;
	USHORT					nCol, nRow;
	SCTAB					nTab;
	const ScPatternAttr*	pPatAttr;

	if( aIter.GetFirst() )
		do
		{	// ueber alle Zellen der ersten Tabelle iterieren
			pPatAttr = aIter.GetPattern();
			pCell = aIter.GetCell();
            SCCOL nScCol;
            SCROW nScRow;
			aIter.GetPos( nScCol, nScRow, nTab );
#if SC_ROWLIMIT_MORE_THAN_64K
#error row limit 64k
#endif
            nCol = static_cast<USHORT>(nScCol);
            nRow = static_cast<USHORT>(nScRow);

			switch( pCell->GetCellType() )
			{
				case CELLTYPE_VALUE:
				{
					double	fVal;
					fVal = ( ( ScValueCell * ) pCell)->GetValue();
					Number( nCol, nRow, fVal, *pPatAttr );
				}
					break;
				case CELLTYPE_STRING:
				{
					String	aStr;
					( ( ScStringCell * ) pCell)->GetString( aStr );
					Label( nCol, nRow, aStr, *pPatAttr );
				}
					break;
				case CELLTYPE_FORMULA:
				{
					Formula( nCol, nRow, ( ScFormulaCell * ) pCell, *pPatAttr );
					WKString( nCol, nRow, ( ScFormulaCell * ) pCell, *pPatAttr );
				}
					break;
				case CELLTYPE_NOTE:
				case CELLTYPE_NONE:
					break;
				default:
					DBG_ASSERT( FALSE, "ExportWK1::Write(): unbekannter Celltype!" );
			}
		}
		while( aIter.GetNext() );

	Eof();

	return eERR_OK;
}
#endif


