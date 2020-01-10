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
#include "precompiled_tools.hxx"

#define _SV_POLY_CXX
#include <osl/endian.h>
#include <tools/bigint.hxx>
#include <tools/debug.hxx>
#include <tools/stream.hxx>
#include <tools/vcompat.hxx>
#include <poly.h>
#include <tools/line.hxx>
#ifndef _VECTOR2D_H
#include <tools/vector2d.hxx>
#endif
#ifndef _POLY_HXX
#include <tools/poly.hxx>
#endif
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/vector/b2dvector.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/curve/b2dcubicbezier.hxx>

#include <vector>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <limits.h>
#include <cmath>


// =======================================================================

DBG_NAME( Polygon )

// -----------------------------------------------------------------------

#define EDGE_LEFT		1
#define EDGE_TOP		2
#define EDGE_RIGHT		4
#define EDGE_BOTTOM 	8
#define EDGE_HORZ		(EDGE_RIGHT | EDGE_LEFT)
#define EDGE_VERT		(EDGE_TOP | EDGE_BOTTOM)
#define	SMALL_DVALUE	0.0000001
#define FSQRT2			1.4142135623730950488016887242097

// -----------------------------------------------------------------------

static ImplPolygonData aStaticImplPolygon =
{
	NULL, NULL, 0, 0
};

// =======================================================================

ImplPolygon::ImplPolygon( USHORT nInitSize, BOOL bFlags  )
{
	if ( nInitSize )
	{
		mpPointAry = (Point*)new char[(ULONG)nInitSize*sizeof(Point)];
		memset( mpPointAry, 0, (ULONG)nInitSize*sizeof(Point) );
	}
	else
		mpPointAry = NULL;

	if( bFlags )
	{
		mpFlagAry = new BYTE[ nInitSize ];
		memset( mpPointAry, 0, nInitSize );
	}
	else
		mpFlagAry = NULL;

	mnRefCount = 1;
	mnPoints = nInitSize;
}

// -----------------------------------------------------------------------

ImplPolygon::ImplPolygon( const ImplPolygon& rImpPoly )
{
	if ( rImpPoly.mnPoints )
	{
		mpPointAry = (Point*)new char[(ULONG)rImpPoly.mnPoints*sizeof(Point)];
		memcpy( mpPointAry, rImpPoly.mpPointAry, (ULONG)rImpPoly.mnPoints*sizeof(Point) );

		if( rImpPoly.mpFlagAry )
		{
			mpFlagAry = new BYTE[ rImpPoly.mnPoints ];
			memcpy( mpFlagAry, rImpPoly.mpFlagAry, rImpPoly.mnPoints );
		}
		else
			mpFlagAry = NULL;
	}
	else
	{
		mpPointAry = NULL;
		mpFlagAry = NULL;
	}

	mnRefCount = 1;
	mnPoints   = rImpPoly.mnPoints;
}

// -----------------------------------------------------------------------

ImplPolygon::ImplPolygon( USHORT nInitSize, const Point* pInitAry, const BYTE* pInitFlags )
{
	if ( nInitSize )
	{
		mpPointAry = (Point*)new char[(ULONG)nInitSize*sizeof(Point)];
		memcpy( mpPointAry, pInitAry, (ULONG)nInitSize*sizeof( Point ) );

		if( pInitFlags )
		{
			mpFlagAry = new BYTE[ nInitSize ];
			memcpy( mpFlagAry, pInitFlags, nInitSize );
		}
		else
			mpFlagAry = NULL;
	}
	else
	{
		mpPointAry = NULL;
		mpFlagAry  = NULL;
	}

	mnRefCount = 1;
	mnPoints   = nInitSize;
}

// -----------------------------------------------------------------------

ImplPolygon::~ImplPolygon()
{
	if ( mpPointAry )
	{
		delete[] (char*) mpPointAry;
	}

	if( mpFlagAry )
		delete[] mpFlagAry;
}

// -----------------------------------------------------------------------

void ImplPolygon::ImplSetSize( USHORT nNewSize, BOOL bResize )
{
	if( mnPoints == nNewSize )
		return;

	Point* pNewAry;

	if ( nNewSize )
	{
		pNewAry = (Point*)new char[(ULONG)nNewSize*sizeof(Point)];

		if ( bResize )
		{
			// Alte Punkte kopieren
			if ( mnPoints < nNewSize )
			{
				// Neue Punkte mit 0 initialisieren
				memset( pNewAry+mnPoints, 0, (ULONG)(nNewSize-mnPoints)*sizeof(Point) );
				if ( mpPointAry )
					memcpy( pNewAry, mpPointAry, mnPoints*sizeof(Point) );
			}
			else
			{
				if ( mpPointAry )
					memcpy( pNewAry, mpPointAry, (ULONG)nNewSize*sizeof(Point) );
			}
		}
	}
	else
		pNewAry = NULL;

	if ( mpPointAry )
		delete[] (char*) mpPointAry;

	// ggf. FlagArray beruecksichtigen
	if( mpFlagAry )
	{
		BYTE* pNewFlagAry;

		if( nNewSize )
		{
			pNewFlagAry = new BYTE[ nNewSize ];

			if( bResize )
			{
				// Alte Flags kopieren
				if ( mnPoints < nNewSize )
				{
					// Neue Punkte mit 0 initialisieren
					memset( pNewFlagAry+mnPoints, 0, nNewSize-mnPoints );
					memcpy( pNewFlagAry, mpFlagAry, mnPoints );
				}
				else
					memcpy( pNewFlagAry, mpFlagAry, nNewSize );
			}
		}
		else
			pNewFlagAry = NULL;

		delete[] mpFlagAry;
		mpFlagAry  = pNewFlagAry;
	}

	mpPointAry = pNewAry;
	mnPoints   = nNewSize;
}

// -----------------------------------------------------------------------

void ImplPolygon::ImplSplit( USHORT nPos, USHORT nSpace, ImplPolygon* pInitPoly )
{
	const ULONG 	nSpaceSize = nSpace * sizeof( Point );
	const USHORT	nNewSize = mnPoints + nSpace;

	if( nPos >= mnPoints )
	{
		// Hinten anhaengen
		nPos = mnPoints;
		ImplSetSize( nNewSize, TRUE );

		if( pInitPoly )
		{
			memcpy( mpPointAry + nPos, pInitPoly->mpPointAry, nSpaceSize );

			if( pInitPoly->mpFlagAry )
				memcpy( mpFlagAry + nPos, pInitPoly->mpFlagAry, nSpace );
		}
	}
	else
	{
		// PointArray ist in diesem Zweig immer vorhanden
		const USHORT	nSecPos = nPos + nSpace;
		const USHORT	nRest = mnPoints - nPos;

		Point* pNewAry = (Point*) new char[ (ULONG) nNewSize * sizeof( Point ) ];

		memcpy( pNewAry, mpPointAry, nPos * sizeof( Point ) );

		if( pInitPoly )
			memcpy( pNewAry + nPos, pInitPoly->mpPointAry, nSpaceSize );
		else
			memset( pNewAry + nPos, 0, nSpaceSize );

		memcpy( pNewAry + nSecPos, mpPointAry + nPos, nRest * sizeof( Point ) );
		delete[] (char*) mpPointAry;

		// ggf. FlagArray beruecksichtigen
		if( mpFlagAry )
		{
			BYTE* pNewFlagAry = new BYTE[ nNewSize ];

			memcpy( pNewFlagAry, mpFlagAry, nPos );

			if( pInitPoly && pInitPoly->mpFlagAry )
				memcpy( pNewFlagAry + nPos, pInitPoly->mpFlagAry, nSpace );
			else
				memset( pNewFlagAry + nPos, 0, nSpace );

			memcpy( pNewFlagAry + nSecPos, mpFlagAry + nPos, nRest );
			delete[] mpFlagAry;
			mpFlagAry = pNewFlagAry;
		}

		mpPointAry = pNewAry;
		mnPoints   = nNewSize;
	}
}

// -----------------------------------------------------------------------

void ImplPolygon::ImplRemove( USHORT nPos, USHORT nCount )
{
	const USHORT nRemoveCount = Min( (USHORT) ( mnPoints - nPos ), (USHORT) nCount );

	if( nRemoveCount )
	{
		const USHORT	nNewSize = mnPoints - nRemoveCount;
		const USHORT	nSecPos = nPos + nRemoveCount;
		const USHORT	nRest = mnPoints - nSecPos;

		Point* pNewAry = (Point*) new char[ (ULONG) nNewSize * sizeof( Point ) ];

		memcpy( pNewAry, mpPointAry, nPos * sizeof( Point ) );
		memcpy( pNewAry + nPos, mpPointAry + nSecPos, nRest * sizeof( Point ) );

		delete[] (char*) mpPointAry;

		// ggf. FlagArray beruecksichtigen
		if( mpFlagAry )
		{
			BYTE* pNewFlagAry = new BYTE[ nNewSize ];

			memcpy( pNewFlagAry, mpFlagAry, nPos );
			memcpy( pNewFlagAry + nPos, mpFlagAry + nSecPos, nRest );
			delete[] mpFlagAry;
			mpFlagAry = pNewFlagAry;
		}

		mpPointAry = pNewAry;
		mnPoints   = nNewSize;
	}
}

// -----------------------------------------------------------------------

void ImplPolygon::ImplCreateFlagArray()
{
	if( !mpFlagAry )
	{
		mpFlagAry = new BYTE[ mnPoints ];
		memset( mpFlagAry, 0, mnPoints );
	}
}

// =======================================================================

inline void Polygon::ImplMakeUnique()
{
	// Falls noch andere Referenzen bestehen, dann kopieren
	if ( mpImplPolygon->mnRefCount != 1 )
	{
		if ( mpImplPolygon->mnRefCount )
			mpImplPolygon->mnRefCount--;
		mpImplPolygon = new ImplPolygon( *mpImplPolygon );
	}
}

// -----------------------------------------------------------------------

inline double ImplGetAngle( const Point& rCenter, const Point& rPt )
{
	const long nDX = rPt.X() - rCenter.X();
	return( atan2( -rPt.Y() + rCenter.Y(), ( ( nDX == 0L ) ? 0.000000001 : nDX ) ) );
}

// -----------------------------------------------------------------------

Polygon::Polygon()
{
	DBG_CTOR( Polygon, NULL );
	mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
}

// -----------------------------------------------------------------------

