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
#include "precompiled_svx.hxx"
#include <svx/dialmgr.hxx>
#ifndef _SVX_DIALOGS_HRC
#include <svx/dialogs.hrc>
#endif
#include <tools/shl.hxx>
#include <i18npool/mslangid.hxx>
#include <svtools/valueset.hxx>
#include <svtools/languageoptions.hxx>
#ifndef _SVX_HELPID_HRC
#include <helpid.hrc>
#endif
#include <svx/numitem.hxx>
#include <svtools/eitem.hxx>
#include <vcl/svapp.hxx>
#include <gallery.hxx>
#include <svtools/urihelper.hxx>
#include <svx/brshitem.hxx>
#include <svtools/intitem.hxx>
#include <sfx2/objsh.hxx>
#include <vcl/graph.hxx>
#include <vcl/msgbox.hxx>
#include <flstitem.hxx>
#include <dlgutil.hxx>
#ifndef _XTABLE_HXX //autogen

#include <svx/xtable.hxx>
#endif
#include <drawitem.hxx>
#include <numvset.hxx>
#include <htmlmode.hxx>
#include <svtools/pathoptions.hxx>
#include <svtools/ctrltool.hxx>
#include <unolingu.hxx>
#include <com/sun/star/style/NumberingType.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/text/XDefaultNumberingProvider.hpp>
#include <com/sun/star/text/XNumberingFormatter.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/text/XNumberingTypeInfo.hpp>

#include <algorithm>
#include "opengrf.hxx"

using namespace com::sun::star::uno;
using namespace com::sun::star::beans;
using namespace com::sun::star::lang;
using namespace com::sun::star::i18n;
using namespace com::sun::star::text;
using namespace com::sun::star::container;
using namespace com::sun::star::style;
using rtl::OUString;

#define C2U(cChar) OUString::createFromAscii(cChar)
#define NUM_PAGETYPE_BULLET			0
#define NUM_PAGETYPE_SINGLENUM      1
#define NUM_PAGETYPE_NUM            2
#define NUM_PAGETYPE_BMP            3
#define PAGETYPE_USER_START         10

#define SHOW_NUMBERING				0
#define SHOW_BULLET					1
#define SHOW_BITMAP					2

#define MAX_BMP_WIDTH				16
#define MAX_BMP_HEIGHT				16

static const sal_Char cNumberingType[] = "NumberingType";
static const sal_Char cValue[] = "Value";
static const sal_Char cParentNumbering[] = "ParentNumbering";
static const sal_Char cPrefix[] = "Prefix";
static const sal_Char cSuffix[] = "Suffix";
static const sal_Char cBulletChar[] = "BulletChar";
static const sal_Char cBulletFontName[] = "BulletFontName";

/* -----------------28.10.98 08:32-------------------
 *
 * --------------------------------------------------*/
// Die Auswahl an Bullets aus den StarSymbol
static const sal_Unicode aBulletTypes[] =
{
	0x2022,
	0x25cf,
	0xe00c,
	0xe00a,
	0x2794,
	0x27a2,
	0x2717,
	0x2714
};

static Font& lcl_GetDefaultBulletFont()
{
	static BOOL bInit = 0;
	static Font aDefBulletFont( UniString::CreateFromAscii(
		                        RTL_CONSTASCII_STRINGPARAM( "StarSymbol" ) ),
								String(), Size( 0, 14 ) );
	if(!bInit)
	{
        aDefBulletFont.SetCharSet( RTL_TEXTENCODING_SYMBOL );
		aDefBulletFont.SetFamily( FAMILY_DONTKNOW );
		aDefBulletFont.SetPitch( PITCH_DONTKNOW );
		aDefBulletFont.SetWeight( WEIGHT_DONTKNOW );
		aDefBulletFont.SetTransparent( TRUE );
		bInit = TRUE;
	}
	return aDefBulletFont;
}

