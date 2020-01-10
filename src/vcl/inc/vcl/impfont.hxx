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

#ifndef _SV_IMPFONT_HXX
#define _SV_IMPFONT_HXX

#include <tools/gen.hxx>
#include <tools/string.hxx>
#include <i18npool/lang.h>
#include <tools/color.hxx>
#include <vcl/dllapi.h>
#include <vcl/vclenum.hxx>
#include <vcl/fntstyle.hxx>

// ------------
// - Impl_Font -
// ------------

class Impl_Font
{
public:
             Impl_Font();
             Impl_Font( const Impl_Font& );

    bool     operator==( const Impl_Font& ) const;

    FontPitch           GetPitch()      { if(mePitch==PITCH_DONTKNOW)    AskConfig(); return mePitch; }
    FontFamily          GetFamily()     { if(meFamily==FAMILY_DONTKNOW)  AskConfig(); return meFamily; }
    FontItalic          GetItalic()     { if(meItalic==ITALIC_DONTKNOW)  AskConfig(); return meItalic; }
    FontWeight          GetWeight()     { if(meWeight==WEIGHT_DONTKNOW)  AskConfig(); return meWeight; }
    FontWidth           GetWidthType()  { if(meWidthType==WIDTH_DONTKNOW)AskConfig(); return meWidthType; }

private:
    friend class Font;
    void                AskConfig();

	int					mnRefCount;
	String				maFamilyName;
	String				maStyleName;
	Size				maSize;
	Color				maColor;		// compatibility, now on output device
	Color				maFillColor;	// compatibility, now on output device
	rtl_TextEncoding	meCharSet;
	LanguageType		meLanguage;
	LanguageType		meCJKLanguage;
	FontFamily			meFamily;
	FontPitch			mePitch;
	TextAlign			meAlign;
	FontWeight			meWeight;
	FontWidth			meWidthType;
	FontItalic			meItalic;
	FontUnderline		meUnderline;
	FontUnderline		meOverline;
	FontStrikeout		meStrikeout;
	FontRelief			meRelief;
	FontEmphasisMark	meEmphasisMark;
	FontType			meType;			// used by metrics only
	short				mnOrientation;
	FontKerning 		mnKerning;
	BOOL				mbWordLine:1,
                        mbOutline:1,
                        mbConfigLookup:1,   // there was a config lookup
						mbShadow:1,
						mbVertical:1,
						mbTransparent:1;	// compatibility, now on output device

	friend SvStream&	operator>>( SvStream& rIStm, Impl_Font& );
	friend SvStream&	operator<<( SvStream& rOStm, const Impl_Font& );
};

// ------------------
// - ImplFontMetric -
// ------------------

class ImplFontMetric
{
    friend class OutputDevice;

private:
    long    mnAscent;      // Ascent
    long    mnDescent;     // Descent
    long    mnIntLeading;  // Internal Leading
    long    mnExtLeading;  // External Leading
    long    mnLineHeight;  // Ascent+Descent+EmphasisMark
    long    mnSlant;       // Slant
    USHORT  mnMiscFlags;   // Misc Flags
    UINT32  mnRefCount;    // Reference Counter

    enum { DEVICE_FLAG=1, SCALABLE_FLAG=2, LATIN_FLAG=4, CJK_FLAG=8, CTL_FLAG=16 };

public:
            ImplFontMetric();
    void    AddReference();
    void    DeReference();

    long    GetAscent() const       { return mnAscent; }
    long    GetDescent() const      { return mnDescent; }
    long    GetIntLeading() const   { return mnIntLeading; }
    long    GetExtLeading() const   { return mnExtLeading; }
    long    GetLineHeight() const   { return mnLineHeight; }
    long    GetSlant() const        { return mnSlant; }

    bool    IsDeviceFont() const    { return ((mnMiscFlags & DEVICE_FLAG) != 0); }
    bool    IsScalable() const      { return ((mnMiscFlags & SCALABLE_FLAG) != 0); }
    bool    SupportsLatin() const   { return ((mnMiscFlags & LATIN_FLAG) != 0); }
    bool    SupportsCJK() const     { return ((mnMiscFlags & CJK_FLAG) != 0); }
    bool    SupportsCTL() const     { return ((mnMiscFlags & CTL_FLAG) != 0); }

    bool    operator==( const ImplFontMetric& ) const;
};

// -------------------
// - ImplFontCharMap -
// -------------------

class CmapResult;

class VCL_DLLPUBLIC ImplFontCharMap
{
public:
   explicit             ImplFontCharMap( const CmapResult& );

	static ImplFontCharMap* GetDefaultMap( bool bSymbols=false);

    bool                IsDefaultMap() const;
    bool                HasChar( sal_uInt32 ) const;
    int                 CountCharsInRange( sal_uInt32 cMin, sal_uInt32 cMax ) const;
    int                 GetCharCount() const;

    sal_uInt32          GetFirstChar() const;
    sal_uInt32          GetLastChar() const;

    sal_uInt32          GetNextChar( sal_uInt32 ) const;
    sal_uInt32          GetPrevChar( sal_uInt32 ) const;

    int                 GetIndexFromChar( sal_uInt32 ) const;
    sal_uInt32          GetCharFromIndex( int ) const;

    void                AddReference();
    void                DeReference();

    int                 GetGlyphIndex( sal_uInt32 ) const;

private:
    /*virtual*/         ~ImplFontCharMap();
    int                 ImplFindRangeIndex( sal_uInt32 ) const;

    // prevent assignment and copy construction
    explicit            ImplFontCharMap( const ImplFontCharMap& );
    void                operator=( const ImplFontCharMap& );

private:
    const sal_uInt32*   mpRangeCodes;     // pairs of StartCode/(EndCode+1)
    const int*          mpStartGlyphs;    // range-specific mapper to glyphs
    const USHORT*       mpGlyphIds;       // individual glyphid mappings
    int                 mnRangeCount;
    int                 mnCharCount;
    int                 mnRefCount;
};

// CmapResult is a normalized version of the many CMAP formats
class
#ifdef UNX
	VCL_DLLPUBLIC // vcl-plugins need it
#endif // UNX
CmapResult
{
public:
	explicit	CmapResult( bool bSymbolic = false,
					const sal_uInt32* pRangeCodes = NULL, int nRangeCount = 0,
					const int* pStartGlyphs = 0, const USHORT* pGlyphIds = NULL );

	const sal_uInt32* mpRangeCodes;
	const int*        mpStartGlyphs;
	const USHORT*     mpGlyphIds;
	int			      mnRangeCount;
	bool		      mbSymbolic;
	bool			  mbRecoded;
};

bool ParseCMAP( const unsigned char* pRawData, int nRawLength, CmapResult& );

#endif // _SV_IMPFONT_HXX

