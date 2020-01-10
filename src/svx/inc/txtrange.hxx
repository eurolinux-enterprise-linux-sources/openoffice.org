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

#ifndef _MyTXTRANGE_HXX
#define _MyTXTRANGE_HXX

#ifndef _TXTRANGE_HXX
#define _SVSTDARR_BOOLS
#define _SVSTDARR_LONGS
#include <svtools/svstdarr.hxx>
#endif
#include "svx/svxdllapi.h"

class PolyPolygon;
class Range;
class Rectangle;

namespace basegfx {
    class B2DPolyPolygon;
}

typedef SvLongs* SvLongsPtr;

/*************************************************************************
|*
|*    class TextRanger
|*
|*    Beschreibung
|*    Ersterstellung       20.01.97
|*    Letzte Aenderung AMA 20.01.97
|*
*************************************************************************/
class SVX_DLLPUBLIC TextRanger
{
	Range *pRangeArr;
	SvLongsPtr *pCache;
	PolyPolygon *mpPolyPolygon; // Flaechenpolygon
	PolyPolygon *mpLinePolyPolygon; // Linienpolygon
	Rectangle *pBound;	// Umfassendes Rechteck
	USHORT nCacheSize;	// Cache-Size
	USHORT nCacheIdx;	// Cache-Index
	USHORT nRight;		// Abstand Kontur-Text
	USHORT nLeft;		// Abstand Text-Kontur
	USHORT nUpper;		// Abstand Kontur-Text
	USHORT nLower;		// Abstand Text-Kontur
	sal_uInt32 nPointCount; // Anzahl der Polygonpunkte
	BOOL bSimple : 1;	// Nur Aussenkante
	BOOL bInner  : 1;	// TRUE: Objekt beschriften (EditEngine);
						// FALSE: Objekt umfliessen (StarWriter);
	BOOL bVertical :1;	// for vertical writing mode
	BOOL bFlag3 :1;
	BOOL bFlag4 :1;
	BOOL bFlag5 :1;
	BOOL bFlag6 :1;
	BOOL bFlag7 :1;
	TextRanger( const TextRanger& ); // not implemented
	const Rectangle& _GetBoundRect();
public:
	TextRanger( const basegfx::B2DPolyPolygon& rPolyPolygon, const basegfx::B2DPolyPolygon* pLinePolyPolygon,
				USHORT nCacheSize, USHORT nLeft, USHORT nRight,
				BOOL bSimple, BOOL bInner, BOOL bVert = sal_False );
	~TextRanger();
	SvLongsPtr GetTextRanges( const Range& rRange );
	USHORT GetRight() const { return nRight; }
	USHORT GetLeft() const { return nLeft; }
	USHORT GetUpper() const { return nUpper; }
	USHORT GetLower() const { return nLower; }
	sal_uInt32 GetPointCount() const { return nPointCount; }
	BOOL IsSimple() const { return bSimple; }
	BOOL IsInner() const { return bInner; }
	BOOL IsVertical() const { return bVertical; }
	BOOL HasBorder() const { return nRight || nLeft; }
	const PolyPolygon& GetPolyPolygon() const { return *mpPolyPolygon; }
	const PolyPolygon* GetLinePolygon() const { return mpLinePolyPolygon; }
	const Rectangle& GetBoundRect()
		{ return pBound ? static_cast< const Rectangle& >(*pBound) : _GetBoundRect(); }
	void SetUpper( USHORT nNew ){ nUpper = nNew; }
	void SetLower( USHORT nNew ){ nLower = nNew; }
	void SetVertical( BOOL bNew );
	BOOL IsFlag3() const { return bFlag3; }
	void SetFlag3( BOOL bNew ) { bFlag3 = bNew; }
	BOOL IsFlag4() const { return bFlag4; }
	void SetFlag4( BOOL bNew ) { bFlag4 = bNew; }
	BOOL IsFlag5() const { return bFlag5; }
	void SetFlag5( BOOL bNew ) { bFlag5 = bNew; }
	BOOL IsFlag6() const { return bFlag6; }
	void SetFlag6( BOOL bNew ) { bFlag6 = bNew; }
	BOOL IsFlag7() const { return bFlag7; }
	void SetFlag7( BOOL bNew ) { bFlag7 = bNew; }
};



#endif      // _TXTRANGE_HXX