static void lcl_PaintLevel(OutputDevice* pVDev, sal_Int16 nNumberingType,
                        const OUString& rBulletChar, const OUString& rText, const OUString& rFontName,
                        Point& rLeft, Font& rRuleFont, const Font& rTextFont)
{

    if(NumberingType::CHAR_SPECIAL == nNumberingType )
    {
        rRuleFont.SetStyleName(rFontName);
        pVDev->SetFont(rRuleFont);
        pVDev->DrawText(rLeft, rBulletChar);
        rLeft.X() += pVDev->GetTextWidth(rBulletChar);
    }
    else
    {
        pVDev->SetFont(rTextFont);
        pVDev->DrawText(rLeft, rText);
        rLeft.X() += pVDev->GetTextWidth(rText);
    }
}
void  SvxNumValueSet::UserDraw( const UserDrawEvent& rUDEvt )
{
	static USHORT __READONLY_DATA aLinesArr[] =
	{
		15, 10,
		20, 30,
		25, 50,
		30, 70,
		35, 90,	// up to here line positions
        05, 10, // character positions
        10, 30,
        15, 50,
        20, 70,
        25, 90,
	};

    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    const Color aBackColor = rStyleSettings.GetFieldColor();
    const Color aTextColor = rStyleSettings.GetFieldTextColor();

    OutputDevice*  pDev = rUDEvt.GetDevice();
	Rectangle aRect = rUDEvt.GetRect();
	USHORT	nItemId = rUDEvt.GetItemId();
	long nRectWidth = aRect.GetWidth();
	long nRectHeight = aRect.GetHeight();
	Size aRectSize(nRectWidth, aRect.GetHeight());
	Point aBLPos = aRect.TopLeft();
	Font aOldFont = pDev->GetFont();
	Color aOldColor = pDev->GetLineColor();
    pDev->SetLineColor(aTextColor);
    Font aFont(OutputDevice::GetDefaultFont(
                DEFAULTFONT_UI_SANS, MsLangId::getSystemLanguage(), DEFAULTFONT_FLAGS_ONLYONE));

    Size aSize = aFont.GetSize();

	Font aRuleFont( lcl_GetDefaultBulletFont() );
	aSize.Height() = nRectHeight/6;
	aRuleFont.SetSize(aSize);
    aRuleFont.SetColor(aTextColor);
    aRuleFont.SetFillColor(aBackColor);
	if(nPageType == NUM_PAGETYPE_BULLET)
		aFont = aRuleFont;
	else if(nPageType == NUM_PAGETYPE_NUM)
	{
		aSize.Height() = nRectHeight/8;
	}
	aFont.SetColor(aTextColor);
	aFont.SetFillColor(aBackColor);
	aFont.SetSize( aSize );
	pDev->SetFont(aFont);

	if(!pVDev)
	{
		// Die Linien werden nur einmalig in das VirtualDevice gepainted
		// nur die Gliederungspage bekommt es aktuell
		pVDev = new VirtualDevice(*pDev);
		pVDev->SetMapMode(pDev->GetMapMode());
		pVDev->EnableRTL( IsRTLEnabled() );
 		pVDev->SetOutputSize( aRectSize );
		aOrgRect = aRect;
		pVDev->SetFillColor( aBackColor );
		pVDev->DrawRect(aOrgRect);

        if(aBackColor == aLineColor)
            aLineColor.Invert();
        pVDev->SetLineColor(aLineColor);
		// Linien nur einmalig Zeichnen
		if(nPageType != NUM_PAGETYPE_NUM)
		{
			Point aStart(aBLPos.X() + nRectWidth *25 / 100,0);
			Point aEnd(aBLPos.X() + nRectWidth * 9 / 10,0);
			for( USHORT i = 11; i < 100; i += 33)
			{
				aStart.Y() = aEnd.Y() = aBLPos.Y() + nRectHeight  * i / 100;
				pVDev->DrawLine(aStart, aEnd);
				aStart.Y() = aEnd.Y() = aBLPos.Y() + nRectHeight  * (i + 11) / 100;
				pVDev->DrawLine(aStart, aEnd);
			}
		}
	}
	pDev->DrawOutDev(	aRect.TopLeft(), aRectSize,
						aOrgRect.TopLeft(), aRectSize,
						*pVDev );
	// jetzt kommt der Text
	const OUString sValue(C2U(cValue));
    if( NUM_PAGETYPE_SINGLENUM == nPageType ||
			NUM_PAGETYPE_BULLET == nPageType )
	{
		Point aStart(aBLPos.X() + nRectWidth / 9,0);
		for( USHORT i = 0; i < 3; i++ )
		{
			USHORT nY = 11 + i * 33;
			aStart.Y() = aBLPos.Y() + nRectHeight  * nY / 100;
			String sText;
			if(nPageType == NUM_PAGETYPE_BULLET)
			{
				sText = aBulletTypes[nItemId - 1];
				aStart.Y() -= pDev->GetTextHeight()/2;
				aStart.X() = aBLPos.X() + 5;
			}
			else
			{
				if(xFormatter.is() && aNumSettings.getLength() > nItemId - 1)
				{
					Sequence<PropertyValue> aLevel = aNumSettings.getConstArray()[nItemId - 1];
					try
					{
						aLevel.realloc(aLevel.getLength() + 1);
						PropertyValue& rValue = aLevel.getArray()[aLevel.getLength() - 1];
						rValue.Name = sValue;
						rValue.Value <<= (sal_Int32)(i + 1);
						sText = xFormatter->makeNumberingString( aLevel, aLocale );
					}
					catch(Exception&)
					{
						DBG_ERROR("Exception in DefaultNumberingProvider::makeNumberingString");
					}
				}
				// knapp neben dem linken Rand beginnen
				aStart.X() = aBLPos.X() + 2;
				aStart.Y() -= pDev->GetTextHeight()/2;
			}
			pDev->DrawText(aStart, sText);
		}
	}
	else if(NUM_PAGETYPE_NUM == nPageType )
	{
		// Outline numbering has to be painted into the virtual device
		// to get correct lines
		// has to be made again
		pVDev->DrawRect(aOrgRect);
        long nStartX = aOrgRect.TopLeft().X();
		long nStartY = aOrgRect.TopLeft().Y();

		if(xFormatter.is() && aOutlineSettings.getLength() > nItemId - 1)
		{
			Reference<XIndexAccess> xLevel = aOutlineSettings.getArray()[nItemId - 1];
			try
			{
                OUString sLevelTexts[5];
                OUString sFontNames[5];
                OUString sBulletChars[5];
                sal_Int16 aNumberingTypes[5];
                OUString sPrefixes[5];
                OUString sSuffixes[5];
                sal_Int16 aParentNumberings[5];

                sal_Int32 nLevelCount = xLevel->getCount();
                if(nLevelCount > 5)
                    nLevelCount = 5;
                for( sal_Int32 i = 0; i < nLevelCount && i < 5; i++)
				{
                    long nTop = nStartY + nRectHeight * (aLinesArr[2 * i + 11])/100 ;
					Point aLeft(nStartX + nRectWidth *  (aLinesArr[2 * i + 10])/ 100, nTop );

                    Any aLevelAny = xLevel->getByIndex(i);
					Sequence<PropertyValue> aLevel;
					aLevelAny >>= aLevel;
                    const PropertyValue* pValues = aLevel.getConstArray();
                    aNumberingTypes[i] = 0;
                    for(sal_Int32 nProperty = 0; nProperty < aLevel.getLength() - 1; nProperty++)
                    {
                        if(pValues[nProperty].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cNumberingType)))
                            pValues[nProperty].Value >>= aNumberingTypes[i];
                        else if(pValues[nProperty].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cBulletFontName)))
                            pValues[nProperty].Value >>= sFontNames[i];
                        else if(pValues[nProperty].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cBulletChar)))
                            pValues[nProperty].Value >>= sBulletChars[i];
                        else if(pValues[nProperty].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cPrefix)))
                            pValues[nProperty].Value >>= sPrefixes[i];
                        else if(pValues[nProperty].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cSuffix)))
                            pValues[nProperty].Value >>= sSuffixes[i];
                        else if(pValues[nProperty].Name.equalsAsciiL(RTL_CONSTASCII_STRINGPARAM(cParentNumbering)))
                            pValues[nProperty].Value >>= aParentNumberings[i];
                    }
                    Sequence< PropertyValue > aProperties(2);
                    PropertyValue* pProperties = aProperties.getArray();
                    pProperties[0].Name = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("NumberingType"));
                    pProperties[0].Value <<= aNumberingTypes[i];
                    pProperties[1].Name = rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Value"));
                    pProperties[1].Value <<= (sal_Int32)1;
                    try
					{
                        sLevelTexts[i] = xFormatter->makeNumberingString( aProperties, aLocale );
					}
					catch(Exception&)
					{
						DBG_ERROR("Exception in DefaultNumberingProvider::makeNumberingString");
					}

                    aLeft.Y() -= (pDev->GetTextHeight()/2);
                    if(sPrefixes[i].getLength() &&
                        !sPrefixes[i].equalsAsciiL(" ", 1) &&
                        sPrefixes[i].getStr()[0] != 0)
                    {
                        pVDev->SetFont(aFont);
                        pVDev->DrawText(aLeft, sPrefixes[i]);                                        
                        aLeft.X() += pDev->GetTextWidth(sPrefixes[i]);                               
                    }
                    if(aParentNumberings[i])
                    {
                        //insert old numberings here
                        sal_Int32 nStartLevel = std::min((sal_Int32)aParentNumberings[i], i);
                        for(sal_Int32 nParentLevel = i - nStartLevel; nParentLevel < i; nParentLevel++)
                        {
                            OUString sTmp(sLevelTexts[nParentLevel]);
                            sTmp += C2U(".");
                            lcl_PaintLevel(pVDev,
                                    aNumberingTypes[nParentLevel],
                                    sBulletChars[nParentLevel],
                                    sTmp,
                                    sFontNames[nParentLevel],
                                    aLeft,
                                    aRuleFont,
                                    aFont);
                        }
                    }
                    lcl_PaintLevel(pVDev,
                                    aNumberingTypes[i],
                                    sBulletChars[i],
                                    sLevelTexts[i],
                                    sFontNames[i],
                                    aLeft,
                                    aRuleFont,
                                    aFont);
                    if(sSuffixes[i].getLength()&&
                        !sSuffixes[i].equalsAsciiL(" ", 1) &&
                        sSuffixes[i].getStr()[0] != 0)
                    {
                        pVDev->SetFont(aFont);
                        pVDev->DrawText(aLeft, sSuffixes[i]);
                        aLeft.X() += pDev->GetTextWidth(sSuffixes[i]);
                    }

					long nLineTop = nStartY + nRectHeight * aLinesArr[2 * i + 1]/100 ;
                    Point aLineLeft(aLeft.X() /*+ nStartX + nRectWidth * aLinesArr[2 * i]/ 100*/, nLineTop );
					Point aLineRight(nStartX + nRectWidth * 90 /100, nLineTop );
					pVDev->DrawLine(aLineLeft,	aLineRight);
                }

            }