Polygon::Polygon( USHORT nSize )
{
	DBG_CTOR( Polygon, NULL );

	if ( nSize )
		mpImplPolygon = new ImplPolygon( nSize );
	else
		mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
}

// -----------------------------------------------------------------------

Polygon::Polygon( USHORT nPoints, const Point* pPtAry, const BYTE* pFlagAry )
{
	DBG_CTOR( Polygon, NULL );

	if( nPoints )
		mpImplPolygon = new ImplPolygon( nPoints, pPtAry, pFlagAry );
	else
		mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
}

// -----------------------------------------------------------------------

Polygon::Polygon( const Polygon& rPoly )
{
	DBG_CTOR( Polygon, NULL );
	DBG_CHKOBJ( &rPoly, Polygon, NULL );
	DBG_ASSERT( rPoly.mpImplPolygon->mnRefCount < 0xFFFFFFFE, "Polygon: RefCount overflow" );

	mpImplPolygon = rPoly.mpImplPolygon;
	if ( mpImplPolygon->mnRefCount )
		mpImplPolygon->mnRefCount++;
}

// -----------------------------------------------------------------------

Polygon::Polygon( const Rectangle& rRect )
{
	DBG_CTOR( Polygon, NULL );

	if ( rRect.IsEmpty() )
		mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
	else
	{
		mpImplPolygon = new ImplPolygon( 5 );
		mpImplPolygon->mpPointAry[0] = rRect.TopLeft();
		mpImplPolygon->mpPointAry[1] = rRect.TopRight();
		mpImplPolygon->mpPointAry[2] = rRect.BottomRight();
		mpImplPolygon->mpPointAry[3] = rRect.BottomLeft();
		mpImplPolygon->mpPointAry[4] = rRect.TopLeft();
	}
}

// -----------------------------------------------------------------------

Polygon::Polygon( const Rectangle& rRect, ULONG nHorzRound, ULONG nVertRound )
{
	DBG_CTOR( Polygon, NULL );

	if ( rRect.IsEmpty() )
		mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
	else
	{
		Rectangle aRect( rRect );
		aRect.Justify();			// SJ: i9140

		nHorzRound = Min( nHorzRound, (ULONG) labs( aRect.GetWidth() >> 1 ) );
		nVertRound = Min( nVertRound, (ULONG) labs( aRect.GetHeight() >> 1 ) );

		if( !nHorzRound && !nVertRound )
		{
			mpImplPolygon = new ImplPolygon( 5 );
			mpImplPolygon->mpPointAry[0] = aRect.TopLeft();
			mpImplPolygon->mpPointAry[1] = aRect.TopRight();
			mpImplPolygon->mpPointAry[2] = aRect.BottomRight();
			mpImplPolygon->mpPointAry[3] = aRect.BottomLeft();
			mpImplPolygon->mpPointAry[4] = aRect.TopLeft();
		}
		else
		{
			const Point		aTL( aRect.Left() + nHorzRound, aRect.Top() + nVertRound );
			const Point		aTR( aRect.Right() - nHorzRound, aRect.Top() + nVertRound );
			const Point		aBR( aRect.Right() - nHorzRound, aRect.Bottom() - nVertRound );
			const Point		aBL( aRect.Left() + nHorzRound, aRect.Bottom() - nVertRound );
			Polygon*		pEllipsePoly = new Polygon( Point(), nHorzRound, nVertRound );
			USHORT			i, nEnd, nSize4 = pEllipsePoly->GetSize() >> 2;

			mpImplPolygon = new ImplPolygon( pEllipsePoly->GetSize() + 1 );
			
			const Point*	pSrcAry = pEllipsePoly->GetConstPointAry();
			Point*			pDstAry = mpImplPolygon->mpPointAry;

			for( i = 0, nEnd = nSize4; i < nEnd; i++ )
				( pDstAry[ i ] = pSrcAry[ i ] ) += aTR;

			for( nEnd = nEnd + nSize4; i < nEnd; i++ )
				( pDstAry[ i ] = pSrcAry[ i ] ) += aTL;

			for( nEnd = nEnd + nSize4; i < nEnd; i++ )
				( pDstAry[ i ] = pSrcAry[ i ] ) += aBL;

			for( nEnd = nEnd + nSize4; i < nEnd; i++ )
				( pDstAry[ i ] = pSrcAry[ i ] ) += aBR;

			pDstAry[ nEnd ] = pDstAry[ 0 ];
			delete pEllipsePoly;
		}
	}
}

// -----------------------------------------------------------------------

Polygon::Polygon( const Point& rCenter, long nRadX, long nRadY, USHORT nPoints )
{
	DBG_CTOR( Polygon, NULL );

	if( nRadX && nRadY )
	{
		// Default berechnen (abhaengig von Groesse)
		if( !nPoints )
		{
			nPoints = (USHORT) ( F_PI * ( 1.5 * ( nRadX + nRadY ) -
								 sqrt( (double) labs( nRadX * nRadY ) ) ) );

			nPoints = (USHORT) MinMax( nPoints, 32, 256 );

			if( ( nRadX > 32 ) && ( nRadY > 32 ) && ( nRadX + nRadY ) < 8192 )
				nPoints >>= 1;
		}

		// Anzahl der Punkte auf durch 4 teilbare Zahl aufrunden
		mpImplPolygon = new ImplPolygon( nPoints = (nPoints + 3) & ~3 );

		Point* pPt;
		USHORT i;
		USHORT nPoints2 = nPoints >> 1;
		USHORT nPoints4 = nPoints >> 2;
		double nAngle;
		double nAngleStep = F_PI2 / ( nPoints4 - 1 );

		for( i=0, nAngle = 0.0; i < nPoints4; i++, nAngle += nAngleStep )
		{
			long nX = FRound( nRadX * cos( nAngle ) );
			long nY = FRound( -nRadY * sin( nAngle ) );

			pPt = &(mpImplPolygon->mpPointAry[i]);
			pPt->X() =	nX + rCenter.X();
			pPt->Y() =	nY + rCenter.Y();
			pPt = &(mpImplPolygon->mpPointAry[nPoints2-i-1]);
			pPt->X() = -nX + rCenter.X();
			pPt->Y() =	nY + rCenter.Y();
			pPt = &(mpImplPolygon->mpPointAry[i+nPoints2]);
			pPt->X() = -nX + rCenter.X();
			pPt->Y() = -nY + rCenter.Y();
			pPt = &(mpImplPolygon->mpPointAry[nPoints-i-1]);
			pPt->X() =	nX + rCenter.X();
			pPt->Y() = -nY + rCenter.Y();
		}
	}
	else
		mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
}

// -----------------------------------------------------------------------

Polygon::Polygon( const Rectangle& rBound,
				  const Point& rStart, const Point& rEnd, PolyStyle eStyle )
{
	DBG_CTOR( Polygon, NULL );

	const long	nWidth = rBound.GetWidth();
	const long	nHeight = rBound.GetHeight();

	if( ( nWidth > 1 ) && ( nHeight > 1 ) )
	{
		const Point aCenter( rBound.Center() );
		const long	nRadX = aCenter.X() - rBound.Left();
		const long	nRadY = aCenter.Y() - rBound.Top();
		USHORT		nPoints;

		nPoints = (USHORT) ( F_PI * ( 1.5 * ( nRadX + nRadY ) -
							 sqrt( (double) labs( nRadX * nRadY ) ) ) );

		nPoints = (USHORT) MinMax( nPoints, 32, 256 );

		if( ( nRadX > 32 ) && ( nRadY > 32 ) && ( nRadX + nRadY ) < 8192 )
			nPoints >>= 1;

		// Winkel berechnen
		const double	fRadX = nRadX;
		const double	fRadY = nRadY;
		const double	fCenterX = aCenter.X();
		const double	fCenterY = aCenter.Y();
		double			fStart = ImplGetAngle( aCenter, rStart );
		double			fEnd = ImplGetAngle( aCenter, rEnd );
		double			fDiff = fEnd - fStart;
		double			fStep;
		USHORT			nStart;
		USHORT			nEnd;

		if( fDiff < 0. )
			fDiff += F_2PI;

		// Punktanzahl proportional verkleinern ( fDiff / (2PI) );
		// ist eingentlich nur fuer einen Kreis richtig; wir
		// machen es hier aber trotzdem
		nPoints = Max( (USHORT) ( ( fDiff * 0.1591549 ) * nPoints ), (USHORT) 16 );
		fStep = fDiff / ( nPoints - 1 );

		if( POLY_PIE == eStyle )
		{
			const Point aCenter2( FRound( fCenterX ), FRound( fCenterY ) );

			nStart = 1;
			nEnd = nPoints + 1;
			mpImplPolygon = new ImplPolygon( nPoints + 2 );
			mpImplPolygon->mpPointAry[ 0 ] = aCenter2;
			mpImplPolygon->mpPointAry[ nEnd ] = aCenter2;
		}
		else
		{
			mpImplPolygon = new ImplPolygon( ( POLY_CHORD == eStyle ) ? ( nPoints + 1 ) : nPoints );
			nStart = 0;
			nEnd = nPoints;
		}

		for(; nStart < nEnd; nStart++, fStart += fStep )
		{
			Point& rPt = mpImplPolygon->mpPointAry[ nStart ];

			rPt.X() = FRound( fCenterX + fRadX * cos( fStart ) );
			rPt.Y() = FRound( fCenterY - fRadY * sin( fStart ) );
		}

		if( POLY_CHORD == eStyle )
			mpImplPolygon->mpPointAry[ nPoints ] = mpImplPolygon->mpPointAry[ 0 ];
	}
	else
		mpImplPolygon = (ImplPolygon*) &aStaticImplPolygon;
}

// -----------------------------------------------------------------------

