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
#include "precompiled_vcl.hxx"

#include <gcach_vdev.hxx>

#include <vcl/svapp.hxx>
#include <vcl/bitmap.hxx>
#include <vcl/outfont.hxx>
#include <vcl/virdev.hxx>
#include <vcl/metric.hxx>

// =======================================================================
// VirtDevServerFont
// =======================================================================

// -----------------------------------------------------------------------

void VirtDevServerFont::AnnounceFonts( ImplDevFontList* pToAdd )
{
	// TODO: get fonts on server but not on client,
	// problem is that currently there is no serverside virtual device...
	VirtualDevice vdev( 1 );
	long nCount = vdev.GetDevFontCount();

	for( int i = 0; i < nCount; ++i )
	{
		const FontInfo aFontInfo = vdev.GetDevFont( i );

		ImplFontData& rData = *new ImplFontData;
		rData.SetSysData( new FontSysData( (void*)SERVERFONT_MAGIC ) );

	    rData.maName		= aFontInfo.GetName();
	    rData.maStyleName	= aFontInfo.GetStyleName();
		rData.mnWidth		= aFontInfo.GetWidth();
		rData.mnHeight		= aFontInfo.GetHeight();
		rData.meFamily		= aFontInfo.GetFamily();
		rData.meCharSet		= aFontInfo.GetCharSet();
		rData.mePitch		= aFontInfo.GetPitch();
		rData.meWidthType	= aFontInfo.GetWidthType();
		rData.meWeight		= aFontInfo.GetWeight();
		rData.meItalic		= aFontInfo.GetItalic();
		rData.meType		= aFontInfo.GetType();
		rData.meFamily		= aFontInfo.GetFamily();

		rData.mbOrientation	= true;			// TODO: where to get this info?
		rData.mbDevice		= false;
		rData.mnQuality		= 0;			// prefer client-side fonts if available

		pToAdd->Add( &rData );
	}
}

// -----------------------------------------------------------------------

void VirtDevServerFont::ClearFontList()
{
	// TODO
}

// -----------------------------------------------------------------------

VirtDevServerFont* VirtDevServerFont::CreateFont( const ImplFontSelectData& rFSD )
{
	VirtDevServerFont* pServerFont = NULL;
	// TODO: search list of VirtDevServerFonts, return NULL if not found
	// pServerFont = new VirtDevServerFont( rFSD );
	return pServerFont;
}

// -----------------------------------------------------------------------

VirtDevServerFont::VirtDevServerFont( const ImplFontSelectData& rFSD )
:	ServerFont( rFSD)
{}

// -----------------------------------------------------------------------

void VirtDevServerFont::FetchFontMetric( ImplFontMetricData& rTo, long& rFactor ) const
{
	const ImplFontSelectData& aFSD = GetFontSelData();

	Font aFont;
	aFont.SetName		( aFSD.maName );
	aFont.SetStyleName	( aFSD.maStyleName );
	aFont.SetHeight		( aFSD.mnHeight );
	aFont.SetWidth		( aFSD.mnWidth );
	aFont.SetOrientation( aFSD.mnOrientation );
	aFont.SetVertical	( GetFontSelData().mbVertical );

	VirtualDevice vdev( 1 );
	FontMetric aMetric( vdev.GetFontMetric( aFont ) );

	rFactor = 0x100;

	rTo.mnAscent		= aMetric.GetAscent();
	rTo.mnDescent		= aMetric.GetDescent();
	rTo.mnIntLeading	= aMetric.GetIntLeading();
	rTo.mnExtLeading	= aMetric.GetExtLeading();
	rTo.mnSlant		= aMetric.GetSlant();
	rTo.meType		= aMetric.GetType();
	rTo.mnFirstChar		= 0x0020;	// TODO: where to get this info?
	rTo.mnLastChar		= 0xFFFE;	// TODO: where to get this info?

	rTo.mnWidth			= aFSD.mnWidth;
	rTo.maName			= aFSD.maName;
	rTo.maStyleName		= aFSD.maStyleName;
	rTo.mnOrientation	= aFSD.mnOrientation;
	rTo.meFamily		= aFSD.meFamily;
	rTo.meCharSet		= aFSD.meCharSet;
	rTo.meWeight		= aFSD.meWeight;
	rTo.meItalic		= aFSD.meItalic;
	rTo.mePitch			= aFSD.mePitch;
	rTo.mbDevice		= FALSE;
}

// -----------------------------------------------------------------------

int VirtDevServerFont::GetGlyphIndex( sal_Unicode aChar ) const
{
	return aChar;
}

// -----------------------------------------------------------------------