#ifdef DBG_UTIL
			catch(Exception&)
			{
				static sal_Bool bAssert = FALSE;
				if(!bAssert)
				{
					DBG_ERROR("exception in ::UserDraw");
					bAssert = sal_True;
				}
			}
#else
			catch(Exception&)
			{
			}
#endif
		}
		pDev->DrawOutDev(	aRect.TopLeft(), aRectSize,
							aOrgRect.TopLeft(), aRectSize,
							*pVDev );
	}

	pDev->SetFont(aOldFont);
	pDev->SetLineColor(aOldColor);
}

/**************************************************************************/
/*                                                                        */
/*                                                                        */
/**************************************************************************/

SvxNumValueSet::SvxNumValueSet( Window* pParent, const ResId& rResId, USHORT nType ) :

	ValueSet( pParent, rResId ),

    aLineColor  ( COL_LIGHTGRAY ),
    nPageType   ( nType ),
    bHTMLMode   ( FALSE ),
    pVDev       ( NULL )
{
	SetColCount( 4 );
    SetLineCount( 2 );
	SetStyle( GetStyle() | WB_ITEMBORDER | WB_DOUBLEBORDER );
	if(NUM_PAGETYPE_BULLET == nType)
	{
		for	( USHORT i = 0; i < 8; i++ )
        {
			InsertItem( i + 1, i );
            SetItemText( i + 1, SVX_RESSTR( RID_SVXSTR_BULLET_DESCRIPTIONS + i ) );
        }
	}
}