Polygon::Polygon( const Point& rBezPt1, const Point& rCtrlPt1, 
				  const Point& rBezPt2, const Point& rCtrlPt2,
				  USHORT nPoints )
{
	DBG_CTOR( Polygon, NULL );
	
	nPoints = ( 0 == nPoints ) ? 25 : ( ( nPoints < 2 ) ? 2 : nPoints );

	const double	fInc = 1.0 / ( nPoints - 1 );
	double			fK_1 = 0.0, fK1_1 = 1.0;
	double			fK_2, fK_3, fK1_2, fK1_3, fK12, fK21;
	const double	fX0 = rBezPt1.X();
	const double	fY0 = rBezPt1.Y();
	const double	fX1 = 3.0 * rCtrlPt1.X();
	const double	fY1 = 3.0 * rCtrlPt1.Y();
	const double	fX2 = 3.0 * rCtrlPt2.X();;
	const double	fY2 = 3.0 * rCtrlPt2.Y();;
	const double	fX3 = rBezPt2.X();
	const double	fY3 = rBezPt2.Y();

	mpImplPolygon = new ImplPolygon( nPoints );

	for( USHORT i = 0; i < nPoints; i++, fK_1 += fInc, fK1_1 -= fInc )
	{
		Point& rPt = mpImplPolygon->mpPointAry[ i ];

		fK_2 = fK_1, fK_3 = ( fK_2 *= fK_1 ), fK_3 *= fK_1;
		fK1_2 = fK1_1, fK1_3 = ( fK1_2 *= fK1_1 ), fK1_3 *= fK1_1;
		fK12 = fK_1 * fK1_2, fK21 = fK_2 * fK1_1;

		rPt.X() = FRound( fK1_3 * fX0 + fK12 * fX1 + fK21 * fX2 + fK_3 * fX3 );
		rPt.Y() = FRound( fK1_3 * fY0 + fK12 * fY1 + fK21 * fY2 + fK_3 * fY3 );
	}
}

// -----------------------------------------------------------------------

Polygon::~Polygon()
{
	DBG_DTOR( Polygon, NULL );

	// Wenn es keine statischen ImpDaten sind, dann loeschen, wenn es
	// die letzte Referenz ist, sonst Referenzcounter decrementieren
	if ( mpImplPolygon->mnRefCount )
	{
		if ( mpImplPolygon->mnRefCount > 1 )
			mpImplPolygon->mnRefCount--;
		else
			delete mpImplPolygon;
	}
}

// -----------------------------------------------------------------------

Point* Polygon::ImplGetPointAry()
{
	DBG_CHKTHIS( Polygon, NULL );

	ImplMakeUnique();
	return (Point*)mpImplPolygon->mpPointAry;
}

// -----------------------------------------------------------------------

BYTE* Polygon::ImplGetFlagAry()
{
	DBG_CHKTHIS( Polygon, NULL );

	ImplMakeUnique();
	mpImplPolygon->ImplCreateFlagArray();
	return mpImplPolygon->mpFlagAry;
}

// -----------------------------------------------------------------------

const Point* Polygon::GetConstPointAry() const
{
	DBG_CHKTHIS( Polygon, NULL );
	return (Point*)mpImplPolygon->mpPointAry;
}

// -----------------------------------------------------------------------

const BYTE* Polygon::GetConstFlagAry() const
{
	DBG_CHKTHIS( Polygon, NULL );
	return mpImplPolygon->mpFlagAry;
}

// -----------------------------------------------------------------------

void Polygon::SetPoint( const Point& rPt, USHORT nPos )
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints,
				"Polygon::SetPoint(): nPos >= nPoints" );

	ImplMakeUnique();
	mpImplPolygon->mpPointAry[nPos] = rPt;
}

// -----------------------------------------------------------------------

void Polygon::SetFlags( USHORT nPos, PolyFlags eFlags )
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints,
				"Polygon::SetFlags(): nPos >= nPoints" );

	// we do only want to create the flag array if there
	// is at least one flag different to POLY_NORMAL
	if ( mpImplPolygon || ( eFlags != POLY_NORMAL ) )
	{
		ImplMakeUnique();
		mpImplPolygon->ImplCreateFlagArray();
		mpImplPolygon->mpFlagAry[ nPos ] = (BYTE) eFlags;
	}
}

// -----------------------------------------------------------------------

const Point& Polygon::GetPoint( USHORT nPos ) const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints,
				"Polygon::GetPoint(): nPos >= nPoints" );

	return mpImplPolygon->mpPointAry[nPos];
}

// -----------------------------------------------------------------------

PolyFlags Polygon::GetFlags( USHORT nPos ) const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints,
				"Polygon::GetFlags(): nPos >= nPoints" );
	return( mpImplPolygon->mpFlagAry ?
			(PolyFlags) mpImplPolygon->mpFlagAry[ nPos ] :
			POLY_NORMAL );
}

// -----------------------------------------------------------------------

sal_Bool Polygon::HasFlags() const
{
	return mpImplPolygon->mpFlagAry != NULL;
}

// -----------------------------------------------------------------------

BOOL Polygon::IsControl(USHORT nPos) const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints,
				"Polygon::GetFlags(): nPos >= nPoints" );
	PolyFlags eFlags = mpImplPolygon->mpFlagAry ?
					   (PolyFlags) mpImplPolygon->mpFlagAry[ nPos ] : POLY_NORMAL;

	return( POLY_CONTROL == eFlags );
}

// -----------------------------------------------------------------------

BOOL Polygon::IsSmooth(USHORT nPos) const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints,
				"Polygon::GetFlags(): nPos >= nPoints" );
	PolyFlags eFlags = mpImplPolygon->mpFlagAry ?
					   (PolyFlags) mpImplPolygon->mpFlagAry[ nPos ] : POLY_NORMAL;

	return( ( POLY_SMOOTH == eFlags ) || ( POLY_SYMMTR == eFlags ) );
}

// -----------------------------------------------------------------------

BOOL Polygon::IsRect() const
{
	BOOL bIsRect = FALSE;
	if ( mpImplPolygon->mpFlagAry == NULL )
	{
		if ( ( ( mpImplPolygon->mnPoints == 5 ) && ( mpImplPolygon->mpPointAry[ 0 ] == mpImplPolygon->mpPointAry[ 4 ] ) ) ||
				( mpImplPolygon->mnPoints == 4 ) )
		{
			if ( ( mpImplPolygon->mpPointAry[ 0 ].X() == mpImplPolygon->mpPointAry[ 3 ].X() ) &&
					( mpImplPolygon->mpPointAry[ 0 ].Y() == mpImplPolygon->mpPointAry[ 1 ].Y() ) &&
						( mpImplPolygon->mpPointAry[ 1 ].X() == mpImplPolygon->mpPointAry[ 2 ].X() ) &&
							( mpImplPolygon->mpPointAry[ 2 ].Y() == mpImplPolygon->mpPointAry[ 3 ].Y() ) )
				bIsRect = TRUE;
		}
	}
	return bIsRect;
}

// -----------------------------------------------------------------------

void Polygon::SetSize( USHORT nNewSize )
{
	DBG_CHKTHIS( Polygon, NULL );
	
	if( nNewSize != mpImplPolygon->mnPoints )
	{
		ImplMakeUnique();
		mpImplPolygon->ImplSetSize( nNewSize );
	}
}

// -----------------------------------------------------------------------

USHORT Polygon::GetSize() const
{
	DBG_CHKTHIS( Polygon, NULL );

	return mpImplPolygon->mnPoints;
}

// -----------------------------------------------------------------------

void Polygon::Clear()
{
	DBG_CHKTHIS( Polygon, NULL );

	if ( mpImplPolygon->mnRefCount )
	{
		if ( mpImplPolygon->mnRefCount > 1 )
			mpImplPolygon->mnRefCount--;
		else
			delete mpImplPolygon;
	}

	mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
}

// -----------------------------------------------------------------------

double Polygon::CalcDistance( USHORT nP1, USHORT nP2 )
{
	DBG_ASSERT( nP1 < mpImplPolygon->mnPoints,
				"Polygon::CalcDistance(): nPos1 >= nPoints" );
	DBG_ASSERT( nP2 < mpImplPolygon->mnPoints,
				"Polygon::CalcDistance(): nPos2 >= nPoints" );

	const Point& rP1 = mpImplPolygon->mpPointAry[ nP1 ];
	const Point& rP2 = mpImplPolygon->mpPointAry[ nP2 ];
	const double fDx = rP2.X() - rP1.X();
	const double fDy = rP2.Y() - rP1.Y();

	return sqrt( fDx * fDx + fDy * fDy );
}

// -----------------------------------------------------------------------

void Polygon::Optimize( ULONG nOptimizeFlags, const PolyOptimizeData* pData )
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( !mpImplPolygon->mpFlagAry, "Optimizing could fail with beziers!" );

	USHORT nSize = mpImplPolygon->mnPoints;

	if( nOptimizeFlags && nSize )
	{
		if( nOptimizeFlags & POLY_OPTIMIZE_EDGES )
		{
			const Rectangle	aBound( GetBoundRect() );
			const double	fArea = ( aBound.GetWidth() + aBound.GetHeight() ) * 0.5;
			const USHORT	nPercent = pData ? pData->GetPercentValue() : 50;

			Optimize( POLY_OPTIMIZE_NO_SAME );
			ImplReduceEdges( *this, fArea, nPercent );
		}
		else if( nOptimizeFlags & ( POLY_OPTIMIZE_REDUCE | POLY_OPTIMIZE_NO_SAME ) )
		{
			Polygon			aNewPoly;
			const Point&	rFirst = mpImplPolygon->mpPointAry[ 0 ];
			ULONG			nReduce;

			if( nOptimizeFlags & ( POLY_OPTIMIZE_REDUCE ) )
				nReduce = pData ? pData->GetAbsValue() : 4UL;
			else
				nReduce = 0UL;

			while( nSize && ( mpImplPolygon->mpPointAry[ nSize - 1 ] == rFirst ) )
				nSize--;

			if( nSize > 1 )
			{
				USHORT nLast = 0, nNewCount = 1;

				aNewPoly.SetSize( nSize );
				aNewPoly[ 0 ] = rFirst;

				for( USHORT i = 1; i < nSize; i++ )
				{
					if( ( mpImplPolygon->mpPointAry[ i ] != mpImplPolygon->mpPointAry[ nLast ] ) &&
						( !nReduce || ( nReduce < (ULONG) FRound( CalcDistance( nLast, i ) ) ) ) )
					{
						aNewPoly[ nNewCount++ ] = mpImplPolygon->mpPointAry[ nLast = i ];
					}
				}

				if( nNewCount == 1 )
					aNewPoly.Clear();
				else
					aNewPoly.SetSize( nNewCount );
			}

			*this = aNewPoly;
		}

		nSize = mpImplPolygon->mnPoints;

		if( nSize > 1 )
		{
			if( ( nOptimizeFlags & POLY_OPTIMIZE_CLOSE ) && 
				( mpImplPolygon->mpPointAry[ 0 ] != mpImplPolygon->mpPointAry[ nSize - 1 ] ) )
			{
				SetSize( mpImplPolygon->mnPoints + 1 );
				mpImplPolygon->mpPointAry[ mpImplPolygon->mnPoints - 1 ] = mpImplPolygon->mpPointAry[ 0 ];
			}
			else if( ( nOptimizeFlags & POLY_OPTIMIZE_OPEN ) && 
					 ( mpImplPolygon->mpPointAry[ 0 ] == mpImplPolygon->mpPointAry[ nSize - 1 ] ) )
			{
				const Point& rFirst = mpImplPolygon->mpPointAry[ 0 ];

				while( nSize && ( mpImplPolygon->mpPointAry[ nSize - 1 ] == rFirst ) )
					nSize--;

				SetSize( nSize );
			}
		}
	}
}