void VirtDevServerFont::InitGlyphData( int nGlyphIndex, GlyphData& rGD ) const
{
	Font aFont;
	aFont.SetName		( GetFontSelData().maName );
	aFont.SetStyleName	( GetFontSelData().maStyleName );
	aFont.SetHeight		( GetFontSelData().mnHeight );
	aFont.SetWidth		( GetFontSelData().mnWidth );
	aFont.SetOrientation( GetFontSelData().mnOrientation );
	aFont.SetVertical	( GetFontSelData().mbVertical );

	VirtualDevice vdev( 1 );
	vdev.SetFont( aFont );

	// get glyph metrics
	sal_Int32 nCharWidth = 10;
// TODO:	vdev.GetCharWidth( nGlyphIndex, nGlyphIndex, &nCharWidth );
	rGD.SetCharWidth( nCharWidth );

    sal_Unicode aChar = nGlyphIndex;
    String aGlyphStr( &aChar, 1 );
    Rectangle aRect;
    if( vdev.GetTextBoundRect( aRect, aGlyphStr, 0, 1 ) )
    {
        rGD.SetOffset( aRect.Top(), aRect.Left() );
        rGD.SetDelta( vdev.GetTextWidth( nGlyphIndex ), 0 );
        rGD.SetSize( aRect.GetSize() );
    }
}

// -----------------------------------------------------------------------

bool VirtDevServerFont::GetAntialiasAdvice( void ) const
{
	return false;
}

// -----------------------------------------------------------------------

bool VirtDevServerFont::GetGlyphBitmap1( int nGlyphIndex, RawBitmap& ) const
{
	/*
    sal_Unicode aChar = nGlyphIndex;
    String aGlyphStr( &aChar, 1 );

    // draw bitmap
	vdev.SetOutputSizePixel( aSize, TRUE );
	vdev.DrawText( Point(0,0)-rGD.GetMetric().GetOffset(), aGlyphStr );

	// create new glyph item

	const Bitmap& rBitmap = vdev.GetBitmap( Point(0,0), aSize );
	rGD.SetBitmap( new Bitmap( rBitmap ) );
	return true;
	*/
	return false;
}

// -----------------------------------------------------------------------

bool	 VirtDevServerFont::GetGlyphBitmap8( int nGlyphIndex, RawBitmap& ) const
{
	return false;
}

// -----------------------------------------------------------------------

int VirtDevServerFont::GetGlyphKernValue( int, int ) const
{
    return 0;
}

// -----------------------------------------------------------------------

ULONG VirtDevServerFont::GetKernPairs( ImplKernPairData** ppImplKernPairs ) const
{
	Font aFont;
	aFont.SetName		( GetFontSelData().maName );
	aFont.SetStyleName	( GetFontSelData().maStyleName );
	aFont.SetHeight		( GetFontSelData().mnHeight );
	aFont.SetWidth		( GetFontSelData().mnWidth );
	aFont.SetOrientation( GetFontSelData().mnOrientation );
	aFont.SetVertical	( GetFontSelData().mbVertical );

	VirtualDevice vdev( 1 );
	vdev.SetFont( aFont );

	ULONG nPairs = vdev.GetKerningPairCount();
	if( nPairs > 0 )
	{
		KerningPair* const pKernPairs = new KerningPair[ nPairs ];
		vdev.GetKerningPairs( nPairs, pKernPairs );

		*ppImplKernPairs = new ImplKernPairData[ nPairs ];
		ImplKernPairData* pTo = *ppImplKernPairs;
		KerningPair* pFrom = pKernPairs;
		for ( ULONG n = 0; n < nPairs; n++ )
		{
			pTo->mnChar1	= pFrom->nChar1;
			pTo->mnChar2	= pFrom->nChar2;
			pTo->mnKern		= pFrom->nKern;
			++pFrom;
			++pTo;
		}

		delete[] pKernPairs;
	}

	return nPairs;
}

// -----------------------------------------------------------------------

bool VirtDevServerFont::GetGlyphOutline( int nGlyphIndex, PolyPolygon& rPolyPoly ) const
{
	return false;
	/*
    Font aFont;
	aFont.SetName		( GetFontSelData().maName );
	aFont.SetStyleName	( GetFontSelData().maStyleName );
	aFont.SetHeight		( GetFontSelData().mnHeight );
	aFont.SetWidth		( GetFontSelData().mnWidth );
	aFont.SetOrientation( GetFontSelData().mnOrientation );
	aFont.SetVertical	( GetFontSelData().mbVertical );

	VirtualDevice vdev( 1 );
	vdev.SetFont( aFont );

	const bool bOptimize = true;

    sal_Unicode aChar = nGlyphIndex;
    String aGlyphStr( &aChar, 1 );
	return vdev.GetTextOutline( rPolyPoly, aGlyphStr, 0, 1, bOptimize );
	*/
}

// =======================================================================