/*-----------------08.02.97 12.38-------------------

--------------------------------------------------*/

 SvxNumValueSet::~SvxNumValueSet()
{
	delete pVDev;
}
/* -----------------------------30.01.01 16:24--------------------------------

 ---------------------------------------------------------------------------*/
void SvxNumValueSet::SetNumberingSettings(
	const Sequence<Sequence<PropertyValue> >& aNum,
	Reference<XNumberingFormatter>& xFormat,
	const Locale& rLocale	)
{
	aNumSettings = aNum;
	xFormatter = xFormat;
	aLocale = rLocale;
    if(aNum.getLength() > 8)
            SetStyle( GetStyle()|WB_VSCROLL);
    for ( USHORT i = 0; i < aNum.getLength(); i++ )
    {
			InsertItem( i + 1, i );
            if( i < 8 )
                SetItemText( i + 1, SVX_RESSTR( RID_SVXSTR_SINGLENUM_DESCRIPTIONS + i ));
    }
}
/* -----------------------------31.01.01 09:50--------------------------------

 ---------------------------------------------------------------------------*/
void SvxNumValueSet::SetOutlineNumberingSettings(
			Sequence<Reference<XIndexAccess> >& rOutline,
			Reference<XNumberingFormatter>& xFormat,
			const Locale& rLocale)
{
	aOutlineSettings = rOutline;
	xFormatter = xFormat;
	aLocale = rLocale;
    if(aOutlineSettings.getLength() > 8)
        SetStyle( GetStyle() | WB_VSCROLL );
    for ( sal_uInt16 i = 0; i < aOutlineSettings.getLength(); i++ )
    {
		InsertItem( i + 1, i );
        if( i < 8 )
            SetItemText( i + 1, SVX_RESSTR( RID_SVXSTR_OUTLINENUM_DESCRIPTIONS + i ));
    }
}