// =======================================================================

/* Recursively subdivide cubic bezier curve via deCasteljau. 
    
   @param rPointIter
   Output iterator, where the subdivided polylines are written to.

   @param d
   Squared difference of curve to a straight line

   @param P*
   Exactly four points, interpreted as support and control points of
   a cubic bezier curve. Must be in device coordinates, since stop
   criterion is based on the following assumption: the device has a 
   finite resolution, it is thus sufficient to stop subdivision if the 
   curve does not deviate more than one pixel from a straight line.

*/
static void ImplAdaptiveSubdivide( ::std::back_insert_iterator< ::std::vector< Point > >& rPointIter,
                                   const double old_d2,
                                   int recursionDepth,
                                   const double d2,
                                   const double P1x, const double P1y,
                                   const double P2x, const double P2y,
                                   const double P3x, const double P3y,
                                   const double P4x, const double P4y )
{
    // Hard limit on recursion depth, empiric number.
    enum {maxRecursionDepth=128};

    // Perform bezier flatness test (lecture notes from R. Schaback,
    // Mathematics of Computer-Aided Design, Uni Goettingen, 2000)
    //
    // ||P(t) - L(t)|| <= max     ||b_j - b_0 - j/n(b_n - b_0)||
    //                    0<=j<=n
    //
    // What is calculated here is an upper bound to the distance from
    // a line through b_0 and b_3 (P1 and P4 in our notation) and the
    // curve. We can drop 0 and n from the running indices, since the
    // argument of max becomes zero for those cases.
    const double fJ1x( P2x - P1x - 1.0/3.0*(P4x - P1x) );
    const double fJ1y( P2y - P1y - 1.0/3.0*(P4y - P1y) );
    const double fJ2x( P3x - P1x - 2.0/3.0*(P4x - P1x) );
    const double fJ2y( P3y - P1y - 2.0/3.0*(P4y - P1y) );
    const double distance2( ::std::max( fJ1x*fJ1x + fJ1y*fJ1y, 
                                        fJ2x*fJ2x + fJ2y*fJ2y) );

    // stop if error measure does not improve anymore. This is a
    // safety guard against floating point inaccuracies.
    // stop at recursion level 128. This is a safety guard against
    // floating point inaccuracies.
    // stop if distance from line is guaranteed to be bounded by d
    if( old_d2 > d2 &&
        recursionDepth < maxRecursionDepth &&
        distance2 >= d2 )
    {
        // deCasteljau bezier arc, split at t=0.5
        // Foley/vanDam, p. 508
        const double L1x( P1x ), 		   	 L1y( P1y );
        const double L2x( (P1x + P2x)*0.5 ), L2y( (P1y + P2y)*0.5 );
        const double Hx ( (P2x + P3x)*0.5 ), Hy ( (P2y + P3y)*0.5 );
        const double L3x( (L2x + Hx)*0.5 ),  L3y( (L2y + Hy)*0.5 );
        const double R4x( P4x ), 		   	 R4y( P4y );
        const double R3x( (P3x + P4x)*0.5 ), R3y( (P3y + P4y)*0.5 );
        const double R2x( (Hx + R3x)*0.5 ),  R2y( (Hy + R3y)*0.5 );
        const double R1x( (L3x + R2x)*0.5 ), R1y( (L3y + R2y)*0.5 );
        const double L4x( R1x ), 		     L4y( R1y );

		// subdivide further
        ++recursionDepth;
        ImplAdaptiveSubdivide(rPointIter, distance2, recursionDepth, d2, L1x, L1y, L2x, L2y, L3x, L3y, L4x, L4y);
        ImplAdaptiveSubdivide(rPointIter, distance2, recursionDepth, d2, R1x, R1y, R2x, R2y, R3x, R3y, R4x, R4y);
    }
	else
	{
		// requested resolution reached.
		// Add end points to output iterator.
		// order is preserved, since this is so to say depth first traversal.
		*rPointIter++ = Point( FRound(P1x), FRound(P1y) );
	}
}

// =======================================================================

void Polygon::AdaptiveSubdivide( Polygon& rResult, const double d ) const
{
	if( !mpImplPolygon->mpFlagAry )
	{
		rResult = *this;
	}
	else
	{
		USHORT i;
		USHORT nPts( GetSize() );
		::std::vector< Point > aPoints;
        aPoints.reserve( nPts );
		::std::back_insert_iterator< ::std::vector< Point > > aPointIter( aPoints );

		for(i=0; i<nPts;)
		{
			if( ( i + 3 ) < nPts )
            { 
                BYTE P1( mpImplPolygon->mpFlagAry[ i ] );
                BYTE P4( mpImplPolygon->mpFlagAry[ i + 3 ] );

                if( ( POLY_NORMAL == P1 || POLY_SMOOTH == P1 || POLY_SYMMTR == P1 ) &&
                    ( POLY_CONTROL == mpImplPolygon->mpFlagAry[ i + 1 ] ) &&
                    ( POLY_CONTROL == mpImplPolygon->mpFlagAry[ i + 2 ] ) &&
                    ( POLY_NORMAL == P4 || POLY_SMOOTH == P4 || POLY_SYMMTR == P4 ) )
                {
                    ImplAdaptiveSubdivide( aPointIter, d*d+1.0, 0, d*d,
                                           mpImplPolygon->mpPointAry[ i ].X(),   mpImplPolygon->mpPointAry[ i ].Y(),
                                           mpImplPolygon->mpPointAry[ i+1 ].X(), mpImplPolygon->mpPointAry[ i+1 ].Y(),
                                           mpImplPolygon->mpPointAry[ i+2 ].X(), mpImplPolygon->mpPointAry[ i+2 ].Y(),
                                           mpImplPolygon->mpPointAry[ i+3 ].X(), mpImplPolygon->mpPointAry[ i+3 ].Y() );
                    i += 3;
                    continue;
                }
            }

            *aPointIter++ = mpImplPolygon->mpPointAry[ i++ ];
		}

		// fill result polygon
		rResult = Polygon( (USHORT)aPoints.size() ); // ensure sufficient size for copy
		::std::copy(aPoints.begin(), aPoints.end(), rResult.mpImplPolygon->mpPointAry);
	}
}

// -----------------------------------------------------------------------

void Polygon::GetIntersection( const PolyPolygon& rPolyPoly, PolyPolygon& rResult ) const
{
	const PolyPolygon aTmp( *this );
	aTmp.GetIntersection( rPolyPoly, rResult );
}

// -----------------------------------------------------------------------

void Polygon::GetUnion( const PolyPolygon& rPolyPoly, PolyPolygon& rResult ) const 
{
	const PolyPolygon aTmp( *this );
	aTmp.GetUnion( rPolyPoly, rResult );
}

// -----------------------------------------------------------------------

void Polygon::GetDifference( const PolyPolygon& rPolyPoly, PolyPolygon& rResult ) const
{
	const PolyPolygon aTmp( *this );
	aTmp.GetDifference( rPolyPoly, rResult );
}

// -----------------------------------------------------------------------

void Polygon::GetXOR( const PolyPolygon& rPolyPoly, PolyPolygon& rResult ) const
{
	const PolyPolygon aTmp( *this );
	aTmp.GetXOR( rPolyPoly, rResult );
}

// -----------------------------------------------------------------------

void Polygon::ImplReduceEdges( Polygon& rPoly, const double& rArea, USHORT nPercent )
{
	const double	fBound = 2000.0 * ( 100 - nPercent ) * 0.01;
	USHORT			nNumNoChange = 0, nNumRuns = 0;

	while( nNumNoChange < 2 ) 
	{
		USHORT	nPntCnt = rPoly.GetSize(), nNewPos = 0;
		Polygon	aNewPoly( nPntCnt );
		BOOL	bChangeInThisRun = FALSE;

		for( USHORT n = 0; n < nPntCnt; n++ )
		{
			BOOL bDeletePoint = FALSE;

			if( ( n + nNumRuns ) % 2 )
			{
				USHORT		nIndPrev = !n ? nPntCnt - 1 : n - 1;
				USHORT		nIndPrevPrev = !nIndPrev ? nPntCnt - 1 : nIndPrev - 1;
				USHORT		nIndNext = ( n == nPntCnt-1 ) ? 0 : n + 1;
				USHORT		nIndNextNext = ( nIndNext == nPntCnt - 1 ) ? 0 : nIndNext + 1;
				Vector2D	aVec1( rPoly[ nIndPrev ] ); aVec1 -= rPoly[ nIndPrevPrev ];
				Vector2D	aVec2( rPoly[ n ] ); aVec2 -= rPoly[ nIndPrev ];
				Vector2D	aVec3( rPoly[ nIndNext ] ); aVec3 -= rPoly[ n ];
				Vector2D	aVec4( rPoly[ nIndNextNext ] ); aVec4 -= rPoly[ nIndNext ];
				double		fDist1 = aVec1.GetLength(), fDist2 = aVec2.GetLength(); 
				double		fDist3 = aVec3.GetLength(), fDist4 = aVec4.GetLength();
				double		fTurnB = aVec2.Normalize().Scalar( aVec3.Normalize() );

				if( fabs( fTurnB ) < ( 1.0 + SMALL_DVALUE ) && fabs( fTurnB ) > ( 1.0 - SMALL_DVALUE ) )
					bDeletePoint = TRUE;
				else
				{
					Vector2D	aVecB( rPoly[ nIndNext ] );
					double		fDistB = ( aVecB -= rPoly[ nIndPrev ] ).GetLength();
					double		fLenWithB = fDist2 + fDist3;
					double		fLenFact = ( fDistB != 0.0 ) ? fLenWithB / fDistB : 1.0;
					double		fTurnPrev = aVec1.Normalize().Scalar( aVec2 );
					double		fTurnNext = aVec3.Scalar( aVec4.Normalize() );
					double		fGradPrev, fGradB, fGradNext;

					if( fabs( fTurnPrev ) < ( 1.0 + SMALL_DVALUE ) && fabs( fTurnPrev ) > ( 1.0 - SMALL_DVALUE ) )
						fGradPrev = 0.0;
					else
						fGradPrev = acos( fTurnPrev ) / ( aVec1.IsNegative( aVec2 ) ? -F_PI180 : F_PI180 );

					fGradB = acos( fTurnB ) / ( aVec2.IsNegative( aVec3 ) ? -F_PI180 : F_PI180 );

					if( fabs( fTurnNext ) < ( 1.0 + SMALL_DVALUE ) && fabs( fTurnNext ) > ( 1.0 - SMALL_DVALUE ) )
						fGradNext = 0.0;
					else
						fGradNext = acos( fTurnNext ) / ( aVec3.IsNegative( aVec4 ) ? -F_PI180 : F_PI180 );

					if( ( fGradPrev > 0.0 && fGradB < 0.0 && fGradNext > 0.0 ) || 
						( fGradPrev < 0.0 && fGradB > 0.0 && fGradNext < 0.0 ) )
					{
						if( ( fLenFact < ( FSQRT2 + SMALL_DVALUE ) ) &&
							( ( ( fDist1 + fDist4 ) / ( fDist2 + fDist3 ) ) * 2000.0 ) > fBound )
						{
							bDeletePoint = TRUE;
						}
					}
					else
					{
						double fRelLen = 1.0 - sqrt( fDistB / rArea );

						if( fRelLen < 0.0 )
							fRelLen = 0.0;
						else if( fRelLen > 1.0 )
							fRelLen = 1.0;

						if( ( (UINT32) ( ( ( fLenFact - 1.0 ) * 1000000.0 ) + 0.5 ) < fBound ) && 
							( fabs( fGradB ) <= ( fRelLen * fBound * 0.01 ) ) )
						{
							bDeletePoint = TRUE;
						}
					}
				}
			}

			if( !bDeletePoint )
				aNewPoly[ nNewPos++ ] = rPoly[ n ];
			else
				bChangeInThisRun = TRUE;
		}

		if( bChangeInThisRun && nNewPos )
		{
			aNewPoly.SetSize( nNewPos );
			rPoly = aNewPoly;
			nNumNoChange = 0;
		}
		else
			nNumNoChange++;

		nNumRuns++;
	}
}

// -----------------------------------------------------------------------

void Polygon::Move( long nHorzMove, long nVertMove )
{
	DBG_CHKTHIS( Polygon, NULL );

	// Diese Abfrage sollte man fuer die DrawEngine durchfuehren
	if ( !nHorzMove && !nVertMove )
		return;

	ImplMakeUnique();

	// Punkte verschieben
	USHORT nCount = mpImplPolygon->mnPoints;
	for ( USHORT i = 0; i < nCount; i++ )
	{
		Point* pPt = &(mpImplPolygon->mpPointAry[i]);
		pPt->X() += nHorzMove;
		pPt->Y() += nVertMove;
	}
}

// -----------------------------------------------------------------------

void Polygon::Translate(const Point& rTrans)
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	for ( USHORT i = 0, nCount = mpImplPolygon->mnPoints; i < nCount; i++ )
		mpImplPolygon->mpPointAry[ i ] += rTrans;
}

// -----------------------------------------------------------------------

void Polygon::Scale( double fScaleX, double fScaleY )
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	for ( USHORT i = 0, nCount = mpImplPolygon->mnPoints; i < nCount; i++ )
	{
		Point& rPnt = mpImplPolygon->mpPointAry[i];
		rPnt.X() = (long) ( fScaleX * rPnt.X() );
		rPnt.Y() = (long) ( fScaleY * rPnt.Y() );
	}
}

// -----------------------------------------------------------------------

void Polygon::Rotate( const Point& rCenter, USHORT nAngle10 )
{
	DBG_CHKTHIS( Polygon, NULL );
	nAngle10 %= 3600;

	if( nAngle10 )
	{
		const double fAngle = F_PI1800 * nAngle10;
		Rotate( rCenter, sin( fAngle ), cos( fAngle ) );
	}
}

// -----------------------------------------------------------------------

void Polygon::Rotate( const Point& rCenter, double fSin, double fCos )
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	long nX, nY;
	long nCenterX = rCenter.X();
	long nCenterY = rCenter.Y();

	for( USHORT i = 0, nCount = mpImplPolygon->mnPoints; i < nCount; i++ )
	{
		Point& rPt = mpImplPolygon->mpPointAry[ i ];

		nX = rPt.X() - nCenterX;
		nY = rPt.Y() - nCenterY;
		rPt.X() = (long) FRound( fCos * nX + fSin * nY ) + nCenterX;
		rPt.Y() = -(long) FRound( fSin * nX - fCos * nY ) + nCenterY;
	}
}

// -----------------------------------------------------------------------

void Polygon::SlantX( long nYRef, double fSin, double fCos )
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	for( USHORT i = 0, nCount = mpImplPolygon->mnPoints; i < nCount; i++ )
	{
		Point&		rPnt = mpImplPolygon->mpPointAry[ i ];
		const long	nDy = rPnt.Y() - nYRef;

		rPnt.X() += (long)( fSin * nDy );
		rPnt.Y() = nYRef + (long)( fCos * nDy );
	}
}

// -----------------------------------------------------------------------

void Polygon::SlantY( long nXRef, double fSin, double fCos )
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	for( USHORT i = 0, nCount = mpImplPolygon->mnPoints; i < nCount; i++ )
	{
		Point&		rPnt = mpImplPolygon->mpPointAry[ i ];
		const long	nDx = rPnt.X() - nXRef;

		rPnt.X() = nXRef + (long)( fCos * nDx );
		rPnt.Y() -= (long)( fSin * nDx );
	}
}

// -----------------------------------------------------------------------

void Polygon::Distort( const Rectangle& rRefRect, const Polygon& rDistortedRect )
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	long	Xr, Wr, X1, X2, X3, X4;
	long	Yr, Hr, Y1, Y2, Y3, Y4;
	double	fTx, fTy, fUx, fUy;

	Xr = rRefRect.Left();
	Yr = rRefRect.Top();
	Wr = rRefRect.GetWidth();
	Hr = rRefRect.GetHeight();

	if( Wr && Hr )
	{
		DBG_ASSERT( rDistortedRect.mpImplPolygon->mnPoints >= 4, "Distort rect too small!" );

		X1 = rDistortedRect[0].X();
		Y1 = rDistortedRect[0].Y();
		X2 = rDistortedRect[1].X();
		Y2 = rDistortedRect[1].Y();
		X3 = rDistortedRect[3].X();
		Y3 = rDistortedRect[3].Y();
		X4 = rDistortedRect[2].X();
		Y4 = rDistortedRect[2].Y();

		for( USHORT i = 0, nCount = mpImplPolygon->mnPoints; i < nCount; i++ )
		{
			Point& rPnt = mpImplPolygon->mpPointAry[ i ];

			fTx = (double)( rPnt.X() - Xr) / Wr;
			fTy = (double)( rPnt.Y() - Yr) / Hr;
			fUx = 1.0 - fTx;
			fUy = 1.0 - fTy;

			rPnt.X() = (long) ( fUy * (fUx * X1 + fTx * X2) + fTy * (fUx * X3 + fTx * X4) );
			rPnt.Y() = (long) ( fUx * (fUy * Y1 + fTy * Y3) + fTx * (fUy * Y2 + fTy * Y4) );
		}
	}
}

// -----------------------------------------------------------------------

class ImplPointFilter
{
public:
	virtual void LastPoint() = 0;
	virtual void Input( const Point& rPoint ) = 0;
};

class ImplPolygonPointFilter : public ImplPointFilter
{
public:
	ImplPolygon*	mpPoly; 	// Nicht loeschen, wird dem Polygon zugewiesen
	USHORT			mnSize;

					ImplPolygonPointFilter( USHORT nDestSize ) :
						mnSize( 0 )
					{
						mpPoly = new ImplPolygon( nDestSize );
					}

	virtual void	LastPoint();
	virtual void	Input( const Point& rPoint );
};

void ImplPolygonPointFilter::Input( const Point& rPoint )
{
	if ( !mnSize || (rPoint != mpPoly->mpPointAry[mnSize-1]) )
	{
		mnSize++;
		if ( mnSize > mpPoly->mnPoints )
			mpPoly->ImplSetSize( mnSize );
		mpPoly->mpPointAry[mnSize-1] = rPoint;
	}
}

void ImplPolygonPointFilter::LastPoint()
{
	if ( mnSize < mpPoly->mnPoints )
		mpPoly->ImplSetSize( mnSize );
};

class ImplEdgePointFilter : public ImplPointFilter
{
	Point				maFirstPoint;
	Point				maLastPoint;
	ImplPointFilter&	mrNextFilter;
	const long			mnLow;
	const long			mnHigh;
	const int			mnEdge;
	int 				mnLastOutside;
	BOOL				mbFirst;

public:
						ImplEdgePointFilter( int nEdge, long nLow, long nHigh,
											 ImplPointFilter& rNextFilter ) :
							mrNextFilter( rNextFilter ),
							mnLow( nLow ),
							mnHigh( nHigh ),
							mnEdge( nEdge ),
							mbFirst( TRUE )
						{
						}

	Point				EdgeSection( const Point& rPoint, int nEdge ) const;
	int 				VisibleSide( const Point& rPoint ) const;
	int 				IsPolygon() const
							{ return maFirstPoint == maLastPoint; }

	virtual void		Input( const Point& rPoint );
	virtual void		LastPoint();
};

inline int ImplEdgePointFilter::VisibleSide( const Point& rPoint ) const
{
	if ( mnEdge & EDGE_HORZ )
	{
		return rPoint.X() < mnLow ? EDGE_LEFT :
									 rPoint.X() > mnHigh ? EDGE_RIGHT : 0;
	}
	else
	{
		return rPoint.Y() < mnLow ? EDGE_TOP :
									 rPoint.Y() > mnHigh ? EDGE_BOTTOM : 0;
	}
}