SvxBmpNumValueSet::SvxBmpNumValueSet( Window* pParent, const ResId& rResId/*, const List& rStrNames*/ ) :

	SvxNumValueSet( pParent, rResId, NUM_PAGETYPE_BMP ),
//    rStrList    ( rStrNames ),
	bGrfNotFound( FALSE )

{
    GalleryExplorer::BeginLocking(GALLERY_THEME_BULLETS);
    SetStyle( GetStyle() | WB_VSCROLL );
	SetLineCount( 3 );
	aFormatTimer.SetTimeout(300);
	aFormatTimer.SetTimeoutHdl(LINK(this, SvxBmpNumValueSet, FormatHdl_Impl));
}

/*-----------------13.02.97 09.41-------------------

--------------------------------------------------*/

 SvxBmpNumValueSet::~SvxBmpNumValueSet()
{
    GalleryExplorer::EndLocking(GALLERY_THEME_BULLETS);
    aFormatTimer.Stop();
}
/*-----------------13.02.97 09.41-------------------

--------------------------------------------------*/

void   	SvxBmpNumValueSet::UserDraw( const UserDrawEvent& rUDEvt )
{
	SvxNumValueSet::UserDraw(rUDEvt);

	Rectangle aRect = rUDEvt.GetRect();
	OutputDevice*  pDev = rUDEvt.GetDevice();
	USHORT	nItemId = rUDEvt.GetItemId();
	Point aBLPos = aRect.TopLeft();

	int nRectHeight = aRect.GetHeight();
	Size aSize(nRectHeight/8, nRectHeight/8);

    Graphic aGraphic;
    if(!GalleryExplorer::GetGraphicObj( GALLERY_THEME_BULLETS, nItemId - 1,
                        &aGraphic, NULL))
    {
        bGrfNotFound = TRUE;
    }
    else
    {
        Point aPos(aBLPos.X() + 5, 0);
        for( USHORT i = 0; i < 3; i++ )
        {
            USHORT nY = 11 + i * 33;
            aPos.Y() = aBLPos.Y() + nRectHeight  * nY / 100;
            aGraphic.Draw( pDev, aPos, aSize );
        }
    }
}

/*-----------------14.02.97 07.34-------------------

--------------------------------------------------*/

IMPL_LINK(SvxBmpNumValueSet, FormatHdl_Impl, Timer*, EMPTYARG)
{
	// nur, wenn eine Grafik nicht da war, muss formatiert werden
	if(bGrfNotFound)
	{
		bGrfNotFound = FALSE;
		Format();
	}
	Invalidate();
	return 0;
}