Point ImplEdgePointFilter::EdgeSection( const Point& rPoint, int nEdge ) const
{
	long lx = maLastPoint.X();
	long ly = maLastPoint.Y();
	long md = rPoint.X() - lx;
	long mn = rPoint.Y() - ly;
	long nNewX;
	long nNewY;

	if ( nEdge & EDGE_VERT )
	{
		nNewY = (nEdge == EDGE_TOP) ? mnLow : mnHigh;
		long dy = nNewY - ly;
		if ( !md )
			nNewX = lx;
		else if ( (LONG_MAX / Abs(md)) >= Abs(dy) )
			nNewX = (dy * md) / mn + lx;
		else
		{
			BigInt ady = dy;
			ady *= md;
			if( ady.IsNeg() )
				if( mn < 0 )
					ady += mn/2;
				else
					ady -= (mn-1)/2;
			else
				if( mn < 0 )
					ady -= (mn+1)/2;
				else
					ady += mn/2;
			ady /= mn;
			nNewX = (long)ady + lx;
		}
	}
	else
	{
		nNewX = (nEdge == EDGE_LEFT) ? mnLow : mnHigh;
		long dx = nNewX - lx;
		if ( !mn )
			nNewY = ly;
		else if ( (LONG_MAX / Abs(mn)) >= Abs(dx) )
			nNewY = (dx * mn) / md + ly;
		else
		{
			BigInt adx = dx;
			adx *= mn;
			if( adx.IsNeg() )
				if( md < 0 )
					adx += md/2;
				else
					adx -= (md-1)/2;
			else
				if( md < 0 )
					adx -= (md+1)/2;
				else
					adx += md/2;
			adx /= md;
			nNewY = (long)adx + ly;
		}
	}

	return Point( nNewX, nNewY );
}

void ImplEdgePointFilter::Input( const Point& rPoint )
{
	int nOutside = VisibleSide( rPoint );

	if ( mbFirst )
	{
		maFirstPoint = rPoint;
		mbFirst 	 = FALSE;
		if ( !nOutside )
			mrNextFilter.Input( rPoint );
	}
	else if ( rPoint == maLastPoint )
		return;
	else if ( !nOutside )
	{
		if ( mnLastOutside )
			mrNextFilter.Input( EdgeSection( rPoint, mnLastOutside ) );
		mrNextFilter.Input( rPoint );
	}
	else if ( !mnLastOutside )
		mrNextFilter.Input( EdgeSection( rPoint, nOutside ) );
	else if ( nOutside != mnLastOutside )
	{
		mrNextFilter.Input( EdgeSection( rPoint, mnLastOutside ) );
		mrNextFilter.Input( EdgeSection( rPoint, nOutside ) );
	}

	maLastPoint    = rPoint;
	mnLastOutside  = nOutside;
}

void ImplEdgePointFilter::LastPoint()
{
	if ( !mbFirst )
	{
		int nOutside = VisibleSide( maFirstPoint );

		if ( nOutside != mnLastOutside )
			Input( maFirstPoint );
		mrNextFilter.LastPoint();
	}
}

// -----------------------------------------------------------------------

void Polygon::Clip( const Rectangle& rRect, BOOL bPolygon )
{
    // #105251# Justify rect befor edge filtering
    Rectangle				aJustifiedRect( rRect );    
    aJustifiedRect.Justify();

	USHORT					nSourceSize = mpImplPolygon->mnPoints;
	ImplPolygonPointFilter	aPolygon( nSourceSize );
	ImplEdgePointFilter 	aHorzFilter( EDGE_HORZ, aJustifiedRect.Left(), aJustifiedRect.Right(),
										 aPolygon );
	ImplEdgePointFilter 	aVertFilter( EDGE_VERT, aJustifiedRect.Top(), aJustifiedRect.Bottom(),
										 aHorzFilter );

	for ( USHORT i = 0; i < nSourceSize; i++ )
		aVertFilter.Input( mpImplPolygon->mpPointAry[i] );
	if ( bPolygon || aVertFilter.IsPolygon() )
		aVertFilter.LastPoint();
	else
		aPolygon.LastPoint();

	// Alte ImpPolygon-Daten loeschen und die vom ImpPolygonPointFilter
	// zuweisen
	if ( mpImplPolygon->mnRefCount )
	{
		if ( mpImplPolygon->mnRefCount > 1 )
			mpImplPolygon->mnRefCount--;
		else
			delete mpImplPolygon;
	}
	mpImplPolygon = aPolygon.mpPoly;
}

// -----------------------------------------------------------------------

Rectangle Polygon::GetBoundRect() const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( !mpImplPolygon->mpFlagAry, "GetBoundRect could fail with beziers!" );

	USHORT	nCount = mpImplPolygon->mnPoints;
	if( ! nCount )
		return Rectangle();

	long	nXMin, nXMax, nYMin, nYMax;

	const Point* pPt = &(mpImplPolygon->mpPointAry[0]);
	nXMin = nXMax = pPt->X();
	nYMin = nYMax = pPt->Y();

	for ( USHORT i = 0; i < nCount; i++ )
	{
		pPt = &(mpImplPolygon->mpPointAry[i]);
	
		if ( pPt->X() < nXMin )
			nXMin = pPt->X();
		if ( pPt->X() > nXMax )
			nXMax = pPt->X();
		if ( pPt->Y() < nYMin )
			nYMin = pPt->Y();
		if ( pPt->Y() > nYMax )
			nYMax = pPt->Y();
	}

	return Rectangle( nXMin, nYMin, nXMax, nYMax );
}

// -----------------------------------------------------------------------

double Polygon::GetArea() const
{
	const double fArea = GetSignedArea();
	return( ( fArea < 0.0 ) ? -fArea : fArea );
}

// -----------------------------------------------------------------------

double Polygon::GetSignedArea() const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( !mpImplPolygon->mpFlagAry, "GetArea could fail with beziers!" );

	double fArea = 0.0;

	if( mpImplPolygon->mnPoints > 2 )
	{
		const USHORT nCount1 = mpImplPolygon->mnPoints - 1;

		for( USHORT i = 0; i < nCount1; )
		{
			const Point& rPt = mpImplPolygon->mpPointAry[ i ];
			const Point& rPt1 = mpImplPolygon->mpPointAry[ ++i ];
			fArea += ( rPt.X() - rPt1.X() ) * ( rPt.Y() + rPt1.Y() );
		}

		const Point& rPt = mpImplPolygon->mpPointAry[ nCount1 ];
		const Point& rPt0 = mpImplPolygon->mpPointAry[ 0 ];
		fArea += ( rPt.X() - rPt0.X() ) * ( rPt.Y() + rPt0.Y() );
	}

	return fArea;
}

// -----------------------------------------------------------------------

BOOL Polygon::IsInside( const Point& rPoint ) const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( !mpImplPolygon->mpFlagAry, "IsInside could fail with beziers!" );

	const Rectangle aBound( GetBoundRect() );
	const Line		aLine( rPoint, Point( aBound.Right() + 100L, rPoint.Y() ) );
	USHORT			nCount = mpImplPolygon->mnPoints;
	USHORT			nPCounter = 0;

	if ( ( nCount > 2 ) && aBound.IsInside( rPoint ) )
	{
		Point	aPt1( mpImplPolygon->mpPointAry[ 0 ] );
		Point	aIntersection;
		Point	aLastIntersection;

		while ( ( aPt1 == mpImplPolygon->mpPointAry[ nCount - 1 ] ) && ( nCount > 3 ) )
			nCount--;

		for ( USHORT i = 1; i <= nCount; i++ )
		{
			const Point& rPt2 = mpImplPolygon->mpPointAry[ ( i < nCount ) ? i : 0 ];

			if ( aLine.Intersection( Line( aPt1, rPt2 ), aIntersection ) )
			{
				// Hiermit verhindern wir das Einfuegen von
				// doppelten Intersections, die gleich hintereinander folgen
				if ( nPCounter )
				{
					if ( aIntersection != aLastIntersection )
					{
						aLastIntersection = aIntersection;
						nPCounter++;
					}
				}
				else
				{
					aLastIntersection = aIntersection;
					nPCounter++;
				}
			}

			aPt1 = rPt2;
		}
	}

	// innerhalb, wenn die Anzahl der Schnittpunkte ungerade ist
	return ( ( nPCounter & 1 ) == 1 );
}

// -----------------------------------------------------------------------

BOOL Polygon::IsRightOrientated() const
{
	DBG_CHKTHIS( Polygon, NULL );
	return GetSignedArea() >= 0.0;
}

// -----------------------------------------------------------------------

void Polygon::Insert( USHORT nPos, const Point& rPt, PolyFlags eFlags )
{
	DBG_CHKTHIS( Polygon, NULL );
	ImplMakeUnique();

	if( nPos >= mpImplPolygon->mnPoints )
		nPos = mpImplPolygon->mnPoints;

	mpImplPolygon->ImplSplit( nPos, 1 );
	mpImplPolygon->mpPointAry[ nPos ] = rPt;

	if( POLY_NORMAL != eFlags )
	{
		mpImplPolygon->ImplCreateFlagArray();
		mpImplPolygon->mpFlagAry[ nPos ] = (BYTE) eFlags;
	}
}

// -----------------------------------------------------------------------

void Polygon::Insert( USHORT nPos, const Polygon& rPoly )
{
	DBG_CHKTHIS( Polygon, NULL );
	const USHORT nInsertCount = rPoly.mpImplPolygon->mnPoints;

	if( nInsertCount )
	{
		ImplMakeUnique();

		if( nPos >= mpImplPolygon->mnPoints )
			nPos = mpImplPolygon->mnPoints;

		if( rPoly.mpImplPolygon->mpFlagAry )
			mpImplPolygon->ImplCreateFlagArray();

		mpImplPolygon->ImplSplit( nPos, nInsertCount, rPoly.mpImplPolygon );
	}
}

// -----------------------------------------------------------------------

void Polygon::Remove( USHORT nPos, USHORT nCount )
{
	DBG_CHKTHIS( Polygon, NULL );
	if( nCount && ( nPos < mpImplPolygon->mnPoints ) )
	{
		ImplMakeUnique();
		mpImplPolygon->ImplRemove( nPos, nCount );
	}
}

// -----------------------------------------------------------------------

Point& Polygon::operator[]( USHORT nPos )
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_ASSERT( nPos < mpImplPolygon->mnPoints, "Polygon::[]: nPos >= nPoints" );

	ImplMakeUnique();
	return mpImplPolygon->mpPointAry[nPos];
}

// -----------------------------------------------------------------------

Polygon& Polygon::operator=( const Polygon& rPoly )
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_CHKOBJ( &rPoly, Polygon, NULL );
	DBG_ASSERT( rPoly.mpImplPolygon->mnRefCount < 0xFFFFFFFE, "Polygon: RefCount overflow" );

	// Zuerst Referenzcounter erhoehen, damit man sich selbst zuweisen kann
	// RefCount == 0 fuer statische Objekte
	if ( rPoly.mpImplPolygon->mnRefCount )
		rPoly.mpImplPolygon->mnRefCount++;

	// Wenn es keine statischen ImpDaten sind, dann loeschen, wenn es
	// die letzte Referenz ist, sonst Referenzcounter decrementieren
	if ( mpImplPolygon->mnRefCount )
	{
		if ( mpImplPolygon->mnRefCount > 1 )
			mpImplPolygon->mnRefCount--;
		else
			delete mpImplPolygon;
	}

	mpImplPolygon = rPoly.mpImplPolygon;
	return *this;
}

// -----------------------------------------------------------------------

BOOL Polygon::operator==( const Polygon& rPoly ) const
{
	DBG_CHKTHIS( Polygon, NULL );
	DBG_CHKOBJ( &rPoly, Polygon, NULL );

	if ( (rPoly.mpImplPolygon == mpImplPolygon) )
		return TRUE;
	else
		return FALSE;
}

// -----------------------------------------------------------------------

sal_Bool Polygon::IsEqual( const Polygon& rPoly ) const
{
	sal_Bool bIsEqual = sal_True;;
	sal_uInt16 i;
	if ( GetSize() != rPoly.GetSize() )
		bIsEqual = sal_False;
	else
	{
		for ( i = 0; i < GetSize(); i++ )
		{
			if ( ( GetPoint( i ) != rPoly.GetPoint( i ) ) ||
				( GetFlags( i ) != rPoly.GetFlags( i ) ) )
			{
				bIsEqual = sal_False;
				break;
			}
		}
	}
	return bIsEqual;
}

// -----------------------------------------------------------------------

SvStream& operator>>( SvStream& rIStream, Polygon& rPoly )
{
	DBG_CHKOBJ( &rPoly, Polygon, NULL );
	DBG_ASSERTWARNING( rIStream.GetVersion(), "Polygon::>> - Solar-Version not set on rIStream" );

	USHORT			i;
	USHORT			nStart;
	USHORT			nCurPoints;
	USHORT			nPoints;
	unsigned char	bShort;
	short			nShortX;
	short			nShortY;
	long			nLongX;
	long			nLongY;

	// Anzahl der Punkte einlesen und Array erzeugen
	rIStream >> nPoints;
	if ( rPoly.mpImplPolygon->mnRefCount != 1 )
	{
		if ( rPoly.mpImplPolygon->mnRefCount )
			rPoly.mpImplPolygon->mnRefCount--;
		rPoly.mpImplPolygon = new ImplPolygon( nPoints );
	}
	else
		rPoly.mpImplPolygon->ImplSetSize( nPoints, FALSE );

	// Je nach CompressMode das Polygon einlesen
	if ( rIStream.GetCompressMode() == COMPRESSMODE_FULL )
	{
		i = 0;
		while ( i < nPoints )
		{
			rIStream >> bShort >> nCurPoints;

			if ( bShort )
			{
				for ( nStart = i; i < nStart+nCurPoints; i++ )
				{
					rIStream >> nShortX >> nShortY;
					rPoly.mpImplPolygon->mpPointAry[i].X() = nShortX;
					rPoly.mpImplPolygon->mpPointAry[i].Y() = nShortY;
				}
			}
			else
			{
				for ( nStart = i; i < nStart+nCurPoints; i++ )
				{
					rIStream >> nLongX >> nLongY;
					rPoly.mpImplPolygon->mpPointAry[i].X() = nLongX;
					rPoly.mpImplPolygon->mpPointAry[i].Y() = nLongY;
				}
			}
		}
	}
	else
	{
		// Feststellen, ob ueber die Operatoren geschrieben werden muss
#if (SAL_TYPES_SIZEOFLONG) != 4
		if ( 1 )
#else
#ifdef OSL_BIGENDIAN
		if ( rIStream.GetNumberFormatInt() != NUMBERFORMAT_INT_BIGENDIAN )
#else
		if ( rIStream.GetNumberFormatInt() != NUMBERFORMAT_INT_LITTLEENDIAN )
#endif
#endif
		{
			for( i = 0; i < nPoints; i++ )
			{
				rIStream >> rPoly.mpImplPolygon->mpPointAry[i].X()
						 >> rPoly.mpImplPolygon->mpPointAry[i].Y();
			}
		}
		else
			rIStream.Read( rPoly.mpImplPolygon->mpPointAry, nPoints*sizeof(Point) );
	}

	return rIStream;
}

// -----------------------------------------------------------------------

SvStream& operator<<( SvStream& rOStream, const Polygon& rPoly )
{
	DBG_CHKOBJ( &rPoly, Polygon, NULL );
	DBG_ASSERTWARNING( rOStream.GetVersion(), "Polygon::<< - Solar-Version not set on rOStream" );

	unsigned char	bShort;
	unsigned char	bCurShort;
	USHORT			nStart;
	USHORT			i;
	USHORT			nPoints = rPoly.GetSize();

	// Anzahl der Punkte rausschreiben
	rOStream << nPoints;

	// Je nach CompressMode das Polygon rausschreiben
	if ( rOStream.GetCompressMode() == COMPRESSMODE_FULL )
	{
		i = 0;
		while ( i < nPoints )
		{
			nStart = i;

			// Feststellen, welcher Typ geschrieben werden soll
			if ( ((rPoly.mpImplPolygon->mpPointAry[nStart].X() >= SHRT_MIN) &&
				  (rPoly.mpImplPolygon->mpPointAry[nStart].X() <= SHRT_MAX)) &&
				 ((rPoly.mpImplPolygon->mpPointAry[nStart].Y() >= SHRT_MIN) &&
				  (rPoly.mpImplPolygon->mpPointAry[nStart].Y() <= SHRT_MAX)) )
				bShort = TRUE;
			else
				bShort = FALSE;
			while ( i < nPoints )
			{
				// Feststellen, welcher Typ geschrieben werden soll
				if ( ((rPoly.mpImplPolygon->mpPointAry[nStart].X() >= SHRT_MIN) &&
					  (rPoly.mpImplPolygon->mpPointAry[nStart].X() <= SHRT_MAX)) &&
					 ((rPoly.mpImplPolygon->mpPointAry[nStart].Y() >= SHRT_MIN) &&
					  (rPoly.mpImplPolygon->mpPointAry[nStart].Y() <= SHRT_MAX)) )
					bCurShort = TRUE;
				else
					bCurShort = FALSE;

				// Wenn sich die Werte in einen anderen Bereich begeben,
				// muessen wir neu rausschreiben
				if ( bCurShort != bShort )
				{
					bShort = bCurShort;
					break;
				}

				i++;
			}

			rOStream << bShort << (USHORT)(i-nStart);

			if ( bShort )
			{
				for( ; nStart < i; nStart++ )
				{
					rOStream << (short)rPoly.mpImplPolygon->mpPointAry[nStart].X()
							 << (short)rPoly.mpImplPolygon->mpPointAry[nStart].Y();
				}
			}
			else
			{
				for( ; nStart < i; nStart++ )
				{
					rOStream << rPoly.mpImplPolygon->mpPointAry[nStart].X()
							 << rPoly.mpImplPolygon->mpPointAry[nStart].Y();
				}
			}
		}
	}
	else
	{
		// Feststellen, ob ueber die Operatoren geschrieben werden muss
#if (SAL_TYPES_SIZEOFLONG) != 4
		if ( 1 )
#else
#ifdef OSL_BIGENDIAN
		if ( rOStream.GetNumberFormatInt() != NUMBERFORMAT_INT_BIGENDIAN )
#else
		if ( rOStream.GetNumberFormatInt() != NUMBERFORMAT_INT_LITTLEENDIAN )
#endif
#endif
		{
			for( i = 0; i < nPoints; i++ )
			{
				rOStream << rPoly.mpImplPolygon->mpPointAry[i].X()
						 << rPoly.mpImplPolygon->mpPointAry[i].Y();
			}
		}
		else
		{
			if ( nPoints )
				rOStream.Write( rPoly.mpImplPolygon->mpPointAry, nPoints*sizeof(Point) );
		}
	}

	return rOStream;
}

// -----------------------------------------------------------------------

void Polygon::ImplRead( SvStream& rIStream )
{
	sal_uInt8	bHasPolyFlags;

	rIStream >> *this
			 >> bHasPolyFlags;

	if ( bHasPolyFlags )
	{
		mpImplPolygon->mpFlagAry = new sal_uInt8[ mpImplPolygon->mnPoints ];
		rIStream.Read( mpImplPolygon->mpFlagAry, mpImplPolygon->mnPoints );
	}
}

// -----------------------------------------------------------------------

void Polygon::Read( SvStream& rIStream )
{
	VersionCompat aCompat( rIStream, STREAM_READ );

    ImplRead( rIStream );
}

// -----------------------------------------------------------------------

void Polygon::ImplWrite( SvStream& rOStream ) const
{
	sal_uInt8	bHasPolyFlags = mpImplPolygon->mpFlagAry != NULL;
	rOStream << *this
			 << bHasPolyFlags;

	if ( bHasPolyFlags )
		rOStream.Write( mpImplPolygon->mpFlagAry, mpImplPolygon->mnPoints );
}

// -----------------------------------------------------------------------

void Polygon::Write( SvStream& rOStream ) const
{
	VersionCompat aCompat( rOStream, STREAM_WRITE, 1 );

    ImplWrite( rOStream );
}

// -----------------------------------------------------------------------
// #i74631# numerical correction method for B2DPolygon
void impCorrectContinuity(basegfx::B2DPolygon& roPolygon, sal_uInt32 nIndex, BYTE nCFlag)
{
	const sal_uInt32 nPointCount(roPolygon.count());
	OSL_ENSURE(nIndex < nPointCount, "impCorrectContinuity: index access out of range (!)");

	if(nIndex < nPointCount && (POLY_SMOOTH == nCFlag || POLY_SYMMTR == nCFlag))
	{
		if(roPolygon.isPrevControlPointUsed(nIndex) && roPolygon.isNextControlPointUsed(nIndex))
		{
			const basegfx::B2DPoint aPoint(roPolygon.getB2DPoint(nIndex));

			if(POLY_SMOOTH == nCFlag)
			{
				// C1: apply inverse direction of prev to next, keep length of next
				const basegfx::B2DVector aOriginalNext(roPolygon.getNextControlPoint(nIndex) - aPoint);
				basegfx::B2DVector aNewNext(aPoint - roPolygon.getPrevControlPoint(nIndex));

				aNewNext.setLength(aOriginalNext.getLength());
				roPolygon.setNextControlPoint(nIndex, basegfx::B2DPoint(aPoint + aNewNext));
			}
			else // POLY_SYMMTR
			{
				// C2: apply inverse control point to next
				roPolygon.setNextControlPoint(nIndex, (2.0 * aPoint) - roPolygon.getPrevControlPoint(nIndex));
			}
		}
	}
}

// -----------------------------------------------------------------------
// convert to basegfx::B2DPolygon and return
basegfx::B2DPolygon Polygon::getB2DPolygon() const
{
	basegfx::B2DPolygon aRetval;
	const sal_uInt16 nCount(mpImplPolygon->mnPoints);

	if(nCount)
	{
		if(mpImplPolygon->mpFlagAry)
		{
			// handling for curves. Add start point
			const Point aStartPoint(mpImplPolygon->mpPointAry[0]);
			BYTE nPointFlag(mpImplPolygon->mpFlagAry[0]);
			aRetval.append(basegfx::B2DPoint(aStartPoint.X(), aStartPoint.Y()));
			Point aControlA, aControlB;

			for(sal_uInt16 a(1); a < nCount;)
			{
				bool bControlA(false);
				bool bControlB(false);

				if(POLY_CONTROL == mpImplPolygon->mpFlagAry[a])
				{
					aControlA = mpImplPolygon->mpPointAry[a++];
					bControlA = true;
				}

				if(a < nCount && POLY_CONTROL == mpImplPolygon->mpFlagAry[a])
				{
					aControlB = mpImplPolygon->mpPointAry[a++];
					bControlB = true;
				}

				// assert invalid polygons
				OSL_ENSURE(bControlA == bControlB, "Polygon::getB2DPolygon: Invalid source polygon (!)");

				if(a < nCount)
				{
					const Point aEndPoint(mpImplPolygon->mpPointAry[a]);

					if(bControlA)
					{
						// bezier edge, add
						aRetval.appendBezierSegment(
							basegfx::B2DPoint(aControlA.X(), aControlA.Y()),
							basegfx::B2DPoint(aControlB.X(), aControlB.Y()),
							basegfx::B2DPoint(aEndPoint.X(), aEndPoint.Y()));

						impCorrectContinuity(aRetval, aRetval.count() - 2, nPointFlag);
					}
					else
					{
						// no bezier edge, add end point
						aRetval.append(basegfx::B2DPoint(aEndPoint.X(), aEndPoint.Y()));
					}

					nPointFlag = mpImplPolygon->mpFlagAry[a++];
				}
			}

			// if exist, remove double first/last points, set closed and correct control points
			basegfx::tools::checkClosed(aRetval);
			
			if(aRetval.isClosed())
			{
				// closeWithGeometryChange did really close, so last point(s) were removed.
				// Correct the continuity in the changed point
				impCorrectContinuity(aRetval, 0, mpImplPolygon->mpFlagAry[0]);
			}
		}
		else
		{
			// extra handling for non-curves (most-used case) for speedup
			for(sal_uInt16 a(0); a < nCount; a++)
			{
				// get point and add
				const Point aPoint(mpImplPolygon->mpPointAry[a]);
				aRetval.append(basegfx::B2DPoint(aPoint.X(), aPoint.Y()));
			}

			// set closed flag
			basegfx::tools::checkClosed(aRetval);
		}
	}

	return aRetval;
}

// -----------------------------------------------------------------------
// constructor to convert from basegfx::B2DPolygon
// #i76891# Needed to change from adding all control points (even for unused
// edges) and creating a fixed-size Polygon in the first run to creating the
// minimal Polygon. This requires a temporary Point- and Flag-Array for curves
// and a memcopy at ImplPolygon creation, but contains no zero-controlpoints
// for straight edges.
Polygon::Polygon(const basegfx::B2DPolygon& rPolygon)
:	mpImplPolygon(0)
{
	DBG_CTOR( Polygon, NULL );

	const bool bCurve(rPolygon.areControlPointsUsed());
	const bool bClosed(rPolygon.isClosed());
	sal_uInt32 nB2DLocalCount(rPolygon.count());

	if(bCurve)
	{
		// #127979# Reduce source point count hard to the limit of the tools Polygon
		if(nB2DLocalCount > ((0x0000ffff / 3L) - 1L))
		{
			DBG_ERROR("Polygon::Polygon: Too many points in given B2DPolygon, need to reduce hard to maximum of tools Polygon (!)");
			nB2DLocalCount = ((0x0000ffff / 3L) - 1L);
		}

		// calculate target point count
		const sal_uInt32 nLoopCount(bClosed ? nB2DLocalCount : (nB2DLocalCount ? nB2DLocalCount - 1L : 0L ));

		if(nLoopCount)
		{
			// calculate maximum array size and allocate; prepare insert index
			const sal_uInt32 nMaxTargetCount((nLoopCount * 3) + 1);
			mpImplPolygon = new ImplPolygon(static_cast< sal_uInt16 >(nMaxTargetCount), true);

			// prepare insert index and current point
			sal_uInt32 nArrayInsert(0);
			basegfx::B2DCubicBezier aBezier;
			aBezier.setStartPoint(rPolygon.getB2DPoint(0));

			for(sal_uInt32 a(0L); a < nLoopCount; a++)
			{
				// add current point (always) and remember StartPointIndex for evtl. later corrections
				const Point aStartPoint(FRound(aBezier.getStartPoint().getX()), FRound(aBezier.getStartPoint().getY()));
				const sal_uInt32 nStartPointIndex(nArrayInsert);
				mpImplPolygon->mpPointAry[nStartPointIndex] = aStartPoint;
				mpImplPolygon->mpFlagAry[nStartPointIndex] = (BYTE)POLY_NORMAL;
				nArrayInsert++;

				// prepare next segment
				const sal_uInt32 nNextIndex((a + 1) % nB2DLocalCount);
				aBezier.setEndPoint(rPolygon.getB2DPoint(nNextIndex));
				aBezier.setControlPointA(rPolygon.getNextControlPoint(a));
				aBezier.setControlPointB(rPolygon.getPrevControlPoint(nNextIndex));

				if(aBezier.isBezier())
				{
					// if one is used, add always two control points due to the old schema
					mpImplPolygon->mpPointAry[nArrayInsert] = Point(FRound(aBezier.getControlPointA().getX()), FRound(aBezier.getControlPointA().getY()));
					mpImplPolygon->mpFlagAry[nArrayInsert] = (BYTE)POLY_CONTROL;
					nArrayInsert++;

					mpImplPolygon->mpPointAry[nArrayInsert] = Point(FRound(aBezier.getControlPointB().getX()), FRound(aBezier.getControlPointB().getY()));
					mpImplPolygon->mpFlagAry[nArrayInsert] = (BYTE)POLY_CONTROL;
					nArrayInsert++;
				}

				// test continuity with previous control point to set flag value
				if(aBezier.getControlPointA() != aBezier.getStartPoint() && (bClosed || a))
				{
					const basegfx::B2VectorContinuity eCont(rPolygon.getContinuityInPoint(a));

					if(basegfx::CONTINUITY_C1 == eCont)
					{
						mpImplPolygon->mpFlagAry[nStartPointIndex] = (BYTE)POLY_SMOOTH;
					}
					else if(basegfx::CONTINUITY_C2 == eCont)
					{
						mpImplPolygon->mpFlagAry[nStartPointIndex] = (BYTE)POLY_SYMMTR;
					}
				}

				// prepare next polygon step
				aBezier.setStartPoint(aBezier.getEndPoint());
			}

			if(bClosed)
			{
				// add first point again as closing point due to old definition
				mpImplPolygon->mpPointAry[nArrayInsert] = mpImplPolygon->mpPointAry[0];
				mpImplPolygon->mpFlagAry[nArrayInsert] = (BYTE)POLY_NORMAL;
				nArrayInsert++;
			}
			else
			{
				// add last point as closing point
				const basegfx::B2DPoint aClosingPoint(rPolygon.getB2DPoint(nB2DLocalCount - 1L));
				const Point aEnd(FRound(aClosingPoint.getX()), FRound(aClosingPoint.getY()));
				mpImplPolygon->mpPointAry[nArrayInsert] = aEnd;
				mpImplPolygon->mpFlagAry[nArrayInsert] = (BYTE)POLY_NORMAL;
				nArrayInsert++;
			}

			DBG_ASSERT(nArrayInsert <= nMaxTargetCount, "Polygon::Polygon from basegfx::B2DPolygon: wrong max point count estimation (!)");
			
			if(nArrayInsert != nMaxTargetCount)
			{
				mpImplPolygon->ImplSetSize(static_cast< sal_uInt16 >(nArrayInsert), true);
			}
		}
	}
	else
	{
		// #127979# Reduce source point count hard to the limit of the tools Polygon
		if(nB2DLocalCount > (0x0000ffff - 1L))
		{
			DBG_ERROR("Polygon::Polygon: Too many points in given B2DPolygon, need to reduce hard to maximum of tools Polygon (!)");
			nB2DLocalCount = (0x0000ffff - 1L);
		}

		if(nB2DLocalCount)
		{
			// point list creation
			const sal_uInt32 nTargetCount(nB2DLocalCount + (bClosed ? 1L : 0L));
			mpImplPolygon = new ImplPolygon( static_cast< sal_uInt16 >(nTargetCount) );
			sal_uInt16 nIndex(0);

			for(sal_uInt32 a(0L); a < nB2DLocalCount; a++)
			{
				basegfx::B2DPoint aB2DPoint(rPolygon.getB2DPoint(a));
				Point aPoint(FRound(aB2DPoint.getX()), FRound(aB2DPoint.getY()));
				mpImplPolygon->mpPointAry[nIndex++] = aPoint;
			}

			if(bClosed)
			{
				// add first point as closing point
				mpImplPolygon->mpPointAry[nIndex] = mpImplPolygon->mpPointAry[0];
			}
		}
	}

	if(!mpImplPolygon)
	{
		// no content yet, create empty polygon
		mpImplPolygon = (ImplPolygon*)(&aStaticImplPolygon);
	}
}

// eof
