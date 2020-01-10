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
#include <cstring>
#include <i18npool/mslangid.hxx>

#ifndef _SV_SVSYS_HXX
#include <svsys.h>
#endif
#include <vcl/salgdi.hxx>
#include <vcl/sallayout.hxx>
#include <rtl/tencinfo.h>
#include <tools/debug.hxx>
#include <vcl/svdata.hxx>
#include <vcl/metric.hxx>
#include <vcl/impfont.hxx>
#include <vcl/metaact.hxx>
#include <vcl/gdimtf.hxx>
#include <vcl/outdata.hxx>
#include <vcl/outfont.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <tools/poly.hxx>
#include <vcl/outdev.h>
#include <vcl/virdev.hxx>
#include <vcl/print.hxx>
#include <vcl/event.hxx>
#include <vcl/window.h>
#include <vcl/window.hxx>
#include <vcl/svapp.hxx>
#include <vcl/bmpacc.hxx>
#include <vcl/fontcvt.hxx>
#include <vcl/outdev.hxx>
#include <vcl/edit.hxx>
#include <vcl/fontcfg.hxx>
#include <vcl/sysdata.hxx>
#ifndef _OSL_FILE_H
#include <osl/file.h>
#endif
#ifdef ENABLE_GRAPHITE
#include <vcl/graphite_features.hxx>
#endif

#include <vcl/unohelp.hxx>
#include <pdfwriter_impl.hxx>
#include <vcl/controllayout.hxx>
#include <rtl/logfile.hxx>

#ifndef _COM_SUN_STAR_BEANS_PROPERTYVALUES_HDL_
#include <com/sun/star/beans/PropertyValues.hdl>
#endif
#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <com/sun/star/i18n/WordType.hpp>
#include <com/sun/star/linguistic2/XLinguServiceManager.hpp>

#if defined UNX
#define GLYPH_FONT_HEIGHT   128
#elif defined OS2
#define GLYPH_FONT_HEIGHT   176
#else
#define GLYPH_FONT_HEIGHT   256
#endif

#include <sal/alloca.h>

#include <cmath>
#include <cstring>

#include <memory>
#include <algorithm>

// =======================================================================

DBG_NAMEEX( OutputDevice )
DBG_NAMEEX( Font )

// =======================================================================

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::rtl;
using namespace ::vcl;

// =======================================================================

#define TEXT_DRAW_ELLIPSIS  (TEXT_DRAW_ENDELLIPSIS | TEXT_DRAW_PATHELLIPSIS | TEXT_DRAW_NEWSELLIPSIS)

// =======================================================================

#define UNDERLINE_LAST      UNDERLINE_BOLDWAVE
#define STRIKEOUT_LAST      STRIKEOUT_X

// =======================================================================

static void ImplRotatePos( long nOriginX, long nOriginY, long& rX, long& rY,
                           int nOrientation )
{
    if ( (nOrientation >= 0) && !(nOrientation % 900) )
    {
        if ( (nOrientation >= 3600) )
            nOrientation %= 3600;

        if ( nOrientation )
        {
            rX -= nOriginX;
            rY -= nOriginY;

            if ( nOrientation == 900 )
            {
                long nTemp = rX;
                rX = rY;
                rY = -nTemp;
            }
            else if ( nOrientation == 1800 )
            {
                rX = -rX;
                rY = -rY;
            }
            else /* ( nOrientation == 2700 ) */
            {
                long nTemp = rX;
                rX = -rY;
                rY = nTemp;
            }

            rX += nOriginX;
            rY += nOriginY;
        }
    }
    else
    {
        double nRealOrientation = nOrientation*F_PI1800;
        double nCos = cos( nRealOrientation );
        double nSin = sin( nRealOrientation );

        // Translation...
        long nX = rX-nOriginX;
        long nY = rY-nOriginY;

        // Rotation...
        rX = +((long)(nCos*nX + nSin*nY)) + nOriginX;
        rY = -((long)(nSin*nX - nCos*nY)) + nOriginY;
    }
}

// =======================================================================

void OutputDevice::ImplUpdateFontData( bool bNewFontLists )
{
    // the currently selected logical font is no longer needed
    if ( mpFontEntry )
    {
        mpFontCache->Release( mpFontEntry );
        mpFontEntry = NULL;
    }

    mbInitFont = true;
    mbNewFont = true;

    if ( bNewFontLists )
    {
        if ( mpGetDevFontList )
        {
            delete mpGetDevFontList;
            mpGetDevFontList = NULL;
        }
        if ( mpGetDevSizeList )
        {
            delete mpGetDevSizeList;
            mpGetDevSizeList = NULL;
        }

        // release all physically selected fonts on this device
	if( ImplGetGraphics() )
	     mpGraphics->ReleaseFonts();
    }

    if ( GetOutDevType() == OUTDEV_PRINTER || mpPDFWriter )
    {
        ImplSVData* pSVData = ImplGetSVData();

        if( mpFontCache && mpFontCache != pSVData->maGDIData.mpScreenFontCache )
            mpFontCache->Invalidate();

        if ( bNewFontLists )
        {
            // we need a graphics
            if ( ImplGetGraphics() )
            {
                if( mpFontList && mpFontList != pSVData->maGDIData.mpScreenFontList )
                    mpFontList->Clear();

                if( mpPDFWriter )
                {
                    if( mpFontList && mpFontList != pSVData->maGDIData.mpScreenFontList )
                        delete mpFontList;
                    if( mpFontCache && mpFontCache != pSVData->maGDIData.mpScreenFontCache )
                        delete mpFontCache;
                    mpFontList = mpPDFWriter->filterDevFontList( pSVData->maGDIData.mpScreenFontList );
                    mpFontCache = new ImplFontCache( FALSE );
                }
                else
                {
                    if( mpOutDevData )
                        mpOutDevData->maDevFontSubst.Clear();
                    mpGraphics->GetDevFontList( mpFontList );
                    mpGraphics->GetDevFontSubstList( this );
                }
            }
        }
    }

    // also update child windows if needed
    if ( GetOutDevType() == OUTDEV_WINDOW )
    {
        Window* pChild = ((Window*)this)->mpWindowImpl->mpFirstChild;
        while ( pChild )
        {
            pChild->ImplUpdateFontData( true );
            pChild = pChild->mpWindowImpl->mpNext;
        }
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplUpdateAllFontData( bool bNewFontLists )
{
    ImplSVData* pSVData = ImplGetSVData();

    // update all windows
    Window* pFrame = pSVData->maWinData.mpFirstFrame;
    while ( pFrame )
    {
        pFrame->ImplUpdateFontData( bNewFontLists );

        Window* pSysWin = pFrame->mpWindowImpl->mpFrameData->mpFirstOverlap;
        while ( pSysWin )
        {
            pSysWin->ImplUpdateFontData( bNewFontLists );
            pSysWin = pSysWin->mpWindowImpl->mpNextOverlap;
        }

        pFrame = pFrame->mpWindowImpl->mpFrameData->mpNextFrame;
    }

    // update all virtual devices
    VirtualDevice* pVirDev = pSVData->maGDIData.mpFirstVirDev;
    while ( pVirDev )
    {
        pVirDev->ImplUpdateFontData( bNewFontLists );
        pVirDev = pVirDev->mpNext;
    }

    // update all printers
    Printer* pPrinter = pSVData->maGDIData.mpFirstPrinter;
    while ( pPrinter )
    {
        pPrinter->ImplUpdateFontData( bNewFontLists );
        pPrinter = pPrinter->mpNext;
    }

    // clear global font lists to have them updated
    pSVData->maGDIData.mpScreenFontCache->Invalidate();
    if ( bNewFontLists )
    {
        pSVData->maGDIData.mpScreenFontList->Clear();
        pFrame = pSVData->maWinData.mpFirstFrame;
        if ( pFrame )
        {
            if ( pFrame->ImplGetGraphics() )
                // MT: Stupid typecast here and somewhere ((OutputDevice*)&aVDev)->, because bug in .NET2002 compiler.
                ((OutputDevice*)pFrame)->mpGraphics->GetDevFontList( pFrame->mpWindowImpl->mpFrameData->mpFontList );
        }
    }
}

// =======================================================================

struct ImplLocalizedFontName
{
    const char*         mpEnglishName;
    const sal_Unicode*  mpLocalizedNames;
};

static sal_Unicode const aBatang[] = { 0xBC14, 0xD0D5, 0, 0 };
static sal_Unicode const aBatangChe[] = { 0xBC14, 0xD0D5, 0xCCB4, 0, 0 };
static sal_Unicode const aGungsuh[] = { 0xAD81, 0xC11C, 0, 0 };
static sal_Unicode const aGungsuhChe[] = { 0xAD81, 0xC11C, 0xCCB4, 0, 0 };
static sal_Unicode const aGulim[] = { 0xAD74, 0xB9BC, 0, 0 };
static sal_Unicode const aGulimChe[] = { 0xAD74, 0xB9BC, 0xCCB4, 0, 0 };
static sal_Unicode const aDotum[] = { 0xB3CB, 0xC6C0, 0, 0 };
static sal_Unicode const aDotumChe[] = { 0xB3CB, 0xC6C0, 0xCCB4, 0, 0 };
static sal_Unicode const aSimSun[] = { 0x5B8B, 0x4F53, 0, 0 };
static sal_Unicode const aNSimSun[] = { 0x65B0, 0x5B8B, 0x4F53, 0, 0 };
static sal_Unicode const aSimHei[] = { 0x9ED1, 0x4F53, 0, 0 };
static sal_Unicode const aSimKai[] = { 0x6977, 0x4F53, 0, 0 };
static sal_Unicode const azycjkSun[] = { 0x4E2D, 0x6613, 0x5B8B, 0x4F53, 0, 0 };
static sal_Unicode const azycjkHei[] = { 0x4E2D, 0x6613, 0x9ED1, 0x4F53, 0, 0 };
static sal_Unicode const azycjkKai[] = { 0x4E2D, 0x6613, 0x6977, 0x4F53, 0, 0 };
static sal_Unicode const aFZHei[] = { 0x65B9, 0x6B63, 0x9ED1, 0x4F53, 0, 0 };
static sal_Unicode const aFZKai[] = { 0x65B9, 0x6B63, 0x6977, 0x4F53, 0, 0 };
static sal_Unicode const aFZSongYI[] = { 0x65B9, 0x6B63, 0x5B8B, 0x4E00, 0, 0 };
static sal_Unicode const aFZShuSong[] = { 0x65B9, 0x6B63, 0x4E66, 0x5B8B, 0, 0 };
static sal_Unicode const aFZFangSong[] = { 0x65B9, 0x6B63, 0x4EFF, 0x5B8B, 0, 0 };
// Attention: this fonts includes the wrong encoding vector - so we double the names with correct and wrong encoding
// First one is the GB-Encoding (we think the correct one), second is the big5 encoded name
static sal_Unicode const aMHei[] = { 'm', 0x7B80, 0x9ED1, 0, 'm', 0x6F60, 0x7AAA, 0, 0 };
static sal_Unicode const aMKai[] = { 'm', 0x7B80, 0x6977, 0x566C, 0, 'm', 0x6F60, 0x7FF1, 0x628E, 0, 0 };
static sal_Unicode const aMSong[] = { 'm', 0x7B80, 0x5B8B, 0, 'm', 0x6F60, 0x51BC, 0, 0 };
static sal_Unicode const aCFangSong[] = { 'm', 0x7B80, 0x592B, 0x5B8B, 0, 'm', 0x6F60, 0x6E98, 0x51BC, 0, 0 };
static sal_Unicode const aMingLiU[] = { 0x7D30, 0x660E, 0x9AD4, 0, 0 };
static sal_Unicode const aPMingLiU[] = { 0x65B0, 0x7D30, 0x660E, 0x9AD4, 0, 0 };
static sal_Unicode const aHei[] = { 0x6865, 0, 0 };
static sal_Unicode const aKai[] = { 0x6B61, 0, 0 };
static sal_Unicode const aMing[] = { 0x6D69, 0x6E67, 0, 0 };
static sal_Unicode const aMSGothic[] = { 'm','s',       0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
static sal_Unicode const aMSPGothic[] = { 'm','s','p',  0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
static sal_Unicode const aMSMincho[] = { 'm', 's',      0x660E, 0x671D, 0 };
static sal_Unicode const aMSPMincho[] = { 'm','s','p',  0x660E, 0x671D, 0 };
static sal_Unicode const aMeiryo[]    = { 0x30e1, 0x30a4, 0x30ea, 0x30aa, 0 };
static sal_Unicode const aHGMinchoL[] = { 'h','g',      0x660E, 0x671D, 'l', 0, 0 };
static sal_Unicode const aHGGothicB[] = { 'h','g',      0x30B4, 0x30B7, 0x30C3, 0x30AF, 'b', 0 };
static sal_Unicode const aHGPMinchoL[] = { 'h','g','p', 0x660E, 0x671D, 'l', 0 };
static sal_Unicode const aHGPGothicB[] = { 'h','g','p', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 'b', 0 };
static sal_Unicode const aHGMinchoLSun[] = { 'h','g',   0x660E, 0x671D, 'l', 's', 'u', 'n', 0 };
static sal_Unicode const aHGPMinchoLSun[] = { 'h','g','p', 0x660E, 0x671D, 'l', 's', 'u', 'n', 0 };
static sal_Unicode const aHGGothicBSun[] = { 'h', 'g', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 'b', 's', 'u', 'n', 0 };
static sal_Unicode const aHGPGothicBSun[] = { 'h', 'g', 'p', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 'b', 's', 'u', 'n', 0 };
static sal_Unicode const aHGHeiseiMin[] = { 'h', 'g', 0x5E73, 0x6210, 0x660E, 0x671D, 0x4F53, 0, 'h', 'g', 0x5E73, 0x6210, 0x660E, 0x671D, 0x4F53, 'w', '3', 'x', '1', '2', 0, 0 };
static sal_Unicode const aIPAMincho[] =  { 'i', 'p', 'a', 0x660E, 0x671D, 0 };
static sal_Unicode const aIPAPMincho[] = { 'i', 'p', 'a', 'p', 0x660E, 0x671D, 0 };
static sal_Unicode const aIPAGothic[] =  { 'i', 'p', 'a',  0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 };
static sal_Unicode const aIPAPGothic[] =  { 'i', 'p', 'a', 'p', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 };
static sal_Unicode const aIPAUIGothic[] =  { 'i', 'p', 'a', 'u', 'i', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0 };
static sal_Unicode const aSazanamiMincho[] = { 0x3055, 0x3056, 0x306A, 0x307F, 0x660E, 0x671D, 0, 0 };
static sal_Unicode const aSazanamiGothic[] = { 0x3055, 0x3056, 0x306A, 0x307F, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
static sal_Unicode const aKochiMincho[] = { 0x6771, 0x98A8, 0x660E, 0x671D, 0, 0 };
static sal_Unicode const aKochiGothic[] = { 0x6771, 0x98A8, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
static sal_Unicode const aSunDotum[] = { 0xC36C, 0xB3CB, 0xC6C0, 0, 0 };
static sal_Unicode const aSunGulim[] = { 0xC36C, 0xAD74, 0xB9BC, 0, 0 };
static sal_Unicode const aSunBatang[] = { 0xC36C, 0xBC14, 0xD0D5, 0, 0 };
static sal_Unicode const aBaekmukDotum[] = { 0xBC31, 0xBB35, 0xB3CB, 0xC6C0, 0, 0 };
static sal_Unicode const aBaekmukGulim[] = { 0xBC31, 0xBB35, 0xAD74, 0xB9BC, 0, 0 };
static sal_Unicode const aBaekmukBatang[] = { 0xBC31, 0xBB35, 0xBC14, 0xD0D5, 0, 0 };
static sal_Unicode const aFzMingTi[] = { 0x65B9, 0x6B63, 0x660E, 0x9AD4, 0, 0 };
static sal_Unicode const aFzHeiTiTW[]= { 0x65B9, 0x6B63, 0x9ED1, 0x9AD4, 0, 0 };
static sal_Unicode const aFzKaiTiTW[]= { 0x65B9, 0x6B63, 0x6977, 0x9AD4, 0, 0 };
static sal_Unicode const aFzHeiTiCN[]= { 0x65B9, 0x6B63, 0x9ED1, 0x4F53, 0, 0 };
static sal_Unicode const aFzKaiTiCN[]= { 0x65B9, 0x6B63, 0x6977, 0x4F53, 0, 0 };
static sal_Unicode const aFzSongTi[] = { 0x65B9, 0x6B63, 0x5B8B, 0x4F53, 0, 0 };
static sal_Unicode const aHYMyeongJoExtra[]         = { 'h', 'y', 0xACAC, 0xBA85, 0xC870, 0, 0 };
static sal_Unicode const aHYSinMyeongJoMedium[]     = { 'h', 'y', 0xC2E0, 0xBA85, 0xC870, 0, 0 };
static sal_Unicode const aHYGothicMedium[]          = { 'h', 'y', 0xC911, 0xACE0, 0xB515, 0, 0 };
static sal_Unicode const aHYGraphicMedium[]         = { 'h', 'y', 0xADF8, 0xB798, 0xD53D, 'm', 0, 0 };
static sal_Unicode const aHYGraphic[]               = { 'h', 'y', 0xADF8, 0xB798, 0xD53D, 0, 0 };
static sal_Unicode const aNewGulim[]                = { 0xC0C8, 0xAD74, 0xB9BC, 0, 0 };
static sal_Unicode const aSunGungseo[]              = { 0xC36C, 0xAD81, 0xC11C, 0, 0 };
static sal_Unicode const aHYGungSoBold[]            = { 'h','y', 0xAD81, 0xC11C, 'b', 0, 0 };
static sal_Unicode const aHYGungSo[]                 = { 'h','y', 0xAD81, 0xC11C, 0, 0 };
static sal_Unicode const aSunHeadLine[]             = { 0xC36C, 0xD5E4, 0xB4DC, 0xB77C, 0xC778, 0, 0 };
static sal_Unicode const aHYHeadLineMedium[]        = { 'h', 'y', 0xD5E4, 0xB4DC, 0xB77C, 0xC778, 'm', 0, 0 };
static sal_Unicode const aHYHeadLine[]              = { 'h', 'y', 0xD5E4, 0xB4DC, 0xB77C, 0xC778, 0, 0 };
static sal_Unicode const aYetR[]                    = { 0xD734, 0xBA3C, 0xC61B, 0xCCB4, 0, 0 };
static sal_Unicode const aHYGothicExtra[]           = { 'h', 'y', 0xACAC, 0xACE0, 0xB515, 0, 0 };
static sal_Unicode const aSunMokPan[]               = { 0xC36C, 0xBAA9, 0xD310, 0, 0 };
static sal_Unicode const aSunYeopseo[]              = { 0xC36C, 0xC5FD, 0xC11C, 0, 0 };
static sal_Unicode const aSunBaekSong[]              = { 0xC36C, 0xBC31, 0xC1A1, 0, 0 };
static sal_Unicode const aHYPostLight[]             = { 'h', 'y', 0xC5FD, 0xC11C, 'l', 0, 0 };
static sal_Unicode const aHYPost[]                  = { 'h', 'y', 0xC5FD, 0xC11C, 0, 0 };
static sal_Unicode const aMagicR[]                  = { 0xD734, 0xBA3C, 0xB9E4, 0xC9C1, 0xCCB4, 0, 0 };
static sal_Unicode const aSunCrystal[]              = { 0xC36C, 0xD06C, 0xB9AC, 0xC2A4, 0xD0C8, 0, 0 };
static sal_Unicode const aSunSaemmul[]              = { 0xC36C, 0xC0D8, 0xBB3C, 0, 0 };
static sal_Unicode const aHaansoftBatang[]          = { 0xD55C, 0xCEF4, 0xBC14, 0xD0D5, 0, 0 };
static sal_Unicode const aHaansoftDotum[]           = { 0xD55C, 0xCEF4, 0xB3CB, 0xC6C0, 0, 0 };
static sal_Unicode const aHyhaeseo[]                = { 0xD55C, 0xC591, 0xD574, 0xC11C, 0, 0 };
static sal_Unicode const aMDSol[]                   = { 'm', 'd', 0xC194, 0xCCB4, 0, 0 };
static sal_Unicode const aMDGaesung[]               = { 'm', 'd', 0xAC1C, 0xC131, 0xCCB4, 0, 0 };
static sal_Unicode const aMDArt[]                   = { 'm', 'd', 0xC544, 0xD2B8, 0xCCB4, 0, 0 };
static sal_Unicode const aMDAlong[]                 = { 'm', 'd', 0xC544, 0xB871, 0xCCB4, 0, 0 };
static sal_Unicode const aMDEasop[]                 = { 'm', 'd', 0xC774, 0xC19D, 0xCCB4, 0, 0 };
static sal_Unicode const aHYShortSamulMedium[]      = { 'h', 'y', 0xC595, 0xC740, 0xC0D8, 0xBB3C, 'm', 0 };
static sal_Unicode const aHYShortSamul[]            = { 'h', 'y', 0xC595, 0xC740, 0xC0D8, 0xBB3C, 0 };
static sal_Unicode const aHGGothicE[]               = { 'h','g', 0xFF7A, 0xFF9E, 0xFF7C, 0xFF6F, 0xFF78, 'e', 0 };
static sal_Unicode const aHGPGothicE[]              = { 'h','g','p', 0xFF7A, 0xFF9E, 0xFF7C, 0xFF6F, 0xFF78, 'e', 0 };
static sal_Unicode const aHGSGothicE[]              = { 'h','g','s', 0xFF7A, 0xFF9E, 0xFF7C, 0xFF6F, 0xFF78, 'e', 0 };
static sal_Unicode const aHGGothicM[]               = { 'h','g', 0xFF7A, 0xFF9E, 0xFF7C, 0xFF6F, 0xFF78, 'm', 0 };
static sal_Unicode const aHGPGothicM[]              = { 'h','g','p', 0xFF7A, 0xFF9E, 0xFF7C, 0xFF6F, 0xFF78, 'm', 0 };
static sal_Unicode const aHGSGothicM[]              = { 'h','g','s', 0xFF7A, 0xFF9E, 0xFF7C, 0xFF6F, 0xFF78, 'm', 0 };
static sal_Unicode const aHGGyoshotai[]             = { 'h','g', 0x884C, 0x66F8, 0x4F53, 0 };
static sal_Unicode const aHGPGyoshotai[]            = { 'h','g','p', 0x884C, 0x66F8, 0x4F53, 0 };
static sal_Unicode const aHGSGyoshotai[]            = { 'h','g','s', 0x884C, 0x66F8, 0x4F53, 0 };
static sal_Unicode const aHGKyokashotai[]           = { 'h','g', 0x6559, 0x79D1, 0x66F8, 0x4F53, 0 };
static sal_Unicode const aHGPKyokashotai[]          = { 'h','g','p', 0x6559, 0x79D1, 0x66F8, 0x4F53, 0 };
static sal_Unicode const aHGSKyokashotai[]          = { 'h','g','s', 0x6559, 0x79D1, 0x66F8, 0x4F53, 0 };
static sal_Unicode const aHGMinchoB[]               = { 'h','g', 0x660E, 0x671D, 'b', 0 };
static sal_Unicode const aHGPMinchoB[]              = { 'h','g','p', 0x660E, 0x671D, 'b', 0 };
static sal_Unicode const aHGSMinchoB[]              = { 'h','g','s', 0x660E, 0x671D, 'b', 0 };
static sal_Unicode const aHGMinchoE[]               = { 'h','g', 0x660E, 0x671D, 'e', 0 };
static sal_Unicode const aHGPMinchoE[]              = { 'h','g','p', 0x660E, 0x671D, 'e', 0 };
static sal_Unicode const aHGSMinchoE[]              = { 'h','g','s', 0x660E, 0x671D, 'e', 0 };
static sal_Unicode const aHGSoeiKakupoptai[]        = { 'h','g', 0x5275,0x82F1,0x89D2,0xFF8E,
							0xFF9F,0xFF6F,0xFF8C,0xFF9F,0x4F53,0};
static sal_Unicode const aHGPSoeiKakupoptai[]       = { 'h','g', 'p', 0x5275,0x82F1,0x89D2,0xFF8E,
							0xFF9F,0xFF6F,0xFF8C,0xFF9F,0x4F53,0};
static sal_Unicode const aHGSSoeiKakupoptai[]       = { 'h','g', 's', 0x5275,0x82F1,0x89D2,0xFF8E,
							0xFF9F,0xFF6F,0xFF8C,0xFF9F,0x4F53,0};
static sal_Unicode const aHGSoeiPresenceEB[]        = { 'h','g', 0x5275,0x82F1,0xFF8C,0xFF9F,
							0xFF9A,0xFF7E,0xFF9E,0xFF9D,0xFF7D, 'e','b',0};
static sal_Unicode const aHGPSoeiPresenceEB[]       = { 'h','g','p', 0x5275,0x82F1,0xFF8C,0xFF9F,
							0xFF9A,0xFF7E,0xFF9E,0xFF9D,0xFF7D, 'e','b',0};
static sal_Unicode const aHGSSoeiPresenceEB[]       = { 'h','g','s', 0x5275,0x82F1,0xFF8C,0xFF9F,
							0xFF9A,0xFF7E,0xFF9E,0xFF9D,0xFF7D, 'e','b',0};
static sal_Unicode const aHGSoeiKakugothicUB[]      = { 'h','g', 0x5275,0x82F1,0x89D2,0xFF7A,
							0xFF9E,0xFF7C,0xFF6F,0xFF78,'u','b',0};
static sal_Unicode const aHGPSoeiKakugothicUB[]     = { 'h','g','p', 0x5275,0x82F1,0x89D2,0xFF7A,
							0xFF9E,0xFF7C,0xFF6F,0xFF78,'u','b',0};
static sal_Unicode const aHGSSoeiKakugothicUB[]     = { 'h','g','s', 0x5275,0x82F1,0x89D2,0xFF7A,
							0xFF9E,0xFF7C,0xFF6F,0xFF78,'u','b',0};
static sal_Unicode const aHGSeikaishotaiPRO[]       = { 'h','g', 0x6B63,0x6977,0x66F8,0x4F53, '-','p','r','o',0};
static sal_Unicode const aHGMaruGothicMPRO[]        = { 'h','g', 0x4E38,0xFF7A,0xFF9E,0xFF7C,0xFF6F,0xFF78, '-','p','r','o',0};
static sal_Unicode const aHiraginoMinchoPro[]       = { 0x30D2, 0x30E9, 0x30AE, 0x30CE, 0x660E, 0x671D, 'p','r','o',0};
static sal_Unicode const aHiraginoMinchoProN[]      = { 0x30D2, 0x30E9, 0x30AE, 0x30CE, 0x660E, 0x671D, 'p','r','o','n',0};
static sal_Unicode const aHiraginoKakuGothicPro[]   = { 0x30D2, 0x30E9, 0x30AE, 0x30CE, 0x89D2, 0x30B4, 'p','r','o',0};
static sal_Unicode const aHiraginoKakuGothicProN[]  = { 0x30D2, 0x30E9, 0x30AE, 0x30CE, 0x89D2, 0x30B4, 'p','r','o','n',0};
static sal_Unicode const aHiraginoMaruGothicPro[]   = { 0x30D2, 0x30E9, 0x30AE, 0x30CE, 0x4E38, 0x30B4, 'p','r','o',0};
static sal_Unicode const aHiraginoMaruGothicProN[]  = { 0x30D2, 0x30E9, 0x30AE, 0x30CE, 0x4E38, 0x30B4, 'p','r','o','n',0};


static ImplLocalizedFontName aImplLocalizedNamesList[] =
{
{   "batang",               aBatang },
{   "batangche",            aBatangChe },
{   "gungshu",              aGungsuh },
{   "gungshuche",           aGungsuhChe },
{   "gulim",                aGulim },
{   "gulimche",             aGulimChe },
{   "dotum",                aDotum },
{   "dotumche",             aDotumChe },
{   "simsun",               aSimSun },
{   "nsimsun",              aNSimSun },
{   "simhei",               aSimHei },
{   "simkai",               aSimKai },
{   "zycjksun",             azycjkSun },
{   "zycjkhei",             azycjkHei },
{   "zycjkkai",             azycjkKai },
{   "fzhei",                aFZHei },
{   "fzkai",                aFZKai },
{   "fzsong",               aFZSongYI },
{   "fzshusong",            aFZShuSong },
{   "fzfangsong",           aFZFangSong },
{   "mhei",                 aMHei },
{   "mkai",                 aMKai },
{   "msong",                aMSong },
{   "cfangsong",            aCFangSong },
{   "mingliu",              aMingLiU },
{   "pmingliu",             aPMingLiU },
{   "hei",                  aHei },
{   "kai",                  aKai },
{   "ming",                 aMing },
{   "msgothic",             aMSGothic },
{   "mspgothic",            aMSPGothic },
{   "msmincho",             aMSMincho },
{   "mspmincho",            aMSPMincho },
{   "meiryo",               aMeiryo },
{   "hgminchol",            aHGMinchoL },
{   "hggothicb",            aHGGothicB },
{   "hgpminchol",           aHGPMinchoL },
{   "hgpgothicb",           aHGPGothicB },
{   "hgmincholsun",         aHGMinchoLSun },
{   "hggothicbsun",         aHGGothicBSun },
{   "hgpmincholsun",        aHGPMinchoLSun },
{   "hgpgothicbsun",        aHGPGothicBSun },
{   "hgheiseimin",          aHGHeiseiMin },
{   "ipamincho",            aIPAMincho },
{   "ipapmincho",           aIPAPMincho },
{   "ipagothic",            aIPAGothic },
{   "ipapgothic",           aIPAPGothic },
{   "ipauigothic",          aIPAUIGothic },
{   "sazanamimincho",       aSazanamiMincho },
{   "sazanamigothic",       aSazanamiGothic },
{   "kochimincho",          aKochiMincho },
{   "kochigothic",          aKochiGothic },
{   "sundotum",             aSunDotum },
{   "sungulim",             aSunGulim },
{   "sunbatang",            aSunBatang },
{   "baekmukdotum",         aBaekmukDotum },
{   "baekmukgulim",         aBaekmukGulim },
{   "baekmukbatang",        aBaekmukBatang },
{   "fzheiti",              aFzHeiTiCN },
{   "fzheiti",              aFzHeiTiTW },
{   "fzkaiti",              aFzKaiTiCN },
{   "fzkaitib",             aFzKaiTiTW },
{   "fzmingtib",            aFzMingTi },
{   "fzsongti",             aFzSongTi },
{   "hymyeongjoextra",      aHYMyeongJoExtra },
{   "hysinmyeongjomedium",  aHYSinMyeongJoMedium },
{   "hygothicmedium",       aHYGothicMedium },
{   "hygraphicmedium",      aHYGraphicMedium },
{   "hygraphic",            aHYGraphic },
{   "newgulim",             aNewGulim },
{   "sungungseo",           aSunGungseo },
{   "hygungsobold",         aHYGungSoBold },
{   "hygungso",             aHYGungSo },
{   "sunheadline",          aSunHeadLine },
{   "hyheadlinemedium",     aHYHeadLineMedium },
{   "hyheadline",           aHYHeadLine },
{   "yetr",                 aYetR },
{   "hygothicextra",        aHYGothicExtra },
{   "sunmokpan",            aSunMokPan },
{   "sunyeopseo",           aSunYeopseo },
{   "sunbaeksong",          aSunBaekSong },
{   "hypostlight",          aHYPostLight },
{   "hypost",               aHYPost },
{   "magicr",               aMagicR },
{   "suncrystal",           aSunCrystal },
{   "sunsaemmul",           aSunSaemmul },
{   "hyshortsamulmedium",   aHYShortSamulMedium },
{   "hyshortsamul",         aHYShortSamul },
{   "haansoftbatang",       aHaansoftBatang },
{   "haansoftdotum",        aHaansoftDotum },
{   "hyhaeseo",             aHyhaeseo },
{   "mdsol",                aMDSol },
{   "mdgaesung",            aMDGaesung },
{   "mdart",                aMDArt },
{   "mdalong",              aMDAlong },
{   "mdeasop",              aMDEasop },
{   "hggothice",            aHGGothicE },
{   "hgpgothice",           aHGPGothicE },
{   "hgpothice",            aHGSGothicE },
{   "hggothicm",            aHGGothicM },
{   "hgpgothicm",           aHGPGothicM },
{   "hgpgothicm",           aHGSGothicM },
{   "hggyoshotai",          aHGGyoshotai },
{   "hgpgyoshotai",         aHGPGyoshotai },
{   "hgsgyoshotai",         aHGSGyoshotai },
{   "hgkyokashotai",        aHGKyokashotai },
{   "hgpkyokashotai",       aHGPKyokashotai },
{   "hgskyokashotai",       aHGSKyokashotai },
{   "hgminchob",            aHGMinchoB },
{   "hgpminchob",           aHGPMinchoB },
{   "hgsminchob",           aHGSMinchoB },
{   "hgminchoe",            aHGMinchoE },
{   "hgpminchoe",           aHGPMinchoE },
{   "hgsminchoe",           aHGSMinchoE },
{   "hgsoeikakupoptai",     aHGSoeiKakupoptai },
{   "hgpsoeikakupopta",     aHGPSoeiKakupoptai },
{   "hgssoeikakupopta",     aHGSSoeiKakupoptai },
{   "hgsoeipresenceeb",     aHGSoeiPresenceEB },
{   "hgpsoeipresenceeb",    aHGPSoeiPresenceEB },
{   "hgssoeipresenceeb",    aHGSSoeiPresenceEB },
{   "hgsoeikakugothicub",   aHGSoeiKakugothicUB },
{   "hgpsoeikakugothicub",  aHGPSoeiKakugothicUB },
{   "hgssoeikakugothicub",  aHGSSoeiKakugothicUB },
{   "hgseikaishotaipro",    aHGSeikaishotaiPRO },
{   "hgmarugothicmpro",     aHGMaruGothicMPRO },
{   "hiraginominchopro",    aHiraginoMinchoPro },
{   "hiraginominchopron",   aHiraginoMinchoProN },
{   "hiraginokakugothicpro", aHiraginoKakuGothicPro },
{   "hiraginokakugothicpron", aHiraginoKakuGothicProN },
{   "hiraginomarugothicpro", aHiraginoMaruGothicPro },
{   "hiraginomarugothicpron", aHiraginoMaruGothicProN },
{   NULL,                   NULL },
};

// -----------------------------------------------------------------------

void ImplGetEnglishSearchFontName( String& rName )
{
    bool        bNeedTranslation = false;
    xub_StrLen  nLen = rName.Len();

    // Remove trailing whitespaces
    xub_StrLen i = nLen;
    while ( i && (rName.GetChar( i-1 ) < 32) )
        i--;
    if ( i != nLen )
        rName.Erase( i );

    // Remove Script at the end
    // Scriptname must be the last part of the fontname and
    // looks like "fontname (scriptname)". So there can only be a
    // script name at the and of the fontname, when the last char is ')'
    if ( (nLen >= 3) && rName.GetChar( nLen-1 ) == ')' )
    {
        int nOpen = 1;
        xub_StrLen nTempLen = nLen-2;
        while ( nTempLen )
        {
            if ( rName.GetChar( nTempLen ) == '(' )
            {
                nOpen--;
                if ( !nOpen )
                {
                    // Remove Space at the end
                    if ( nTempLen && (rName.GetChar( nTempLen-1 ) == ' ') )
                        nTempLen--;
                    rName.Erase( nTempLen );
                    nLen = nTempLen;
                    break;
                }
            }
            if ( rName.GetChar( nTempLen ) == ')' )
                nOpen++;
            nTempLen--;
        }
    }

    // remove all whitespaces and converts to lower case ASCII
    // TODO: better transliteration to ASCII e.g. all digits
    i = 0;
    while ( i < nLen )
    {
        sal_Unicode c = rName.GetChar( i );
        if ( c > 127 )
        {
            // Translate to Lowercase-ASCII
            // FullWidth-ASCII to half ASCII
            if ( (c >= 0xFF00) && (c <= 0xFF5E) )
            {
                c -= 0xFF00-0x0020;
                // Upper to Lower
                if ( (c >= 'A') && (c <= 'Z') )
                    c += 'a' - 'A';
                rName.SetChar( i, c );
            }
            else
            {
                // Only Fontnames with None-Ascii-Characters must be translated
                bNeedTranslation = true;
            }
        }
        // not lowercase Ascii
        else if ( !((c >= 'a') && (c <= 'z')) )
        {
            // To Lowercase-Ascii
            if ( (c >= 'A') && (c <= 'Z') )
            {
                c += 'a' - 'A';
                rName.SetChar( i, c );
            }
            else if( ((c < '0') || (c > '9')) && (c != ';') ) // not 0-9 or semicolon
            {
                // Remove white spaces and special characters
                rName.Erase( i, 1 );
                nLen--;
                continue;
            }
        }

        i++;
    }

    // translate normalized localized name to its normalized English ASCII name
    if( bNeedTranslation )
    {
        typedef std::hash_map<const String, const char*,FontNameHash> FontNameDictionary;
        static FontNameDictionary aDictionary( sizeof(aImplLocalizedNamesList) / sizeof(*aImplLocalizedNamesList) );
        // the font name dictionary needs to be intialized once
        if( aDictionary.empty() )
        {
            // TODO: check if all dictionary entries are already normalized?
            const ImplLocalizedFontName* pList = aImplLocalizedNamesList;
            for(; pList->mpEnglishName; ++pList )
                aDictionary[ pList->mpLocalizedNames ] = pList->mpEnglishName;
        }

        FontNameDictionary::const_iterator it = aDictionary.find( rName );
        if( it != aDictionary.end() )
            rName.AssignAscii( it->second );
    }
}

// -----------------------------------------------------------------------

static String GetNextFontToken( const String& rTokenStr, xub_StrLen& rIndex )
{
    // check for valid start index
    int nStringLen = rTokenStr.Len();
    if( rIndex >= nStringLen )
    {
        rIndex = STRING_NOTFOUND;
        return String();
    }

    // find the next token delimiter and return the token substring
    const sal_Unicode* pStr = rTokenStr.GetBuffer() + rIndex;
    const sal_Unicode* pEnd = rTokenStr.GetBuffer() + nStringLen;
    for(; pStr < pEnd; ++pStr )
        if( (*pStr == ';') || (*pStr == ',') )
            break;

    xub_StrLen nTokenStart = rIndex;
    xub_StrLen nTokenLen;
    if( pStr < pEnd )
    {
        rIndex = sal::static_int_cast<xub_StrLen>(pStr - rTokenStr.GetBuffer());
        nTokenLen = rIndex - nTokenStart;
        ++rIndex; // skip over token separator
    }
    else
    {
        // no token delimiter found => handle last token
        rIndex = STRING_NOTFOUND;
        nTokenLen = STRING_LEN;

        // optimize if the token string consists of just one token
        if( !nTokenStart )
            return rTokenStr;
    }

    return String( rTokenStr, nTokenStart, nTokenLen );
}

// TODO: get rid of this in another incompatible build with SW project.
// SW's WW8 and RTF filters still use this (from fontcvt.hxx)
String GetFontToken( const String& rTokenStr, xub_StrLen nToken, xub_StrLen& rIndex )
{
    // skip nToken Tokens
    for( xub_StrLen i = 0; (i < nToken) && (rIndex != STRING_NOTFOUND); ++i )
        GetNextFontToken( rTokenStr, rIndex );

    return GetNextFontToken( rTokenStr, rIndex );
}

// =======================================================================

// TODO: remove this method when the CWS-gfbfcfg dust has settled
void ImplFreeOutDevFontData()
{}

// =======================================================================

void OutputDevice::BeginFontSubstitution()
{
    ImplSVData* pSVData = ImplGetSVData();
    pSVData->maGDIData.mbFontSubChanged = FALSE;
}

// -----------------------------------------------------------------------

void OutputDevice::EndFontSubstitution()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( pSVData->maGDIData.mbFontSubChanged )
    {
        ImplUpdateAllFontData( false );

        Application* pApp = GetpApp();
        DataChangedEvent aDCEvt( DATACHANGED_FONTSUBSTITUTION );
        pApp->DataChanged( aDCEvt );
        pApp->NotifyAllWindows( aDCEvt );
        pSVData->maGDIData.mbFontSubChanged = FALSE;
    }
}

// -----------------------------------------------------------------------

void OutputDevice::AddFontSubstitute( const XubString& rFontName,
                                      const XubString& rReplaceFontName,
                                      USHORT nFlags )
{
    ImplDirectFontSubstitution*& rpSubst = ImplGetSVData()->maGDIData.mpDirectFontSubst;
    if( !rpSubst )
        rpSubst = new ImplDirectFontSubstitution();
    rpSubst->AddFontSubstitute( rFontName, rReplaceFontName, nFlags );
    ImplGetSVData()->maGDIData.mbFontSubChanged = TRUE;
}

// -----------------------------------------------------------------------

void ImplDirectFontSubstitution::AddFontSubstitute( const String& rFontName,
	const String& rSubstFontName, USHORT nFlags )
{
    maFontSubstList.push_back( ImplFontSubstEntry( rFontName, rSubstFontName, nFlags ) );
}

// -----------------------------------------------------------------------

ImplFontSubstEntry::ImplFontSubstEntry( const String& rFontName,
    const String& rSubstFontName, USHORT nSubstFlags )
:   maName( rFontName )
,   maReplaceName( rSubstFontName )
,   mnFlags( nSubstFlags )
{
    maSearchName        = rFontName;
    maSearchReplaceName = rSubstFontName;
    ImplGetEnglishSearchFontName( maSearchName );
    ImplGetEnglishSearchFontName( maSearchReplaceName );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplAddDevFontSubstitute( const XubString& rFontName,
                                             const XubString& rReplaceFontName,
                                             USHORT nFlags )
{
    ImplInitOutDevData();
    mpOutDevData->maDevFontSubst.AddFontSubstitute( rFontName, rReplaceFontName, nFlags );
}

// -----------------------------------------------------------------------

void OutputDevice::RemoveFontSubstitute( USHORT n )
{
    ImplDirectFontSubstitution* pSubst = ImplGetSVData()->maGDIData.mpDirectFontSubst;
    if( pSubst )
        pSubst->RemoveFontSubstitute( n );
}

// -----------------------------------------------------------------------

void ImplDirectFontSubstitution::RemoveFontSubstitute( int nIndex )
{
    FontSubstList::iterator it = maFontSubstList.begin();
    for( int nCount = 0; (it != maFontSubstList.end()) && (nCount++ != nIndex); ++it ) ;
    if( it != maFontSubstList.end() )
        maFontSubstList.erase( it );
}

// -----------------------------------------------------------------------

USHORT OutputDevice::GetFontSubstituteCount()
{
    const ImplDirectFontSubstitution* pSubst = ImplGetSVData()->maGDIData.mpDirectFontSubst;
    if( !pSubst )
	return 0;
    int nCount =  pSubst->GetFontSubstituteCount();
    return (USHORT)nCount;
}

// -----------------------------------------------------------------------

void OutputDevice::GetFontSubstitute( USHORT n,
                                      XubString& rFontName,
                                      XubString& rReplaceFontName,
                                      USHORT& rFlags )
{
    const ImplDirectFontSubstitution* pSubst = ImplGetSVData()->maGDIData.mpDirectFontSubst;
    if( pSubst )
        pSubst->GetFontSubstitute( n, rFontName, rReplaceFontName, rFlags );
}

// -----------------------------------------------------------------------

bool ImplDirectFontSubstitution::GetFontSubstitute( int nIndex,
    String& rFontName, String& rSubstFontName, USHORT& rFlags ) const
{
    FontSubstList::const_iterator it = maFontSubstList.begin();
	for( int nCount = 0; (it != maFontSubstList.end()) && (nCount++ != nIndex); ++it ) ;    
	if( it == maFontSubstList.end() )
	    return false;

    const ImplFontSubstEntry* pEntry = &(*it);
    rFontName       = pEntry->maName;
    rSubstFontName  = pEntry->maReplaceName;
    rFlags          = pEntry->mnFlags;
    return true;
}

// -----------------------------------------------------------------------

bool ImplDirectFontSubstitution::FindFontSubstitute( String& rSubstName,
    const String& rSearchName, USHORT nFlags ) const
{
    // TODO: get rid of O(N) searches
    FontSubstList::const_iterator it = maFontSubstList.begin();
    for(; it != maFontSubstList.end(); ++it )
    {
        const ImplFontSubstEntry& rEntry = *it;
        if( ((rEntry.mnFlags & nFlags) || !nFlags)
        &&   (rEntry.maSearchName == rSearchName) )
        {
            rSubstName = rEntry.maSearchReplaceName;
            return true;
        }
    }

    return false;
}

// -----------------------------------------------------------------------

static void ImplFontSubstitute( String& rFontName,
    USHORT nFlags, ImplDirectFontSubstitution* pDevSpecific )
{
#ifdef DBG_UTIL
    String aTempName = rFontName;
    ImplGetEnglishSearchFontName( aTempName );
    DBG_ASSERT( aTempName == rFontName, "ImplFontSubstitute() called without a searchname" );
#endif

    String aSubstFontName;

    // apply user-configurable font replacement (eg, from the list in Tools->Options)
    const ImplDirectFontSubstitution* pSubst = ImplGetSVData()->maGDIData.mpDirectFontSubst;
    if( pSubst && pSubst->FindFontSubstitute( aSubstFontName, rFontName, FONT_SUBSTITUTE_ALWAYS ) )
    {
        rFontName = aSubstFontName;
        return;
    }

    // apply device specific font replacement (e.g. to use printer builtin fonts)
    if( !pDevSpecific )
        return;

    if( pDevSpecific->FindFontSubstitute( aSubstFontName, rFontName, nFlags ) )
    {
        rFontName = aSubstFontName;
        return;
    }
}

// =======================================================================

static bool ImplIsFontToken( const String& rName, const String& rToken )
{
    String      aTempName;
    xub_StrLen  nIndex = 0;
    do
    {
        aTempName = GetNextFontToken( rName, nIndex );
        if ( rToken == aTempName )
            return true;
    }
    while ( nIndex != STRING_NOTFOUND );

    return false;
}

// -----------------------------------------------------------------------

static void ImplAppendFontToken( String& rName, const String& rNewToken )
{
    if ( rName.Len() )
    {
        rName.Append( ';' );
        rName.Append( rNewToken );
    }
    else
        rName = rNewToken;
}

// -----------------------------------------------------------------------

static void ImplAddTokenFontName( String& rName, const String& rNewToken )
{
    if ( !ImplIsFontToken( rName, rNewToken ) )
        ImplAppendFontToken( rName, rNewToken );
}

// -----------------------------------------------------------------------

Font OutputDevice::GetDefaultFont( USHORT nType, LanguageType eLang,
                                   ULONG nFlags, const OutputDevice* pOutDev )
{
    DBG_TRACE( "OutputDevice::GetDefaultFont()" );

    com::sun::star::lang::Locale aLocale;
    if( eLang == LANGUAGE_NONE || eLang == LANGUAGE_SYSTEM || eLang == LANGUAGE_DONTKNOW )
    {
        aLocale = Application::GetSettings().GetUILocale();
    }
    else
    {
        MsLangId::convertLanguageToLocale( eLang, aLocale );
    }

    DefaultFontConfiguration& rDefaults = *DefaultFontConfiguration::get();
    String aSearch = rDefaults.getUserInterfaceFont( aLocale ); // ensure a fallback
    String aDefault = rDefaults.getDefaultFont( aLocale, nType );
    if( aDefault.Len() )
        aSearch = aDefault;

    int nDefaultHeight = 12;

	Font aFont;
    aFont.SetPitch( PITCH_VARIABLE );

    switch ( nType )
    {
        case DEFAULTFONT_SANS_UNICODE:
        case DEFAULTFONT_UI_SANS:
            aFont.SetFamily( FAMILY_SWISS );
            break;

        case DEFAULTFONT_SANS:
        case DEFAULTFONT_LATIN_HEADING:
        case DEFAULTFONT_LATIN_SPREADSHEET:
        case DEFAULTFONT_LATIN_DISPLAY:
            aFont.SetFamily( FAMILY_SWISS );
            break;

        case DEFAULTFONT_SERIF:
        case DEFAULTFONT_LATIN_TEXT:
        case DEFAULTFONT_LATIN_PRESENTATION:
            aFont.SetFamily( FAMILY_ROMAN );
            break;

        case DEFAULTFONT_FIXED:
        case DEFAULTFONT_LATIN_FIXED:
        case DEFAULTFONT_UI_FIXED:
            aFont.SetPitch( PITCH_FIXED );
            aFont.SetFamily( FAMILY_MODERN );
            break;

        case DEFAULTFONT_SYMBOL:
            aFont.SetCharSet( RTL_TEXTENCODING_SYMBOL );
            break;

        case DEFAULTFONT_CJK_TEXT:
        case DEFAULTFONT_CJK_PRESENTATION:
        case DEFAULTFONT_CJK_SPREADSHEET:
        case DEFAULTFONT_CJK_HEADING:
        case DEFAULTFONT_CJK_DISPLAY:
            aFont.SetFamily( FAMILY_SYSTEM );	// don't care, but don't use font subst config later...
            break;

        case DEFAULTFONT_CTL_TEXT:
        case DEFAULTFONT_CTL_PRESENTATION:
        case DEFAULTFONT_CTL_SPREADSHEET:
        case DEFAULTFONT_CTL_HEADING:
        case DEFAULTFONT_CTL_DISPLAY:
            aFont.SetFamily( FAMILY_SYSTEM );	// don't care, but don't use font subst config later...
            break;
    }

    if ( aSearch.Len() )
    {
        aFont.SetHeight( nDefaultHeight );
        aFont.SetWeight( WEIGHT_NORMAL );

		if ( aFont.GetCharSet() == RTL_TEXTENCODING_DONTKNOW )
            aFont.SetCharSet( gsl_getSystemTextEncoding() );

        // Should we only return available fonts on the given device
        if ( pOutDev )
        {
            pOutDev->ImplInitFontList();

            // Search Font in the FontList
            String      aName;
            String      aSearchName;
            xub_StrLen  nIndex = 0;
            do
            {
                aSearchName = GetNextFontToken( aSearch, nIndex );
                ImplGetEnglishSearchFontName( aSearchName );
                ImplDevFontListData* pFontFamily = pOutDev->mpFontList->ImplFindBySearchName( aSearchName );
                if( pFontFamily )
                {
                    ImplAddTokenFontName( aName, pFontFamily->GetFamilyName() );
                    if( nFlags & DEFAULTFONT_FLAGS_ONLYONE )
                        break;
                }
            }
            while ( nIndex != STRING_NOTFOUND );
            aFont.SetName( aName );
        }

        // No Name, than set all names
        if ( !aFont.GetName().Len() )
        {
            xub_StrLen nIndex = 0;
            if ( nFlags & DEFAULTFONT_FLAGS_ONLYONE )
            {
                //aFont.SetName( aSearch.GetToken( 0, ';', nIndex ) );
                if( !pOutDev )
                    pOutDev = (const OutputDevice *)ImplGetSVData()->mpDefaultWin;
                if( !pOutDev )
                    aFont.SetName( aSearch.GetToken( 0, ';', nIndex ) );
                else
                {
                    pOutDev->ImplInitFontList();

                    aFont.SetName( aSearch );

                    // convert to pixel height
                    Size aSize = pOutDev->ImplLogicToDevicePixel( aFont.GetSize() );
                    if ( !aSize.Height() )
                    {
                        // use default pixel height only when logical height is zero
                        if ( aFont.GetHeight() )
                            aSize.Height() = 1;
                        else
                            aSize.Height() = (12*pOutDev->mnDPIY)/72;
                    }

                    // use default width only when logical width is zero
                    if( (0 == aSize.Width()) && (0 != aFont.GetSize().Width()) )
                        aSize.Width() = 1;

                    // get the name of the first available font
                    float fExactHeight = static_cast<float>(aSize.Height());
                    ImplFontEntry* pEntry = pOutDev->mpFontCache->GetFontEntry( pOutDev->mpFontList, aFont, aSize, fExactHeight, pOutDev->mpOutDevData ? &pOutDev->mpOutDevData->maDevFontSubst : NULL );
                    if( pEntry->maFontSelData.mpFontData )
                        aFont.SetName( pEntry->maFontSelData.mpFontData->maName );
                    else
                        aFont.SetName( pEntry->maFontSelData.maTargetName );
                }
            }
            else
                aFont.SetName( aSearch );
        }
    }

#if OSL_DEBUG_LEVEL > 2
    const char* s = "DEFAULTFONT_SANS_UNKNOWN";
    switch ( nType )
    {
	case DEFAULTFONT_SANS_UNICODE:	s = "DEFAULTFONT_SANS_UNICODE"; break;
	case DEFAULTFONT_UI_SANS:	s = "DEFAULTFONT_UI_SANS"; break;

	case DEFAULTFONT_SANS:	s = "DEFAULTFONT_SANS"; break;
	case DEFAULTFONT_LATIN_HEADING:	s = "DEFAULTFONT_LATIN_HEADING"; break;
	case DEFAULTFONT_LATIN_SPREADSHEET:	s = "DEFAULTFONT_LATIN_SPREADSHEET"; break;
	case DEFAULTFONT_LATIN_DISPLAY:	s = "DEFAULTFONT_LATIN_DISPLAY"; break;

	case DEFAULTFONT_SERIF:	s = "DEFAULTFONT_SERIF"; break;
	case DEFAULTFONT_LATIN_TEXT:	s = "DEFAULTFONT_LATIN_TEXT"; break;
	case DEFAULTFONT_LATIN_PRESENTATION:	s = "DEFAULTFONT_LATIN_PRESENTATION"; break;

	case DEFAULTFONT_FIXED:	s = "DEFAULTFONT_FIXED"; break;
	case DEFAULTFONT_LATIN_FIXED:	s = "DEFAULTFONT_LATIN_FIXED"; break;
	case DEFAULTFONT_UI_FIXED:	s = "DEFAULTFONT_UI_FIXED"; break;

	case DEFAULTFONT_SYMBOL:	s = "DEFAULTFONT_SYMBOL"; break;

	case DEFAULTFONT_CJK_TEXT:	s = "DEFAULTFONT_CJK_TEXT"; break;
	case DEFAULTFONT_CJK_PRESENTATION:	s = "DEFAULTFONT_CJK_PRESENTATION"; break;
	case DEFAULTFONT_CJK_SPREADSHEET:	s = "DEFAULTFONT_CJK_SPREADSHEET"; break;
	case DEFAULTFONT_CJK_HEADING:	s = "DEFAULTFONT_CJK_HEADING"; break;
	case DEFAULTFONT_CJK_DISPLAY:	s = "DEFAULTFONT_CJK_DISPLAY"; break;

	case DEFAULTFONT_CTL_TEXT:	s = "DEFAULTFONT_CTL_TEXT"; break;
	case DEFAULTFONT_CTL_PRESENTATION:	s = "DEFAULTFONT_CTL_PRESENTATION"; break;
	case DEFAULTFONT_CTL_SPREADSHEET:	s = "DEFAULTFONT_CTL_SPREADSHEET"; break;
	case DEFAULTFONT_CTL_HEADING:	s = "DEFAULTFONT_CTL_HEADING"; break;
	case DEFAULTFONT_CTL_DISPLAY:	s = "DEFAULTFONT_CTL_DISPLAY"; break;
    }
    fprintf( stderr, "   OutputDevice::GetDefaultFont() Type=\"%s\" lang=%d flags=%ld FontName=\"%s\"\n",
	     s, eLang, nFlags,
	     OUStringToOString( aFont.GetName(), RTL_TEXTENCODING_UTF8 ).getStr()
	     );
#endif

    return aFont;
}

// =======================================================================

String GetSubsFontName( const String& rName, ULONG nFlags )
{
    String aName;

    xub_StrLen nIndex = 0;
    String aOrgName = GetNextFontToken( rName, nIndex );
    ImplGetEnglishSearchFontName( aOrgName );

    // #93662# do not try to replace StarSymbol with MS only font
    if( nFlags == (SUBSFONT_MS|SUBSFONT_ONLYONE)
    &&  ( aOrgName.EqualsAscii( "starsymbol" )
      ||  aOrgName.EqualsAscii( "opensymbol" ) ) )
        return aName;

    const FontNameAttr* pAttr = FontSubstConfiguration::get()->getSubstInfo( aOrgName );
    if ( pAttr )
    {
        for( int i = 0; i < 3; i++ )
        {
            const ::std::vector< String >* pVector = NULL;
            switch( i )
            {
                case 0:
                    if( nFlags & SUBSFONT_MS  &&  pAttr->MSSubstitutions.size() )
                        pVector = &pAttr->MSSubstitutions;
                    break;
                case 1:
                    if( nFlags & SUBSFONT_PS  &&  pAttr->PSSubstitutions.size() )
                        pVector = &pAttr->PSSubstitutions;
                    break;
                case 2:
                    if( nFlags & SUBSFONT_HTML  &&  pAttr->HTMLSubstitutions.size() )
                        pVector = &pAttr->HTMLSubstitutions;
                    break;
            }
            if( ! pVector )
                continue;
            for( ::std::vector< String >::const_iterator it = pVector->begin(); it != pVector->end(); ++it )
                if( ! ImplIsFontToken( rName, *it ) )
                {
                    ImplAppendFontToken( aName, *it );
                    if( nFlags & SUBSFONT_ONLYONE )
                    {
                        i = 4;
                        break;
                    }
                }
        }
    }

    return aName;
}

// =======================================================================

static unsigned ImplIsCJKFont( const String& rFontName )
{
    // Test, if Fontname includes CJK characters --> In this case we
    // mention that it is a CJK font
    const sal_Unicode* pStr = rFontName.GetBuffer();
    while ( *pStr )
    {
        // japanese
        if ( ((*pStr >= 0x3040) && (*pStr <= 0x30FF)) ||
             ((*pStr >= 0x3190) && (*pStr <= 0x319F)) )
            return IMPL_FONT_ATTR_CJK|IMPL_FONT_ATTR_CJK_JP;

        // korean
        if ( ((*pStr >= 0xAC00) && (*pStr <= 0xD7AF)) ||
             ((*pStr >= 0x3130) && (*pStr <= 0x318F)) ||
             ((*pStr >= 0x1100) && (*pStr <= 0x11FF)) )
            return IMPL_FONT_ATTR_CJK|IMPL_FONT_ATTR_CJK_KR;

        // chinese
        if ( ((*pStr >= 0x3400) && (*pStr <= 0x9FFF)) )
            return IMPL_FONT_ATTR_CJK|IMPL_FONT_ATTR_CJK_TC|IMPL_FONT_ATTR_CJK_SC;

        // cjk
        if ( ((*pStr >= 0x3000) && (*pStr <= 0xD7AF)) ||
             ((*pStr >= 0xFF00) && (*pStr <= 0xFFEE)) )
            return IMPL_FONT_ATTR_CJK;

        pStr++;
    }

    return 0;
}

// -----------------------------------------------------------------------

static void ImplCalcType( ULONG& rType, FontWeight& rWeight, FontWidth& rWidth,
                          FontFamily eFamily, const FontNameAttr* pFontAttr )
{
    if ( eFamily != FAMILY_DONTKNOW )
    {
        if ( eFamily == FAMILY_SWISS )
            rType |= IMPL_FONT_ATTR_SANSSERIF;
        else if ( eFamily == FAMILY_ROMAN )
            rType |= IMPL_FONT_ATTR_SERIF;
        else if ( eFamily == FAMILY_SCRIPT )
            rType |= IMPL_FONT_ATTR_SCRIPT;
        else if ( eFamily == FAMILY_MODERN )
            rType |= IMPL_FONT_ATTR_FIXED;
        else if ( eFamily == FAMILY_DECORATIVE )
            rType |= IMPL_FONT_ATTR_DECORATIVE;
    }

    if ( pFontAttr )
    {
        rType |= pFontAttr->Type;

        if ( ((rWeight == WEIGHT_DONTKNOW) || (rWeight == WEIGHT_NORMAL)) &&
             (pFontAttr->Weight != WEIGHT_DONTKNOW) )
            rWeight = pFontAttr->Weight;
        if ( ((rWidth == WIDTH_DONTKNOW) || (rWidth == WIDTH_NORMAL)) &&
             (pFontAttr->Width != WIDTH_DONTKNOW) )
            rWidth = pFontAttr->Width;
    }
}

// =======================================================================

ImplFontData::ImplFontData( const ImplDevFontAttributes& rDFA, int nMagic )
:   ImplDevFontAttributes( rDFA ),
    mnWidth(0),
    mnHeight(0),
    mnMagic( nMagic ),
    mpNext( NULL )
{
    // StarSymbol is a unicode font, but it still deserves the symbol flag
    if( !mbSymbolFlag )
        if( 0 == GetFamilyName().CompareIgnoreCaseToAscii( "starsymbol", 10)
        ||  0 == GetFamilyName().CompareIgnoreCaseToAscii( "opensymbol", 10) )
            mbSymbolFlag = true;
}

// -----------------------------------------------------------------------

StringCompare ImplFontData::CompareIgnoreSize( const ImplFontData& rOther ) const
{
    // compare their width, weight, italic and style name
    if( meWidthType < rOther.meWidthType )
        return COMPARE_LESS;
    else if( meWidthType > rOther.meWidthType )
        return COMPARE_GREATER;

    if( meWeight < rOther.meWeight )
        return COMPARE_LESS;
    else if( meWeight > rOther.meWeight )
        return COMPARE_GREATER;

    if( meItalic < rOther.meItalic )
        return COMPARE_LESS;
    else if( meItalic > rOther.meItalic )
        return COMPARE_GREATER;

    StringCompare eCompare = maName.CompareTo( rOther.maName );
    return eCompare;
}

// -----------------------------------------------------------------------

StringCompare ImplFontData::CompareWithSize( const ImplFontData& rOther ) const
{
    StringCompare eCompare = CompareIgnoreSize( rOther );
    if( eCompare != COMPARE_EQUAL )
        return eCompare;

    if( mnHeight < rOther.mnHeight )
        return COMPARE_LESS;
    else if( mnHeight > rOther.mnHeight )
        return COMPARE_GREATER;

    if( mnWidth < rOther.mnWidth )
        return COMPARE_LESS;
    else if( mnWidth > rOther.mnWidth )
        return COMPARE_GREATER;

    return COMPARE_EQUAL;
}

// -----------------------------------------------------------------------

struct FontMatchStatus
{
public:
    int                 mnFaceMatch;
    int                 mnHeightMatch;
    int                 mnWidthMatch;
    const xub_Unicode*  mpTargetStyleName;
};

bool ImplFontData::IsBetterMatch( const ImplFontSelectData& rFSD, FontMatchStatus& rStatus ) const
{
    int nMatch = 0;

    const String& rFontName = rFSD.maTargetName;
    if( (rFontName == maName) || rFontName.EqualsIgnoreCaseAscii( maName ) )
        nMatch += 240000;

    if( rStatus.mpTargetStyleName
    &&  maStyleName.EqualsIgnoreCaseAscii( rStatus.mpTargetStyleName ) )
        nMatch += 120000;

    if( (rFSD.mePitch != PITCH_DONTKNOW) && (rFSD.mePitch == mePitch) )
        nMatch += 20000;

    // prefer NORMAL font width
    // TODO: change when the upper layers can tell their width preference
    if( meWidthType == WIDTH_NORMAL )
        nMatch += 400;
    else if( (meWidthType == WIDTH_SEMI_EXPANDED) || (meWidthType == WIDTH_SEMI_CONDENSED) )
        nMatch += 300;

    if( rFSD.meWeight != WEIGHT_DONTKNOW )
    {
        // if not bold prefer light fonts to bold fonts
        int nReqWeight = (int)rFSD.meWeight;
        if ( rFSD.meWeight > WEIGHT_MEDIUM )
            nReqWeight += 100;

        int nGivenWeight = (int)meWeight;
        if( meWeight > WEIGHT_MEDIUM )
            nGivenWeight += 100;

        int nWeightDiff = nReqWeight - nGivenWeight;

        if ( nWeightDiff == 0 )
            nMatch += 1000;
        else if ( nWeightDiff == +1 || nWeightDiff == -1 )
            nMatch += 700;
        else if ( nWeightDiff < +50 && nWeightDiff > -50)
            nMatch += 200;
    }
    else // requested weight == WEIGHT_DONTKNOW
    {
        // prefer NORMAL font weight
        // TODO: change when the upper layers can tell their weight preference
        if( meWeight == WEIGHT_NORMAL )
            nMatch += 450;
        else if( meWeight == WEIGHT_MEDIUM )
            nMatch += 350;
        else if( (meWeight == WEIGHT_SEMILIGHT) || (meWeight == WEIGHT_SEMIBOLD) )
            nMatch += 200;
        else if( meWeight == WEIGHT_LIGHT )
            nMatch += 150;
    }

    if ( rFSD.meItalic == ITALIC_NONE )
    {
        if( meItalic == ITALIC_NONE )
            nMatch += 900;
    }
    else
    {
        if( rFSD.meItalic == meItalic )
            nMatch += 900;
        else if( meItalic != ITALIC_NONE )
            nMatch += 600;
    }

    if( mbDevice )
        nMatch += 1;

    int nHeightMatch = 0;
    int nWidthMatch = 0;

    if( IsScalable() )
    {
        if( rFSD.mnOrientation != 0 )
            nMatch += 80;
        else if( rFSD.mnWidth != 0 )
            nMatch += 25;
        else
            nMatch += 5;
    }
    else
    {
        if( rFSD.mnHeight == mnHeight )
        {
            nMatch += 20;
            if( rFSD.mnWidth == mnWidth )
                nMatch += 10;
        }
        else
        {
            // for non-scalable fonts the size difference is very important
            // prefer the smaller font face because of clipping/overlapping issues
            int nHeightDiff = (rFSD.mnHeight - mnHeight) * 1000;
            nHeightMatch = (nHeightDiff >= 0) ? -nHeightDiff : 100+nHeightDiff;
            if( rFSD.mnHeight )
                nHeightMatch /= rFSD.mnHeight;

            if( (rFSD.mnWidth != 0) && (mnWidth != 0) && (rFSD.mnWidth != mnWidth) )
            {
                int nWidthDiff = (rFSD.mnWidth - mnWidth) * 100;
                nWidthMatch = (nWidthDiff >= 0) ? -nWidthDiff : +nWidthDiff;
            }
        }
    }

    if( rStatus.mnFaceMatch > nMatch )
        return false;
    else if( rStatus.mnFaceMatch < nMatch )
    {
        rStatus.mnFaceMatch      = nMatch;
        rStatus.mnHeightMatch    = nHeightMatch;
        rStatus.mnWidthMatch     = nWidthMatch;
        return true;
    }

    // when two fonts are still competing prefer the
    // one with the best matching height
    if( rStatus.mnHeightMatch > nHeightMatch )
        return false;
    else if( rStatus.mnHeightMatch < nHeightMatch )
    {
        rStatus.mnHeightMatch    = nHeightMatch;
        rStatus.mnWidthMatch     = nWidthMatch;
        return true;
    }

    if( rStatus.mnWidthMatch > nWidthMatch )
        return false;

    rStatus.mnWidthMatch = nWidthMatch;
    return true;
}

// =======================================================================

ImplFontEntry::ImplFontEntry( const ImplFontSelectData& rFontSelData )
:   maFontSelData( rFontSelData ),
    maMetric( rFontSelData ),
    mpConversion( NULL ),
    mnRefCount( 1 ),
    mnSetFontFlags( 0 ),
    mnOwnOrientation( 0 ),
    mnOrientation( 0 ),
    mbInit( false ),
    mpUnicodeFallbackList( NULL )
{
    maFontSelData.mpFontEntry = this;
}

// -----------------------------------------------------------------------

ImplFontEntry::~ImplFontEntry()
{
    delete mpUnicodeFallbackList;
}

// -----------------------------------------------------------------------

inline void ImplFontEntry::AddFallbackForUnicode( sal_UCS4 cChar, const String& rFontName )
{
    if( !mpUnicodeFallbackList )
        mpUnicodeFallbackList = new UnicodeFallbackList;
    (*mpUnicodeFallbackList)[cChar] = rFontName;
}

// -----------------------------------------------------------------------

inline bool ImplFontEntry::GetFallbackForUnicode( sal_UCS4 cChar, String* pFontName ) const
{
    if( !mpUnicodeFallbackList )
        return false;

    UnicodeFallbackList::const_iterator it = mpUnicodeFallbackList->find( cChar );
    if( it == mpUnicodeFallbackList->end() )
        return false;

    *pFontName = (*it).second;
    return true;
}

// -----------------------------------------------------------------------

inline void ImplFontEntry::IgnoreFallbackForUnicode( sal_UCS4 cChar, const String& rFontName )
{
//  DBG_ASSERT( mpUnicodeFallbackList, "ImplFontEntry::IgnoreFallbackForUnicode no list" );
    UnicodeFallbackList::iterator it = mpUnicodeFallbackList->find( cChar );
//  DBG_ASSERT( it != mpUnicodeFallbackList->end(), "ImplFontEntry::IgnoreFallbackForUnicode no match" );
    if( it == mpUnicodeFallbackList->end() )
        return;
    if( (*it).second == rFontName )
        mpUnicodeFallbackList->erase( it );
}

// =======================================================================

ImplDevFontListData::ImplDevFontListData( const String& rSearchName )
:   mpFirst( NULL ),
    maSearchName( rSearchName ),
    mnTypeFaces( 0 ),
    mnMatchType( 0 ),
    meMatchWeight( WEIGHT_DONTKNOW ),
    meMatchWidth( WIDTH_DONTKNOW ),
    meFamily( FAMILY_DONTKNOW ),
    mePitch( PITCH_DONTKNOW ),
    mnMinQuality( -1 )
{}

// -----------------------------------------------------------------------

ImplDevFontListData::~ImplDevFontListData()
{
    // release all physical font faces
    while( mpFirst )
    {
        ImplFontData* pFace = mpFirst;
        mpFirst = pFace->GetNextFace();
        delete pFace;
    }
}

// -----------------------------------------------------------------------

bool ImplDevFontListData::AddFontFace( ImplFontData* pNewData )
{
    pNewData->mpNext = NULL;

    if( !mpFirst )
    {
        maName         = pNewData->maName;
        maMapNames     = pNewData->maMapNames;
        meFamily       = pNewData->meFamily;
        mePitch        = pNewData->mePitch;
        mnMinQuality   = pNewData->mnQuality;
    }
    else
    {
        if( meFamily == FAMILY_DONTKNOW )
            meFamily = pNewData->meFamily;
        if( mePitch == PITCH_DONTKNOW )
            mePitch = pNewData->mePitch;
        if( mnMinQuality > pNewData->mnQuality )
            mnMinQuality = pNewData->mnQuality;
    }

    // set attributes for attribute based font matching
    if( pNewData->IsScalable() )
        mnTypeFaces |= IMPL_DEVFONT_SCALABLE;

    if( pNewData->IsSymbolFont() )
        mnTypeFaces |= IMPL_DEVFONT_SYMBOL;
    else
        mnTypeFaces |= IMPL_DEVFONT_NONESYMBOL;

    if( pNewData->meWeight != WEIGHT_DONTKNOW )
    {
        if( pNewData->meWeight >= WEIGHT_SEMIBOLD )
            mnTypeFaces |= IMPL_DEVFONT_BOLD;
        else if( pNewData->meWeight <= WEIGHT_SEMILIGHT )
            mnTypeFaces |= IMPL_DEVFONT_LIGHT;
        else
            mnTypeFaces |= IMPL_DEVFONT_NORMAL;
    }

    if( pNewData->meItalic == ITALIC_NONE )
        mnTypeFaces |= IMPL_DEVFONT_NONEITALIC;
    else if( (pNewData->meItalic == ITALIC_NORMAL)
         ||  (pNewData->meItalic == ITALIC_OBLIQUE) )
        mnTypeFaces |= IMPL_DEVFONT_ITALIC;

    if( (meMatchWeight == WEIGHT_DONTKNOW)
    ||  (meMatchWidth  == WIDTH_DONTKNOW)
    ||  (mnMatchType   == 0) )
    {
        // TODO: is it cheaper to calc matching attributes now or on demand?
        // calc matching attributes if other entries are already initialized

        // MT: Perform05: Do lazy, quite expensive, not needed in start-up!
        // const FontSubstConfiguration& rFontSubst = *FontSubstConfiguration::get();
        // InitMatchData( rFontSubst, maSearchName );
		// mbMatchData=true; // Somewhere else???
    }

    // reassign name (sharing saves memory)
    if( pNewData->maName == maName )
        pNewData->maName = maName;

    // insert new physical font face into linked list
    // TODO: get rid of linear search?
    ImplFontData* pData;
    ImplFontData** ppHere = &mpFirst;
    for(; (pData=*ppHere) != NULL; ppHere=&pData->mpNext )
    {
        StringCompare eComp = pNewData->CompareWithSize( *pData );
        if( eComp == COMPARE_GREATER )
            continue;
        if( eComp == COMPARE_LESS )
            break;

        // ignore duplicate if its quality is worse
        if( pNewData->mnQuality < pData->mnQuality )
            return false;

        // keep the device font if its quality is good enough
        if( (pNewData->mnQuality == pData->mnQuality)
        &&  (pData->mbDevice || !pNewData->mbDevice) )
            return false;

        // replace existing font face with a better one
        pNewData->mpNext = pData->mpNext;
        *ppHere = pNewData;
        delete pData;
        return true;
    }

    // insert into or append to list of physical font faces
    pNewData->mpNext = pData;
    *ppHere = pNewData;
    return true;
}

// -----------------------------------------------------------------------

// get font attributes using the normalized font family name
void ImplDevFontListData::InitMatchData( const vcl::FontSubstConfiguration& rFontSubst,
    const String& rSearchName )
{
    String aShortName;
    // get font attributes from the decorated font name
    rFontSubst.getMapName( rSearchName, aShortName, maMatchFamilyName,
                            meMatchWeight, meMatchWidth, mnMatchType );
    const FontNameAttr* pFontAttr = rFontSubst.getSubstInfo( rSearchName );
    // eventually use the stripped name
    if( !pFontAttr )
        if( aShortName != rSearchName )
            pFontAttr = rFontSubst.getSubstInfo( aShortName );
    ImplCalcType( mnMatchType, meMatchWeight, meMatchWidth, meFamily, pFontAttr );
    mnMatchType |= ImplIsCJKFont( maName );
}

// -----------------------------------------------------------------------

ImplFontData* ImplDevFontListData::FindBestFontFace( const ImplFontSelectData& rFSD ) const
{
    if( !mpFirst )
        return NULL;
    if( !mpFirst->GetNextFace() )
        return mpFirst;

    // FontName+StyleName should map to FamilyName+StyleName
    const String& rSearchName = rFSD.maTargetName;
    const xub_Unicode* pTargetStyleName = NULL;
    if( (rSearchName.Len() > maSearchName.Len())
    &&   rSearchName.Equals( maSearchName, 0, maSearchName.Len() ) )
        pTargetStyleName = rSearchName.GetBuffer() + maSearchName.Len() + 1;

    // linear search, TODO: improve?
    ImplFontData* pFontFace = mpFirst;
    ImplFontData* pBestFontFace = pFontFace;
    FontMatchStatus aFontMatchStatus = {0,0,0, pTargetStyleName};
    for(; pFontFace; pFontFace = pFontFace->GetNextFace() )
        if( pFontFace->IsBetterMatch( rFSD, aFontMatchStatus ) )
            pBestFontFace = pFontFace;

    return pBestFontFace;
}

// -----------------------------------------------------------------------

// update device font list with unique font faces, with uniqueness
// meaning different font attributes, but not different fonts sizes
void ImplDevFontListData::UpdateDevFontList( ImplGetDevFontList& rDevFontList ) const
{
    ImplFontData* pPrevFace = NULL;
    for( ImplFontData* pFace = mpFirst; pFace; pFace = pFace->GetNextFace() )
    {
        if( !pPrevFace || pFace->CompareIgnoreSize( *pPrevFace ) )
            rDevFontList.Add( pFace );
        pPrevFace = pFace;
    }
}

// -----------------------------------------------------------------------

void ImplDevFontListData::GetFontHeights( std::set<int>& rHeights ) const
{
    // add all available font heights
    for( const ImplFontData* pFace = mpFirst; pFace; pFace = pFace->GetNextFace() )
        rHeights.insert( pFace->GetHeight() );
}

// -----------------------------------------------------------------------

void ImplDevFontListData::UpdateCloneFontList( ImplDevFontList& rDevFontList,
    bool bScalable, bool bEmbeddable ) const
{
    for( ImplFontData* pFace = mpFirst; pFace; pFace = pFace->GetNextFace() )
    {
        if( bScalable && !pFace->IsScalable() )
            continue;
        if( bEmbeddable && !pFace->IsEmbeddable() && !pFace->IsSubsettable() )
            continue;

        ImplFontData* pClonedFace = pFace->Clone();
        rDevFontList.Add( pClonedFace );
    }
}

// =======================================================================

ImplDevFontList::ImplDevFontList()
:   mbMatchData( false )
,   mbMapNames( false )
,   mpPreMatchHook( NULL )
,   mpFallbackHook( NULL )
,   mpFallbackList( NULL )
,   mnFallbackCount( -1 )
{}

// -----------------------------------------------------------------------

ImplDevFontList::~ImplDevFontList()
{
    Clear();
}

// -----------------------------------------------------------------------

void ImplDevFontList::SetPreMatchHook( ImplPreMatchFontSubstitution* pHook )
{
    mpPreMatchHook = pHook;
}

// -----------------------------------------------------------------------

void ImplDevFontList::SetFallbackHook( ImplGlyphFallbackFontSubstitution* pHook )
{
    mpFallbackHook = pHook;
}

// -----------------------------------------------------------------------

void ImplDevFontList::Clear()
{
    // remove fallback lists
    delete[] mpFallbackList;
    mpFallbackList = NULL;
    mnFallbackCount = -1;

    // clear all entries in the device font list
    DevFontList::iterator it = maDevFontList.begin();
    for(; it != maDevFontList.end(); ++it )
    {
        ImplDevFontListData* pEntry = (*it).second;
        delete pEntry;
    }

    maDevFontList.clear();

    // match data must be recalculated too
    mbMatchData = false;
}

// -----------------------------------------------------------------------

// TODO: use a more generic String hash
int FontNameHash::operator()( const String& rStr ) const
{
    // this simple hash just has to be good enough for font names
    int nHash = 0;
    const int nLen = rStr.Len();
    const sal_Unicode* p = rStr.GetBuffer();
    switch( nLen )
    {
        default: nHash = (p[0]<<16) - (p[1]<<8) + p[2];
                 nHash += nLen;
                 p += nLen - 3;
                 // fall through
        case 3:  nHash += (p[2]<<16);   // fall through
        case 2:  nHash += (p[1]<<8);    // fall through
        case 1:  nHash += p[0];         // fall through
        case 0:  break;
    };

    return nHash;
}

// -----------------------------------------------------------------------

void ImplDevFontList::InitGenericGlyphFallback( void ) const
{
    // normalized family names of fonts suited for glyph fallback
    // if a font is available related fonts can be ignored
    // TODO: implement dynamic lists
    static const char* aGlyphFallbackList[] = {
        // empty strings separate the names of unrelated fonts
        "eudc", "",
        "arialunicodems", "cyberbit", "code2000", "",
        "andalesansui", "",
        "starsymbol", "opensymbol", "",
        "msmincho", "fzmingti", "fzheiti", "ipamincho", "sazanamimincho", "kochimincho", "",
        "sunbatang", "sundotum", "baekmukdotum", "gulim", "batang", "dotum", "",
        "hgmincholightj", "msunglightsc", "msunglighttc", "hymyeongjolightk", "",
        "tahoma", "dejavusans", "timesnewroman", "lucidatypewriter", "lucidasans", "nimbussansl", "",
        "shree", "mangal", "",
        "raavi", "shruti", "tunga", "",
        "latha", "gautami", "kartika", "vrinda", "",
        "shayyalmt", "naskmt", "",
        "david", "nachlieli", "lucidagrande", "",
        "norasi", "angsanaupc", "",
        "khmerossystem", "",
        "muktinarrow", "",
        "phetsarathot", "",
        "padauk", "pinlonmyanmar", "",
        "iskoolapota", "lklug", "",
        0
    };

    bool bHasEudc = false;
    int nMaxLevel = 0;
    int nBestQuality = 0;
    ImplDevFontListData** pFallbackList = NULL;
    for( const char** ppNames = &aGlyphFallbackList[0];; ++ppNames )
    {
        // advance to next sub-list when end-of-sublist marker
        if( !**ppNames )    // #i46456# check for empty string, i.e., deref string itself not only ptr to it
        {
            if( nBestQuality > 0 )
                if( ++nMaxLevel >= MAX_FALLBACK )
                    break;
            if( !ppNames[1] )
                break;
            nBestQuality = 0;
            continue;
        }

        // test if the glyph fallback candidate font is available and scalable
        String aTokenName( *ppNames, RTL_TEXTENCODING_UTF8 );
        ImplDevFontListData* pFallbackFont = FindFontFamily( aTokenName );
        if( !pFallbackFont )
            continue;
        if( !pFallbackFont->IsScalable() )
            continue;

        // keep the best font of the glyph fallback sub-list
        if( nBestQuality < pFallbackFont->GetMinQuality() )
        {
            nBestQuality = pFallbackFont->GetMinQuality();
            // store available glyph fallback fonts
            if( !pFallbackList )
                pFallbackList = new ImplDevFontListData*[ MAX_FALLBACK ];
            pFallbackList[ nMaxLevel ] = pFallbackFont;
            if( !bHasEudc && !nMaxLevel )
                bHasEudc = !strncmp( *ppNames, "eudc", 5 );
        }
    }

    // sort the list of fonts for glyph fallback by quality (highest first)
    // #i33947# keep the EUDC font at the front of the list
    // an insertion sort is good enough for this short list
    const int nSortStart = bHasEudc ? 1 : 0;
    for( int i = nSortStart+1, j; i < nMaxLevel; ++i )
    {
        ImplDevFontListData* pTestFont = pFallbackList[ i ];
        int nTestQuality = pTestFont->GetMinQuality();
        for( j = i; --j >= nSortStart; )
            if( nTestQuality > pFallbackList[j]->GetMinQuality() )
                pFallbackList[ j+1 ] = pFallbackList[ j ];
            else
                break;
        pFallbackList[ j+1 ] = pTestFont;
    }

#if defined(HDU_DEBUG)
    for( int i = 0; i < nMaxLevel; ++i )
    {
        ImplDevFontListData* pFont = pFallbackList[ i ];
        ByteString aFontName( pFont->GetFamilyName(), RTL_TEXTENCODING_UTF8 );
        fprintf( stderr, "GlyphFallbackFont[%d] (quality=%05d): \"%s\"\n",
            i, pFont->GetMinQuality(), aFontName.GetBuffer() );
    }
#endif

    mnFallbackCount = nMaxLevel;
    mpFallbackList  = pFallbackList;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::GetGlyphFallbackFont( ImplFontSelectData& rFontSelData,
    rtl::OUString& rMissingCodes, int nFallbackLevel ) const
{
    ImplDevFontListData* pFallbackData = NULL;

    // find a matching font candidate for platform specific glyph fallback
    if( mpFallbackHook )
    {
        // check cache for the first matching entry
        // to avoid calling the expensive fallback hook (#i83491#)
        sal_UCS4 cChar = 0;
        bool bCached = true;
        sal_Int32 nStrIndex = 0;
        while( nStrIndex < rMissingCodes.getLength() )
        {
            cChar = rMissingCodes.iterateCodePoints( &nStrIndex );
            bCached = rFontSelData.mpFontEntry->GetFallbackForUnicode( cChar, &rFontSelData.maSearchName );
            // ignore entries which don't have a fallback
            if( !bCached || (rFontSelData.maSearchName.Len() != 0) )
                break;
        }

        if( bCached )
        {
            // there is a matching fallback in the cache
            // so update rMissingCodes with codepoints not yet resolved by this fallback
            int nRemainingLength = 0;
            sal_UCS4* pRemainingCodes = (sal_UCS4*)alloca( rMissingCodes.getLength() * sizeof(sal_UCS4) );
            String aFontName;
            while( nStrIndex < rMissingCodes.getLength() )
            {
                cChar = rMissingCodes.iterateCodePoints( &nStrIndex );
                bCached = rFontSelData.mpFontEntry->GetFallbackForUnicode( cChar, &aFontName );
                if( !bCached || (rFontSelData.maSearchName != aFontName) )
                    pRemainingCodes[ nRemainingLength++ ] = cChar;
            }
            rMissingCodes = rtl::OUString( pRemainingCodes, nRemainingLength );
        }
        else
        {
            rtl::OUString aOldMissingCodes = rMissingCodes;
            // call the hook to query the best matching glyph fallback font
            if( mpFallbackHook->FindFontSubstitute( rFontSelData, rMissingCodes ) )
                // apply outdev3.cxx specific fontname normalization
                ImplGetEnglishSearchFontName( rFontSelData.maSearchName );
            else
                rFontSelData.maSearchName = String();

            // cache the result even if there was no match
            for(;;)
            {
                 if( !rFontSelData.mpFontEntry->GetFallbackForUnicode( cChar, &rFontSelData.maSearchName ) )
                     rFontSelData.mpFontEntry->AddFallbackForUnicode( cChar, rFontSelData.maSearchName );
                 if( nStrIndex >= aOldMissingCodes.getLength() )
                     break;
                 cChar = aOldMissingCodes.iterateCodePoints( &nStrIndex );
            }
            if( rFontSelData.maSearchName.Len() != 0 )
            {
                // remove cache entries that were still not resolved
                for( nStrIndex = 0; nStrIndex < rMissingCodes.getLength(); )
                {
                    cChar = rMissingCodes.iterateCodePoints( &nStrIndex );
                    rFontSelData.mpFontEntry->IgnoreFallbackForUnicode( cChar, rFontSelData.maSearchName );
                }
            }
        }

        // find the matching device font
        if( rFontSelData.maSearchName.Len() != 0 )
            pFallbackData = FindFontFamily( rFontSelData.maSearchName );
    }

    // else find a matching font candidate for generic glyph fallback
    if( !pFallbackData )
    {
        // initialize font candidates for generic glyph fallback if needed
        if( mnFallbackCount < 0 )
            InitGenericGlyphFallback();
        // TODO: adjust nFallbackLevel by number of levels resolved by the fallback hook
        if( nFallbackLevel < mnFallbackCount )
            pFallbackData = mpFallbackList[ nFallbackLevel ];
    }

    return pFallbackData;
}

// -----------------------------------------------------------------------

void ImplDevFontList::Add( ImplFontData* pNewData )
{
    int nAliasQuality = pNewData->mnQuality - 100;
    String aMapNames = pNewData->maMapNames;
    pNewData->maMapNames = String();

    bool bKeepNewData = false;
    for( xub_StrLen nMapNameIndex = 0; nMapNameIndex != STRING_NOTFOUND; )
    {
        String aSearchName = pNewData->maName;
        ImplGetEnglishSearchFontName( aSearchName );

        DevFontList::const_iterator it = maDevFontList.find( aSearchName );
        ImplDevFontListData* pFoundData = NULL;
        if( it != maDevFontList.end() )
            pFoundData = (*it).second;

        if( !pFoundData )
        {
            pFoundData = new ImplDevFontListData( aSearchName );
            maDevFontList[ aSearchName ] = pFoundData;
        }

        bKeepNewData = pFoundData->AddFontFace( pNewData );

        // add font alias if available
        // a font alias should never win against an original font with similar quality
        if( aMapNames.Len() >= nMapNameIndex )
            break;
        if( bKeepNewData ) // try to recycle obsoleted object
            pNewData = pNewData->CreateAlias();
        bKeepNewData = false;
        pNewData->mnQuality = nAliasQuality;
        pNewData->maName = GetNextFontToken( aMapNames, nMapNameIndex );
    }

    if( !bKeepNewData )
        delete pNewData;
}

// -----------------------------------------------------------------------

// find the font from the normalized font family name
ImplDevFontListData* ImplDevFontList::ImplFindBySearchName( const String& rSearchName ) const
{
#ifdef DEBUG
    String aTempName = rSearchName;
    ImplGetEnglishSearchFontName( aTempName );
    DBG_ASSERT( aTempName == rSearchName, "ImplDevFontList::ImplFindBySearchName() called with non-normalized name" );
#endif

    DevFontList::const_iterator it = maDevFontList.find( rSearchName );
    if( it == maDevFontList.end() )
        return NULL;

    ImplDevFontListData* pFoundData = (*it).second;
    return pFoundData;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::ImplFindByAliasName( const String& rSearchName, const String& rShortName ) const
{
    // short circuit for impossible font name alias
    if( !rSearchName.Len() )
        return NULL;

    // short circuit if no alias names are available
    if( !mbMapNames )
        return NULL;

    // use the font's alias names to find the font
    // TODO: get rid of linear search
    DevFontList::const_iterator it = maDevFontList.begin();
    while( it != maDevFontList.end() )
    {
        ImplDevFontListData* pData = (*it).second;
        if( !pData->maMapNames.Len() )
            continue;

        // if one alias name matches we found a matching font
        String aTempName;
        xub_StrLen nIndex = 0;
        do
        {
           aTempName = GetNextFontToken( pData->maMapNames, nIndex );
           // Test, if the Font name match with one of the mapping names
           if ( (aTempName == rSearchName) || (aTempName == rShortName) )
              return pData;
        }
        while ( nIndex != STRING_NOTFOUND );
     }

     return NULL;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::FindFontFamily( const String& rFontName ) const
{
    // normalize the font fomily name and
    String aName = rFontName;
    ImplGetEnglishSearchFontName( aName );
    ImplDevFontListData* pFound = ImplFindBySearchName( aName );
    return pFound;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::ImplFindByTokenNames( const String& rTokenStr ) const
{
    ImplDevFontListData* pFoundData = NULL;

    // use normalized font name tokens to find the font
    for( xub_StrLen nTokenPos = 0; nTokenPos != STRING_NOTFOUND; )
    {
        String aSearchName = GetNextFontToken( rTokenStr, nTokenPos );
        if( !aSearchName.Len() )
            continue;
        ImplGetEnglishSearchFontName( aSearchName );
        pFoundData = ImplFindBySearchName( aSearchName );
        if( pFoundData )
            break;
    }

    return pFoundData;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::ImplFindBySubstFontAttr( const vcl::FontNameAttr& rFontAttr ) const
{
    ImplDevFontListData* pFoundData = NULL;

    // use the font substitutions suggested by the FontNameAttr to find the font
    ::std::vector< String >::const_iterator it = rFontAttr.Substitutions.begin();
    for(; it != rFontAttr.Substitutions.end(); ++it )
    {
        String aSearchName( *it );
        ImplGetEnglishSearchFontName( aSearchName );

        pFoundData = ImplFindBySearchName( aSearchName );
        if( pFoundData )
            break;
    }

    return pFoundData;
}

// -----------------------------------------------------------------------

void ImplDevFontList::InitMatchData() const
{
    // short circuit if already done
    if( mbMatchData )
        return;
    mbMatchData = true;

    // calculate MatchData for all entries
    const FontSubstConfiguration& rFontSubst = *FontSubstConfiguration::get();

    DevFontList::const_iterator it = maDevFontList.begin();
    for(; it != maDevFontList.end(); ++it )
    {
        const String& rSearchName = (*it).first;
        ImplDevFontListData* pEntry = (*it).second;

        pEntry->InitMatchData( rFontSubst, rSearchName );
    }
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::ImplFindByAttributes( ULONG nSearchType,
    FontWeight eSearchWeight, FontWidth eSearchWidth, FontFamily /*eSearchFamily*/,
    FontItalic eSearchItalic, const String& rSearchFamilyName ) const
{
    if( (eSearchItalic != ITALIC_NONE) && (eSearchItalic != ITALIC_DONTKNOW) )
        nSearchType |= IMPL_FONT_ATTR_ITALIC;

    // don't bother to match attributes if the attributes aren't worth matching
    if( !nSearchType
    && ((eSearchWeight == WEIGHT_DONTKNOW) || (eSearchWeight == WEIGHT_NORMAL))
    && ((eSearchWidth == WIDTH_DONTKNOW) || (eSearchWidth == WIDTH_NORMAL)) )
        return NULL;

    InitMatchData();
    ImplDevFontListData* pFoundData = NULL;

    long    nTestMatch;
    long    nBestMatch = 40000;
    ULONG   nBestType = 0;

    DevFontList::const_iterator it = maDevFontList.begin();
    for(; it != maDevFontList.end(); ++it )
    {
        ImplDevFontListData* pData = (*it).second;

        // Get all information about the matching font
        ULONG       nMatchType  = pData->mnMatchType;
        FontWeight  eMatchWeight= pData->meMatchWeight;
        FontWidth   eMatchWidth = pData->meMatchWidth;

        // Calculate Match Value
        // 1000000000
        //  100000000
        //   10000000   CJK, CTL, None-Latin, Symbol
        //    1000000   FamilyName, Script, Fixed, -Special, -Decorative,
        //              Titling, Capitals, Outline, Shadow
        //     100000   Match FamilyName, Serif, SansSerif, Italic,
        //              Width, Weight
        //      10000   Scalable, Standard, Default,
        //              full, Normal, Knownfont,
        //              Otherstyle, +Special, +Decorative,
        //       1000   Typewriter, Rounded, Gothic, Schollbook
        //        100
        nTestMatch = 0;

        // test CJK script attributes
        if ( nSearchType & IMPL_FONT_ATTR_CJK )
        {
            // Matching language
            if( 0 == ((nSearchType ^ nMatchType) & IMPL_FONT_ATTR_CJK_ALLLANG) )
                nTestMatch += 10000000*3;
            if( nMatchType & IMPL_FONT_ATTR_CJK )
                nTestMatch += 10000000*2;
            if( nMatchType & IMPL_FONT_ATTR_FULL )
                nTestMatch += 10000000;
        }
        else if ( nMatchType & IMPL_FONT_ATTR_CJK )
            nTestMatch -= 10000000;

        // test CTL script attributes
        if( nSearchType & IMPL_FONT_ATTR_CTL )
        {
            if( nMatchType & IMPL_FONT_ATTR_CTL )
                nTestMatch += 10000000*2;
            if( nMatchType & IMPL_FONT_ATTR_FULL )
                nTestMatch += 10000000;
        }
        else if ( nMatchType & IMPL_FONT_ATTR_CTL )
            nTestMatch -= 10000000;

        // test LATIN script attributes
        if( nSearchType & IMPL_FONT_ATTR_NONELATIN )
        {
            if( nMatchType & IMPL_FONT_ATTR_NONELATIN )
                nTestMatch += 10000000*2;
            if( nMatchType & IMPL_FONT_ATTR_FULL )
                nTestMatch += 10000000;
        }

        // test SYMBOL attributes
        if ( nSearchType & IMPL_FONT_ATTR_SYMBOL )
        {
            const String& rSearchName = it->first;
            // prefer some special known symbol fonts
            if ( rSearchName.EqualsAscii( "starsymbol" ) )
                nTestMatch += 10000000*6+(10000*3);
            else if ( rSearchName.EqualsAscii( "opensymbol" ) )
                nTestMatch += 10000000*6;
            else if ( rSearchName.EqualsAscii( "starbats" )
            ||        rSearchName.EqualsAscii( "wingdings" )
            ||        rSearchName.EqualsAscii( "monotypesorts" )
            ||        rSearchName.EqualsAscii( "dingbats" )
            ||        rSearchName.EqualsAscii( "zapfdingbats" ) )
                nTestMatch += 10000000*5;
            else if ( pData->mnTypeFaces & IMPL_DEVFONT_SYMBOL )
                nTestMatch += 10000000*4;
            else
            {
                if( nMatchType & IMPL_FONT_ATTR_SYMBOL )
                    nTestMatch += 10000000*2;
                if( nMatchType & IMPL_FONT_ATTR_FULL )
                    nTestMatch += 10000000;
            }
        }
        else if ( (pData->mnTypeFaces & (IMPL_DEVFONT_SYMBOL | IMPL_DEVFONT_NONESYMBOL)) == IMPL_DEVFONT_SYMBOL )
            nTestMatch -= 10000000;
        else if ( nMatchType & IMPL_FONT_ATTR_SYMBOL )
            nTestMatch -= 10000;

        // match stripped family name
        if( rSearchFamilyName.Len() && (rSearchFamilyName == pData->maMatchFamilyName) )
            nTestMatch += 1000000*3;

        // match ALLSCRIPT? attribute
        if( nSearchType & IMPL_FONT_ATTR_ALLSCRIPT )
        {
            if( nMatchType & IMPL_FONT_ATTR_ALLSCRIPT )
                nTestMatch += 1000000*2;
            if( nSearchType & IMPL_FONT_ATTR_ALLSUBSCRIPT )
            {
                if( 0 == ((nSearchType ^ nMatchType) & IMPL_FONT_ATTR_ALLSUBSCRIPT) )
                    nTestMatch += 1000000*2;
                if( 0 != ((nSearchType ^ nMatchType) & IMPL_FONT_ATTR_BRUSHSCRIPT) )
                    nTestMatch -= 1000000;
            }
        }
        else if( nMatchType & IMPL_FONT_ATTR_ALLSCRIPT )
            nTestMatch -= 1000000;

        // test MONOSPACE+TYPEWRITER attributes
        if( nSearchType & IMPL_FONT_ATTR_FIXED )
        {
            if( nMatchType & IMPL_FONT_ATTR_FIXED )
                nTestMatch += 1000000*2;
            // a typewriter attribute is even better
            if( 0 == ((nSearchType ^ nMatchType) & IMPL_FONT_ATTR_TYPEWRITER) )
                nTestMatch += 10000*2;
        }
        else if( nMatchType & IMPL_FONT_ATTR_FIXED )
            nTestMatch -= 1000000;

        // test SPECIAL attribute
        if( nSearchType & IMPL_FONT_ATTR_SPECIAL )
        {
            if( nMatchType & IMPL_FONT_ATTR_SPECIAL )
                nTestMatch += 10000;
            else if( !(nSearchType & IMPL_FONT_ATTR_ALLSERIFSTYLE) )
            {
                 if( nMatchType & IMPL_FONT_ATTR_SERIF )
                     nTestMatch += 1000*2;
                 else if( nMatchType & IMPL_FONT_ATTR_SANSSERIF )
                     nTestMatch += 1000;
             }
        }
        else if( (nMatchType & IMPL_FONT_ATTR_SPECIAL) && !(nSearchType & IMPL_FONT_ATTR_SYMBOL) )
            nTestMatch -= 1000000;

        // test DECORATIVE attribute
        if( nSearchType & IMPL_FONT_ATTR_DECORATIVE )
        {
            if( nMatchType & IMPL_FONT_ATTR_DECORATIVE )
                nTestMatch += 10000;
            else if( !(nSearchType & IMPL_FONT_ATTR_ALLSERIFSTYLE) )
            {
                if( nMatchType & IMPL_FONT_ATTR_SERIF )
                    nTestMatch += 1000*2;
                else if ( nMatchType & IMPL_FONT_ATTR_SANSSERIF )
                    nTestMatch += 1000;
            }
        }
        else if( nMatchType & IMPL_FONT_ATTR_DECORATIVE )
            nTestMatch -= 1000000;

        // test TITLE+CAPITALS attributes
        if( nSearchType & (IMPL_FONT_ATTR_TITLING | IMPL_FONT_ATTR_CAPITALS) )
        {
            if( nMatchType & (IMPL_FONT_ATTR_TITLING | IMPL_FONT_ATTR_CAPITALS) )
                nTestMatch += 1000000*2;
            if( 0 == ((nSearchType^nMatchType) & (IMPL_FONT_ATTR_TITLING | IMPL_FONT_ATTR_CAPITALS)))
                nTestMatch += 1000000;
            else if( (nMatchType & (IMPL_FONT_ATTR_TITLING | IMPL_FONT_ATTR_CAPITALS))
            &&       (nMatchType & (IMPL_FONT_ATTR_STANDARD | IMPL_FONT_ATTR_DEFAULT)) )
                nTestMatch += 1000000;
        }
        else if( nMatchType & (IMPL_FONT_ATTR_TITLING | IMPL_FONT_ATTR_CAPITALS) )
            nTestMatch -= 1000000;

        // test OUTLINE+SHADOW attributes
        if( nSearchType & (IMPL_FONT_ATTR_OUTLINE | IMPL_FONT_ATTR_SHADOW) )
        {
            if( nMatchType & (IMPL_FONT_ATTR_OUTLINE | IMPL_FONT_ATTR_SHADOW) )
                nTestMatch += 1000000*2;
            if( 0 == ((nSearchType ^ nMatchType) & (IMPL_FONT_ATTR_OUTLINE | IMPL_FONT_ATTR_SHADOW)) )
                nTestMatch += 1000000;
            else if( (nMatchType & (IMPL_FONT_ATTR_OUTLINE | IMPL_FONT_ATTR_SHADOW))
            &&       (nMatchType & (IMPL_FONT_ATTR_STANDARD | IMPL_FONT_ATTR_DEFAULT)) )
                nTestMatch += 1000000;
        }
        else if ( nMatchType & (IMPL_FONT_ATTR_OUTLINE | IMPL_FONT_ATTR_SHADOW) )
            nTestMatch -= 1000000;

        // test font name substrings
        if( (rSearchFamilyName.Len() && pData->maMatchFamilyName.Len())
        &&    ((rSearchFamilyName.Search( pData->maMatchFamilyName ) != STRING_NOTFOUND)
            || (pData->maMatchFamilyName.Search( rSearchFamilyName ) != STRING_NOTFOUND)) )
                    nTestMatch += 100000*2;

        // test SERIF attribute
        if( nSearchType & IMPL_FONT_ATTR_SERIF )
        {
            if( nMatchType & IMPL_FONT_ATTR_SERIF )
                nTestMatch += 1000000*2;
            else if( nMatchType & IMPL_FONT_ATTR_SANSSERIF )
                nTestMatch -= 1000000;
        }

        // test SANSERIF attribute
        if( nSearchType & IMPL_FONT_ATTR_SANSSERIF )
        {
            if( nMatchType & IMPL_FONT_ATTR_SANSSERIF )
                nTestMatch += 1000000;
            else if ( nMatchType & IMPL_FONT_ATTR_SERIF )
                nTestMatch -= 1000000;
        }

        // test ITALIC attribute
        if( nSearchType & IMPL_FONT_ATTR_ITALIC )
        {
            if( pData->mnTypeFaces & IMPL_DEVFONT_ITALIC )
                nTestMatch += 1000000*3;
            if( nMatchType & IMPL_FONT_ATTR_ITALIC )
                nTestMatch += 1000000;
        }
        else if( !(nSearchType & IMPL_FONT_ATTR_ALLSCRIPT)
            &&  ((nMatchType & IMPL_FONT_ATTR_ITALIC)
                || !(pData->mnTypeFaces & IMPL_DEVFONT_NONEITALIC)) )
            nTestMatch -= 1000000*2;

        // test WIDTH attribute
        if( (eSearchWidth != WIDTH_DONTKNOW) && (eSearchWidth != WIDTH_NORMAL) )
        {
            if( eSearchWidth < WIDTH_NORMAL )
            {
                if( eSearchWidth == eMatchWidth )
                    nTestMatch += 1000000*3;
                else if( (eMatchWidth < WIDTH_NORMAL) && (eMatchWidth != WIDTH_DONTKNOW) )
                    nTestMatch += 1000000;
            }
            else
            {
                if( eSearchWidth == eMatchWidth )
                    nTestMatch += 1000000*3;
                else if( eMatchWidth > WIDTH_NORMAL )
                    nTestMatch += 1000000;
            }
        }
        else if( (eMatchWidth != WIDTH_DONTKNOW) && (eMatchWidth != WIDTH_NORMAL) )
            nTestMatch -= 1000000;

        // test WEIGHT attribute
        if( (eSearchWeight != WEIGHT_DONTKNOW) && (eSearchWeight != WEIGHT_NORMAL) && (eSearchWeight != WEIGHT_MEDIUM) )
        {
            if( eSearchWeight < WEIGHT_NORMAL )
            {
                if( pData->mnTypeFaces & IMPL_DEVFONT_LIGHT )
                    nTestMatch += 1000000;
                if( (eMatchWeight < WEIGHT_NORMAL) && (eMatchWeight != WEIGHT_DONTKNOW) )
                    nTestMatch += 1000000;
            }
            else
            {
                if( pData->mnTypeFaces & IMPL_DEVFONT_BOLD )
                    nTestMatch += 1000000;
                if( eMatchWeight > WEIGHT_BOLD )
                    nTestMatch += 1000000;
            }
        }
        else if( ((eMatchWeight != WEIGHT_DONTKNOW) && (eMatchWeight != WEIGHT_NORMAL) && (eMatchWeight != WEIGHT_MEDIUM))
            || !(pData->mnTypeFaces & IMPL_DEVFONT_NORMAL) )
            nTestMatch -= 1000000;

        // prefer scalable fonts
        if( pData->mnTypeFaces & IMPL_DEVFONT_SCALABLE )
            nTestMatch += 10000*4;
        else
            nTestMatch -= 10000*4;

        // test STANDARD+DEFAULT+FULL+NORMAL attributes
        if( nMatchType & IMPL_FONT_ATTR_STANDARD )
            nTestMatch += 10000*2;
        if( nMatchType & IMPL_FONT_ATTR_DEFAULT )
            nTestMatch += 10000;
        if( nMatchType & IMPL_FONT_ATTR_FULL )
            nTestMatch += 10000;
        if( nMatchType & IMPL_FONT_ATTR_NORMAL )
            nTestMatch += 10000;

        // test OTHERSTYLE attribute
        if( nMatchType & IMPL_FONT_ATTR_OTHERSTYLE )
        {
            if( !(nMatchType & IMPL_FONT_ATTR_OTHERSTYLE) )
                nTestMatch -= 10000;
        }
        else if( nMatchType & IMPL_FONT_ATTR_OTHERSTYLE )
            nTestMatch -= 10000;

        // test ROUNDED attribute
        if( 0 == ((nSearchType ^ nMatchType) & IMPL_FONT_ATTR_ROUNDED) )
            nTestMatch += 1000;

        // test TYPEWRITER attribute
        if( 0 == ((nSearchType ^ nMatchType) & IMPL_FONT_ATTR_TYPEWRITER) )
            nTestMatch += 1000;

        // test GOTHIC attribute
        if( nSearchType & IMPL_FONT_ATTR_GOTHIC )
        {
            if( nMatchType & IMPL_FONT_ATTR_GOTHIC )
                nTestMatch += 1000*3;
            if( nMatchType & IMPL_FONT_ATTR_SANSSERIF )
                nTestMatch += 1000*2;
        }

        // test SCHOOLBOOK attribute
        if( nSearchType & IMPL_FONT_ATTR_SCHOOLBOOK )
        {
            if( nMatchType & IMPL_FONT_ATTR_SCHOOLBOOK )
                nTestMatch += 1000*3;
            if( nMatchType & IMPL_FONT_ATTR_SERIF )
                nTestMatch += 1000*2;
        }

        // compare with best matching font yet
        if ( nTestMatch > nBestMatch )
        {
            pFoundData  = pData;
            nBestMatch  = nTestMatch;
            nBestType   = nMatchType;
        }
        else if( nTestMatch == nBestMatch )
        {
            // some fonts are more suitable defaults
            if( nMatchType & IMPL_FONT_ATTR_DEFAULT )
            {
                pFoundData  = pData;
                nBestType   = nMatchType;
            }
            else if( (nMatchType & IMPL_FONT_ATTR_STANDARD) &&
                    !(nBestType & IMPL_FONT_ATTR_DEFAULT) )
            {
                 pFoundData  = pData;
                 nBestType   = nMatchType;
            }
        }
    }

    return pFoundData;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::FindDefaultFont() const
{
    // try to find one of the default fonts of the
    // UNICODE, SANSSERIF, SERIF or FIXED default font lists
    const DefaultFontConfiguration& rDefaults = *DefaultFontConfiguration::get();
    com::sun::star::lang::Locale aLocale( OUString( RTL_CONSTASCII_USTRINGPARAM("en") ), OUString(), OUString() );
    String aFontname = rDefaults.getDefaultFont( aLocale, DEFAULTFONT_SANS_UNICODE );
    ImplDevFontListData* pFoundData = ImplFindByTokenNames( aFontname );
    if( pFoundData )
        return pFoundData;

    aFontname = rDefaults.getDefaultFont( aLocale, DEFAULTFONT_SANS );
    pFoundData = ImplFindByTokenNames( aFontname );
    if( pFoundData )
        return pFoundData;

    aFontname = rDefaults.getDefaultFont( aLocale, DEFAULTFONT_SERIF );
    pFoundData = ImplFindByTokenNames( aFontname );
    if( pFoundData )
        return pFoundData;

    aFontname = rDefaults.getDefaultFont( aLocale, DEFAULTFONT_FIXED );
    pFoundData = ImplFindByTokenNames( aFontname );
    if( pFoundData )
        return pFoundData;

    // now try to find a reasonable non-symbol font

    InitMatchData();

    DevFontList::const_iterator it = maDevFontList.begin();
    for(; it !=  maDevFontList.end(); ++it )
    {
        ImplDevFontListData* pData = (*it).second;
        if( pData->mnMatchType & IMPL_FONT_ATTR_SYMBOL )
            continue;
        pFoundData = pData;
        if( pData->mnMatchType & (IMPL_FONT_ATTR_DEFAULT|IMPL_FONT_ATTR_STANDARD) )
            break;
    }
    if( pFoundData )
        return pFoundData;

    // finding any font is better than finding no font at all
    it = maDevFontList.begin();
    if( it !=  maDevFontList.end() )
        pFoundData = (*it).second;

    return pFoundData;
}

// -----------------------------------------------------------------------

ImplDevFontList* ImplDevFontList::Clone( bool bScalable, bool bEmbeddable ) const
{
    ImplDevFontList* pClonedList = new ImplDevFontList;
//  pClonedList->mbMatchData    = mbMatchData;
    pClonedList->mbMapNames     = mbMapNames;
    pClonedList->mpPreMatchHook = mpPreMatchHook;
    pClonedList->mpFallbackHook = mpFallbackHook;

    // TODO: clone the config-font attributes too?
    pClonedList->mbMatchData    = false;

    DevFontList::const_iterator it = maDevFontList.begin();
    for(; it != maDevFontList.end(); ++it )
    {
        const ImplDevFontListData* pFontFace = (*it).second;
        pFontFace->UpdateCloneFontList( *pClonedList, bScalable, bEmbeddable );
    }

    return pClonedList;
}

// -----------------------------------------------------------------------

ImplGetDevFontList* ImplDevFontList::GetDevFontList() const
{
    ImplGetDevFontList* pGetDevFontList = new ImplGetDevFontList;

    DevFontList::const_iterator it = maDevFontList.begin();
    for(; it != maDevFontList.end(); ++it )
    {
        const ImplDevFontListData* pFontFamily = (*it).second;
        pFontFamily->UpdateDevFontList( *pGetDevFontList );
    }

    return pGetDevFontList;
}

// -----------------------------------------------------------------------

ImplGetDevSizeList* ImplDevFontList::GetDevSizeList( const String& rFontName ) const
{
    ImplGetDevSizeList* pGetDevSizeList = new ImplGetDevSizeList( rFontName );

    ImplDevFontListData* pFontFamily = FindFontFamily( rFontName );
    if( pFontFamily != NULL )
    {
        std::set<int> rHeights;
        pFontFamily->GetFontHeights( rHeights );

        std::set<int>::const_iterator it = rHeights.begin();
        for(; it != rHeights.begin(); ++it )
            pGetDevSizeList->Add( *it );
    }

    return pGetDevSizeList;
}

// =======================================================================

ImplFontSelectData::ImplFontSelectData( const Font& rFont,
    const String& rSearchName, const Size& rSize, float fExactHeight)
:   maSearchName( rSearchName ),
    mnWidth( rSize.Width() ),
    mnHeight( rSize.Height() ),
    mfExactHeight( fExactHeight),
    mnOrientation( rFont.GetOrientation() ),
    meLanguage( rFont.GetLanguage() ),
    mbVertical( rFont.IsVertical() ),
    mbNonAntialiased( false ),
    mpFontData( NULL ),
    mpFontEntry( NULL )
{
    maTargetName = maName;

    rFont.GetFontAttributes( *this );

    // normalize orientation between 0 and 3600
    if( 3600 <= (unsigned)mnOrientation )
    {
        if( mnOrientation >= 0 )
            mnOrientation %= 3600;
        else
            mnOrientation = 3600 - (-mnOrientation % 3600);
    }

    // normalize width and height
    if( mnHeight < 0 )
        mnHeight = -mnHeight;
    if( mnWidth < 0 )
        mnWidth = -mnWidth;
}

// -----------------------------------------------------------------------

ImplFontSelectData::ImplFontSelectData( const ImplFontData& rFontData,
    const Size& rSize, float fExactHeight, int nOrientation, bool bVertical )
:   ImplFontAttributes( rFontData ),
    mnWidth( rSize.Width() ),
    mnHeight( rSize.Height() ),
    mfExactHeight( fExactHeight ),
    mnOrientation( nOrientation ),
    meLanguage( 0 ),
    mbVertical( bVertical ),
    mbNonAntialiased( false ),
    mpFontData( &rFontData ),
    mpFontEntry( NULL )
{
    maTargetName = maSearchName = maName;
    // NOTE: no normalization for width/height/orientation
}

// =======================================================================

size_t ImplFontCache::IFSD_Hash::operator()( const ImplFontSelectData& rFSD ) const
{
    // TODO: does it pay off to improve this hash function?
    static FontNameHash aFontNameHash;
    size_t nHash = aFontNameHash( rFSD.maSearchName );
#ifdef ENABLE_GRAPHITE
    // check for features and generate a unique hash if necessary
    if (rFSD.maTargetName.Search(grutils::GrFeatureParser::FEAT_PREFIX)
        != STRING_NOTFOUND)
    {
        nHash = aFontNameHash( rFSD.maTargetName );
    }
#endif
    nHash += 11 * rFSD.mnHeight;
    nHash += 19 * rFSD.meWeight;
    nHash += 29 * rFSD.meItalic;
    nHash += 37 * rFSD.mnOrientation;
    nHash += 41 * rFSD.meLanguage;
    if( rFSD.mbVertical )
        nHash += 53;
    return nHash;
}

// -----------------------------------------------------------------------

bool ImplFontCache::IFSD_Equal::operator()(const ImplFontSelectData& rA, const ImplFontSelectData& rB) const
{
    // check normalized font family name
    if( rA.maSearchName != rB.maSearchName )
        return false;

    // check font transformation
    if( (rA.mnHeight       != rB.mnHeight)
    ||  (rA.mnWidth        != rB.mnWidth)
    ||  (rA.mnOrientation  != rB.mnOrientation) )
        return false;

    // check mapping relevant attributes
    if( (rA.mbVertical     != rB.mbVertical)
    ||  (rA.meLanguage     != rB.meLanguage) )
        return false;

    // check font face attributes
    if( (rA.meWeight       != rB.meWeight)
    ||  (rA.meItalic       != rB.meItalic)
//    ||  (rA.meFamily       != rB.meFamily) // TODO: remove this mostly obsolete member
    ||  (rA.mePitch        != rB.mePitch) )
        return false;

    // check style name
    if( rA.maStyleName != rB.maStyleName)
        return false;

    // Symbol fonts may recode from one type to another So they are only
    // safely equivalent for equal targets
    if (
        (rA.mpFontData && rA.mpFontData->IsSymbolFont()) ||
        (rB.mpFontData && rB.mpFontData->IsSymbolFont())
       )
    {
        if (rA.maTargetName != rB.maTargetName)
            return false;
    }

#ifdef ENABLE_GRAPHITE
    // check for features
    if ((rA.maTargetName.Search(grutils::GrFeatureParser::FEAT_PREFIX)
         != STRING_NOTFOUND ||
         rB.maTargetName.Search(grutils::GrFeatureParser::FEAT_PREFIX)
         != STRING_NOTFOUND) && rA.maTargetName != rB.maTargetName)
        return false;
#endif

    return true;
}

// -----------------------------------------------------------------------

ImplFontCache::ImplFontCache( bool bPrinter )
:   mpFirstEntry( NULL ),
    mnRef0Count( 0 ),
    mbPrinter( bPrinter )
{}

// -----------------------------------------------------------------------

ImplFontCache::~ImplFontCache()
{
    FontInstanceList::iterator it = maFontInstanceList.begin();
    for(; it != maFontInstanceList.end(); ++it )
    {
        ImplFontEntry* pEntry = (*it).second;
        delete pEntry;
    }
}

// -----------------------------------------------------------------------

ImplFontEntry* ImplFontCache::GetFontEntry( ImplDevFontList* pFontList,
    const Font& rFont, const Size& rSize, float fExactHeight, ImplDirectFontSubstitution* pDevSpecific )
{
    String aSearchName = rFont.GetName();

    // TODO: also add device specific name caching
    if( !pDevSpecific )
    {
        // check if the requested font name is already known
        // if it is already known get its normalized search name
        FontNameList::const_iterator it_name = maFontNameList.find( aSearchName );
        if( it_name != maFontNameList.end() )
            if( !(*it_name).second.EqualsAscii( "hg", 0, 2)
#ifdef ENABLE_GRAPHITE
                && (aSearchName.Search(grutils::GrFeatureParser::FEAT_PREFIX)
                    == STRING_NOTFOUND)
#endif
            )
                aSearchName = (*it_name).second;
    }

    // initialize internal font request object
    ImplFontSelectData aFontSelData( rFont, aSearchName, rSize, fExactHeight );
    return GetFontEntry( pFontList, aFontSelData, pDevSpecific );
}

// -----------------------------------------------------------------------

ImplFontEntry* ImplFontCache::GetFontEntry( ImplDevFontList* pFontList,
    ImplFontSelectData& aFontSelData, ImplDirectFontSubstitution* pDevSpecific )
{
    // check if a directly matching logical font instance is already cached,
    // the most recently used font usually has a hit rate of >50%
    ImplFontEntry *pEntry = NULL;
    ImplDevFontListData* pFontFamily = NULL;
    IFSD_Equal aIFSD_Equal;
    if( mpFirstEntry && aIFSD_Equal( aFontSelData, mpFirstEntry->maFontSelData ) )
        pEntry = mpFirstEntry;
    else
    {
        FontInstanceList::iterator it = maFontInstanceList.find( aFontSelData );
        if( it != maFontInstanceList.end() )
            pEntry = (*it).second;
    }

    if( !pEntry ) // no direct cache hit
    {
        // find the best matching logical font family and update font selector accordingly
        pFontFamily = pFontList->ImplFindByFont( aFontSelData, mbPrinter, pDevSpecific );
        DBG_ASSERT( (pFontFamily != NULL), "ImplFontCache::Get() No logical font found!" );
        if( pFontFamily )
            aFontSelData.maSearchName = pFontFamily->GetSearchName();

        // check if an indirectly matching logical font instance is already cached
        FontInstanceList::iterator it = maFontInstanceList.find( aFontSelData );
        if( it != maFontInstanceList.end() )
        {
            // we have an indirect cache hit
            pEntry = (*it).second;
            // cache the requested and the selected font names
            // => next time there is a good chance for a direct cache hit
            // don't allow the cache to grow too big
            // TODO: implement some fancy LRU caching?
            if( maFontNameList.size() >= 4000 )
                maFontNameList.clear();
            // TODO: also add device specific name caching
            if( !pDevSpecific )
                if( aFontSelData.maName != aFontSelData.maSearchName )
                    maFontNameList[ aFontSelData.maName ] = aFontSelData.maSearchName;
        }
    }

    if( pEntry ) // cache hit => use existing font instance
    {
        // increase the font instance's reference count
        if( !pEntry->mnRefCount++ )
            --mnRef0Count;
    }
    else // no cache hit => create a new font instance
    {
        // find the best matching physical font face
        ImplFontData* pFontData = pFontFamily->FindBestFontFace( aFontSelData );
        aFontSelData.mpFontData = pFontData;

        // create a new logical font instance from this physical font face
        pEntry = pFontData->CreateFontInstance( aFontSelData );

        // if we found a different symbol font we need a symbol conversion table
        if( pFontData->IsSymbolFont() )
            if( aFontSelData.maTargetName != aFontSelData.maSearchName )
                pEntry->mpConversion = ImplGetRecodeData( aFontSelData.maTargetName, aFontSelData.maSearchName );

        // add the new entry to the cache
        maFontInstanceList[ aFontSelData ] = pEntry;
    }

    mpFirstEntry = pEntry;
    return pEntry;
}

// -----------------------------------------------------------------------

ImplDevFontListData* ImplDevFontList::ImplFindByFont( ImplFontSelectData& rFSD,
    bool bPrinter, ImplDirectFontSubstitution* pDevSpecific ) const
{
    // give up if no fonts are available
    if( !Count() )
        return NULL;

    // test if a font in the token list is available
    // substitute the font if this was requested
    USHORT nSubstFlags = FONT_SUBSTITUTE_ALWAYS;
    if ( bPrinter )
        nSubstFlags |= FONT_SUBSTITUTE_SCREENONLY;

    bool bMultiToken = false;
    xub_StrLen nTokenPos = 0;
    String& aSearchName = rFSD.maSearchName; // TODO: get rid of reference
    for(;;)
    {
        rFSD.maTargetName = GetNextFontToken( rFSD.maName, nTokenPos );
        aSearchName = rFSD.maTargetName;

#ifdef ENABLE_GRAPHITE
        // Until features are properly supported, they are appended to the
        // font name, so we need to strip them off so the font is found.
        xub_StrLen nFeat = aSearchName.Search(grutils::GrFeatureParser::FEAT_PREFIX);
        String aOrigName = rFSD.maTargetName;
        String aBaseFontName(aSearchName, 0, (nFeat != STRING_NOTFOUND)?nFeat:aSearchName.Len());
        if (nFeat != STRING_NOTFOUND && STRING_NOTFOUND !=
            aSearchName.Search(grutils::GrFeatureParser::FEAT_ID_VALUE_SEPARATOR, nFeat))
        {
            aSearchName = aBaseFontName;
            rFSD.maTargetName = aBaseFontName;
        }

#endif

        ImplGetEnglishSearchFontName( aSearchName );
        ImplFontSubstitute( aSearchName, nSubstFlags, pDevSpecific );
        // #114999# special emboldening for Ricoh fonts
        // TODO: smarter check for special cases by using PreMatch infrastructure?
        if( (rFSD.meWeight > WEIGHT_MEDIUM)
        &&  aSearchName.EqualsAscii( "hg", 0, 2) )
        {
            String aBoldName;
            if( aSearchName.EqualsAscii( "hggothicb", 0, 9) )
                aBoldName = String(RTL_CONSTASCII_USTRINGPARAM("hggothice"));
            else if( aSearchName.EqualsAscii( "hgpgothicb", 0, 10) )
                aBoldName = String(RTL_CONSTASCII_USTRINGPARAM("hgpgothice"));
            else if( aSearchName.EqualsAscii( "hgminchol", 0, 9) )
                aBoldName = String(RTL_CONSTASCII_USTRINGPARAM("hgminchob"));
            else if( aSearchName.EqualsAscii( "hgpminchol", 0, 10) )
                aBoldName = String(RTL_CONSTASCII_USTRINGPARAM("hgpminchob"));
            else if( aSearchName.EqualsAscii( "hgminchob" ) )
                aBoldName = String(RTL_CONSTASCII_USTRINGPARAM("hgminchoe"));
            else if( aSearchName.EqualsAscii( "hgpminchob" ) )
                aBoldName = String(RTL_CONSTASCII_USTRINGPARAM("hgpminchoe"));

            if( aBoldName.Len() && ImplFindBySearchName( aBoldName ) )
            {
                // the other font is available => use it
                aSearchName = aBoldName;
                // prevent synthetic emboldening of bold version
                rFSD.meWeight = WEIGHT_DONTKNOW;
            }
        }

#ifdef ENABLE_GRAPHITE
		// restore the features to make the font selection data unique
		rFSD.maTargetName = aOrigName;
#endif
        // check if the current font name token or its substitute is valid
        ImplDevFontListData* pFoundData = ImplFindBySearchName( aSearchName );
        if( pFoundData )
            return pFoundData;

        // some systems provide special customization
        // e.g. they suggest "serif" as UI-font, but this name cannot be used directly
        //      because the system wants to map it to another font first, e.g. "Helvetica"
#ifdef ENABLE_GRAPHITE
		// use the target name to search in the prematch hook
		rFSD.maTargetName = aBaseFontName;
#endif
        if( mpPreMatchHook )
            if( mpPreMatchHook->FindFontSubstitute( rFSD ) )
                ImplGetEnglishSearchFontName( aSearchName );
#ifdef ENABLE_GRAPHITE
	    // the prematch hook uses the target name to search, but we now need
	    // to restore the features to make the font selection data unique
	    rFSD.maTargetName = aOrigName;
#endif
		pFoundData = ImplFindBySearchName( aSearchName );
		if( pFoundData )
			return pFoundData;

        // break after last font name token was checked unsuccessfully
        if( nTokenPos == STRING_NOTFOUND)
            break;
        bMultiToken = true;
    }

    // if the first font was not available find the next available font in
    // the semicolon separated list of font names. A font is also considered
    // available when there is a matching entry in the Tools->Options->Fonts
    // dialog witho neither ALWAYS nor SCREENONLY flags set and the substitution
    // font is available
    for( nTokenPos = 0; nTokenPos != STRING_NOTFOUND; )
    {
        if( bMultiToken )
        {
            rFSD.maTargetName = GetNextFontToken( rFSD.maName, nTokenPos );
            aSearchName = rFSD.maTargetName;
            ImplGetEnglishSearchFontName( aSearchName );
        }
        else
            nTokenPos = STRING_NOTFOUND;
        if( mpPreMatchHook )
            if( mpPreMatchHook->FindFontSubstitute( rFSD ) )
                ImplGetEnglishSearchFontName( aSearchName );
        ImplFontSubstitute( aSearchName, nSubstFlags, pDevSpecific );
        ImplDevFontListData* pFoundData = ImplFindBySearchName( aSearchName );
        if( pFoundData )
            return pFoundData;
    }

    // if no font with a directly matching name is available use the
    // first font name token and get its attributes to find a replacement
    if ( bMultiToken )
    {
        nTokenPos = 0;
        rFSD.maTargetName = GetNextFontToken( rFSD.maName, nTokenPos );
        aSearchName = rFSD.maTargetName;
        ImplGetEnglishSearchFontName( aSearchName );
    }

    String      aSearchShortName;
    String      aSearchFamilyName;
    FontWeight  eSearchWeight   = rFSD.meWeight;
    FontWidth   eSearchWidth    = rFSD.meWidthType;
    ULONG       nSearchType     = 0;
    FontSubstConfiguration::getMapName( aSearchName, aSearchShortName, aSearchFamilyName,
                                        eSearchWeight, eSearchWidth, nSearchType );

    // note: the search name was already translated to english (if possible)

    // use the font's shortened name if needed
    if ( aSearchShortName != aSearchName )
    {
       ImplDevFontListData* pFoundData = ImplFindBySearchName( aSearchShortName );
       if( pFoundData )
       {
#ifdef UNX
            /* #96738# don't use mincho as an replacement for "MS Mincho" on X11: Mincho is
            a korean bitmap font that is not suitable here. Use the font replacement table,
            that automatically leads to the desired "HG Mincho Light J". Same story for
            MS Gothic, there are thai and korean "Gothic" fonts, so we even prefer Andale */
            static String aMS_Mincho( RTL_CONSTASCII_USTRINGPARAM("msmincho") );
            static String aMS_Gothic( RTL_CONSTASCII_USTRINGPARAM("msgothic") );
            if ((aSearchName != aMS_Mincho) && (aSearchName != aMS_Gothic))
                // TODO: add heuristic to only throw out the fake ms* fonts
#endif
            {
                return pFoundData;
            }
        }
    }

    // use font fallback
    const FontNameAttr* pFontAttr = NULL;
    if( aSearchName.Len() )
    {
        // get fallback info using FontSubstConfiguration and
        // the target name, it's shortened name and family name in that order
        const FontSubstConfiguration& rFontSubst = *FontSubstConfiguration::get();
        pFontAttr = rFontSubst.getSubstInfo( aSearchName );
        if ( !pFontAttr && (aSearchShortName != aSearchName) )
            pFontAttr = rFontSubst.getSubstInfo( aSearchShortName );
        if ( !pFontAttr && (aSearchFamilyName != aSearchShortName) )
            pFontAttr = rFontSubst.getSubstInfo( aSearchFamilyName );

        // try the font substitutions suggested by the fallback info
        if( pFontAttr )
        {
            ImplDevFontListData* pFoundData = ImplFindBySubstFontAttr( *pFontAttr );
            if( pFoundData )
                return pFoundData;
        }
    }

    // if a target symbol font is not available use a default symbol font
    if( rFSD.IsSymbolFont() )
    {
        com::sun::star::lang::Locale aDefaultLocale( OUString( RTL_CONSTASCII_USTRINGPARAM("en") ), OUString(), OUString() );
        aSearchName = DefaultFontConfiguration::get()->getDefaultFont( aDefaultLocale, DEFAULTFONT_SYMBOL );
        ImplDevFontListData* pFoundData = ImplFindByTokenNames( aSearchName );
        if( pFoundData )
            return pFoundData;
    }

    // now try the other font name tokens
    while( nTokenPos != STRING_NOTFOUND )
    {
        rFSD.maTargetName = GetNextFontToken( rFSD.maName, nTokenPos );
        if( !rFSD.maTargetName.Len() )
            continue;

        aSearchName = rFSD.maTargetName;
        ImplGetEnglishSearchFontName( aSearchName );

        String      aTempShortName;
        String      aTempFamilyName;
        ULONG       nTempType   = 0;
        FontWeight  eTempWeight = rFSD.meWeight;
        FontWidth   eTempWidth  = WIDTH_DONTKNOW;
        FontSubstConfiguration::getMapName( aSearchName, aTempShortName, aTempFamilyName,
                                            eTempWeight, eTempWidth, nTempType );

        // use a shortend token name if available
        if( aTempShortName != aSearchName )
        {
            ImplDevFontListData* pFoundData = ImplFindBySearchName( aTempShortName );
            if( pFoundData )
                return pFoundData;
        }

        // use a font name from font fallback list to determine font attributes

        // get fallback info using FontSubstConfiguration and
        // the target name, it's shortened name and family name in that order
        const FontSubstConfiguration& rFontSubst = *FontSubstConfiguration::get();
        const FontNameAttr* pTempFontAttr = rFontSubst.getSubstInfo( aSearchName );
        if ( !pTempFontAttr && (aTempShortName != aSearchName) )
            pTempFontAttr = rFontSubst.getSubstInfo( aTempShortName );
        if ( !pTempFontAttr && (aTempFamilyName != aTempShortName) )
            pTempFontAttr = rFontSubst.getSubstInfo( aTempFamilyName );

        // try the font substitutions suggested by the fallback info
        if( pTempFontAttr )
        {
            ImplDevFontListData* pFoundData = ImplFindBySubstFontAttr( *pTempFontAttr );
            if( pFoundData )
                return pFoundData;
            if( !pFontAttr )
                pFontAttr = pTempFontAttr;
        }
    }

    // if still needed use the alias names of the installed fonts
    if( mbMapNames )
    {
        ImplDevFontListData* pFoundData = ImplFindByAliasName( rFSD.maTargetName, aSearchShortName );
        if( pFoundData )
            return pFoundData;
    }

    // if still needed use the font request's attributes to find a good match
    switch( rFSD.meLanguage )
    {
        case LANGUAGE_CHINESE:
        case LANGUAGE_CHINESE_SIMPLIFIED:
        case LANGUAGE_CHINESE_SINGAPORE:
            nSearchType |= IMPL_FONT_ATTR_CJK | IMPL_FONT_ATTR_CJK_SC;
            break;
        case LANGUAGE_CHINESE_TRADITIONAL:
        case LANGUAGE_CHINESE_HONGKONG:
        case LANGUAGE_CHINESE_MACAU:
            nSearchType |= IMPL_FONT_ATTR_CJK | IMPL_FONT_ATTR_CJK_TC;
            break;
        case LANGUAGE_KOREAN:
        case LANGUAGE_KOREAN_JOHAB:
            nSearchType |= IMPL_FONT_ATTR_CJK | IMPL_FONT_ATTR_CJK_KR;
            break;
        case LANGUAGE_JAPANESE:
            nSearchType |= IMPL_FONT_ATTR_CJK | IMPL_FONT_ATTR_CJK_JP;
            break;
        default:
            nSearchType |= ImplIsCJKFont( rFSD.maName );
            if( rFSD.IsSymbolFont() )
                nSearchType |= IMPL_FONT_ATTR_SYMBOL;
            break;
    }

    ImplCalcType( nSearchType, eSearchWeight, eSearchWidth, rFSD.meFamily, pFontAttr );
    ImplDevFontListData* pFoundData = ImplFindByAttributes( nSearchType,
        eSearchWeight, eSearchWidth, rFSD.meFamily, rFSD.meItalic, aSearchFamilyName );

    if( pFoundData )
    {
        // overwrite font selection attributes using info from the typeface flags
        if( (eSearchWeight >= WEIGHT_BOLD)
        &&  (eSearchWeight > rFSD.meWeight)
        &&  (pFoundData->mnTypeFaces & IMPL_DEVFONT_BOLD) )
            rFSD.meWeight = eSearchWeight;
        else if( (eSearchWeight < WEIGHT_NORMAL)
        &&  (eSearchWeight < rFSD.meWeight)
        &&  (eSearchWeight != WEIGHT_DONTKNOW)
        &&  (pFoundData->mnTypeFaces & IMPL_DEVFONT_LIGHT) )
            rFSD.meWeight = eSearchWeight;

        if( (nSearchType & IMPL_FONT_ATTR_ITALIC)
        &&  ((rFSD.meItalic == ITALIC_DONTKNOW) || (rFSD.meItalic == ITALIC_NONE))
        &&  (pFoundData->mnTypeFaces & IMPL_DEVFONT_ITALIC) )
            rFSD.meItalic = ITALIC_NORMAL;
    }
    else
    {
        // if still needed fall back to default fonts
        pFoundData = FindDefaultFont();
    }

    return pFoundData;
}

// -----------------------------------------------------------------------

ImplFontEntry* ImplFontCache::GetGlyphFallbackFont( ImplDevFontList* pFontList,
    ImplFontSelectData& rFontSelData, int nFallbackLevel, rtl::OUString& rMissingCodes )
{
    // get a candidate font for glyph fallback
    // unless the previously selected font got a device specific substitution
    // e.g. PsPrint Arial->Helvetica for udiaeresis when Helvetica doesn't support it
    if( nFallbackLevel >= 1)
    {
        ImplDevFontListData* pFallbackData = pFontList->GetGlyphFallbackFont(
            rFontSelData, rMissingCodes, nFallbackLevel-1 );
        // escape when there are no font candidates
        if( !pFallbackData  )
            return NULL;
        // override the font name
        rFontSelData.maName = pFallbackData->GetFamilyName();
        // clear the cached normalized name
        rFontSelData.maSearchName = String();
    }

    // get device font without doing device specific substitutions
    ImplFontEntry* pFallbackFont = GetFontEntry( pFontList, rFontSelData, NULL );
    return pFallbackFont;
}

// -----------------------------------------------------------------------

void ImplFontCache::Release( ImplFontEntry* pEntry )
{
    static const int FONTCACHE_MAX = 50;

    DBG_ASSERT( (pEntry->mnRefCount > 0), "ImplFontCache::Release() - font refcount underflow" );
    if( --pEntry->mnRefCount > 0 )
        return;

    if( ++mnRef0Count < FONTCACHE_MAX )
        return;

    // remove unused entries from font instance cache
    FontInstanceList::iterator it_next = maFontInstanceList.begin();
    while( it_next != maFontInstanceList.end() )
    {
        FontInstanceList::iterator it = it_next++;
        ImplFontEntry* pFontEntry = (*it).second;
        if( pFontEntry->mnRefCount > 0 )
            continue;

        maFontInstanceList.erase( it );
        delete pFontEntry;
        --mnRef0Count;
        DBG_ASSERT( (mnRef0Count>=0), "ImplFontCache::Release() - refcount0 underflow" );

        if( mpFirstEntry == pFontEntry )
            mpFirstEntry = NULL;
    }

    DBG_ASSERT( (mnRef0Count==0), "ImplFontCache::Release() - refcount0 mismatch" );
}

// -----------------------------------------------------------------------

void ImplFontCache::Invalidate()
{
    // delete unreferenced entries
    FontInstanceList::iterator it = maFontInstanceList.begin();
    for(; it != maFontInstanceList.end(); ++it )
    {
        ImplFontEntry* pFontEntry = (*it).second;
        if( pFontEntry->mnRefCount > 0 )
            continue;

        delete pFontEntry;
        --mnRef0Count;
    }

    // #112304# make sure the font cache is really clean
    mpFirstEntry = NULL;
    maFontInstanceList.clear();

    DBG_ASSERT( (mnRef0Count==0), "ImplFontCache::Invalidate() - mnRef0Count non-zero" );
}

// =======================================================================

ImplMultiTextLineInfo::ImplMultiTextLineInfo()
{
    mpLines = new PImplTextLineInfo[MULTITEXTLINEINFO_RESIZE];
    mnLines = 0;
    mnSize  = MULTITEXTLINEINFO_RESIZE;
}


ImplMultiTextLineInfo::~ImplMultiTextLineInfo()
{
    for ( xub_StrLen i = 0; i < mnLines; i++ )
        delete mpLines[i];
    delete [] mpLines;
}

void ImplMultiTextLineInfo::AddLine( ImplTextLineInfo* pLine )
{
    if ( mnSize == mnLines )
    {
        mnSize += MULTITEXTLINEINFO_RESIZE;
        PImplTextLineInfo* pNewLines = new PImplTextLineInfo[mnSize];
        memcpy( pNewLines, mpLines, mnLines*sizeof(PImplTextLineInfo) );
        mpLines = pNewLines;
    }

    mpLines[mnLines] = pLine;
    mnLines++;
}

void ImplMultiTextLineInfo::Clear()
{
    for ( xub_StrLen i = 0; i < mnLines; i++ )
        delete mpLines[i];
    mnLines = 0;
}

// =======================================================================

FontEmphasisMark OutputDevice::ImplGetEmphasisMarkStyle( const Font& rFont )
{
    FontEmphasisMark nEmphasisMark = rFont.GetEmphasisMark();

    // If no Position is set, then calculate the default position, which
    // depends on the language
    if ( !(nEmphasisMark & (EMPHASISMARK_POS_ABOVE | EMPHASISMARK_POS_BELOW)) )
    {
        LanguageType eLang = rFont.GetLanguage();
        // In Chinese Simplified the EmphasisMarks are below/left
        if ( (eLang == LANGUAGE_CHINESE_SIMPLIFIED) ||
             (eLang == LANGUAGE_CHINESE_SINGAPORE) )
            nEmphasisMark |= EMPHASISMARK_POS_BELOW;
        else
        {
            eLang = rFont.GetCJKContextLanguage();
            // In Chinese Simplified the EmphasisMarks are below/left
            if ( (eLang == LANGUAGE_CHINESE_SIMPLIFIED) ||
                 (eLang == LANGUAGE_CHINESE_SINGAPORE) )
                nEmphasisMark |= EMPHASISMARK_POS_BELOW;
            else
                nEmphasisMark |= EMPHASISMARK_POS_ABOVE;
        }
    }

    return nEmphasisMark;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::ImplIsUnderlineAbove( const Font& rFont )
{
    if ( !rFont.IsVertical() )
        return FALSE;

    if( (LANGUAGE_JAPANESE == rFont.GetLanguage())
    ||  (LANGUAGE_JAPANESE == rFont.GetCJKContextLanguage()) )
        // the underline is right for Japanese only
        return TRUE;

    return FALSE;
}

// =======================================================================

void OutputDevice::ImplInitFontList() const
{
    if( ! mpFontList->Count() )
    {
        if( mpGraphics || ImplGetGraphics() )
        {
			RTL_LOGFILE_CONTEXT( aLog, "OutputDevice::ImplInitFontList()" );
            mpGraphics->GetDevFontList( mpFontList );
        }
    }
}

// =======================================================================

void OutputDevice::ImplInitFont() const
{
    DBG_TESTSOLARMUTEX();

    if ( mbInitFont )
    {
        if ( meOutDevType != OUTDEV_PRINTER )
        {
            // decide if antialiasing is appropriate
            bool bNonAntialiased = (GetAntialiasing() & ANTIALIASING_DISABLE_TEXT) != 0;
            const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
            bNonAntialiased |= ((rStyleSettings.GetDisplayOptions() & DISPLAY_OPTION_AA_DISABLE) != 0);
            bNonAntialiased |= (int(rStyleSettings.GetAntialiasingMinPixelHeight()) > mpFontEntry->maFontSelData.mnHeight);
            mpFontEntry->maFontSelData.mbNonAntialiased = bNonAntialiased;
        }

        if( !mpPDFWriter || !mpPDFWriter->isBuiltinFont( mpFontEntry->maFontSelData.mpFontData ) )
        {
            // select font in the device layers
            mpFontEntry->mnSetFontFlags = mpGraphics->SetFont( &(mpFontEntry->maFontSelData), 0 );
        }
        mbInitFont = false;
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplInitTextColor()
{
    DBG_TESTSOLARMUTEX();

    if ( mbInitTextColor )
    {
        mpGraphics->SetTextColor( ImplColorToSal( GetTextColor() ) );
        mbInitTextColor = FALSE;
    }
}

// -----------------------------------------------------------------------

bool OutputDevice::ImplNewFont() const
{
    DBG_TESTSOLARMUTEX();

    // get correct font list on the PDF writer if necessary
    if( mpPDFWriter )
    {
        const ImplSVData* pSVData = ImplGetSVData();
        if( mpFontList == pSVData->maGDIData.mpScreenFontList
        ||  mpFontCache == pSVData->maGDIData.mpScreenFontCache )
            const_cast<OutputDevice&>(*this).ImplUpdateFontData( true );
    }

    if ( !mbNewFont )
        return true;

    // we need a graphics
    if ( !mpGraphics && !ImplGetGraphics() )
        return false;
    SalGraphics* pGraphics = mpGraphics;
    ImplInitFontList();

    // convert to pixel height
    // TODO: replace integer based aSize completely with subpixel accurate type
    float fExactHeight = ImplFloatLogicHeightToDevicePixel( static_cast<float>(maFont.GetHeight()) );
    Size aSize = ImplLogicToDevicePixel( maFont.GetSize() );
    if ( !aSize.Height() )
    {
        // use default pixel height only when logical height is zero
        if ( maFont.GetSize().Height() )
            aSize.Height() = 1;
        else
            aSize.Height() = (12*mnDPIY)/72;
        fExactHeight =  static_cast<float>(aSize.Height());
    }

    // select the default width only when logical width is zero
    if( (0 == aSize.Width()) && (0 != maFont.GetSize().Width()) )
        aSize.Width() = 1;

    // get font entry
    ImplDirectFontSubstitution* pDevSpecificSubst = NULL;
    if( mpOutDevData )
        pDevSpecificSubst = &mpOutDevData->maDevFontSubst;
    ImplFontEntry* pOldEntry = mpFontEntry;
    mpFontEntry = mpFontCache->GetFontEntry( mpFontList, maFont, aSize, fExactHeight, pDevSpecificSubst );
    if( pOldEntry )
        mpFontCache->Release( pOldEntry );

    ImplFontEntry* pFontEntry = mpFontEntry;
    // mark when lower layers need to get involved
    mbNewFont = FALSE;
    if( pFontEntry != pOldEntry )
        mbInitFont = TRUE;

    // select font when it has not been initialized yet
    if ( !pFontEntry->mbInit )
    {
        ImplInitFont();

        // get metric data from device layers
        if ( pGraphics )
        {
            pFontEntry->mbInit = true;

            pFontEntry->maMetric.mnOrientation  = sal::static_int_cast<short>(pFontEntry->maFontSelData.mnOrientation);
            if( mpPDFWriter && mpPDFWriter->isBuiltinFont( pFontEntry->maFontSelData.mpFontData ) )
                mpPDFWriter->getFontMetric( &pFontEntry->maFontSelData, &(pFontEntry->maMetric) );
            else
                pGraphics->GetFontMetric( &(pFontEntry->maMetric) );

            pFontEntry->maMetric.ImplInitTextLineSize( this );
            pFontEntry->maMetric.ImplInitAboveTextLineSize();

            pFontEntry->mnLineHeight = pFontEntry->maMetric.mnAscent + pFontEntry->maMetric.mnDescent;

            if( pFontEntry->maFontSelData.mnOrientation
            && !pFontEntry->maMetric.mnOrientation
            && (meOutDevType != OUTDEV_PRINTER) )
            {
                pFontEntry->mnOwnOrientation = sal::static_int_cast<short>(pFontEntry->maFontSelData.mnOrientation);
                pFontEntry->mnOrientation = pFontEntry->mnOwnOrientation;
            }
            else
                pFontEntry->mnOrientation = pFontEntry->maMetric.mnOrientation;
        }
    }

    // enable kerning array if requested
    if ( maFont.GetKerning() & KERNING_FONTSPECIFIC )
    {
        // TODO: test if physical font supports kerning and disable if not
        if( pFontEntry->maMetric.mbKernableFont )
            mbKerning = true;
    }
    else
        mbKerning = false;
    if ( maFont.GetKerning() & KERNING_ASIAN )
        mbKerning = true;

    // calculate EmphasisArea
    mnEmphasisAscent = 0;
    mnEmphasisDescent = 0;
    if ( maFont.GetEmphasisMark() & EMPHASISMARK_STYLE )
    {
        FontEmphasisMark    nEmphasisMark = ImplGetEmphasisMarkStyle( maFont );
        long                nEmphasisHeight = (pFontEntry->mnLineHeight*250)/1000;
        if ( nEmphasisHeight < 1 )
            nEmphasisHeight = 1;
        if ( nEmphasisMark & EMPHASISMARK_POS_BELOW )
            mnEmphasisDescent = nEmphasisHeight;
        else
            mnEmphasisAscent = nEmphasisHeight;
    }

    // calculate text offset depending on TextAlignment
    TextAlign eAlign = maFont.GetAlign();
    if ( eAlign == ALIGN_BASELINE )
    {
        mnTextOffX = 0;
        mnTextOffY = 0;
    }
    else if ( eAlign == ALIGN_TOP )
    {
        mnTextOffX = 0;
        mnTextOffY = +pFontEntry->maMetric.mnAscent + mnEmphasisAscent;
        if ( pFontEntry->mnOrientation )
            ImplRotatePos( 0, 0, mnTextOffX, mnTextOffY, pFontEntry->mnOrientation );
    }
    else // eAlign == ALIGN_BOTTOM
    {
        mnTextOffX = 0;
        mnTextOffY = -pFontEntry->maMetric.mnDescent + mnEmphasisDescent;
        if ( pFontEntry->mnOrientation )
            ImplRotatePos( 0, 0, mnTextOffX, mnTextOffY, pFontEntry->mnOrientation );
    }

    mbTextLines     = ((maFont.GetUnderline() != UNDERLINE_NONE) && (maFont.GetUnderline() != UNDERLINE_DONTKNOW)) ||
                      ((maFont.GetOverline()  != UNDERLINE_NONE) && (maFont.GetOverline()  != UNDERLINE_DONTKNOW)) ||
                      ((maFont.GetStrikeout() != STRIKEOUT_NONE) && (maFont.GetStrikeout() != STRIKEOUT_DONTKNOW));
    mbTextSpecial   = maFont.IsShadow() || maFont.IsOutline() ||
                      (maFont.GetRelief() != RELIEF_NONE);

    // #95414# fix for OLE objects which use scale factors very creatively
    if( mbMap && !aSize.Width() )
    {
        int nOrigWidth = pFontEntry->maMetric.mnWidth;
        float fStretch = (float)maMapRes.mnMapScNumX * maMapRes.mnMapScDenomY;
        fStretch /= (float)maMapRes.mnMapScNumY * maMapRes.mnMapScDenomX;
        int nNewWidth = (int)(nOrigWidth * fStretch + 0.5);
        if( (nNewWidth != nOrigWidth) && (nNewWidth != 0) )
        {
            Size aOrigSize = maFont.GetSize();
            const_cast<Font&>(maFont).SetSize( Size( nNewWidth, aSize.Height() ) );
            mbMap = FALSE;
            mbNewFont = TRUE;
            ImplNewFont();  // recurse once using stretched width
            mbMap = TRUE;
            const_cast<Font&>(maFont).SetSize( aOrigSize );
        }
    }

    return true;
}

// -----------------------------------------------------------------------

long OutputDevice::ImplGetTextWidth( const SalLayout& rSalLayout ) const
{
    long nWidth = rSalLayout.GetTextWidth();
    nWidth /= rSalLayout.GetUnitsPerPixel();
    return nWidth;
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawTextRect( long nBaseX, long nBaseY,
                                     long nX, long nY, long nWidth, long nHeight )
{
    short nOrientation = mpFontEntry->mnOrientation;
    if ( nOrientation )
    {
        // Rotate rect without rounding problems for 90 degree rotations
        if ( !(nOrientation % 900) )
        {
            nX -= nBaseX;
            nY -= nBaseY;

            if ( nOrientation == 900 )
            {
                long nTemp = nX;
                nX = nY;
                nY = -nTemp;
                nTemp = nWidth;
                nWidth = nHeight;
                nHeight = nTemp;
                nY -= nHeight;
            }
            else if ( nOrientation == 1800 )
            {
                nX = -nX;
                nY = -nY;
                nX -= nWidth;
                nY -= nHeight;
            }
            else /* ( nOrientation == 2700 ) */
            {
                long nTemp = nX;
                nX = -nY;
                nY = nTemp;
                nTemp = nWidth;
                nWidth = nHeight;
                nHeight = nTemp;
                nX -= nWidth;
            }

            nX += nBaseX;
            nY += nBaseY;
        }
        else
        {
            // inflate because polygons are drawn smaller
            Rectangle aRect( Point( nX, nY ), Size( nWidth+1, nHeight+1 ) );
            Polygon   aPoly( aRect );
            aPoly.Rotate( Point( nBaseX, nBaseY ), mpFontEntry->mnOrientation );
            ImplDrawPolygon( aPoly );
            return;
        }
    }

    mpGraphics->DrawRect( nX, nY, nWidth, nHeight, this );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawTextBackground( const SalLayout& rSalLayout )
{
    const long nWidth = rSalLayout.GetTextWidth() / rSalLayout.GetUnitsPerPixel();
    const Point aBase = rSalLayout.DrawBase();
    const long nX = aBase.X();
    const long nY = aBase.Y();

    if ( mbLineColor || mbInitLineColor )
    {
        mpGraphics->SetLineColor();
        mbInitLineColor = TRUE;
    }
    mpGraphics->SetFillColor( ImplColorToSal( GetTextFillColor() ) );
    mbInitFillColor = TRUE;

    ImplDrawTextRect( nX, nY, nX, nY-mpFontEntry->maMetric.mnAscent-mnEmphasisAscent,
                      nWidth,
                      mpFontEntry->mnLineHeight+mnEmphasisAscent+mnEmphasisDescent );
}

// -----------------------------------------------------------------------

Rectangle OutputDevice::ImplGetTextBoundRect( const SalLayout& rSalLayout )
{
    Point aPoint = rSalLayout.GetDrawPosition();
    long nX = aPoint.X();
    long nY = aPoint.Y();

    long nWidth = rSalLayout.GetTextWidth();
    long nHeight = mpFontEntry->mnLineHeight + mnEmphasisAscent + mnEmphasisDescent;

    nY -= mpFontEntry->maMetric.mnAscent + mnEmphasisAscent;

    if ( mpFontEntry->mnOrientation )
    {
        long nBaseX = nX, nBaseY = nY;
        if ( !(mpFontEntry->mnOrientation % 900) )
        {
            long nX2 = nX+nWidth;
            long nY2 = nY+nHeight;
            ImplRotatePos( nBaseX, nBaseY, nX, nY, mpFontEntry->mnOrientation );
            ImplRotatePos( nBaseX, nBaseY, nX2, nY2, mpFontEntry->mnOrientation );
            nWidth = nX2-nX;
            nHeight = nY2-nY;
        }
        else
        {
            // inflate by +1+1 because polygons are drawn smaller
            Rectangle aRect( Point( nX, nY ), Size( nWidth+1, nHeight+1 ) );
            Polygon   aPoly( aRect );
            aPoly.Rotate( Point( nBaseX, nBaseY ), mpFontEntry->mnOrientation );
            return aPoly.GetBoundRect();
        }
    }

    return Rectangle( Point( nX, nY ), Size( nWidth, nHeight ) );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplInitTextLineSize()
{
    mpFontEntry->maMetric.ImplInitTextLineSize( this );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplInitAboveTextLineSize()
{
    mpFontEntry->maMetric.ImplInitAboveTextLineSize();
}

// -----------------------------------------------------------------------

ImplFontMetricData::ImplFontMetricData( const ImplFontSelectData& rFontSelData )
:   ImplFontAttributes( rFontSelData )
{
	// initialize the members provided by the font request
    mnWidth        = rFontSelData.mnWidth;
    mnOrientation  = sal::static_int_cast<short>(rFontSelData.mnOrientation);

	// intialize the used font name 
    if( rFontSelData.mpFontData )
    {
        maName     = rFontSelData.mpFontData->maName;
        maStyleName= rFontSelData.mpFontData->maStyleName;
        mbDevice   = rFontSelData.mpFontData->mbDevice;
        mbKernableFont = true;
    }
    else
    {
        xub_StrLen nTokenPos = 0;
        maName     = GetNextFontToken( rFontSelData.maName, nTokenPos );
        maStyleName= rFontSelData.maStyleName;
        mbDevice   = false;
        mbKernableFont = false;
    }

	// reset metrics that are usually measured for the font instance 
    mnAscent       = 0;
    mnDescent      = 0;
    mnIntLeading   = 0;
    mnExtLeading   = 0;
    mnSlant        = 0;
    mnMinKashida   = 0;

    // reset metrics that are usually derived from the measurements
    mnUnderlineSize            = 0;
    mnUnderlineOffset          = 0;
    mnBUnderlineSize           = 0;
    mnBUnderlineOffset         = 0;
    mnDUnderlineSize           = 0;
    mnDUnderlineOffset1        = 0;
    mnDUnderlineOffset2        = 0;
    mnWUnderlineSize           = 0;
    mnWUnderlineOffset         = 0;
    mnAboveUnderlineSize       = 0;
    mnAboveUnderlineOffset     = 0;
    mnAboveBUnderlineSize      = 0;
    mnAboveBUnderlineOffset    = 0;
    mnAboveDUnderlineSize      = 0;
    mnAboveDUnderlineOffset1   = 0;
    mnAboveDUnderlineOffset2   = 0;
    mnAboveWUnderlineSize      = 0;
    mnAboveWUnderlineOffset    = 0;
    mnStrikeoutSize            = 0;
    mnStrikeoutOffset          = 0;
    mnBStrikeoutSize           = 0;
    mnBStrikeoutOffset         = 0;
    mnDStrikeoutSize           = 0;
    mnDStrikeoutOffset1        = 0;
    mnDStrikeoutOffset2        = 0;
}

// -----------------------------------------------------------------------

void ImplFontMetricData::ImplInitTextLineSize( const OutputDevice* pDev )
{
    long nDescent = mnDescent;
    if ( nDescent <= 0 )
    {
        nDescent = mnAscent / 10;
        if ( !nDescent )
            nDescent = 1;
    }

    // #i55341# for some fonts it is not a good idea to calculate
    // their text line metrics from the real font descent
    // => work around this problem just for these fonts
    if( 3*nDescent > mnAscent )
        nDescent = mnAscent / 3;

    long nLineHeight = ((nDescent*25)+50) / 100;
    if ( !nLineHeight )
        nLineHeight = 1;
    long nLineHeight2 = nLineHeight / 2;
    if ( !nLineHeight2 )
        nLineHeight2 = 1;

    long nBLineHeight = ((nDescent*50)+50) / 100;
    if ( nBLineHeight == nLineHeight )
        nBLineHeight++;
    long nBLineHeight2 = nBLineHeight/2;
    if ( !nBLineHeight2 )
        nBLineHeight2 = 1;

    long n2LineHeight = ((nDescent*16)+50) / 100;
    if ( !n2LineHeight )
        n2LineHeight = 1;
    long n2LineDY = n2LineHeight;
     /* #117909#
      * add some pixels to minimum double line distance on higher resolution devices
      */
    long nMin2LineDY = 1 + pDev->ImplGetDPIY()/150;
    if ( n2LineDY < nMin2LineDY )
        n2LineDY = nMin2LineDY;
    long n2LineDY2 = n2LineDY/2;
    if ( !n2LineDY2 )
        n2LineDY2 = 1;

    long nUnderlineOffset = mnDescent/2 + 1;
    long nStrikeoutOffset = -((mnAscent - mnIntLeading) / 3);

    mnUnderlineSize        = nLineHeight;
    mnUnderlineOffset      = nUnderlineOffset - nLineHeight2;

    mnBUnderlineSize       = nBLineHeight;
    mnBUnderlineOffset     = nUnderlineOffset - nBLineHeight2;

    mnDUnderlineSize       = n2LineHeight;
    mnDUnderlineOffset1    = nUnderlineOffset - n2LineDY2 - n2LineHeight;
    mnDUnderlineOffset2    = mnDUnderlineOffset1 + n2LineDY + n2LineHeight;

    long nWCalcSize = mnDescent;
    if ( nWCalcSize < 6 )
    {
        if ( (nWCalcSize == 1) || (nWCalcSize == 2) )
            mnWUnderlineSize = nWCalcSize;
        else
            mnWUnderlineSize = 3;
    }
    else
        mnWUnderlineSize = ((nWCalcSize*50)+50) / 100;

    // #109280# the following line assures that wavelnes are never placed below the descent, however
    // for most fonts the waveline then is drawn into the text, so we better keep the old solution
    // pFontEntry->maMetric.mnWUnderlineOffset     = pFontEntry->maMetric.mnDescent + 1 - pFontEntry->maMetric.mnWUnderlineSize;
    mnWUnderlineOffset     = nUnderlineOffset;

    mnStrikeoutSize        = nLineHeight;
    mnStrikeoutOffset      = nStrikeoutOffset - nLineHeight2;

    mnBStrikeoutSize       = nBLineHeight;
    mnBStrikeoutOffset     = nStrikeoutOffset - nBLineHeight2;

    mnDStrikeoutSize       = n2LineHeight;
    mnDStrikeoutOffset1    = nStrikeoutOffset - n2LineDY2 - n2LineHeight;
    mnDStrikeoutOffset2    = mnDStrikeoutOffset1 + n2LineDY + n2LineHeight;
}

// -----------------------------------------------------------------------

void ImplFontMetricData::ImplInitAboveTextLineSize()
{
    long nIntLeading = mnIntLeading;
    // TODO: assess usage of nLeading below (changed in extleading CWS)
    // if no leading is available, we assume 15% of the ascent
    if ( nIntLeading <= 0 )
    {
        nIntLeading = mnAscent*15/100;
        if ( !nIntLeading )
            nIntLeading = 1;
    }

    long nLineHeight = ((nIntLeading*25)+50) / 100;
    if ( !nLineHeight )
        nLineHeight = 1;

    long nBLineHeight = ((nIntLeading*50)+50) / 100;
    if ( nBLineHeight == nLineHeight )
        nBLineHeight++;

    long n2LineHeight = ((nIntLeading*16)+50) / 100;
    if ( !n2LineHeight )
        n2LineHeight = 1;

    long nCeiling = -mnAscent;

    mnAboveUnderlineSize       = nLineHeight;
    mnAboveUnderlineOffset     = nCeiling + (nIntLeading - nLineHeight + 1) / 2;

    mnAboveBUnderlineSize      = nBLineHeight;
    mnAboveBUnderlineOffset    = nCeiling + (nIntLeading - nBLineHeight + 1) / 2;

    mnAboveDUnderlineSize      = n2LineHeight;
    mnAboveDUnderlineOffset1   = nCeiling + (nIntLeading - 3*n2LineHeight + 1) / 2;
    mnAboveDUnderlineOffset2   = nCeiling + (nIntLeading +   n2LineHeight + 1) / 2;

    long nWCalcSize = nIntLeading;
    if ( nWCalcSize < 6 )
    {
        if ( (nWCalcSize == 1) || (nWCalcSize == 2) )
            mnAboveWUnderlineSize = nWCalcSize;
        else
            mnAboveWUnderlineSize = 3;
    }
    else
        mnAboveWUnderlineSize = ((nWCalcSize*50)+50) / 100;

    mnAboveWUnderlineOffset = nCeiling + (nIntLeading + 1) / 2;
}

// -----------------------------------------------------------------------

static void ImplDrawWavePixel( long nOriginX, long nOriginY,
                               long nCurX, long nCurY,
                               short nOrientation,
                               SalGraphics* pGraphics,
                               OutputDevice* pOutDev,
                               BOOL bDrawPixAsRect,

                               long nPixWidth, long nPixHeight )
{
    if ( nOrientation )
        ImplRotatePos( nOriginX, nOriginY, nCurX, nCurY, nOrientation );

    if ( bDrawPixAsRect )
    {

        pGraphics->DrawRect( nCurX, nCurY, nPixWidth, nPixHeight, pOutDev );
    }
    else
    {
        pGraphics->DrawPixel( nCurX, nCurY, pOutDev );
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawWaveLine( long nBaseX, long nBaseY,
                                     long nStartX, long nStartY,
                                     long nWidth, long nHeight,
                                     long nLineWidth, short nOrientation,
                                     const Color& rColor )
{
    if ( !nHeight )
        return;

    // Bei Hoehe von 1 Pixel reicht es, eine Linie auszugeben
    if ( (nLineWidth == 1) && (nHeight == 1) )
    {
        mpGraphics->SetLineColor( ImplColorToSal( rColor ) );
        mbInitLineColor = TRUE;

        long nEndX = nStartX+nWidth;
        long nEndY = nStartY;
        if ( nOrientation )
        {
            ImplRotatePos( nBaseX, nBaseY, nStartX, nStartY, nOrientation );
            ImplRotatePos( nBaseX, nBaseY, nEndX, nEndY, nOrientation );
        }
        mpGraphics->DrawLine( nStartX, nStartY, nEndX, nEndY, this );

    }
    else
    {
        long    nCurX = nStartX;
        long    nCurY = nStartY;
        long    nDiffX = 2;
        long    nDiffY = nHeight-1;
        long    nCount = nWidth;
        long    nOffY = -1;
        long    nFreq;
        long    i;
        long    nPixWidth;
        long    nPixHeight;
        BOOL    bDrawPixAsRect;
        // Auf Druckern die Pixel per DrawRect() ausgeben
        if ( (GetOutDevType() == OUTDEV_PRINTER) || (nLineWidth > 1) )
        {
            if ( mbLineColor || mbInitLineColor )
            {
                mpGraphics->SetLineColor();
                mbInitLineColor = TRUE;
            }
            mpGraphics->SetFillColor( ImplColorToSal( rColor ) );
            mbInitFillColor = TRUE;
            bDrawPixAsRect  = TRUE;
            nPixWidth       = nLineWidth;
            nPixHeight      = ((nLineWidth*mnDPIX)+(mnDPIY/2))/mnDPIY;
        }
        else
        {
            mpGraphics->SetLineColor( ImplColorToSal( rColor ) );
            mbInitLineColor = TRUE;
            nPixWidth       = 1;
            nPixHeight      = 1;
            bDrawPixAsRect  = FALSE;
        }

        if ( !nDiffY )
        {
            while ( nWidth )
            {
                ImplDrawWavePixel( nBaseX, nBaseY, nCurX, nCurY, nOrientation,
                                   mpGraphics, this,
                                   bDrawPixAsRect, nPixWidth, nPixHeight );
                nCurX++;
                nWidth--;
            }
        }
        else
        {
            nCurY += nDiffY;
            nFreq = nCount / (nDiffX+nDiffY);
            while ( nFreq-- )
            {
                for( i = nDiffY; i; --i )
                {
                    ImplDrawWavePixel( nBaseX, nBaseY, nCurX, nCurY, nOrientation,
                                       mpGraphics, this,
                                       bDrawPixAsRect, nPixWidth, nPixHeight );
                    nCurX++;
                    nCurY += nOffY;
                }
                for( i = nDiffX; i; --i )
                {
                    ImplDrawWavePixel( nBaseX, nBaseY, nCurX, nCurY, nOrientation,
                                       mpGraphics, this,
                                       bDrawPixAsRect, nPixWidth, nPixHeight );
                    nCurX++;
                }
                nOffY = -nOffY;
            }
            nFreq = nCount % (nDiffX+nDiffY);
            if ( nFreq )
            {
                for( i = nDiffY; i && nFreq; --i, --nFreq )
                {
                    ImplDrawWavePixel( nBaseX, nBaseY, nCurX, nCurY, nOrientation,
                                       mpGraphics, this,
                                       bDrawPixAsRect, nPixWidth, nPixHeight );
                    nCurX++;
                    nCurY += nOffY;

                }
                for( i = nDiffX; i && nFreq; --i, --nFreq )
                {
                    ImplDrawWavePixel( nBaseX, nBaseY, nCurX, nCurY, nOrientation,
                                       mpGraphics, this,
                                       bDrawPixAsRect, nPixWidth, nPixHeight );
                    nCurX++;
                }
            }
        }

    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawWaveTextLine( long nBaseX, long nBaseY,
                                         long nX, long nY, long nWidth,
                                         FontUnderline eTextLine,
                                         Color aColor,
                                         BOOL bIsAbove )
{
    ImplFontEntry*  pFontEntry = mpFontEntry;
    long            nLineHeight;
    long            nLinePos;

    if ( bIsAbove )
    {
        nLineHeight = pFontEntry->maMetric.mnAboveWUnderlineSize;
        nLinePos = pFontEntry->maMetric.mnAboveWUnderlineOffset;
    }
    else
    {
        nLineHeight = pFontEntry->maMetric.mnWUnderlineSize;
        nLinePos = pFontEntry->maMetric.mnWUnderlineOffset;
    }
    if ( (eTextLine == UNDERLINE_SMALLWAVE) && (nLineHeight > 3) )
        nLineHeight = 3;
    long nLineWidth = (mnDPIX/300);
    if ( !nLineWidth )
        nLineWidth = 1;
    if ( eTextLine == UNDERLINE_BOLDWAVE )
        nLineWidth *= 2;
    nLinePos += nY - (nLineHeight / 2);
    long nLineWidthHeight = ((nLineWidth*mnDPIX)+(mnDPIY/2))/mnDPIY;
    if ( eTextLine == UNDERLINE_DOUBLEWAVE )
    {
        long nOrgLineHeight = nLineHeight;
        nLineHeight /= 3;
        if ( nLineHeight < 2 )
        {
            if ( nOrgLineHeight > 1 )
                nLineHeight = 2;
            else
                nLineHeight = 1;
        }
        long nLineDY = nOrgLineHeight-(nLineHeight*2);
        if ( nLineDY < nLineWidthHeight )
            nLineDY = nLineWidthHeight;
        long nLineDY2 = nLineDY/2;
        if ( !nLineDY2 )
            nLineDY2 = 1;

        nLinePos -= nLineWidthHeight-nLineDY2;
        ImplDrawWaveLine( nBaseX, nBaseY, nX, nLinePos, nWidth, nLineHeight,
                          nLineWidth, mpFontEntry->mnOrientation, aColor );
        nLinePos += nLineWidthHeight+nLineDY;
        ImplDrawWaveLine( nBaseX, nBaseY, nX, nLinePos, nWidth, nLineHeight,
                          nLineWidth, mpFontEntry->mnOrientation, aColor );
    }
    else
    {
        nLinePos -= nLineWidthHeight/2;
        ImplDrawWaveLine( nBaseX, nBaseY, nX, nLinePos, nWidth, nLineHeight,
                          nLineWidth, mpFontEntry->mnOrientation, aColor );
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawStraightTextLine( long nBaseX, long nBaseY,
                                             long nX, long nY, long nWidth,
                                             FontUnderline eTextLine,
                                             Color aColor,
                                             BOOL bIsAbove )
{
    ImplFontEntry*  pFontEntry = mpFontEntry;
    long            nLineHeight = 0;
    long            nLinePos  = 0;
    long            nLinePos2 = 0;

    if ( eTextLine > UNDERLINE_LAST )
        eTextLine = UNDERLINE_SINGLE;

    switch ( eTextLine )
    {
        case UNDERLINE_SINGLE:
        case UNDERLINE_DOTTED:
        case UNDERLINE_DASH:
        case UNDERLINE_LONGDASH:
        case UNDERLINE_DASHDOT:
        case UNDERLINE_DASHDOTDOT:
            if ( bIsAbove )
            {
                nLineHeight = pFontEntry->maMetric.mnAboveUnderlineSize;
                nLinePos    = nY + pFontEntry->maMetric.mnAboveUnderlineOffset;
            }
            else
            {
                nLineHeight = pFontEntry->maMetric.mnUnderlineSize;
                nLinePos    = nY + pFontEntry->maMetric.mnUnderlineOffset;
            }
            break;
        case UNDERLINE_BOLD:
        case UNDERLINE_BOLDDOTTED:
        case UNDERLINE_BOLDDASH:
        case UNDERLINE_BOLDLONGDASH:
        case UNDERLINE_BOLDDASHDOT:
        case UNDERLINE_BOLDDASHDOTDOT:
            if ( bIsAbove )
            {
                nLineHeight = pFontEntry->maMetric.mnAboveBUnderlineSize;
                nLinePos    = nY + pFontEntry->maMetric.mnAboveBUnderlineOffset;
            }
            else
            {
                nLineHeight = pFontEntry->maMetric.mnBUnderlineSize;
                nLinePos    = nY + pFontEntry->maMetric.mnBUnderlineOffset;
            }
            break;
        case UNDERLINE_DOUBLE:
            if ( bIsAbove )
            {
                nLineHeight = pFontEntry->maMetric.mnAboveDUnderlineSize;
                nLinePos    = nY + pFontEntry->maMetric.mnAboveDUnderlineOffset1;
                nLinePos2   = nY + pFontEntry->maMetric.mnAboveDUnderlineOffset2;
            }
            else
            {
                nLineHeight = pFontEntry->maMetric.mnDUnderlineSize;
                nLinePos    = nY + pFontEntry->maMetric.mnDUnderlineOffset1;
                nLinePos2   = nY + pFontEntry->maMetric.mnDUnderlineOffset2;
            }
            break;
        default:
            break;
    }

    if ( nLineHeight )
    {
        if ( mbLineColor || mbInitLineColor )
        {
            mpGraphics->SetLineColor();
            mbInitLineColor = TRUE;
        }
        mpGraphics->SetFillColor( ImplColorToSal( aColor ) );
        mbInitFillColor = TRUE;

        long nLeft = nX;

        switch ( eTextLine )
        {
            case UNDERLINE_SINGLE:
            case UNDERLINE_BOLD:
                ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nWidth, nLineHeight );
                break;
            case UNDERLINE_DOUBLE:
                ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos,  nWidth, nLineHeight );
                ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos2, nWidth, nLineHeight );
                break;
            case UNDERLINE_DOTTED:
            case UNDERLINE_BOLDDOTTED:
                {
                    long nDotWidth = nLineHeight*mnDPIY;
                    nDotWidth += mnDPIY/2;
                    nDotWidth /= mnDPIY;
                    long nTempWidth = nDotWidth;
                    long nEnd = nLeft+nWidth;
                    while ( nLeft < nEnd )
                    {
                        if ( nLeft+nTempWidth > nEnd )
                            nTempWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempWidth, nLineHeight );
                        nLeft += nDotWidth*2;
                    }
                }
                break;
            case UNDERLINE_DASH:
            case UNDERLINE_LONGDASH:
            case UNDERLINE_BOLDDASH:
            case UNDERLINE_BOLDLONGDASH:
                {
                    long nDotWidth = nLineHeight*mnDPIY;
                    nDotWidth += mnDPIY/2;
                    nDotWidth /= mnDPIY;
                    long nMinDashWidth;
                    long nMinSpaceWidth;
                    long nSpaceWidth;
                    long nDashWidth;
                    if ( (eTextLine == UNDERLINE_LONGDASH) ||
                         (eTextLine == UNDERLINE_BOLDLONGDASH) )
                    {
                        nMinDashWidth = nDotWidth*6;
                        nMinSpaceWidth = nDotWidth*2;
                        nDashWidth = 200;
                        nSpaceWidth = 100;
                    }
                    else
                    {
                        nMinDashWidth = nDotWidth*4;
                        nMinSpaceWidth = (nDotWidth*150)/100;
                        nDashWidth = 100;
                        nSpaceWidth = 50;
                    }
                    nDashWidth = ((nDashWidth*mnDPIX)+1270)/2540;
                    nSpaceWidth = ((nSpaceWidth*mnDPIX)+1270)/2540;
                    // DashWidth wird gegebenenfalls verbreitert, wenn
                    // die dicke der Linie im Verhaeltnis zur Laenge
                    // zu dick wird
                    if ( nDashWidth < nMinDashWidth )
                        nDashWidth = nMinDashWidth;
                    if ( nSpaceWidth < nMinSpaceWidth )
                        nSpaceWidth = nMinSpaceWidth;
                    long nTempWidth = nDashWidth;
                    long nEnd = nLeft+nWidth;
                    while ( nLeft < nEnd )
                    {
                        if ( nLeft+nTempWidth > nEnd )
                            nTempWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempWidth, nLineHeight );
                        nLeft += nDashWidth+nSpaceWidth;
                    }
                }
                break;
            case UNDERLINE_DASHDOT:
            case UNDERLINE_BOLDDASHDOT:
                {
                    long nDotWidth = nLineHeight*mnDPIY;
                    nDotWidth += mnDPIY/2;
                    nDotWidth /= mnDPIY;
                    long nDashWidth = ((100*mnDPIX)+1270)/2540;
                    long nMinDashWidth = nDotWidth*4;
                    // DashWidth wird gegebenenfalls verbreitert, wenn
                    // die dicke der Linie im Verhaeltnis zur Laenge
                    // zu dick wird
                    if ( nDashWidth < nMinDashWidth )
                        nDashWidth = nMinDashWidth;
                    long nTempDotWidth = nDotWidth;
                    long nTempDashWidth = nDashWidth;
                    long nEnd = nLeft+nWidth;
                    while ( nLeft < nEnd )
                    {
                        if ( nLeft+nTempDotWidth > nEnd )
                            nTempDotWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempDotWidth, nLineHeight );
                        nLeft += nDotWidth*2;
                        if ( nLeft > nEnd )
                            break;
                        if ( nLeft+nTempDashWidth > nEnd )
                            nTempDashWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempDashWidth, nLineHeight );
                        nLeft += nDashWidth+nDotWidth;
                    }
                }
                break;
            case UNDERLINE_DASHDOTDOT:
            case UNDERLINE_BOLDDASHDOTDOT:
                {
                    long nDotWidth = nLineHeight*mnDPIY;
                    nDotWidth += mnDPIY/2;
                    nDotWidth /= mnDPIY;
                    long nDashWidth = ((100*mnDPIX)+1270)/2540;
                    long nMinDashWidth = nDotWidth*4;
                    // DashWidth wird gegebenenfalls verbreitert, wenn
                    // die dicke der Linie im Verhaeltnis zur Laenge
                    // zu dick wird
                    if ( nDashWidth < nMinDashWidth )
                        nDashWidth = nMinDashWidth;
                    long nTempDotWidth = nDotWidth;
                    long nTempDashWidth = nDashWidth;
                    long nEnd = nLeft+nWidth;
                    while ( nLeft < nEnd )
                    {
                        if ( nLeft+nTempDotWidth > nEnd )
                            nTempDotWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempDotWidth, nLineHeight );
                        nLeft += nDotWidth*2;
                        if ( nLeft > nEnd )
                            break;
                        if ( nLeft+nTempDotWidth > nEnd )
                            nTempDotWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempDotWidth, nLineHeight );
                        nLeft += nDotWidth*2;
                        if ( nLeft > nEnd )
                            break;
                        if ( nLeft+nTempDashWidth > nEnd )
                            nTempDashWidth = nEnd-nLeft;
                        ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nTempDashWidth, nLineHeight );
                        nLeft += nDashWidth+nDotWidth;
                    }
                }
                break;
            default:
                break;
        }
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawStrikeoutLine( long nBaseX, long nBaseY,
                                          long nX, long nY, long nWidth,
                                          FontStrikeout eStrikeout,
                                          Color aColor )
{
    ImplFontEntry*  pFontEntry = mpFontEntry;
    long            nLineHeight = 0;
    long            nLinePos  = 0;
    long            nLinePos2 = 0;

    if ( eStrikeout > STRIKEOUT_LAST )
        eStrikeout = STRIKEOUT_SINGLE;

    switch ( eStrikeout )
    {
        case STRIKEOUT_SINGLE:
            nLineHeight = pFontEntry->maMetric.mnStrikeoutSize;
            nLinePos    = nY + pFontEntry->maMetric.mnStrikeoutOffset;
            break;
        case STRIKEOUT_BOLD:
            nLineHeight = pFontEntry->maMetric.mnBStrikeoutSize;
            nLinePos    = nY + pFontEntry->maMetric.mnBStrikeoutOffset;
            break;
        case STRIKEOUT_DOUBLE:
            nLineHeight = pFontEntry->maMetric.mnDStrikeoutSize;
            nLinePos    = nY + pFontEntry->maMetric.mnDStrikeoutOffset1;
            nLinePos2   = nY + pFontEntry->maMetric.mnDStrikeoutOffset2;
            break;
        default:
            break;
    }

    if ( nLineHeight )
    {
        if ( mbLineColor || mbInitLineColor )
        {
            mpGraphics->SetLineColor();
            mbInitLineColor = TRUE;
        }
        mpGraphics->SetFillColor( ImplColorToSal( aColor ) );
        mbInitFillColor = TRUE;

        long nLeft = nX;

        switch ( eStrikeout )
        {
            case STRIKEOUT_SINGLE:
            case STRIKEOUT_BOLD:
                ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nWidth, nLineHeight );
                break;
            case STRIKEOUT_DOUBLE:
                ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos, nWidth, nLineHeight );
                ImplDrawTextRect( nBaseX, nBaseY, nLeft, nLinePos2, nWidth, nLineHeight );
                break;
            default:
                break;
        }
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawStrikeoutChar( long nBaseX, long nBaseY,
                                          long nX, long nY, long nWidth,
                                          FontStrikeout eStrikeout,
                                          Color aColor )
{
    // PDF-export does its own strikeout drawing... why again?
    if( mpPDFWriter && mpPDFWriter->isBuiltinFont(mpFontEntry->maFontSelData.mpFontData) )
        return;

    // prepare string for strikeout measurement
    static char cStrikeoutChar;
    if ( eStrikeout == STRIKEOUT_SLASH )
        cStrikeoutChar = '/';
    else // ( eStrikeout == STRIKEOUT_X )
        cStrikeoutChar = 'X';
	static const int nTestStrLen = 4;
    static const int nMaxStrikeStrLen = 2048;
    xub_Unicode aChars[ nMaxStrikeStrLen +1]; // +1 for valgrind...
    for( int i = 0; i < nTestStrLen; ++i)
        aChars[i] = cStrikeoutChar;
    const String aStrikeoutTest( aChars, nTestStrLen );

    // calculate approximation of strikeout atom size
    long nStrikeoutWidth = nWidth;
    SalLayout* pLayout = ImplLayout( aStrikeoutTest, 0, nTestStrLen );
    if( pLayout )
    {
        nStrikeoutWidth = (pLayout->GetTextWidth() +nTestStrLen/2) / (nTestStrLen * pLayout->GetUnitsPerPixel());        
        pLayout->Release();
    }
    if( nStrikeoutWidth <= 0 ) // sanity check
        return;

    // calculate acceptable strikeout length
    // allow the strikeout to be one pixel larger than the text it strikes out
    long nMaxWidth = nStrikeoutWidth / 2;
    if ( nMaxWidth < 2 )
        nMaxWidth = 2;
    nMaxWidth += nWidth + 1;

    int nStrikeStrLen = (nMaxWidth + nStrikeoutWidth - 1) / nStrikeoutWidth;
    // if the text width is smaller than the strikeout text, then do not
    // strike out at all. This case requires user interaction, e.g. adding
    // a space to the text
    if( nStrikeStrLen <= 0 )
        return;
    if( nStrikeStrLen > nMaxStrikeStrLen )
        nStrikeStrLen = nMaxStrikeStrLen;

    // build the strikeout string
    for( int i = nTestStrLen; i < nStrikeStrLen; ++i)
        aChars[i] = cStrikeoutChar;
    const String aStrikeoutText( aChars, xub_StrLen(nStrikeStrLen) );

    if( mpFontEntry->mnOrientation )
        ImplRotatePos( nBaseX, nBaseY, nX, nY, mpFontEntry->mnOrientation );

    // strikeout text has to be left aligned
    ULONG nOrigTLM = mnTextLayoutMode;
    mnTextLayoutMode = TEXT_LAYOUT_BIDI_STRONG | TEXT_LAYOUT_COMPLEX_DISABLED;
    pLayout = ImplLayout( aStrikeoutText, 0, STRING_LEN );
    mnTextLayoutMode = nOrigTLM;

    if( !pLayout )
        return;

    // draw the strikeout text
    const Color aOldColor = GetTextColor();
    SetTextColor( aColor );
    ImplInitTextColor();

    pLayout->DrawBase() = Point( nX+mnTextOffX, nY+mnTextOffY );
    pLayout->DrawText( *mpGraphics );
    pLayout->Release();

    SetTextColor( aOldColor );
    ImplInitTextColor();
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawTextLine( long nBaseX,
                                     long nX, long nY, long nWidth,
                                     FontStrikeout eStrikeout,
                                     FontUnderline eUnderline,
                                     FontUnderline eOverline,
                                     BOOL bUnderlineAbove )
{
    if ( !nWidth )
        return;

    Color           aStrikeoutColor = GetTextColor();
    Color           aUnderlineColor = GetTextLineColor();
    Color           aOverlineColor  = GetOverlineColor();
    BOOL            bStrikeoutDone = FALSE;
    BOOL            bUnderlineDone = FALSE;
    BOOL            bOverlineDone  = FALSE;

    // TODO: fix rotated text
    if ( IsRTLEnabled() )
        // --- RTL --- mirror at basex
        nX = nBaseX - nWidth - (nX - nBaseX - 1);

    if ( !IsTextLineColor() )
        aUnderlineColor = GetTextColor();

    if ( !IsOverlineColor() )
        aOverlineColor = GetTextColor();

    if ( (eUnderline == UNDERLINE_SMALLWAVE) ||
         (eUnderline == UNDERLINE_WAVE) ||
         (eUnderline == UNDERLINE_DOUBLEWAVE) ||
         (eUnderline == UNDERLINE_BOLDWAVE) )
    {
        ImplDrawWaveTextLine( nBaseX, nY, nX, nY, nWidth, eUnderline, aUnderlineColor, bUnderlineAbove );
        bUnderlineDone = TRUE;
    }
    if ( (eOverline == UNDERLINE_SMALLWAVE) ||
         (eOverline == UNDERLINE_WAVE) ||
         (eOverline == UNDERLINE_DOUBLEWAVE) ||
         (eOverline == UNDERLINE_BOLDWAVE) )
    {
        ImplDrawWaveTextLine( nBaseX, nY, nX, nY, nWidth, eOverline, aOverlineColor, TRUE );
        bOverlineDone = TRUE;
    }

    if ( (eStrikeout == STRIKEOUT_SLASH) ||
         (eStrikeout == STRIKEOUT_X) )
    {
        ImplDrawStrikeoutChar( nBaseX, nY, nX, nY, nWidth, eStrikeout, aStrikeoutColor );
        bStrikeoutDone = TRUE;
    }

    if ( !bUnderlineDone )
        ImplDrawStraightTextLine( nBaseX, nY, nX, nY, nWidth, eUnderline, aUnderlineColor, bUnderlineAbove );

    if ( !bOverlineDone )
        ImplDrawStraightTextLine( nBaseX, nY, nX, nY, nWidth, eOverline, aOverlineColor, TRUE );

    if ( !bStrikeoutDone )
        ImplDrawStrikeoutLine( nBaseX, nY, nX, nY, nWidth, eStrikeout, aStrikeoutColor );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawTextLines( SalLayout& rSalLayout,
    FontStrikeout eStrikeout, FontUnderline eUnderline, FontUnderline eOverline, BOOL bWordLine, BOOL bUnderlineAbove )
{
    if( bWordLine )
    {
        Point aPos, aStartPt;
        sal_Int32 nWidth = 0, nAdvance=0;
        for( int nStart = 0;;)
        {
            sal_GlyphId nGlyphIndex;
            if( !rSalLayout.GetNextGlyphs( 1, &nGlyphIndex, aPos, nStart, &nAdvance ) )
                break;

            if( !rSalLayout.IsSpacingGlyph( nGlyphIndex ) )
            {
                if( !nWidth )
                {
                    aStartPt = aPos;//rSalLayout.DrawBase() - (aPos - rSalLayout.DrawOffset());
                }

                nWidth += nAdvance;
            }
            else if( nWidth > 0 )
            {
                ImplDrawTextLine( rSalLayout.DrawBase().X(), aStartPt.X(), aStartPt.Y(), nWidth,
                    eStrikeout, eUnderline, eOverline, bUnderlineAbove );
                nWidth = 0;
            }
        }

        if( nWidth > 0 )
        {
            ImplDrawTextLine( rSalLayout.DrawBase().X(), aStartPt.X(), aStartPt.Y(), nWidth,
                eStrikeout, eUnderline, eOverline, bUnderlineAbove );
        }
    }
    else
    {
        Point aStartPt = rSalLayout.GetDrawPosition();
        int nWidth = rSalLayout.GetTextWidth() / rSalLayout.GetUnitsPerPixel();
        ImplDrawTextLine( rSalLayout.DrawBase().X(), aStartPt.X(), aStartPt.Y(), nWidth,
            eStrikeout, eUnderline, eOverline, bUnderlineAbove );
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawMnemonicLine( long nX, long nY, long nWidth )
{
    long nBaseX = nX;
    if( /*ImplHasMirroredGraphics() &&*/ IsRTLEnabled() )
    {
        // --- RTL ---
        // add some strange offset
        nX += 2;
        // revert the hack that will be done later in ImplDrawTextLine
        nX = nBaseX - nWidth - (nX - nBaseX - 1);
    }

    ImplDrawTextLine( nBaseX, nX, nY, nWidth, STRIKEOUT_NONE, UNDERLINE_SINGLE, UNDERLINE_NONE, FALSE );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplGetEmphasisMark( PolyPolygon& rPolyPoly, BOOL& rPolyLine,
                                        Rectangle& rRect1, Rectangle& rRect2,
                                        long& rYOff, long& rWidth,
                                        FontEmphasisMark eEmphasis,
                                        long nHeight, short /*nOrient*/ )
{
    static const BYTE aAccentPolyFlags[24] =
    {
        0, 2, 2, 0, 2, 2, 0, 2, 2, 0, 2, 2, 0, 2, 2, 0, 2, 2, 0, 0, 2, 0, 2, 2
    };

    static const long aAccentPos[48] =
    {
         78,      0,
        348,     79,
        599,    235,
        843,    469,
        938,    574,
        990,    669,
        990,    773,
        990,    843,
        964,    895,
        921,    947,
        886,    982,
        860,    999,
        825,    999,
        764,    999,
        721,    964,
        686,    895,
        625,    791,
        556,    660,
        469,    504,
        400,    400,
        261,    252,
         61,     61,
          0,     27,
          9,      0
    };

    rWidth      = 0;
    rYOff       = 0;
    rPolyLine   = FALSE;

    if ( !nHeight )
        return;

    FontEmphasisMark    nEmphasisStyle = eEmphasis & EMPHASISMARK_STYLE;
    long                nDotSize = 0;
    switch ( nEmphasisStyle )
    {
        case EMPHASISMARK_DOT:
            // Dot has 55% of the height
            nDotSize = (nHeight*550)/1000;
            if ( !nDotSize )
                nDotSize = 1;
            if ( nDotSize <= 2 )
                rRect1 = Rectangle( Point(), Size( nDotSize, nDotSize ) );
            else
            {
                long nRad = nDotSize/2;
                Polygon aPoly( Point( nRad, nRad ), nRad, nRad );
                rPolyPoly.Insert( aPoly );
            }
            rYOff = ((nHeight*250)/1000)/2; // Center to the anthoer EmphasisMarks
            rWidth = nDotSize;
            break;

        case EMPHASISMARK_CIRCLE:
            // Dot has 80% of the height
            nDotSize = (nHeight*800)/1000;
            if ( !nDotSize )
                nDotSize = 1;
            if ( nDotSize <= 2 )
                rRect1 = Rectangle( Point(), Size( nDotSize, nDotSize ) );
            else
            {
                long nRad = nDotSize/2;
                Polygon aPoly( Point( nRad, nRad ), nRad, nRad );
                rPolyPoly.Insert( aPoly );
                // BorderWidth is 15%
                long nBorder = (nDotSize*150)/1000;
                if ( nBorder <= 1 )
                    rPolyLine = TRUE;
                else
                {
                    Polygon aPoly2( Point( nRad, nRad ),
                                    nRad-nBorder, nRad-nBorder );
                    rPolyPoly.Insert( aPoly2 );
                }
            }
            rWidth = nDotSize;
            break;

        case EMPHASISMARK_DISC:
            // Dot has 80% of the height
            nDotSize = (nHeight*800)/1000;
            if ( !nDotSize )
                nDotSize = 1;
            if ( nDotSize <= 2 )
                rRect1 = Rectangle( Point(), Size( nDotSize, nDotSize ) );
            else
            {
                long nRad = nDotSize/2;
                Polygon aPoly( Point( nRad, nRad ), nRad, nRad );
                rPolyPoly.Insert( aPoly );
            }
            rWidth = nDotSize;
            break;

        case EMPHASISMARK_ACCENT:
            // Dot has 80% of the height
            nDotSize = (nHeight*800)/1000;
            if ( !nDotSize )
                nDotSize = 1;
            if ( nDotSize <= 2 )
            {
                if ( nDotSize == 1 )
                {
                    rRect1 = Rectangle( Point(), Size( nDotSize, nDotSize ) );
                    rWidth = nDotSize;
                }
                else
                {
                    rRect1 = Rectangle( Point(), Size( 1, 1 ) );
                    rRect2 = Rectangle( Point( 1, 1 ), Size( 1, 1 ) );
                }
            }
            else
            {
                Polygon aPoly( sizeof( aAccentPos ) / sizeof( long ) / 2,
                               (const Point*)aAccentPos,
                               aAccentPolyFlags );
                double dScale = ((double)nDotSize)/1000.0;
                aPoly.Scale( dScale, dScale );
                Polygon aTemp;
                aPoly.AdaptiveSubdivide( aTemp );
                Rectangle aBoundRect = aTemp.GetBoundRect();
                rWidth = aBoundRect.GetWidth();
                nDotSize = aBoundRect.GetHeight();
                rPolyPoly.Insert( aTemp );
            }
            break;
    }

    // calculate position
    long nOffY = 1+(mnDPIY/300); // one visible pixel space
    long nSpaceY = nHeight-nDotSize;
    if ( nSpaceY >= nOffY*2 )
        rYOff += nOffY;
    if ( !(eEmphasis & EMPHASISMARK_POS_BELOW) )
        rYOff += nDotSize;
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawEmphasisMark( long nBaseX, long nX, long nY,
                                         const PolyPolygon& rPolyPoly, BOOL bPolyLine,
                                         const Rectangle& rRect1, const Rectangle& rRect2 )
{
    // TODO: pass nWidth as width of this mark
    long nWidth = 0;

    if( IsRTLEnabled() )
        // --- RTL --- mirror at basex
        nX = nBaseX - nWidth - (nX - nBaseX - 1);

    nX -= mnOutOffX;
    nY -= mnOutOffY;

    if ( rPolyPoly.Count() )
    {
        if ( bPolyLine )
        {
            Polygon aPoly = rPolyPoly.GetObject( 0 );
            aPoly.Move( nX, nY );
            DrawPolyLine( aPoly );
        }
        else
        {
            PolyPolygon aPolyPoly = rPolyPoly;
            aPolyPoly.Move( nX, nY );
            DrawPolyPolygon( aPolyPoly );
        }
    }

    if ( !rRect1.IsEmpty() )
    {
        Rectangle aRect( Point( nX+rRect1.Left(),
                                nY+rRect1.Top() ), rRect1.GetSize() );
        DrawRect( aRect );
    }

    if ( !rRect2.IsEmpty() )
    {
        Rectangle aRect( Point( nX+rRect2.Left(),
                                nY+rRect2.Top() ), rRect2.GetSize() );

        DrawRect( aRect );
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawEmphasisMarks( SalLayout& rSalLayout )
{
    Color               aOldColor       = GetTextColor();
    Color               aOldLineColor   = GetLineColor();
    Color               aOldFillColor   = GetFillColor();
    BOOL                bOldMap         = mbMap;
    GDIMetaFile*        pOldMetaFile    = mpMetaFile;
    mpMetaFile = NULL;
    EnableMapMode( FALSE );

    FontEmphasisMark    nEmphasisMark = ImplGetEmphasisMarkStyle( maFont );
    PolyPolygon         aPolyPoly;
    Rectangle           aRect1;
    Rectangle           aRect2;
    long                nEmphasisYOff;
    long                nEmphasisWidth;
    long                nEmphasisHeight;
    BOOL                bPolyLine;

    if ( nEmphasisMark & EMPHASISMARK_POS_BELOW )
        nEmphasisHeight = mnEmphasisDescent;
    else
        nEmphasisHeight = mnEmphasisAscent;

    ImplGetEmphasisMark( aPolyPoly, bPolyLine,
                         aRect1, aRect2,
                         nEmphasisYOff, nEmphasisWidth,
                         nEmphasisMark,
                         nEmphasisHeight, mpFontEntry->mnOrientation );

    if ( bPolyLine )
    {
        SetLineColor( GetTextColor() );
        SetFillColor();
    }
    else
    {
        SetLineColor();
        SetFillColor( GetTextColor() );
    }

    Point aOffset = Point(0,0);

    if ( nEmphasisMark & EMPHASISMARK_POS_BELOW )
        aOffset.Y() += mpFontEntry->maMetric.mnDescent + nEmphasisYOff;
    else
        aOffset.Y() -= mpFontEntry->maMetric.mnAscent + nEmphasisYOff;

    long nEmphasisWidth2  = nEmphasisWidth / 2;
    long nEmphasisHeight2 = nEmphasisHeight / 2;
    aOffset += Point( nEmphasisWidth2, nEmphasisHeight2 );

    Point aOutPoint;
    Rectangle aRectangle;
    for( int nStart = 0;;)
    {
        sal_GlyphId nGlyphIndex;
        if( !rSalLayout.GetNextGlyphs( 1, &nGlyphIndex, aOutPoint, nStart ) )
            break;

        if( !mpGraphics->GetGlyphBoundRect( nGlyphIndex, aRectangle ) )
            continue;

        if( !rSalLayout.IsSpacingGlyph( nGlyphIndex ) )
        {
            Point aAdjPoint = aOffset;
            aAdjPoint.X() += aRectangle.Left() + (aRectangle.GetWidth() - nEmphasisWidth) / 2;
            if ( mpFontEntry->mnOrientation )
                ImplRotatePos( 0, 0, aAdjPoint.X(), aAdjPoint.Y(), mpFontEntry->mnOrientation );
            aOutPoint += aAdjPoint;
            aOutPoint -= Point( nEmphasisWidth2, nEmphasisHeight2 );
            ImplDrawEmphasisMark( rSalLayout.DrawBase().X(),
                                  aOutPoint.X(), aOutPoint.Y(),
                                  aPolyPoly, bPolyLine, aRect1, aRect2 );
        }
    }

    SetLineColor( aOldLineColor );
    SetFillColor( aOldFillColor );
    EnableMapMode( bOldMap );
    mpMetaFile = pOldMetaFile;
}

// -----------------------------------------------------------------------

bool OutputDevice::ImplDrawRotateText( SalLayout& rSalLayout )
{
    int nX = rSalLayout.DrawBase().X();
    int nY = rSalLayout.DrawBase().Y();

    Rectangle aBoundRect;
    rSalLayout.DrawBase() = Point( 0, 0 );
    rSalLayout.DrawOffset() = Point( 0, 0 );
    if( !rSalLayout.GetBoundRect( *mpGraphics, aBoundRect ) )
    {
        // guess vertical text extents if GetBoundRect failed
        int nRight = rSalLayout.GetTextWidth();
        int nTop = mpFontEntry->maMetric.mnAscent + mnEmphasisAscent;
        long nHeight = mpFontEntry->mnLineHeight + mnEmphasisAscent + mnEmphasisDescent;
        aBoundRect = Rectangle( 0, -nTop, nRight, nHeight - nTop );
    }

    // cache virtual device for rotation
    if ( !mpOutDevData )
        ImplInitOutDevData();
    if ( !mpOutDevData->mpRotateDev )
        mpOutDevData->mpRotateDev = new VirtualDevice( *this, 1 );
    VirtualDevice* pVDev = mpOutDevData->mpRotateDev;

    // size it accordingly
    if( !pVDev->SetOutputSizePixel( aBoundRect.GetSize() ) )
        return false;

    Font aFont( GetFont() );
    aFont.SetOrientation( 0 );
    aFont.SetSize( Size( mpFontEntry->maFontSelData.mnWidth, mpFontEntry->maFontSelData.mnHeight ) );
    pVDev->SetFont( aFont );
    pVDev->SetTextColor( Color( COL_BLACK ) );
    pVDev->SetTextFillColor();
    pVDev->ImplNewFont();
    pVDev->ImplInitFont();
    pVDev->ImplInitTextColor();

    // draw text into upper left corner
    rSalLayout.DrawBase() -= aBoundRect.TopLeft();
    rSalLayout.DrawText( *((OutputDevice*)pVDev)->mpGraphics );

    Bitmap aBmp = pVDev->GetBitmap( Point(), aBoundRect.GetSize() );
    if ( !aBmp || !aBmp.Rotate( mpFontEntry->mnOwnOrientation, COL_WHITE ) )
        return false;

    // calculate rotation offset
    Polygon aPoly( aBoundRect );
    aPoly.Rotate( Point(), mpFontEntry->mnOwnOrientation );
    Point aPoint = aPoly.GetBoundRect().TopLeft();
    aPoint += Point( nX, nY );

    // mask output with text colored bitmap
    GDIMetaFile* pOldMetaFile = mpMetaFile;
    long nOldOffX = mnOutOffX;
    long nOldOffY = mnOutOffY;
    BOOL bOldMap = mbMap;

    mnOutOffX   = 0L;
    mnOutOffY   = 0L;
    mpMetaFile  = NULL;
    EnableMapMode( FALSE );

    DrawMask( aPoint, aBmp, GetTextColor() );

    EnableMapMode( bOldMap );
    mnOutOffX   = nOldOffX;
    mnOutOffY   = nOldOffY;
    mpMetaFile  = pOldMetaFile;

    return true;
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawTextDirect( SalLayout& rSalLayout, BOOL bTextLines )
{
    if( mpFontEntry->mnOwnOrientation )
        if( ImplDrawRotateText( rSalLayout ) )
            return;

    long nOldX = rSalLayout.DrawBase().X();
    if( ! (mpPDFWriter && mpPDFWriter->isBuiltinFont(mpFontEntry->maFontSelData.mpFontData) ) )
    {
        if( ImplHasMirroredGraphics() )
        {
            long w = meOutDevType == OUTDEV_VIRDEV ? mnOutWidth : mpGraphics->GetGraphicsWidth();
            long x = rSalLayout.DrawBase().X();
       		rSalLayout.DrawBase().X() = w - 1 - x;
            if( !IsRTLEnabled() )
            {
                OutputDevice *pOutDevRef = (OutputDevice *)this;
                // mirror this window back
                long devX = w-pOutDevRef->mnOutWidth-pOutDevRef->mnOutOffX;   // re-mirrored mnOutOffX
                rSalLayout.DrawBase().X() = devX + ( pOutDevRef->mnOutWidth - 1 - (rSalLayout.DrawBase().X() - devX) ) ;
            }
        }
        else if( IsRTLEnabled() )
        {
            //long w = meOutDevType == OUTDEV_VIRDEV ? mnOutWidth : mpGraphics->GetGraphicsWidth();
            //long x = rSalLayout.DrawBase().X();
            OutputDevice *pOutDevRef = (OutputDevice *)this;
            // mirror this window back
            long devX = pOutDevRef->mnOutOffX;   // re-mirrored mnOutOffX
            rSalLayout.DrawBase().X() = pOutDevRef->mnOutWidth - 1 - (rSalLayout.DrawBase().X() - devX) + devX;
        }

        rSalLayout.DrawText( *mpGraphics );
    }

    rSalLayout.DrawBase().X() = nOldX;

    if( bTextLines )
        ImplDrawTextLines( rSalLayout,
            maFont.GetStrikeout(), maFont.GetUnderline(), maFont.GetOverline(),
            maFont.IsWordLineMode(), ImplIsUnderlineAbove( maFont ) );

    // emphasis marks
    if( maFont.GetEmphasisMark() & EMPHASISMARK_STYLE )
        ImplDrawEmphasisMarks( rSalLayout );
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawSpecialText( SalLayout& rSalLayout )
{
    Color       aOldColor           = GetTextColor();
    Color       aOldTextLineColor   = GetTextLineColor();
    Color       aOldOverlineColor   = GetOverlineColor();
    FontRelief  eRelief             = maFont.GetRelief();

    Point aOrigPos = rSalLayout.DrawBase();
    if ( eRelief != RELIEF_NONE )
    {
        Color   aReliefColor( COL_LIGHTGRAY );
        Color   aTextColor( aOldColor );

        Color   aTextLineColor( aOldTextLineColor );
        Color   aOverlineColor( aOldOverlineColor );

        // we don't have a automatic color, so black is always drawn on white
        if ( aTextColor.GetColor() == COL_BLACK )
            aTextColor = Color( COL_WHITE );
        if ( aTextLineColor.GetColor() == COL_BLACK )
            aTextLineColor = Color( COL_WHITE );
        if ( aOverlineColor.GetColor() == COL_BLACK )
            aOverlineColor = Color( COL_WHITE );

        // relief-color is black for white text, in all other cases
        // we set this to LightGray
        if ( aTextColor.GetColor() == COL_WHITE )
            aReliefColor = Color( COL_BLACK );
        SetTextLineColor( aReliefColor );
        SetOverlineColor( aReliefColor );
        SetTextColor( aReliefColor );
        ImplInitTextColor();

        // calculate offset - for high resolution printers the offset
        // should be greater so that the effect is visible
        long nOff = 1;
        nOff += mnDPIX/300;

        if ( eRelief == RELIEF_ENGRAVED )
            nOff = -nOff;
        rSalLayout.DrawOffset() += Point( nOff, nOff);
        ImplDrawTextDirect( rSalLayout, mbTextLines );
        rSalLayout.DrawOffset() -= Point( nOff, nOff);

        SetTextLineColor( aTextLineColor );
        SetOverlineColor( aOverlineColor );
        SetTextColor( aTextColor );
        ImplInitTextColor();
        ImplDrawTextDirect( rSalLayout, mbTextLines );

        SetTextLineColor( aOldTextLineColor );
        SetOverlineColor( aOldOverlineColor );

        if ( aTextColor != aOldColor )
        {
            SetTextColor( aOldColor );
            ImplInitTextColor();
        }
    }
    else
    {
        if ( maFont.IsShadow() )
        {
            long nOff = 1 + ((mpFontEntry->mnLineHeight-24)/24);
            if ( maFont.IsOutline() )
                nOff++;
            SetTextLineColor();
            SetOverlineColor();
            if ( (GetTextColor().GetColor() == COL_BLACK)
            ||   (GetTextColor().GetLuminance() < 8) )
                SetTextColor( Color( COL_LIGHTGRAY ) );
            else
                SetTextColor( Color( COL_BLACK ) );
            ImplInitTextColor();
            rSalLayout.DrawBase() += Point( nOff, nOff );
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() -= Point( nOff, nOff );
            SetTextColor( aOldColor );
            SetTextLineColor( aOldTextLineColor );
            SetOverlineColor( aOldOverlineColor );
            ImplInitTextColor();

            if ( !maFont.IsOutline() )
                ImplDrawTextDirect( rSalLayout, mbTextLines );
        }

        if ( maFont.IsOutline() )
        {
            rSalLayout.DrawBase() = aOrigPos + Point(-1,-1);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(+1,+1);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(-1,+0);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(-1,+1);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(+0,+1);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(+0,-1);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(+1,-1);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos + Point(+1,+0);
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            rSalLayout.DrawBase() = aOrigPos;

            SetTextColor( Color( COL_WHITE ) );
            SetTextLineColor( Color( COL_WHITE ) );
            SetOverlineColor( Color( COL_WHITE ) );
            ImplInitTextColor();
            ImplDrawTextDirect( rSalLayout, mbTextLines );
            SetTextColor( aOldColor );
            SetTextLineColor( aOldTextLineColor );
            SetOverlineColor( aOldOverlineColor );
            ImplInitTextColor();
        }
    }
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawText( SalLayout& rSalLayout )
{
    if( mbInitClipRegion )
        ImplInitClipRegion();
    if( mbOutputClipped )
        return;
    if( mbInitTextColor )
        ImplInitTextColor();

    rSalLayout.DrawBase() += Point( mnTextOffX, mnTextOffY );

    if( IsTextFillColor() )
        ImplDrawTextBackground( rSalLayout );

    if( mbTextSpecial )
        ImplDrawSpecialText( rSalLayout );
    else
        ImplDrawTextDirect( rSalLayout, mbTextLines );
}

// -----------------------------------------------------------------------

long OutputDevice::ImplGetTextLines( ImplMultiTextLineInfo& rLineInfo,
                                     long nWidth, const XubString& rStr,
                                     USHORT nStyle ) const
{
    DBG_ASSERTWARNING( nWidth >= 0, "ImplGetTextLines: nWidth <= 0!" );

    if ( nWidth <= 0 )
        nWidth = 1;

    long nMaxLineWidth  = 0;
    rLineInfo.Clear();
    if ( rStr.Len() && (nWidth > 0) )
    {
        ::rtl::OUString aText( rStr );
        uno::Reference < i18n::XBreakIterator > xBI;
        // get service provider
        uno::Reference< lang::XMultiServiceFactory > xSMgr( unohelper::GetMultiServiceFactory() );
        
        uno::Reference< linguistic2::XHyphenator > xHyph;
        if( xSMgr.is() )
        {
            uno::Reference< linguistic2::XLinguServiceManager> xLinguMgr(xSMgr->createInstance(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.linguistic2.LinguServiceManager"))),uno::UNO_QUERY);
            if ( xLinguMgr.is() )
            {
                xHyph = xLinguMgr->getHyphenator();
            }
        }
        
        i18n::LineBreakHyphenationOptions aHyphOptions( xHyph, uno::Sequence <beans::PropertyValue>(), 1 );
        i18n::LineBreakUserOptions aUserOptions;

        xub_StrLen nPos = 0;
        xub_StrLen nLen = rStr.Len();
        while ( nPos < nLen )
        {
            xub_StrLen nBreakPos = nPos;

            while ( ( nBreakPos < nLen ) && ( rStr.GetChar( nBreakPos ) != _CR ) && ( rStr.GetChar( nBreakPos ) != _LF ) )
                nBreakPos++;

            long nLineWidth = GetTextWidth( rStr, nPos, nBreakPos-nPos );
            if ( ( nLineWidth > nWidth ) && ( nStyle & TEXT_DRAW_WORDBREAK ) )
            {
                if ( !xBI.is() )
                    xBI = vcl::unohelper::CreateBreakIterator();

                if ( xBI.is() )
                {
                    const com::sun::star::lang::Locale& rDefLocale(Application::GetSettings().GetUILocale());
                    xub_StrLen nSoftBreak = GetTextBreak( rStr, nWidth, nPos, nBreakPos - nPos );
                    DBG_ASSERT( nSoftBreak < nBreakPos, "Break?!" );
                    //aHyphOptions.hyphenIndex = nSoftBreak;
                    i18n::LineBreakResults aLBR = xBI->getLineBreak( aText, nSoftBreak, rDefLocale, nPos, aHyphOptions, aUserOptions );
                    nBreakPos = (xub_StrLen)aLBR.breakIndex;
                    if ( nBreakPos <= nPos )
                        nBreakPos = nSoftBreak;
                    if ( (nStyle & TEXT_DRAW_WORDBREAK_HYPHENATION) == TEXT_DRAW_WORDBREAK_HYPHENATION )
                    {
                        // Egal ob Trenner oder nicht: Das Wort nach dem Trenner durch
	                    // die Silbentrennung jagen...
	                    // nMaxBreakPos ist das letzte Zeichen was in die Zeile passt,
	                    // nBreakPos ist der Wort-Anfang
	                    // Ein Problem gibt es, wenn das Dok so schmal ist, dass ein Wort
	                    // auf mehr als Zwei Zeilen gebrochen wird...
	                    if ( xHyph.is() )
	                    {
                            sal_Unicode cAlternateReplChar = 0;
	                        sal_Unicode cAlternateExtraChar = 0;
			                i18n::Boundary aBoundary = xBI->getWordBoundary( aText, nBreakPos, rDefLocale, ::com::sun::star::i18n::WordType::DICTIONARY_WORD, sal_True );
                //		    sal_uInt16 nWordStart = nBreakPos;
                //		    sal_uInt16 nBreakPos_OLD = nBreakPos;
		                    sal_uInt16 nWordStart = nPos;
                            sal_uInt16 nWordEnd = (USHORT) aBoundary.endPos;
                            DBG_ASSERT( nWordEnd > nWordStart, "ImpBreakLine: Start >= End?" );

                            USHORT nWordLen = nWordEnd - nWordStart;
		                    if ( ( nWordEnd >= nSoftBreak ) && ( nWordLen > 3 ) )
		                    {
                                // #104415# May happen, because getLineBreak may differ from getWordBoudary with DICTIONARY_WORD
			                    // DBG_ASSERT( nWordEnd >= nMaxBreakPos, "Hyph: Break?" );
		                        String aWord( aText, nWordStart, nWordLen );
			                    sal_uInt16 nMinTrail = static_cast<sal_uInt16>(nWordEnd-nSoftBreak+1); 	//+1: Vor dem angeknacksten Buchstaben
                                uno::Reference< linguistic2::XHyphenatedWord > xHyphWord;
			                    if (xHyph.is())
                                    xHyphWord = xHyph->hyphenate( aWord, rDefLocale, aWord.Len() - nMinTrail, uno::Sequence< beans::PropertyValue >() );
			                    if (xHyphWord.is())
			                    {
				                    sal_Bool bAlternate = xHyphWord->isAlternativeSpelling();
				                    sal_uInt16 _nWordLen = 1 + xHyphWord->getHyphenPos();

				                    if ( ( _nWordLen >= 2 ) && ( (nWordStart+_nWordLen) >= ( 2 ) ) )
				                    {
					                    if ( !bAlternate )
					                    {
						                    nBreakPos = nWordStart + _nWordLen;
					                    }
					                    else
					                    {
						                    String aAlt( xHyphWord->getHyphenatedWord() );

						                    // Wir gehen von zwei Faellen aus, die nun
						                    // vorliegen koennen:
						                    // 1) packen wird zu pak-ken
						                    // 2) Schiffahrt wird zu Schiff-fahrt
						                    // In Fall 1 muss ein Zeichen ersetzt werden,
						                    // in Fall 2 wird ein Zeichen hinzugefuegt.
						                    // Die Identifikation wird erschwert durch Worte wie
						                    // "Schiffahrtsbrennesseln", da der Hyphenator alle
						                    // Position des Wortes auftrennt und "Schifffahrtsbrennnesseln"
						                    // ermittelt. Wir koennen also eigentlich nicht unmittelbar vom
						                    // Index des AlternativWord auf aWord schliessen.

						                    // Das ganze geraffel wird durch eine Funktion am
						                    // Hyphenator vereinfacht werden, sobald AMA sie einbaut...
						                    sal_uInt16 nAltStart = _nWordLen - 1;
						                    sal_uInt16 nTxtStart = nAltStart - (aAlt.Len() - aWord.Len());
						                    sal_uInt16 nTxtEnd = nTxtStart;
						                    sal_uInt16 nAltEnd = nAltStart;

						                    // Die Bereiche zwischen den nStart und nEnd ist
						                    // die Differenz zwischen Alternativ- und OriginalString.
						                    while( nTxtEnd < aWord.Len() && nAltEnd < aAlt.Len() &&
							                       aWord.GetChar(nTxtEnd) != aAlt.GetChar(nAltEnd) )
						                    {
							                    ++nTxtEnd;
							                    ++nAltEnd;
						                    }

						                    // Wenn ein Zeichen hinzugekommen ist, dann bemerken wir es jetzt:
						                    if( nAltEnd > nTxtEnd && nAltStart == nAltEnd &&
							                    aWord.GetChar( nTxtEnd ) == aAlt.GetChar(nAltEnd) )
						                    {
							                    ++nAltEnd;
							                    ++nTxtStart;
							                    ++nTxtEnd;
						                    }

						                    DBG_ASSERT( ( nAltEnd - nAltStart ) == 1, "Alternate: Falsche Annahme!" );

						                    if ( nTxtEnd > nTxtStart )
							                    cAlternateReplChar = aAlt.GetChar( nAltStart );
						                    else
							                    cAlternateExtraChar = aAlt.GetChar( nAltStart );

						                    nBreakPos = nWordStart + nTxtStart;
						                    if ( cAlternateReplChar )
							                    nBreakPos++;
					                    }
			                        } // if (xHyphWord.is())
	                            } // if ( ( nWordEnd >= nSoftBreak ) && ( nWordLen > 3 ) )
                            } // if ( xHyph.is() )
                        } // if ( (nStyle & TEXT_DRAW_WORDBREAK_HYPHENATION) == TEXT_DRAW_WORDBREAK_HYPHENATION )
	                }
                    nLineWidth = GetTextWidth( rStr, nPos, nBreakPos-nPos );
                }
                else
                {
                    // fallback to something really simple
                    USHORT nSpacePos = STRING_LEN;
                    long nW = 0;
                    do
                    {
                        nSpacePos = rStr.SearchBackward( sal_Unicode(' '), nSpacePos );
                        if( nSpacePos != STRING_NOTFOUND )
                        {
                            if( nSpacePos > nPos )
                                nSpacePos--;
                            nW = GetTextWidth( rStr, nPos, nSpacePos-nPos );
                        }
                    } while( nW > nWidth );
                    
                    if( nSpacePos != STRING_NOTFOUND )
                    {
                        nBreakPos = nSpacePos;
                        nLineWidth = GetTextWidth( rStr, nPos, nBreakPos-nPos );
                        if( nBreakPos < rStr.Len()-1 )
                            nBreakPos++;
                    }
                }
            }

            if ( nLineWidth > nMaxLineWidth )
                nMaxLineWidth = nLineWidth;

            rLineInfo.AddLine( new ImplTextLineInfo( nLineWidth, nPos, nBreakPos-nPos ) );

            if ( nBreakPos == nPos )
                nBreakPos++;
            nPos = nBreakPos;

            if ( ( rStr.GetChar( nPos ) == _CR ) || ( rStr.GetChar( nPos ) == _LF ) )
            {
                nPos++;
                // CR/LF?
                if ( ( nPos < nLen ) && ( rStr.GetChar( nPos ) == _LF ) && ( rStr.GetChar( nPos-1 ) == _CR ) )
                    nPos++;
            }
        }
    }
#ifdef DBG_UTIL
    for ( USHORT nL = 0; nL < rLineInfo.Count(); nL++ )
    {
        ImplTextLineInfo* pLine = rLineInfo.GetLine( nL );
        String aLine( rStr, pLine->GetIndex(), pLine->GetLen() );
        DBG_ASSERT( aLine.Search( _CR ) == STRING_NOTFOUND, "ImplGetTextLines - Found CR!" );
        DBG_ASSERT( aLine.Search( _LF ) == STRING_NOTFOUND, "ImplGetTextLines - Found LF!" );
    }
#endif

    return nMaxLineWidth;
}

// =======================================================================

void OutputDevice::SetAntialiasing( USHORT nMode )
{
    if ( mnAntialiasing != nMode )
    {
        mnAntialiasing = nMode;
        mbInitFont = TRUE;

        if(mpGraphics)
        {
            mpGraphics->setAntiAliasB2DDraw(mnAntialiasing & ANTIALIASING_ENABLE_B2DDRAW);
        }
    }

    if( mpAlphaVDev )
        mpAlphaVDev->SetAntialiasing( nMode );
}

// -----------------------------------------------------------------------

void OutputDevice::SetFont( const Font& rNewFont )
{
    DBG_TRACE( "OutputDevice::SetFont()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );
    DBG_CHKOBJ( &rNewFont, Font, NULL );

    Font aFont( rNewFont );
    aFont.SetLanguage(rNewFont.GetLanguage());
    if ( mnDrawMode & (DRAWMODE_BLACKTEXT | DRAWMODE_WHITETEXT | DRAWMODE_GRAYTEXT | DRAWMODE_GHOSTEDTEXT | DRAWMODE_SETTINGSTEXT |
                       DRAWMODE_BLACKFILL | DRAWMODE_WHITEFILL | DRAWMODE_GRAYFILL | DRAWMODE_NOFILL |
                       DRAWMODE_GHOSTEDFILL | DRAWMODE_SETTINGSFILL ) )
    {
        Color aTextColor( aFont.GetColor() );

        if ( mnDrawMode & DRAWMODE_BLACKTEXT )
            aTextColor = Color( COL_BLACK );
        else if ( mnDrawMode & DRAWMODE_WHITETEXT )
            aTextColor = Color( COL_WHITE );
        else if ( mnDrawMode & DRAWMODE_GRAYTEXT )
        {
            const UINT8 cLum = aTextColor.GetLuminance();
            aTextColor = Color( cLum, cLum, cLum );
        }
        else if ( mnDrawMode & DRAWMODE_SETTINGSTEXT )
            aTextColor = GetSettings().GetStyleSettings().GetFontColor();

        if ( mnDrawMode & DRAWMODE_GHOSTEDTEXT )
        {
            aTextColor = Color( (aTextColor.GetRed() >> 1 ) | 0x80,
                                (aTextColor.GetGreen() >> 1 ) | 0x80,
                                (aTextColor.GetBlue() >> 1 ) | 0x80 );
        }

        aFont.SetColor( aTextColor );

        BOOL bTransFill = aFont.IsTransparent();
        if ( !bTransFill )
        {
            Color aTextFillColor( aFont.GetFillColor() );

            if ( mnDrawMode & DRAWMODE_BLACKFILL )
                aTextFillColor = Color( COL_BLACK );
            else if ( mnDrawMode & DRAWMODE_WHITEFILL )
                aTextFillColor = Color( COL_WHITE );
            else if ( mnDrawMode & DRAWMODE_GRAYFILL )
            {
                const UINT8 cLum = aTextFillColor.GetLuminance();
                aTextFillColor = Color( cLum, cLum, cLum );
            }
            else if( mnDrawMode & DRAWMODE_SETTINGSFILL )
                aTextFillColor = GetSettings().GetStyleSettings().GetWindowColor();
            else if ( mnDrawMode & DRAWMODE_NOFILL )
            {
                aTextFillColor = Color( COL_TRANSPARENT );
                bTransFill = TRUE;
            }

            if ( !bTransFill && (mnDrawMode & DRAWMODE_GHOSTEDFILL) )
            {
                aTextFillColor = Color( (aTextFillColor.GetRed() >> 1) | 0x80,
                                        (aTextFillColor.GetGreen() >> 1) | 0x80,
                                        (aTextFillColor.GetBlue() >> 1) | 0x80 );
            }

            aFont.SetFillColor( aTextFillColor );
        }
    }

    if ( mpMetaFile )
    {
        mpMetaFile->AddAction( new MetaFontAction( aFont ) );
        // the color and alignment actions don't belong here
        // TODO: get rid of them without breaking anything...
        mpMetaFile->AddAction( new MetaTextAlignAction( aFont.GetAlign() ) );
        mpMetaFile->AddAction( new MetaTextFillColorAction( aFont.GetFillColor(), !aFont.IsTransparent() ) );
    }

#if (OSL_DEBUG_LEVEL > 2) || defined (HDU_DEBUG)
    fprintf( stderr, "   OutputDevice::SetFont( name=\"%s\", h=%ld)\n",
         OUStringToOString( aFont.GetName(), RTL_TEXTENCODING_UTF8 ).getStr(),
         aFont.GetSize().Height() );
#endif

    if ( !maFont.IsSameInstance( aFont ) )
    {
        // Optimization MT/HDU: COL_TRANSPARENT means SetFont should ignore the font color,
        // because SetTextColor() is used for this.
        // #i28759# maTextColor might have been changed behind our back, commit then, too.
        if( aFont.GetColor() != COL_TRANSPARENT
        && (aFont.GetColor() != maFont.GetColor() || aFont.GetColor() != maTextColor ) )
        {
            maTextColor = aFont.GetColor();
            mbInitTextColor = TRUE;
            if( mpMetaFile )
                mpMetaFile->AddAction( new MetaTextColorAction( aFont.GetColor() ) );
        }
        maFont      = aFont;
        mbNewFont   = TRUE;

        if( mpAlphaVDev )
        {
            // #i30463#
            // Since SetFont might change the text color, apply that only
            // selectively to alpha vdev (which normally paints opaque text
            // with COL_BLACK)
            if( aFont.GetColor() != COL_TRANSPARENT )
            {
                mpAlphaVDev->SetTextColor( COL_BLACK );
                aFont.SetColor( COL_TRANSPARENT );
            }

            mpAlphaVDev->SetFont( aFont );
        }
    }
}

// -----------------------------------------------------------------------

void OutputDevice::SetLayoutMode( ULONG nTextLayoutMode )
{
    DBG_TRACE( "OutputDevice::SetTextLayoutMode()" );

    if( mpMetaFile )
        mpMetaFile->AddAction( new MetaLayoutModeAction( nTextLayoutMode ) );

    mnTextLayoutMode = nTextLayoutMode;

    if( mpAlphaVDev )
        mpAlphaVDev->SetLayoutMode( nTextLayoutMode );
}

// -----------------------------------------------------------------------

void OutputDevice::SetDigitLanguage( LanguageType eTextLanguage )
{
    DBG_TRACE( "OutputDevice::SetTextLanguage()" );

    if( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextLanguageAction( eTextLanguage ) );

    meTextLanguage = eTextLanguage;

    if( mpAlphaVDev )
        mpAlphaVDev->SetDigitLanguage( eTextLanguage );
}

// -----------------------------------------------------------------------

void OutputDevice::SetTextColor( const Color& rColor )
{
    DBG_TRACE( "OutputDevice::SetTextColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    Color aColor( rColor );

    if ( mnDrawMode & ( DRAWMODE_BLACKTEXT | DRAWMODE_WHITETEXT |
                        DRAWMODE_GRAYTEXT | DRAWMODE_GHOSTEDTEXT |
                        DRAWMODE_SETTINGSTEXT ) )
    {
        if ( mnDrawMode & DRAWMODE_BLACKTEXT )
            aColor = Color( COL_BLACK );
        else if ( mnDrawMode & DRAWMODE_WHITETEXT )
            aColor = Color( COL_WHITE );
        else if ( mnDrawMode & DRAWMODE_GRAYTEXT )
        {
            const UINT8 cLum = aColor.GetLuminance();
            aColor = Color( cLum, cLum, cLum );
        }
        else if ( mnDrawMode & DRAWMODE_SETTINGSTEXT )
            aColor = GetSettings().GetStyleSettings().GetFontColor();

        if ( mnDrawMode & DRAWMODE_GHOSTEDTEXT )
        {
            aColor = Color( (aColor.GetRed() >> 1) | 0x80,
                            (aColor.GetGreen() >> 1) | 0x80,
                            (aColor.GetBlue() >> 1) | 0x80 );
        }
    }

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextColorAction( aColor ) );

    if ( maTextColor != aColor )
    {
        maTextColor = aColor;
        mbInitTextColor = TRUE;
    }

    if( mpAlphaVDev )
        mpAlphaVDev->SetTextColor( COL_BLACK );
}

// -----------------------------------------------------------------------

void OutputDevice::SetTextFillColor()
{
    DBG_TRACE( "OutputDevice::SetTextFillColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextFillColorAction( Color(), FALSE ) );

    if ( maFont.GetColor() != Color( COL_TRANSPARENT ) )
        maFont.SetFillColor( Color( COL_TRANSPARENT ) );
    if ( !maFont.IsTransparent() )
        maFont.SetTransparent( TRUE );

    if( mpAlphaVDev )
        mpAlphaVDev->SetTextFillColor();
}

// -----------------------------------------------------------------------

void OutputDevice::SetTextFillColor( const Color& rColor )
{
    DBG_TRACE( "OutputDevice::SetTextFillColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    Color aColor( rColor );
    BOOL bTransFill = ImplIsColorTransparent( aColor ) ? TRUE : FALSE;

    if ( !bTransFill )
    {
        if ( mnDrawMode & ( DRAWMODE_BLACKFILL | DRAWMODE_WHITEFILL |
                            DRAWMODE_GRAYFILL | DRAWMODE_NOFILL |
                            DRAWMODE_GHOSTEDFILL | DRAWMODE_SETTINGSFILL ) )
        {
            if ( mnDrawMode & DRAWMODE_BLACKFILL )
                aColor = Color( COL_BLACK );
            else if ( mnDrawMode & DRAWMODE_WHITEFILL )
                aColor = Color( COL_WHITE );
            else if ( mnDrawMode & DRAWMODE_GRAYFILL )
            {
                const UINT8 cLum = aColor.GetLuminance();
                aColor = Color( cLum, cLum, cLum );
            }
            else if( mnDrawMode & DRAWMODE_SETTINGSFILL )
                aColor = GetSettings().GetStyleSettings().GetWindowColor();
            else if ( mnDrawMode & DRAWMODE_NOFILL )
            {
                aColor = Color( COL_TRANSPARENT );
                bTransFill = TRUE;
            }

            if ( !bTransFill && (mnDrawMode & DRAWMODE_GHOSTEDFILL) )
            {
                aColor = Color( (aColor.GetRed() >> 1) | 0x80,
                                (aColor.GetGreen() >> 1) | 0x80,
                                (aColor.GetBlue() >> 1) | 0x80 );
            }
        }
    }

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextFillColorAction( aColor, TRUE ) );

    if ( maFont.GetFillColor() != aColor )
        maFont.SetFillColor( aColor );
    if ( maFont.IsTransparent() != bTransFill )
        maFont.SetTransparent( bTransFill );

    if( mpAlphaVDev )
        mpAlphaVDev->SetTextFillColor( COL_BLACK );
}

// -----------------------------------------------------------------------

Color OutputDevice::GetTextFillColor() const
{
    if ( maFont.IsTransparent() )
        return Color( COL_TRANSPARENT );
    else
        return maFont.GetFillColor();
}

// -----------------------------------------------------------------------

void OutputDevice::SetTextLineColor()
{
    DBG_TRACE( "OutputDevice::SetTextLineColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextLineColorAction( Color(), FALSE ) );

    maTextLineColor = Color( COL_TRANSPARENT );

    if( mpAlphaVDev )
        mpAlphaVDev->SetTextLineColor();
}

// -----------------------------------------------------------------------

void OutputDevice::SetTextLineColor( const Color& rColor )
{
    DBG_TRACE( "OutputDevice::SetTextLineColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    Color aColor( rColor );

    if ( mnDrawMode & ( DRAWMODE_BLACKTEXT | DRAWMODE_WHITETEXT |
                        DRAWMODE_GRAYTEXT | DRAWMODE_GHOSTEDTEXT |
                        DRAWMODE_SETTINGSTEXT ) )
    {
        if ( mnDrawMode & DRAWMODE_BLACKTEXT )
            aColor = Color( COL_BLACK );
        else if ( mnDrawMode & DRAWMODE_WHITETEXT )
            aColor = Color( COL_WHITE );
        else if ( mnDrawMode & DRAWMODE_GRAYTEXT )
        {
            const UINT8 cLum = aColor.GetLuminance();
            aColor = Color( cLum, cLum, cLum );
        }
        else if ( mnDrawMode & DRAWMODE_SETTINGSTEXT )
            aColor = GetSettings().GetStyleSettings().GetFontColor();

        if( (mnDrawMode & DRAWMODE_GHOSTEDTEXT)
        &&  (aColor.GetColor() != COL_TRANSPARENT) )
        {
            aColor = Color( (aColor.GetRed() >> 1) | 0x80,
                            (aColor.GetGreen() >> 1) | 0x80,
                            (aColor.GetBlue() >> 1) | 0x80 );
        }
    }

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextLineColorAction( aColor, TRUE ) );

    maTextLineColor = aColor;

    if( mpAlphaVDev )
        mpAlphaVDev->SetTextLineColor( COL_BLACK );
}

// -----------------------------------------------------------------------

void OutputDevice::SetOverlineColor()
{
    DBG_TRACE( "OutputDevice::SetOverlineColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaOverlineColorAction( Color(), FALSE ) );

    maOverlineColor = Color( COL_TRANSPARENT );

    if( mpAlphaVDev )
        mpAlphaVDev->SetOverlineColor();
}

// -----------------------------------------------------------------------

void OutputDevice::SetOverlineColor( const Color& rColor )
{
    DBG_TRACE( "OutputDevice::SetOverlineColor()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    Color aColor( rColor );

    if ( mnDrawMode & ( DRAWMODE_BLACKTEXT | DRAWMODE_WHITETEXT |
                        DRAWMODE_GRAYTEXT | DRAWMODE_GHOSTEDTEXT |
                        DRAWMODE_SETTINGSTEXT ) )
    {
        if ( mnDrawMode & DRAWMODE_BLACKTEXT )
            aColor = Color( COL_BLACK );
        else if ( mnDrawMode & DRAWMODE_WHITETEXT )
            aColor = Color( COL_WHITE );
        else if ( mnDrawMode & DRAWMODE_GRAYTEXT )
        {
            const UINT8 cLum = aColor.GetLuminance();
            aColor = Color( cLum, cLum, cLum );
        }
        else if ( mnDrawMode & DRAWMODE_SETTINGSTEXT )
            aColor = GetSettings().GetStyleSettings().GetFontColor();

        if( (mnDrawMode & DRAWMODE_GHOSTEDTEXT)
        &&  (aColor.GetColor() != COL_TRANSPARENT) )
        {
            aColor = Color( (aColor.GetRed() >> 1) | 0x80,
                            (aColor.GetGreen() >> 1) | 0x80,
                            (aColor.GetBlue() >> 1) | 0x80 );
        }
    }

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaOverlineColorAction( aColor, TRUE ) );

    maOverlineColor = aColor;

    if( mpAlphaVDev )
        mpAlphaVDev->SetOverlineColor( COL_BLACK );
}

// -----------------------------------------------------------------------


void OutputDevice::SetTextAlign( TextAlign eAlign )
{
    DBG_TRACE( "OutputDevice::SetTextAlign()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextAlignAction( eAlign ) );

    if ( maFont.GetAlign() != eAlign )
    {
        maFont.SetAlign( eAlign );
        mbNewFont = TRUE;
    }

    if( mpAlphaVDev )
        mpAlphaVDev->SetTextAlign( eAlign );
}

// -----------------------------------------------------------------------

void OutputDevice::DrawTextLine( const Point& rPos, long nWidth,
                                 FontStrikeout eStrikeout,
                                 FontUnderline eUnderline,
                                 FontUnderline eOverline,
                                 BOOL bUnderlineAbove )
{
    DBG_TRACE( "OutputDevice::DrawTextLine()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextLineAction( rPos, nWidth, eStrikeout, eUnderline, eOverline ) );

    if ( ((eUnderline == UNDERLINE_NONE) || (eUnderline == UNDERLINE_DONTKNOW)) &&
         ((eOverline  == UNDERLINE_NONE) || (eOverline  == UNDERLINE_DONTKNOW)) &&
         ((eStrikeout == STRIKEOUT_NONE) || (eStrikeout == STRIKEOUT_DONTKNOW)) )
        return;

    if ( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
        return;

    // we need a graphics
    if( !mpGraphics && !ImplGetGraphics() )
        return;
    if( mbInitClipRegion )
        ImplInitClipRegion();
    if( mbOutputClipped )
        return;

    // initialize font if needed to get text offsets
    // TODO: only needed for mnTextOff!=(0,0)
    if( mbNewFont )
        if( !ImplNewFont() )
            return;
    if( mbInitFont )
        ImplInitFont();

    Point aPos = ImplLogicToDevicePixel( rPos );
    nWidth = ImplLogicWidthToDevicePixel( nWidth );
    aPos += Point( mnTextOffX, mnTextOffY );
    ImplDrawTextLine( aPos.X(), aPos.X(), aPos.Y(), nWidth, eStrikeout, eUnderline, eOverline, bUnderlineAbove );

    if( mpAlphaVDev )
        mpAlphaVDev->DrawTextLine( rPos, nWidth, eStrikeout, eUnderline, eOverline, bUnderlineAbove );
}

// ------------------------------------------------------------------------

BOOL OutputDevice::IsTextUnderlineAbove( const Font& rFont )
{
    return ImplIsUnderlineAbove( rFont );
}

// ------------------------------------------------------------------------

void OutputDevice::DrawWaveLine( const Point& rStartPos, const Point& rEndPos,
                                 USHORT nStyle )
{
    DBG_TRACE( "OutputDevice::DrawWaveLine()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
        return;

    // we need a graphics
    if( !mpGraphics )
        if( !ImplGetGraphics() )
            return;

    if ( mbInitClipRegion )
        ImplInitClipRegion();
    if ( mbOutputClipped )
        return;

    if( mbNewFont )
        if( !ImplNewFont() )
            return;

    Point   aStartPt = ImplLogicToDevicePixel( rStartPos );
    Point   aEndPt = ImplLogicToDevicePixel( rEndPos );
    long    nStartX = aStartPt.X();
    long    nStartY = aStartPt.Y();
    long    nEndX = aEndPt.X();
    long    nEndY = aEndPt.Y();
    short   nOrientation = 0;

    // when rotated
    if ( (nStartY != nEndY) || (nStartX > nEndX) )
    {
        long nDX = nEndX - nStartX;
        double nO = atan2( -nEndY + nStartY, ((nDX == 0L) ? 0.000000001 : nDX) );
        nO /= F_PI1800;
        nOrientation = (short)nO;
        ImplRotatePos( nStartX, nStartY, nEndX, nEndY, -nOrientation );
    }

    long nWaveHeight;
    if ( nStyle == WAVE_NORMAL )
    {
        nWaveHeight = 3;
        nStartY++;
        nEndY++;
    }
    else if( nStyle == WAVE_SMALL )
    {
        nWaveHeight = 2;
        nStartY++;
        nEndY++;
    }
    else // WAVE_FLAT
        nWaveHeight = 1;

     // #109280# make sure the waveline does not exceed the descent to avoid paint problems
     ImplFontEntry* pFontEntry = mpFontEntry;
     if( nWaveHeight > pFontEntry->maMetric.mnWUnderlineSize )
         nWaveHeight = pFontEntry->maMetric.mnWUnderlineSize;

     ImplDrawWaveLine( nStartX, nStartY, nStartX, nStartY,
                      nEndX-nStartX, nWaveHeight, 1,
                      nOrientation, GetLineColor() );
    if( mpAlphaVDev )
        mpAlphaVDev->DrawWaveLine( rStartPos, rEndPos, nStyle );
}

// -----------------------------------------------------------------------

void OutputDevice::DrawText( const Point& rStartPt, const String& rStr,
                             xub_StrLen nIndex, xub_StrLen nLen,
                             MetricVector* pVector, String* pDisplayText
                             )
{
    if( mpOutDevData && mpOutDevData->mpRecordLayout )
    {
        pVector = &mpOutDevData->mpRecordLayout->m_aUnicodeBoundRects;
        pDisplayText = &mpOutDevData->mpRecordLayout->m_aDisplayText;
    }

    DBG_TRACE( "OutputDevice::DrawText()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

#if OSL_DEBUG_LEVEL > 2
    fprintf( stderr, "   OutputDevice::DrawText(\"%s\")\n",
	     OUStringToOString( rStr, RTL_TEXTENCODING_UTF8 ).getStr() );
#endif

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextAction( rStartPt, rStr, nIndex, nLen ) );
    if( pVector )
    {
        Region aClip( GetClipRegion() );
        if( meOutDevType == OUTDEV_WINDOW )
            aClip.Intersect( Rectangle( Point(), GetOutputSize() ) );
        if( mpOutDevData && mpOutDevData->mpRecordLayout )
		{
            mpOutDevData->mpRecordLayout->m_aLineIndices.push_back( mpOutDevData->mpRecordLayout->m_aDisplayText.Len() );
			aClip.Intersect( mpOutDevData->maRecordRect );
		}
        if( ! aClip.IsNull() )
        {
            MetricVector aTmp;
            GetGlyphBoundRects( rStartPt, rStr, nIndex, nLen, nIndex, aTmp );

            bool bInserted = false;
            for( MetricVector::const_iterator it = aTmp.begin(); it != aTmp.end(); ++it, nIndex++ )
            {
                bool bAppend = false;

                if( aClip.IsOver( *it ) )
                    bAppend = true;
                else if( rStr.GetChar( nIndex ) == ' ' && bInserted )
                {
                    MetricVector::const_iterator next = it;
                    ++next;
                    if( next != aTmp.end() && aClip.IsOver( *next ) )
                        bAppend = true;
                }

                if( bAppend )
                {
                    pVector->push_back( *it );
                    if( pDisplayText )
                        pDisplayText->Append( rStr.GetChar( nIndex ) );
                    bInserted = true;
                }
            }
        }
        else
        {
            GetGlyphBoundRects( rStartPt, rStr, nIndex, nLen, nIndex, *pVector );
            if( pDisplayText )
                pDisplayText->Append( rStr.Copy( nIndex, nLen ) );
        }
    }

    if ( !IsDeviceOutputNecessary() || pVector )
        return;

    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen, rStartPt, 0, NULL, true );
    if( pSalLayout )
    {
        ImplDrawText( *pSalLayout );
        pSalLayout->Release();
    }

    if( mpAlphaVDev )
        mpAlphaVDev->DrawText( rStartPt, rStr, nIndex, nLen, pVector, pDisplayText );
}

// -----------------------------------------------------------------------

long OutputDevice::GetTextWidth( const String& rStr,
                                 xub_StrLen nIndex, xub_StrLen nLen ) const
{
    DBG_TRACE( "OutputDevice::GetTextWidth()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    long nWidth = GetTextArray( rStr, NULL, nIndex, nLen );
    return nWidth;
}

// -----------------------------------------------------------------------

long OutputDevice::GetTextHeight() const
{
    DBG_TRACE( "OutputDevice::GetTextHeight()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if( mbNewFont )
        if( !ImplNewFont() )
            return 0;
    if( mbInitFont )
        if( !ImplNewFont() )
            return 0;

    long nHeight = mpFontEntry->mnLineHeight + mnEmphasisAscent + mnEmphasisDescent;

    if ( mbMap )
        nHeight = ImplDevicePixelToLogicHeight( nHeight );

    return nHeight;
}

// -----------------------------------------------------------------------

void OutputDevice::DrawTextArray( const Point& rStartPt, const String& rStr,
                                  const sal_Int32* pDXAry,
                                  xub_StrLen nIndex, xub_StrLen nLen )
{
    DBG_TRACE( "OutputDevice::DrawTextArray()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextArrayAction( rStartPt, rStr, pDXAry, nIndex, nLen ) );

    if ( !IsDeviceOutputNecessary() )
        return;

    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen, rStartPt, 0, pDXAry, true );
    if( pSalLayout )
    {
        ImplDrawText( *pSalLayout );
        pSalLayout->Release();
    }

    if( mpAlphaVDev )
        mpAlphaVDev->DrawTextArray( rStartPt, rStr, pDXAry, nIndex, nLen );
}

// -----------------------------------------------------------------------

long OutputDevice::GetTextArray( const String& rStr, sal_Int32* pDXAry,
                                 xub_StrLen nIndex, xub_StrLen nLen ) const
{
    DBG_TRACE( "OutputDevice::GetTextArray()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if( nIndex >= rStr.Len() )
        return 0;
    if( (ULONG)nIndex+nLen >= rStr.Len() )
        nLen = rStr.Len() - nIndex;

    // do layout
    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen );
    if( !pSalLayout )
        return 0;

    long nWidth = pSalLayout->FillDXArray( pDXAry );
    int nWidthFactor = pSalLayout->GetUnitsPerPixel();
    pSalLayout->Release();

    // convert virtual char widths to virtual absolute positions
    if( pDXAry )
        for( int i = 1; i < nLen; ++i )
            pDXAry[ i ] += pDXAry[ i-1 ];

    // convert from font units to logical units
    if( mbMap )
    {
        if( pDXAry )
            for( int i = 0; i < nLen; ++i )
                pDXAry[i] = ImplDevicePixelToLogicWidth( pDXAry[i] );
        nWidth = ImplDevicePixelToLogicWidth( nWidth );
    }

    if( nWidthFactor > 1 )
    {
        if( pDXAry )
            for( int i = 0; i < nLen; ++i )
                pDXAry[i] /= nWidthFactor;
        nWidth /= nWidthFactor;
    }

    return nWidth;
}

// -----------------------------------------------------------------------

bool OutputDevice::GetCaretPositions( const XubString& rStr, sal_Int32* pCaretXArray,
    xub_StrLen nIndex, xub_StrLen nLen,
    sal_Int32* pDXAry, long nLayoutWidth,
    BOOL bCellBreaking ) const
{
    DBG_TRACE( "OutputDevice::GetCaretPositions()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if( nIndex >= rStr.Len() )
        return false;
    if( (ULONG)nIndex+nLen >= rStr.Len() )
        nLen = rStr.Len() - nIndex;

    // layout complex text
    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen,
        Point(0,0), nLayoutWidth, pDXAry );
    if( !pSalLayout )
        return false;

    int nWidthFactor = pSalLayout->GetUnitsPerPixel();
    pSalLayout->GetCaretPositions( 2*nLen, pCaretXArray );
    long nWidth = pSalLayout->GetTextWidth();
    pSalLayout->Release();

    // fixup unknown caret positions
    int i;
    for( i = 0; i < 2 * nLen; ++i )
        if( pCaretXArray[ i ] >= 0 )
            break;
    long nXPos = pCaretXArray[ i ];
    for( i = 0; i < 2 * nLen; ++i )
    {
        if( pCaretXArray[ i ] >= 0 )
            nXPos = pCaretXArray[ i ];
        else
            pCaretXArray[ i ] = nXPos;
    }

    // handle window mirroring
    if( IsRTLEnabled() )
    {
        for( i = 0; i < 2 * nLen; ++i )
            pCaretXArray[i] = nWidth - pCaretXArray[i] - 1;
    }

    // convert from font units to logical units
    if( mbMap )
    {
        for( i = 0; i < 2*nLen; ++i )
            pCaretXArray[i] = ImplDevicePixelToLogicWidth( pCaretXArray[i] );
    }

    if( nWidthFactor != 1 )
    {
        for( i = 0; i < 2*nLen; ++i )
            pCaretXArray[i] /= nWidthFactor;
    }

    // if requested move caret position to cell limits
    if( bCellBreaking )
    {
        ; // TODO
    }

    return true;
}

// -----------------------------------------------------------------------

void OutputDevice::DrawStretchText( const Point& rStartPt, ULONG nWidth,
                                    const String& rStr,
                                    xub_StrLen nIndex, xub_StrLen nLen )
{
    DBG_TRACE( "OutputDevice::DrawStretchText()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaStretchTextAction( rStartPt, nWidth, rStr, nIndex, nLen ) );

    if ( !IsDeviceOutputNecessary() )
        return;

    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen, rStartPt, nWidth, NULL, true );
    if( pSalLayout )
    {
        ImplDrawText( *pSalLayout );
        pSalLayout->Release();
    }

    if( mpAlphaVDev )
        mpAlphaVDev->DrawStretchText( rStartPt, nWidth, rStr, nIndex, nLen );
}

// -----------------------------------------------------------------------

ImplLayoutArgs OutputDevice::ImplPrepareLayoutArgs( String& rStr,
                                       xub_StrLen nMinIndex, xub_StrLen nLen,
                                       long nPixelWidth, const sal_Int32* pDXArray ) const
{
    // get string length for calculating extents
    xub_StrLen nEndIndex = rStr.Len();
    if( (ULONG)nMinIndex + nLen < nEndIndex )
        nEndIndex = nMinIndex + nLen;

    // don't bother if there is nothing to do
    if( nEndIndex < nMinIndex )
        nEndIndex = nMinIndex;

    int nLayoutFlags = 0;
    if( mnTextLayoutMode & TEXT_LAYOUT_BIDI_RTL )
        nLayoutFlags |= SAL_LAYOUT_BIDI_RTL;
    if( mnTextLayoutMode & TEXT_LAYOUT_BIDI_STRONG )
        nLayoutFlags |= SAL_LAYOUT_BIDI_STRONG;
    else if( 0 == (mnTextLayoutMode & TEXT_LAYOUT_BIDI_RTL) )
    {
        // disable Bidi if no RTL hint and no RTL codes used
        const xub_Unicode* pStr = rStr.GetBuffer() + nMinIndex;
        const xub_Unicode* pEnd = rStr.GetBuffer() + nEndIndex;
        for( ; pStr < pEnd; ++pStr )
            if( ((*pStr >= 0x0580) && (*pStr < 0x0800))   // middle eastern scripts
            ||  ((*pStr >= 0xFB18) && (*pStr < 0xFE00))   // hebrew + arabic A presentation forms
            ||  ((*pStr >= 0xFE70) && (*pStr < 0xFEFF)) ) // arabic presentation forms B
                break;
        if( pStr >= pEnd )
            nLayoutFlags |= SAL_LAYOUT_BIDI_STRONG;
    }

    if( mbKerning )
        nLayoutFlags |= SAL_LAYOUT_KERNING_PAIRS;
    if( maFont.GetKerning() & KERNING_ASIAN )
        nLayoutFlags |= SAL_LAYOUT_KERNING_ASIAN;
    if( maFont.IsVertical() )
        nLayoutFlags |= SAL_LAYOUT_VERTICAL;

    if( mnTextLayoutMode & TEXT_LAYOUT_ENABLE_LIGATURES )
        nLayoutFlags |= SAL_LAYOUT_ENABLE_LIGATURES;
    else if( mnTextLayoutMode & TEXT_LAYOUT_COMPLEX_DISABLED )
        nLayoutFlags |= SAL_LAYOUT_COMPLEX_DISABLED;
    else
    {
        // disable CTL for non-CTL text
        const sal_Unicode* pStr = rStr.GetBuffer() + nMinIndex;
        const sal_Unicode* pEnd = rStr.GetBuffer() + nEndIndex;
        for( ; pStr < pEnd; ++pStr )
            if( ((*pStr >= 0x0300) && (*pStr < 0x0370))   // diacritical marks
            ||  ((*pStr >= 0x0590) && (*pStr < 0x10A0))   // many CTL scripts
            ||  ((*pStr >= 0x1100) && (*pStr < 0x1200))   // hangul jamo
            ||  ((*pStr >= 0x1700) && (*pStr < 0x1900))   // many CTL scripts
            ||  ((*pStr >= 0xFB1D) && (*pStr < 0xFE00))   // middle east presentation
            ||  ((*pStr >= 0xFE70) && (*pStr < 0xFEFF)) ) // arabic presentation B
                break;
        if( pStr >= pEnd )
            nLayoutFlags |= SAL_LAYOUT_COMPLEX_DISABLED;
    }

    if( meTextLanguage ) //TODO: (mnTextLayoutMode & TEXT_LAYOUT_SUBSTITUTE_DIGITS)
    {
        // disable character localization when no digits used
        const sal_Unicode* pBase = rStr.GetBuffer();
        const sal_Unicode* pStr = pBase + nMinIndex;
        const sal_Unicode* pEnd = pBase + nEndIndex;
        for( ; pStr < pEnd; ++pStr )
        {
            // TODO: are there non-digit localizations?
            if( (*pStr >= '0') && (*pStr <= '9') )
            {
                // translate characters to local preference
                sal_UCS4 cChar = GetLocalizedChar( *pStr, meTextLanguage );
                if( cChar != *pStr )
                    // TODO: are the localized digit surrogates?
                    rStr.SetChar( static_cast<USHORT>(pStr - pBase),
                                 static_cast<sal_Unicode>(cChar) );
            }
        }
    }

    // right align for RTL text, DRAWPOS_REVERSED, RTL window style
    bool bRightAlign = ((mnTextLayoutMode & TEXT_LAYOUT_BIDI_RTL) != 0);
    if( mnTextLayoutMode & TEXT_LAYOUT_TEXTORIGIN_LEFT )
        bRightAlign = false;
    else if ( mnTextLayoutMode & TEXT_LAYOUT_TEXTORIGIN_RIGHT )
        bRightAlign = true;
    // SSA: hack for western office, ie text get right aligned
    //      for debugging purposes of mirrored UI
    //static const char* pEnv = getenv( "SAL_RTL_MIRRORTEXT" );
    bool bRTLWindow = IsRTLEnabled();
    bRightAlign ^= bRTLWindow;
    if( bRightAlign )
        nLayoutFlags |= SAL_LAYOUT_RIGHT_ALIGN;

    // set layout options
    ImplLayoutArgs aLayoutArgs( rStr.GetBuffer(), rStr.Len(), nMinIndex, nEndIndex, nLayoutFlags );

    int nOrientation = mpFontEntry ? mpFontEntry->mnOrientation : 0;
    aLayoutArgs.SetOrientation( nOrientation );

    aLayoutArgs.SetLayoutWidth( nPixelWidth );
    aLayoutArgs.SetDXArray( pDXArray );

    return aLayoutArgs;
}

// -----------------------------------------------------------------------

SalLayout* OutputDevice::ImplLayout( const String& rOrigStr,
                                     xub_StrLen nMinIndex,
                                     xub_StrLen nLen,
                                     const Point& rLogicalPos,
                                     long nLogicalWidth,
                                     const sal_Int32* pDXArray,
                                     bool bFilter ) const
{
    // we need a graphics
    if( !mpGraphics )
        if( !ImplGetGraphics() )
            return NULL;

    // initialize font if needed
    if( mbNewFont )
        if( !ImplNewFont() )
            return NULL;
    if( mbInitFont )
        ImplInitFont();

    // check string index and length
    String aStr = rOrigStr;
    if( (ULONG)nMinIndex + nLen >= aStr.Len() )
    {
        if( nMinIndex < aStr.Len() )
            nLen = aStr.Len() - nMinIndex;
        else
            return NULL;
    }

    // filter out special markers
    if( bFilter )
    {
        xub_StrLen nCutStart, nCutStop, nOrgLen = nLen;
        bool bFiltered = mpGraphics->filterText( rOrigStr, aStr, nMinIndex, nLen, nCutStart, nCutStop );
        if( !nLen )
            return NULL;

        if( bFiltered && nCutStop != nCutStart && pDXArray )
        {
            if( !nLen )
                pDXArray = NULL;
            else
            {
                sal_Int32* pAry = (sal_Int32*)alloca(sizeof(sal_Int32)*nLen);
                if( nCutStart > nMinIndex )
                    memcpy( pAry, pDXArray, sizeof(sal_Int32)*(nCutStart-nMinIndex) );
                // note: nCutStart will never be smaller than nMinIndex
                memcpy( pAry+nCutStart-nMinIndex,
                        pDXArray + nOrgLen - (nCutStop-nMinIndex),
                        sizeof(sal_Int32)*(nLen - (nCutStart-nMinIndex)) );
                pDXArray = pAry;
            }
        }
    }

    // convert from logical units to physical units
    // recode string if needed
    if( mpFontEntry->mpConversion )
        ImplRecodeString( mpFontEntry->mpConversion, aStr, 0, aStr.Len() );

    long nPixelWidth = nLogicalWidth;
    if( nLogicalWidth && mbMap )
        nPixelWidth = ImplLogicWidthToDevicePixel( nLogicalWidth );
    if( pDXArray && mbMap )
    {
        // convert from logical units to font units using a temporary array
        sal_Int32* pTempDXAry = (sal_Int32*)alloca( nLen * sizeof(sal_Int32) );
        // using base position for better rounding a.k.a. "dancing characters"
        int nPixelXOfs = ImplLogicWidthToDevicePixel( rLogicalPos.X() );
        for( int i = 0; i < nLen; ++i )
            pTempDXAry[i] = ImplLogicWidthToDevicePixel( rLogicalPos.X() + pDXArray[i] ) - nPixelXOfs;

        pDXArray = pTempDXAry;
    }

    ImplLayoutArgs aLayoutArgs = ImplPrepareLayoutArgs( aStr, nMinIndex, nLen, nPixelWidth, pDXArray );

    // get matching layout object for base font
    SalLayout* pSalLayout = NULL;
    if( mpPDFWriter )
        pSalLayout = mpPDFWriter->GetTextLayout( aLayoutArgs, &mpFontEntry->maFontSelData );

    if( !pSalLayout )
        pSalLayout = mpGraphics->GetTextLayout( aLayoutArgs, 0 );

    // layout text
    if( pSalLayout && !pSalLayout->LayoutText( aLayoutArgs ) )
    {
        pSalLayout->Release();
        pSalLayout = NULL;
    }

    if( !pSalLayout )
        return NULL;

    // do glyph fallback if needed
    // #105768# avoid fallback for very small font sizes
    if( aLayoutArgs.NeedFallback() )
        if( mpFontEntry && (mpFontEntry->maFontSelData.mnHeight >= 3) )
            pSalLayout = ImplGlyphFallbackLayout( pSalLayout, aLayoutArgs );

    // position, justify, etc. the layout
    pSalLayout->AdjustLayout( aLayoutArgs );
    pSalLayout->DrawBase() = ImplLogicToDevicePixel( rLogicalPos );
    // adjust to right alignment if necessary
    if( aLayoutArgs.mnFlags & SAL_LAYOUT_RIGHT_ALIGN )
    {
        long nRTLOffset;
        if( pDXArray )
            nRTLOffset = pDXArray[ nLen - 1 ];
        else if( nPixelWidth )
            nRTLOffset = nPixelWidth;
        else
            nRTLOffset = pSalLayout->GetTextWidth() / pSalLayout->GetUnitsPerPixel();
        pSalLayout->DrawOffset().X() = 1 - nRTLOffset;
    }

    return pSalLayout;
}

// -----------------------------------------------------------------------

SalLayout* OutputDevice::ImplGlyphFallbackLayout( SalLayout* pSalLayout, ImplLayoutArgs& rLayoutArgs ) const
{
    // prepare multi level glyph fallback
    MultiSalLayout* pMultiSalLayout = NULL;
    ImplLayoutRuns aLayoutRuns = rLayoutArgs.maRuns;
    rLayoutArgs.PrepareFallback();
    rLayoutArgs.mnFlags |= SAL_LAYOUT_FOR_FALLBACK;

#if defined(HDU_DEBUG)
    {
        int nCharPos = -1;
        bool bRTL = false;
        fprintf(stderr,"OD:ImplLayout Glyph Fallback for");
        for( int i=0; i<8 && rLayoutArgs.GetNextPos( &nCharPos, &bRTL); ++i )
            fprintf(stderr," U+%04X", rLayoutArgs.mpStr[ nCharPos ] );
        fprintf(stderr,"\n");
        rLayoutArgs.ResetPos();
    }
#endif
    // get list of unicodes that need glyph fallback
    int nCharPos = -1;
    bool bRTL = false;
    rtl::OUStringBuffer aMissingCodeBuf;
    while( rLayoutArgs.GetNextPos( &nCharPos, &bRTL) )
        aMissingCodeBuf.append( rLayoutArgs.mpStr[ nCharPos ] );
    rLayoutArgs.ResetPos();
    rtl::OUString aMissingCodes = aMissingCodeBuf.makeStringAndClear();

    ImplFontSelectData aFontSelData = mpFontEntry->maFontSelData;
    // when device specific font substitution may have been performed for
    // the originally selected font then make sure that a fallback to that
    // font is performed first
    int nDevSpecificFallback = 0;
    if( mpOutDevData && !mpOutDevData->maDevFontSubst.Empty() )
        nDevSpecificFallback = 1;

    // try if fallback fonts support the missing unicodes
    for( int nFallbackLevel = 1; nFallbackLevel < MAX_FALLBACK; ++nFallbackLevel )
    {
        // find a font family suited for glyph fallback
#ifndef FONTFALLBACK_HOOKS_DISABLED 
        // GetGlyphFallbackFont() needs a valid aFontSelData.mpFontEntry
        // if the system-specific glyph fallback is active
        aFontSelData.mpFontEntry = mpFontEntry; // reset the fontentry to base-level
#endif
        ImplFontEntry* pFallbackFont = mpFontCache->GetGlyphFallbackFont( mpFontList,
            aFontSelData, nFallbackLevel-nDevSpecificFallback, aMissingCodes );
        if( !pFallbackFont )
            break;

        aFontSelData.mpFontEntry = pFallbackFont;
        aFontSelData.mpFontData = pFallbackFont->maFontSelData.mpFontData;
        if( mpFontEntry && nFallbackLevel < MAX_FALLBACK-1)
        {
            // ignore fallback font if it is the same as the original font
            if( mpFontEntry->maFontSelData.mpFontData == aFontSelData.mpFontData )
            {
                mpFontCache->Release( pFallbackFont );
                continue;
            }
        }

#if defined(HDU_DEBUG)
        {
            ByteString aOrigFontName( maFont.GetName(), RTL_TEXTENCODING_UTF8);
            ByteString aFallbackName( aFontSelData.mpFontData->GetFamilyName(),
                RTL_TEXTENCODING_UTF8);
            fprintf(stderr,"\tGlyphFallback[lvl=%d] \"%s\" -> \"%s\" (q=%d)\n",
                nFallbackLevel, aOrigFontName.GetBuffer(), aFallbackName.GetBuffer(),
                aFontSelData.mpFontData->GetQuality());
        }
#endif

        pFallbackFont->mnSetFontFlags = mpGraphics->SetFont( &aFontSelData, nFallbackLevel );

        // create and add glyph fallback layout to multilayout
        rLayoutArgs.ResetPos();
        SalLayout* pFallback = mpGraphics->GetTextLayout( rLayoutArgs, nFallbackLevel );
        if( pFallback )
        {
            if( pFallback->LayoutText( rLayoutArgs ) )
            {
                if( !pMultiSalLayout )
                    pMultiSalLayout = new MultiSalLayout( *pSalLayout );
                pMultiSalLayout->AddFallback( *pFallback,
                    rLayoutArgs.maRuns, aFontSelData.mpFontData );
                if (nFallbackLevel == MAX_FALLBACK-1)
                    pMultiSalLayout->SetInComplete();
            }
            else
            {
                // there is no need for a font that couldn't resolve anything
                pFallback->Release();
            }
        }

        mpFontCache->Release( pFallbackFont );

        // break when this fallback was sufficient
        if( !rLayoutArgs.PrepareFallback() )
            break;
    }

    if( pMultiSalLayout && pMultiSalLayout->LayoutText( rLayoutArgs ) )
        pSalLayout = pMultiSalLayout;

    // restore orig font settings
    pSalLayout->InitFont();
    rLayoutArgs.maRuns = aLayoutRuns;

    return pSalLayout;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetTextIsRTL(
            const String& rString,
            xub_StrLen nIndex, xub_StrLen nLen ) const
{
    String aStr( rString );
    ImplLayoutArgs aArgs = ImplPrepareLayoutArgs( aStr, nIndex, nLen, 0, NULL );
    bool bRTL = false;
    int nCharPos = -1;
    aArgs.GetNextPos( &nCharPos, &bRTL );
    return (nCharPos != nIndex) ? TRUE : FALSE;
}

// -----------------------------------------------------------------------

xub_StrLen OutputDevice::GetTextBreak( const String& rStr, long nTextWidth,
                                       xub_StrLen nIndex, xub_StrLen nLen,
                                       long nCharExtra, BOOL /*TODO: bCellBreaking*/ ) const
{
    DBG_TRACE( "OutputDevice::GetTextBreak()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen );
    xub_StrLen nRetVal = STRING_LEN;
    if( pSalLayout )
    {
        // convert logical widths into layout units
        // NOTE: be very careful to avoid rounding errors for nCharExtra case
        // problem with rounding errors especially for small nCharExtras
        // TODO: remove when layout units have subpixel granularity
        long nWidthFactor = pSalLayout->GetUnitsPerPixel();
        long nSubPixelFactor = (nWidthFactor < 64 ) ? 64 : 1;
        nTextWidth *= nWidthFactor * nSubPixelFactor;
        long nTextPixelWidth = ImplLogicWidthToDevicePixel( nTextWidth );
        long nExtraPixelWidth = 0;
        if( nCharExtra != 0 )
        {
            nCharExtra *= nWidthFactor * nSubPixelFactor;
            nExtraPixelWidth = ImplLogicWidthToDevicePixel( nCharExtra );
        }
        nRetVal = sal::static_int_cast<xub_StrLen>(pSalLayout->GetTextBreak( nTextPixelWidth, nExtraPixelWidth, nSubPixelFactor ));

        pSalLayout->Release();
    }

    return nRetVal;
}

// -----------------------------------------------------------------------

xub_StrLen OutputDevice::GetTextBreak( const String& rStr, long nTextWidth,
                                       sal_Unicode nHyphenatorChar, xub_StrLen& rHyphenatorPos,
                                       xub_StrLen nIndex, xub_StrLen nLen,
                                       long nCharExtra ) const
{
    DBG_TRACE( "OutputDevice::GetTextBreak()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    rHyphenatorPos = STRING_LEN;

    SalLayout* pSalLayout = ImplLayout( rStr, nIndex, nLen );
    if( !pSalLayout )
        return STRING_LEN;

    // convert logical widths into layout units
    // NOTE: be very careful to avoid rounding errors for nCharExtra case
    // problem with rounding errors especially for small nCharExtras
    // TODO: remove when layout units have subpixel granularity
    long nWidthFactor = pSalLayout->GetUnitsPerPixel();
    long nSubPixelFactor = (nWidthFactor < 64 ) ? 64 : 1;

    nTextWidth *= nWidthFactor * nSubPixelFactor;
    long nTextPixelWidth = ImplLogicWidthToDevicePixel( nTextWidth );
    long nExtraPixelWidth = 0;
    if( nCharExtra != 0 )
    {
        nCharExtra *= nWidthFactor * nSubPixelFactor;
        nExtraPixelWidth = ImplLogicWidthToDevicePixel( nCharExtra );
    }

    // calculate un-hyphenated break position
    xub_StrLen nRetVal = sal::static_int_cast<xub_StrLen>(pSalLayout->GetTextBreak( nTextPixelWidth, nExtraPixelWidth, nSubPixelFactor ));

    // calculate hyphenated break position
    String aHyphenatorStr( &nHyphenatorChar, 1 );
    xub_StrLen nTempLen = 1;
    SalLayout* pHyphenatorLayout = ImplLayout( aHyphenatorStr, 0, nTempLen );
    if( pHyphenatorLayout )
    {
        // calculate subpixel width of hyphenation character
        long nHyphenatorPixelWidth = pHyphenatorLayout->GetTextWidth() * nSubPixelFactor;
        pHyphenatorLayout->Release();

        // calculate hyphenated break position
        nTextPixelWidth -= nHyphenatorPixelWidth;
        if( nExtraPixelWidth > 0 )
            nTextPixelWidth -= nExtraPixelWidth;

        rHyphenatorPos = sal::static_int_cast<xub_StrLen>(pSalLayout->GetTextBreak( nTextPixelWidth, nExtraPixelWidth, nSubPixelFactor ));

        if( rHyphenatorPos > nRetVal )
            rHyphenatorPos = nRetVal;
    }

    pSalLayout->Release();
    return nRetVal;
}

// -----------------------------------------------------------------------

void OutputDevice::ImplDrawText( const Rectangle& rRect,
                                 const String& rOrigStr, USHORT nStyle,
                                 MetricVector* pVector, String* pDisplayText )
{
    Color aOldTextColor;
    Color aOldTextFillColor;
    BOOL  bRestoreFillColor = false;
    if ( (nStyle & TEXT_DRAW_DISABLE) && ! pVector )
    {
        BOOL  bHighContrastBlack = FALSE;
        BOOL  bHighContrastWhite = FALSE;
        const StyleSettings& rStyleSettings( GetSettings().GetStyleSettings() );
        if( rStyleSettings.GetHighContrastMode() )
        {
            Color aCol;
            if( IsBackground() )
                aCol = GetBackground().GetColor();
            else
                // best guess is the face color here
                // but it may be totally wrong. the background color
                // was typically already reset
                aCol = rStyleSettings.GetFaceColor();
    
            bHighContrastBlack = aCol.IsDark();
            bHighContrastWhite = aCol.IsBright();
        }
    
        aOldTextColor = GetTextColor();
        if ( IsTextFillColor() )
        {
            bRestoreFillColor = TRUE;
            aOldTextFillColor = GetTextFillColor();
        }
        if( bHighContrastBlack )
            SetTextColor( COL_GREEN );
        else if( bHighContrastWhite )
            SetTextColor( COL_LIGHTGREEN );
        else
        {
            // draw disabled text always without shadow
            // as it fits better with native look
            /*
            SetTextColor( GetSettings().GetStyleSettings().GetLightColor() );
            Rectangle aRect = rRect;
            aRect.Move( 1, 1 );
            DrawText( aRect, rOrigStr, nStyle & ~TEXT_DRAW_DISABLE );
            */
            SetTextColor( GetSettings().GetStyleSettings().GetDisableColor() );
        }
    }

    long        nWidth          = rRect.GetWidth();
    long        nHeight         = rRect.GetHeight();

    if ( ((nWidth <= 0) || (nHeight <= 0)) && (nStyle & TEXT_DRAW_CLIP) )
        return;

    Point       aPos            = rRect.TopLeft();

    long        nTextHeight     = GetTextHeight();
    TextAlign   eAlign          = GetTextAlign();
    xub_StrLen  nMnemonicPos    = STRING_NOTFOUND;

    String aStr = rOrigStr;
    if ( nStyle & TEXT_DRAW_MNEMONIC )
        aStr = GetNonMnemonicString( aStr, nMnemonicPos );

    // Mehrzeiligen Text behandeln wir anders
    if ( nStyle & TEXT_DRAW_MULTILINE )
    {

        XubString               aLastLine;
        ImplMultiTextLineInfo   aMultiLineInfo;
        ImplTextLineInfo*       pLineInfo;
        long                    nMaxTextWidth;
        xub_StrLen              i;
        xub_StrLen              nLines;
        xub_StrLen              nFormatLines;

        if ( nTextHeight )
        {
            nMaxTextWidth = ImplGetTextLines( aMultiLineInfo, nWidth, aStr, nStyle );
            nLines = (xub_StrLen)(nHeight/nTextHeight);
            nFormatLines = aMultiLineInfo.Count();
            if ( !nLines )
                nLines = 1;
            if ( nFormatLines > nLines )
            {
                if ( nStyle & TEXT_DRAW_ENDELLIPSIS )
                {
                    // Letzte Zeile zusammenbauen und kuerzen
                    nFormatLines = nLines-1;

                    pLineInfo = aMultiLineInfo.GetLine( nFormatLines );
                    aLastLine = aStr.Copy( pLineInfo->GetIndex() );
                    aLastLine.ConvertLineEnd( LINEEND_LF );
                    // Alle LineFeed's durch Spaces ersetzen
                    xub_StrLen nLastLineLen = aLastLine.Len();
                    for ( i = 0; i < nLastLineLen; i++ )
                    {
                        if ( aLastLine.GetChar( i ) == _LF )
                            aLastLine.SetChar( i, ' ' );
                    }
                    aLastLine = GetEllipsisString( aLastLine, nWidth, nStyle );
                    nStyle &= ~(TEXT_DRAW_VCENTER | TEXT_DRAW_BOTTOM);
                    nStyle |= TEXT_DRAW_TOP;
                }
            }
            else
            {
                if ( nMaxTextWidth <= nWidth )
                    nStyle &= ~TEXT_DRAW_CLIP;
            }

            // Muss in der Hoehe geclippt werden?
            if ( nFormatLines*nTextHeight > nHeight )
                nStyle |= TEXT_DRAW_CLIP;

            // Clipping setzen
            if ( nStyle & TEXT_DRAW_CLIP )
            {
                Push( PUSH_CLIPREGION );
                IntersectClipRegion( rRect );
            }

            // Vertikales Alignment
            if ( nStyle & TEXT_DRAW_BOTTOM )
                aPos.Y() += nHeight-(nFormatLines*nTextHeight);
            else if ( nStyle & TEXT_DRAW_VCENTER )
                aPos.Y() += (nHeight-(nFormatLines*nTextHeight))/2;

            // Font Alignment
            if ( eAlign == ALIGN_BOTTOM )
                aPos.Y() += nTextHeight;
            else if ( eAlign == ALIGN_BASELINE )
                aPos.Y() += GetFontMetric().GetAscent();

            // Alle Zeilen ausgeben, bis auf die letzte
            for ( i = 0; i < nFormatLines; i++ )
            {
                pLineInfo = aMultiLineInfo.GetLine( i );
                if ( nStyle & TEXT_DRAW_RIGHT )
                    aPos.X() += nWidth-pLineInfo->GetWidth();
                else if ( nStyle & TEXT_DRAW_CENTER )
                    aPos.X() += (nWidth-pLineInfo->GetWidth())/2;
                xub_StrLen nIndex   = pLineInfo->GetIndex();
                xub_StrLen nLineLen = pLineInfo->GetLen();
                DrawText( aPos, aStr, nIndex, nLineLen, pVector, pDisplayText );
                if ( !(GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_NOMNEMONICS) && !pVector )
                {
                    if ( (nMnemonicPos >= nIndex) && (nMnemonicPos < nIndex+nLineLen) )
                    {
                        long        nMnemonicX;
                        long        nMnemonicY;
                        long        nMnemonicWidth;

                        sal_Int32* pCaretXArray = (sal_Int32*) alloca( 2 * sizeof(sal_Int32) * nLineLen );
                        /*BOOL bRet =*/ GetCaretPositions( aStr, pCaretXArray,
                                                nIndex, nLineLen);
                        long lc_x1 = pCaretXArray[2*(nMnemonicPos - nIndex)];
                        long lc_x2 = pCaretXArray[2*(nMnemonicPos - nIndex)+1];
                        nMnemonicWidth = ::abs((int)(lc_x1 - lc_x2));

                        Point       aTempPos = LogicToPixel( aPos );
                        nMnemonicX = mnOutOffX + aTempPos.X() + ImplLogicWidthToDevicePixel( Min( lc_x1, lc_x2 ) );
                        nMnemonicY = mnOutOffY + aTempPos.Y() + ImplLogicWidthToDevicePixel( GetFontMetric().GetAscent() );
                        ImplDrawMnemonicLine( nMnemonicX, nMnemonicY, nMnemonicWidth );
                    }
                }
                aPos.Y() += nTextHeight;
                aPos.X() = rRect.Left();
            }


            // Gibt es noch eine letzte Zeile, dann diese linksbuendig ausgeben,
            // da die Zeile gekuerzt wurde
            if ( aLastLine.Len() )
                DrawText( aPos, aLastLine, 0, STRING_LEN, pVector, pDisplayText );

            // Clipping zuruecksetzen
            if ( nStyle & TEXT_DRAW_CLIP )
                Pop();
        }
    }
    else
    {
        long nTextWidth = GetTextWidth( aStr );

        // Evt. Text kuerzen
        if ( nTextWidth > nWidth )
        {
            if ( nStyle & TEXT_DRAW_ELLIPSIS )
            {
                aStr = GetEllipsisString( aStr, nWidth, nStyle );
                nStyle &= ~(TEXT_DRAW_CENTER | TEXT_DRAW_RIGHT);
                nStyle |= TEXT_DRAW_LEFT;
                nTextWidth = GetTextWidth( aStr );
            }
        }
        else
        {
            if ( nTextHeight <= nHeight )
                nStyle &= ~TEXT_DRAW_CLIP;
        }

        // horizontal text alignment
        if ( nStyle & TEXT_DRAW_RIGHT )
            aPos.X() += nWidth-nTextWidth;
        else if ( nStyle & TEXT_DRAW_CENTER )
            aPos.X() += (nWidth-nTextWidth)/2;

        // vertical font alignment
        if ( eAlign == ALIGN_BOTTOM )
            aPos.Y() += nTextHeight;
        else if ( eAlign == ALIGN_BASELINE )
            aPos.Y() += GetFontMetric().GetAscent();

        if ( nStyle & TEXT_DRAW_BOTTOM )
            aPos.Y() += nHeight-nTextHeight;
        else if ( nStyle & TEXT_DRAW_VCENTER )
            aPos.Y() += (nHeight-nTextHeight)/2;

        long        nMnemonicX = 0;
        long        nMnemonicY = 0;
        long        nMnemonicWidth = 0;
        if ( nMnemonicPos != STRING_NOTFOUND )
        {
            sal_Int32* pCaretXArray = (sal_Int32*) alloca( 2 * sizeof(sal_Int32) * aStr.Len() );
            /*BOOL bRet =*/ GetCaretPositions( aStr, pCaretXArray, 0, aStr.Len() );
            long lc_x1 = pCaretXArray[2*(nMnemonicPos)];
            long lc_x2 = pCaretXArray[2*(nMnemonicPos)+1];
            nMnemonicWidth = ::abs((int)(lc_x1 - lc_x2));

            Point aTempPos = LogicToPixel( aPos );
            nMnemonicX = mnOutOffX + aTempPos.X() + ImplLogicWidthToDevicePixel( Min(lc_x1, lc_x2) );
            nMnemonicY = mnOutOffY + aTempPos.Y() + ImplLogicWidthToDevicePixel( GetFontMetric().GetAscent() );
        }

        if ( nStyle & TEXT_DRAW_CLIP )
        {
            Push( PUSH_CLIPREGION );
            IntersectClipRegion( rRect );
            DrawText( aPos, aStr, 0, STRING_LEN, pVector, pDisplayText );
            if ( !(GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_NOMNEMONICS) && !pVector )
            {
                if ( nMnemonicPos != STRING_NOTFOUND )
                    ImplDrawMnemonicLine( nMnemonicX, nMnemonicY, nMnemonicWidth );
            }
            Pop();
        }
        else
        {
            DrawText( aPos, aStr, 0, STRING_LEN, pVector, pDisplayText );
            if ( !(GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_NOMNEMONICS) && !pVector )
            {
                if ( nMnemonicPos != STRING_NOTFOUND )
                    ImplDrawMnemonicLine( nMnemonicX, nMnemonicY, nMnemonicWidth );
            }
        }
    }

    if ( nStyle & TEXT_DRAW_DISABLE && !pVector )
    {
        SetTextColor( aOldTextColor );
        if ( bRestoreFillColor )
            SetTextFillColor( aOldTextFillColor );
    }
}

// -----------------------------------------------------------------------

void OutputDevice::AddTextRectActions( const Rectangle& rRect,
                                       const String&    rOrigStr,
                                       USHORT           nStyle,
                                       GDIMetaFile&     rMtf )
{
    DBG_TRACE( "OutputDevice::AddTextRectActions( const Rectangle& )" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( !rOrigStr.Len() || rRect.IsEmpty() )
        return;

    // we need a graphics
    if( !mpGraphics && !ImplGetGraphics() )
        return;
    if( mbInitClipRegion )
        ImplInitClipRegion();

    // temporarily swap in passed mtf for action generation, and
    // disable output generation.
    const BOOL bOutputEnabled( IsOutputEnabled() );
    GDIMetaFile* pMtf = mpMetaFile;

    mpMetaFile = &rMtf;
    EnableOutput( FALSE );

    // #i47157# Factored out to ImplDrawTextRect(), to be shared
    // between us and DrawText()
    ImplDrawText( rRect, rOrigStr, nStyle, NULL, NULL );

    // and restore again
    EnableOutput( bOutputEnabled );
    mpMetaFile = pMtf;
}

// -----------------------------------------------------------------------

void OutputDevice::DrawText( const Rectangle& rRect,
                             const String& rOrigStr, USHORT nStyle,
                             MetricVector* pVector, String* pDisplayText )

{
    if( mpOutDevData && mpOutDevData->mpRecordLayout )
    {
        pVector = &mpOutDevData->mpRecordLayout->m_aUnicodeBoundRects;
        pDisplayText = &mpOutDevData->mpRecordLayout->m_aDisplayText;
    }

    DBG_TRACE( "OutputDevice::DrawText( const Rectangle& )" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( mpMetaFile )
        mpMetaFile->AddAction( new MetaTextRectAction( rRect, rOrigStr, nStyle ) );

    if ( ( !IsDeviceOutputNecessary() && ! pVector ) || !rOrigStr.Len() || rRect.IsEmpty() )
        return;

    // we need a graphics
    if( !mpGraphics && !ImplGetGraphics() )
        return;
    if( mbInitClipRegion )
        ImplInitClipRegion();
    if( mbOutputClipped )
        return;

    // temporarily disable mtf action generation (ImplDrawText _does_
    // create META_TEXT_ACTIONs otherwise)
    GDIMetaFile* pMtf = mpMetaFile;
    mpMetaFile = NULL;

    // #i47157# Factored out to ImplDrawTextRect(), to be used also
    // from AddTextRectActions()
    ImplDrawText( rRect, rOrigStr, nStyle, pVector, pDisplayText );

    // and enable again
    mpMetaFile = pMtf;

    if( mpAlphaVDev )
        mpAlphaVDev->DrawText( rRect, rOrigStr, nStyle, pVector, pDisplayText );
}

// -----------------------------------------------------------------------

Rectangle OutputDevice::GetTextRect( const Rectangle& rRect,
                                     const String& rOrigStr, USHORT nStyle,
                                     TextRectInfo* pInfo ) const
{
    DBG_TRACE( "OutputDevice::GetTextRect()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    Rectangle           aRect = rRect;
    xub_StrLen          nLines;
    long                nWidth = rRect.GetWidth();
    long                nMaxWidth;
    long                nTextHeight = GetTextHeight();

    String aStr = rOrigStr;
    if ( nStyle & TEXT_DRAW_MNEMONIC )
        aStr = GetNonMnemonicString( aStr );

    if ( nStyle & TEXT_DRAW_MULTILINE )
    {
        ImplMultiTextLineInfo   aMultiLineInfo;
        ImplTextLineInfo*       pLineInfo;
        xub_StrLen              nFormatLines;
        xub_StrLen              i;

        nMaxWidth = 0;
        ImplGetTextLines( aMultiLineInfo, nWidth, aStr, nStyle );
        nFormatLines = aMultiLineInfo.Count();
        if ( !nTextHeight )
            nTextHeight = 1;
        nLines = (USHORT)(aRect.GetHeight()/nTextHeight);
        if ( pInfo )
            pInfo->mnLineCount = nFormatLines;
        if ( !nLines )
            nLines = 1;
        if ( nFormatLines <= nLines )
            nLines = nFormatLines;
        else
        {
            if ( !(nStyle & TEXT_DRAW_ENDELLIPSIS) )
                nLines = nFormatLines;
            else
            {
                if ( pInfo )
                    pInfo->mbEllipsis = TRUE;
                nMaxWidth = nWidth;
            }
        }
        if ( pInfo )
        {
            BOOL bMaxWidth = nMaxWidth == 0;
            pInfo->mnMaxWidth = 0;
            for ( i = 0; i < nLines; i++ )
            {
                pLineInfo = aMultiLineInfo.GetLine( i );
                if ( bMaxWidth && (pLineInfo->GetWidth() > nMaxWidth) )
                    nMaxWidth = pLineInfo->GetWidth();
                if ( pLineInfo->GetWidth() > pInfo->mnMaxWidth )
                    pInfo->mnMaxWidth = pLineInfo->GetWidth();
            }
        }
        else if ( !nMaxWidth )
        {
            for ( i = 0; i < nLines; i++ )
            {
                pLineInfo = aMultiLineInfo.GetLine( i );
                if ( pLineInfo->GetWidth() > nMaxWidth )
                    nMaxWidth = pLineInfo->GetWidth();
            }
        }
    }
    else
    {
        nLines      = 1;
        nMaxWidth   = GetTextWidth( aStr );

        if ( pInfo )
        {
            pInfo->mnLineCount  = 1;
            pInfo->mnMaxWidth   = nMaxWidth;
        }

        if ( (nMaxWidth > nWidth) && (nStyle & TEXT_DRAW_ELLIPSIS) )
        {
            if ( pInfo )
                pInfo->mbEllipsis = TRUE;
            nMaxWidth = nWidth;
        }
    }

    if ( nStyle & TEXT_DRAW_RIGHT )
        aRect.Left() = aRect.Right()-nMaxWidth+1;
    else if ( nStyle & TEXT_DRAW_CENTER )
    {
        aRect.Left() += (nWidth-nMaxWidth)/2;
        aRect.Right() = aRect.Left()+nMaxWidth-1;
    }
    else
        aRect.Right() = aRect.Left()+nMaxWidth-1;

    if ( nStyle & TEXT_DRAW_BOTTOM )
        aRect.Top() = aRect.Bottom()-(nTextHeight*nLines)+1;
    else if ( nStyle & TEXT_DRAW_VCENTER )
    {
        aRect.Top()   += (aRect.GetHeight()-(nTextHeight*nLines))/2;
        aRect.Bottom() = aRect.Top()+(nTextHeight*nLines)-1;
    }
    else
        aRect.Bottom() = aRect.Top()+(nTextHeight*nLines)-1;

    aRect.Right()++; // #99188# get rid of rounding problems when using this rect later
    return aRect;
}

// -----------------------------------------------------------------------

static BOOL ImplIsCharIn( xub_Unicode c, const sal_Char* pStr )
{
    while ( *pStr )
    {
        if ( *pStr == c )
            return TRUE;
        pStr++;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

String OutputDevice::GetEllipsisString( const String& rOrigStr, long nMaxWidth,
                                        USHORT nStyle ) const
{
    DBG_TRACE( "OutputDevice::GetEllipsisString()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    String aStr = rOrigStr;
    xub_StrLen nIndex = GetTextBreak( aStr, nMaxWidth );


    if ( nIndex != STRING_LEN )
    {
        if ( nStyle & TEXT_DRAW_ENDELLIPSIS )
        {
            aStr.Erase( nIndex );
            if ( nIndex > 1 )
            {
                aStr.AppendAscii( "..." );
                while ( aStr.Len() && (GetTextWidth( aStr ) > nMaxWidth) )
                {
                    if ( (nIndex > 1) || (nIndex == aStr.Len()) )
                        nIndex--;
                    aStr.Erase( nIndex, 1 );
                }
            }

            if ( !aStr.Len() && (nStyle & TEXT_DRAW_CLIP) )
                aStr += rOrigStr.GetChar( 0 );
        }
        else if ( nStyle & TEXT_DRAW_PATHELLIPSIS )
        {
            rtl::OUString aPath( rOrigStr );
            rtl::OUString aAbbreviatedPath;
            osl_abbreviateSystemPath( aPath.pData, &aAbbreviatedPath.pData, nIndex, NULL );
            aStr = aAbbreviatedPath;
        }
        else if ( nStyle & TEXT_DRAW_NEWSELLIPSIS )
        {
            static sal_Char const   pSepChars[] = ".";
            // Letztes Teilstueck ermitteln
            xub_StrLen nLastContent = aStr.Len();
            while ( nLastContent )
            {
                nLastContent--;
                if ( ImplIsCharIn( aStr.GetChar( nLastContent ), pSepChars ) )
                    break;
            }
            while ( nLastContent &&
                    ImplIsCharIn( aStr.GetChar( nLastContent-1 ), pSepChars ) )
                nLastContent--;

            XubString aLastStr( aStr, nLastContent, aStr.Len() );
            XubString aTempLastStr1( RTL_CONSTASCII_USTRINGPARAM( "..." ) );
            aTempLastStr1 += aLastStr;
            if ( GetTextWidth( aTempLastStr1 ) > nMaxWidth )
                aStr = GetEllipsisString( aStr, nMaxWidth, nStyle | TEXT_DRAW_ENDELLIPSIS );
            else
            {
                USHORT nFirstContent = 0;
                while ( nFirstContent < nLastContent )
                {
                    nFirstContent++;
                    if ( ImplIsCharIn( aStr.GetChar( nFirstContent ), pSepChars ) )
                        break;
                }
                while ( (nFirstContent < nLastContent) &&
                        ImplIsCharIn( aStr.GetChar( nFirstContent ), pSepChars ) )
                    nFirstContent++;

                if ( nFirstContent >= nLastContent )
                    aStr = GetEllipsisString( aStr, nMaxWidth, nStyle | TEXT_DRAW_ENDELLIPSIS );
                else
                {
                    if ( nFirstContent > 4 )
                        nFirstContent = 4;
                    XubString aFirstStr( aStr, 0, nFirstContent );
                    aFirstStr.AppendAscii( "..." );
                    XubString aTempStr = aFirstStr;
                    aTempStr += aLastStr;
                    if ( GetTextWidth( aTempStr ) > nMaxWidth )
                        aStr = GetEllipsisString( aStr, nMaxWidth, nStyle | TEXT_DRAW_ENDELLIPSIS );
                    else
                    {
                        do
                        {
                            aStr = aTempStr;
                            if( nLastContent > aStr.Len() )
                                nLastContent = aStr.Len();
                            while ( nFirstContent < nLastContent )
                            {
                                nLastContent--;
                                if ( ImplIsCharIn( aStr.GetChar( nLastContent ), pSepChars ) )
                                    break;

                            }
                            while ( (nFirstContent < nLastContent) &&
                                    ImplIsCharIn( aStr.GetChar( nLastContent-1 ), pSepChars ) )
                                nLastContent--;

                            if ( nFirstContent < nLastContent )
                            {
                                XubString aTempLastStr( aStr, nLastContent, aStr.Len() );
                                aTempStr = aFirstStr;
                                aTempStr += aTempLastStr;
                                if ( GetTextWidth( aTempStr ) > nMaxWidth )
                                    break;
                            }
                        }
                        while ( nFirstContent < nLastContent );
                    }
                }
            }
        }
    }

    return aStr;
}

// -----------------------------------------------------------------------

void OutputDevice::DrawCtrlText( const Point& rPos, const XubString& rStr,
                                 xub_StrLen nIndex, xub_StrLen nLen,
                                 USHORT nStyle, MetricVector* pVector, String* pDisplayText )
{
    DBG_TRACE( "OutputDevice::DrawCtrlText()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( !IsDeviceOutputNecessary() || (nIndex >= rStr.Len()) )
        return;

    // better get graphics here because ImplDrawMnemonicLine() will not
    // we need a graphics
    if( !mpGraphics && !ImplGetGraphics() )
        return;
    if( mbInitClipRegion )
        ImplInitClipRegion();
    if ( mbOutputClipped )
        return;

    if( nIndex >= rStr.Len() )
        return;
    if( (ULONG)nIndex+nLen >= rStr.Len() )
        nLen = rStr.Len() - nIndex;

    XubString   aStr = rStr;
    xub_StrLen  nMnemonicPos = STRING_NOTFOUND;

    long        nMnemonicX = 0;
    long        nMnemonicY = 0;
    long        nMnemonicWidth = 0;
    if ( (nStyle & TEXT_DRAW_MNEMONIC) && nLen > 1 )
    {
        aStr = GetNonMnemonicString( aStr, nMnemonicPos );
        if ( nMnemonicPos != STRING_NOTFOUND )
        {
            if( nMnemonicPos < nIndex )
                --nIndex;
            else if( nLen < STRING_LEN )
            {
                if( nMnemonicPos < (nIndex+nLen) )
                    --nLen;
                DBG_ASSERT( nMnemonicPos < (nIndex+nLen), "Mnemonic underline marker after last character" );
            }
            BOOL bInvalidPos = FALSE;

            if( nMnemonicPos >= nLen )
            {
                // #106952#
                // may occur in BiDi-Strings: the '~' is sometimes found behind the last char
                // due to some strange BiDi text editors
                // ->place the underline behind the string to indicate a failure
                bInvalidPos = TRUE;
                nMnemonicPos = nLen-1;
            }

            sal_Int32* pCaretXArray = (sal_Int32*)alloca( 2 * sizeof(sal_Int32) * nLen );
            /*BOOL bRet =*/ GetCaretPositions( aStr, pCaretXArray, nIndex, nLen );
            long lc_x1 = pCaretXArray[ 2*(nMnemonicPos - nIndex) ];
            long lc_x2 = pCaretXArray[ 2*(nMnemonicPos - nIndex)+1 ];
            nMnemonicWidth = ::abs((int)(lc_x1 - lc_x2));

            Point aTempPos( Min(lc_x1,lc_x2), GetFontMetric().GetAscent() );
            if( bInvalidPos )  // #106952#, place behind the (last) character
                aTempPos = Point( Max(lc_x1,lc_x2), GetFontMetric().GetAscent() );

			aTempPos += rPos;
            aTempPos = LogicToPixel( aTempPos );
            nMnemonicX = mnOutOffX + aTempPos.X();
            nMnemonicY = mnOutOffY + aTempPos.Y();
        }
    }

    if ( nStyle & TEXT_DRAW_DISABLE && ! pVector )
    {
        Color aOldTextColor;
        Color aOldTextFillColor;
        BOOL  bRestoreFillColor;
        BOOL  bHighContrastBlack = FALSE;
        BOOL  bHighContrastWhite = FALSE;
        const StyleSettings& rStyleSettings( GetSettings().GetStyleSettings() );
        if( rStyleSettings.GetHighContrastMode() )
        {
            if( IsBackground() )
            {
                Wallpaper aWall = GetBackground();
                Color aCol = aWall.GetColor();
                bHighContrastBlack = aCol.IsDark();
                bHighContrastWhite = aCol.IsBright();
            }
        }
        
        aOldTextColor = GetTextColor();
        if ( IsTextFillColor() )
        {
            bRestoreFillColor = TRUE;
            aOldTextFillColor = GetTextFillColor();
        }
        else
            bRestoreFillColor = FALSE;

        if( bHighContrastBlack )
            SetTextColor( COL_GREEN );
        else if( bHighContrastWhite )
            SetTextColor( COL_LIGHTGREEN );
        else
            SetTextColor( GetSettings().GetStyleSettings().GetDisableColor() );

        DrawText( rPos, aStr, nIndex, nLen, pVector, pDisplayText );
        if ( !(GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_NOMNEMONICS) && !pVector )
        {
            if ( nMnemonicPos != STRING_NOTFOUND )
                ImplDrawMnemonicLine( nMnemonicX, nMnemonicY, nMnemonicWidth );
        }
        SetTextColor( aOldTextColor );
        if ( bRestoreFillColor )
            SetTextFillColor( aOldTextFillColor );
    }
    else
    {
        DrawText( rPos, aStr, nIndex, nLen, pVector, pDisplayText );
        if ( !(GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_NOMNEMONICS) && !pVector )
        {
            if ( nMnemonicPos != STRING_NOTFOUND )
                ImplDrawMnemonicLine( nMnemonicX, nMnemonicY, nMnemonicWidth );
        }
    }

    if( mpAlphaVDev )
        mpAlphaVDev->DrawCtrlText( rPos, rStr, nIndex, nLen, nStyle, pVector, pDisplayText );
}

// -----------------------------------------------------------------------

long OutputDevice::GetCtrlTextWidth( const String& rStr,
                                     xub_StrLen nIndex, xub_StrLen nLen,
                                     USHORT nStyle ) const
{
    DBG_TRACE( "OutputDevice::GetCtrlTextSize()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if ( nStyle & TEXT_DRAW_MNEMONIC )
    {
        xub_StrLen  nMnemonicPos;
        XubString   aStr = GetNonMnemonicString( rStr, nMnemonicPos );
        if ( nMnemonicPos != STRING_NOTFOUND )
        {
            if ( nMnemonicPos < nIndex )
                nIndex--;
            else if ( (nLen < STRING_LEN) &&
                      (nMnemonicPos >= nIndex) && (nMnemonicPos < (ULONG)(nIndex+nLen)) )
                nLen--;
        }
        return GetTextWidth( aStr, nIndex, nLen );
    }
    else
        return GetTextWidth( rStr, nIndex, nLen );
}

// -----------------------------------------------------------------------

String OutputDevice::GetNonMnemonicString( const String& rStr, xub_StrLen& rMnemonicPos )
{
    String   aStr    = rStr;
    xub_StrLen  nLen    = aStr.Len();
    xub_StrLen  i       = 0;

    rMnemonicPos = STRING_NOTFOUND;
    while ( i < nLen )
    {
        if ( aStr.GetChar( i ) == '~' )
        {
            if ( aStr.GetChar( i+1 ) != '~' )
            {
                if ( rMnemonicPos == STRING_NOTFOUND )
                    rMnemonicPos = i;
                aStr.Erase( i, 1 );
                nLen--;
            }
            else
            {
                aStr.Erase( i, 1 );
                nLen--;
                i++;
            }
        }
        else
            i++;
    }

    return aStr;
}

// -----------------------------------------------------------------------

int OutputDevice::GetDevFontCount() const
{
    DBG_TRACE( "OutputDevice::GetDevFontCount()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if( !mpGetDevFontList )
        mpGetDevFontList = mpFontList->GetDevFontList();
    return mpGetDevFontList->Count();
}

// -----------------------------------------------------------------------

FontInfo OutputDevice::GetDevFont( int nDevFontIndex ) const
{
    DBG_TRACE( "OutputDevice::GetDevFont()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    FontInfo aFontInfo;

    ImplInitFontList();

    int nCount = GetDevFontCount();
    if( nDevFontIndex < nCount )
    {
        const ImplFontData& rData = *mpGetDevFontList->Get( nDevFontIndex );
        aFontInfo.SetName( rData.maName );
        aFontInfo.SetStyleName( rData.maStyleName );
        aFontInfo.SetCharSet( rData.mbSymbolFlag ? RTL_TEXTENCODING_SYMBOL : RTL_TEXTENCODING_UNICODE );
        aFontInfo.SetFamily( rData.meFamily );
        aFontInfo.SetPitch( rData.mePitch );
        aFontInfo.SetWeight( rData.meWeight );
        aFontInfo.SetItalic( rData.meItalic );
        aFontInfo.SetWidthType( rData.meWidthType );
        if( rData.IsScalable() )
            aFontInfo.mpImplMetric->mnMiscFlags |= ImplFontMetric::SCALABLE_FLAG;
        if( rData.mbDevice )
            aFontInfo.mpImplMetric->mnMiscFlags |= ImplFontMetric::DEVICE_FLAG;
    }

    return aFontInfo;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::AddTempDevFont( const String& rFileURL, const String& rFontName )
{
    DBG_TRACE( "OutputDevice::AddTempDevFont()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    ImplInitFontList();

    if( !mpGraphics && !ImplGetGraphics() )
        return FALSE;

    bool bRC = mpGraphics->AddTempDevFont( mpFontList, rFileURL, rFontName );
    if( !bRC )
        return FALSE;

    if( mpAlphaVDev )
        mpAlphaVDev->AddTempDevFont( rFileURL, rFontName );

    mpFontCache->Invalidate();
    return TRUE;
}

// -----------------------------------------------------------------------

int OutputDevice::GetDevFontSizeCount( const Font& rFont ) const
{
    DBG_TRACE( "OutputDevice::GetDevFontSizeCount()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    delete mpGetDevSizeList;

    ImplInitFontList();
    mpGetDevSizeList = mpFontList->GetDevSizeList( rFont.GetName() );
    return mpGetDevSizeList->Count();
}

// -----------------------------------------------------------------------

Size OutputDevice::GetDevFontSize( const Font& rFont, int nSizeIndex ) const
{
    DBG_TRACE( "OutputDevice::GetDevFontSize()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    // check range
    int nCount = GetDevFontSizeCount( rFont );
    if ( nSizeIndex >= nCount )
        return Size();

    // when mapping is enabled round to .5 points
    Size aSize( 0, mpGetDevSizeList->Get( nSizeIndex ) );
    if ( mbMap )
    {
        aSize.Height() *= 10;
        MapMode aMap( MAP_10TH_INCH, Point(), Fraction( 1, 72 ), Fraction( 1, 72 ) );
        aSize = PixelToLogic( aSize, aMap );
        aSize.Height() += 5;
        aSize.Height() /= 10;
        long nRound = aSize.Height() % 5;
        if ( nRound >= 3 )
            aSize.Height() += (5-nRound);
        else
            aSize.Height() -= nRound;
        aSize.Height() *= 10;
        aSize = LogicToPixel( aSize, aMap );
        aSize = PixelToLogic( aSize );
        aSize.Height() += 5;
        aSize.Height() /= 10;
    }
    return aSize;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::IsFontAvailable( const String& rFontName ) const
{
    DBG_TRACE( "OutputDevice::IsFontAvailable()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    ImplDevFontListData* pFound = mpFontList->FindFontFamily( rFontName );
    return (pFound != NULL);
}

// -----------------------------------------------------------------------

FontMetric OutputDevice::GetFontMetric() const
{
    DBG_TRACE( "OutputDevice::GetFontMetric()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    FontMetric aMetric;
    if( mbNewFont && !ImplNewFont() )
        return aMetric;

    ImplFontEntry*      pEntry = mpFontEntry;
    ImplFontMetricData* pMetric = &(pEntry->maMetric);

    // prepare metric
    aMetric.Font::operator=( maFont );

    // set aMetric with info from font
    aMetric.SetName( maFont.GetName() );
    aMetric.SetStyleName( pMetric->maStyleName );
    aMetric.SetSize( PixelToLogic( Size( pMetric->mnWidth, pMetric->mnAscent+pMetric->mnDescent-pMetric->mnIntLeading ) ) );
    aMetric.SetCharSet( pMetric->mbSymbolFlag ? RTL_TEXTENCODING_SYMBOL : RTL_TEXTENCODING_UNICODE );
    aMetric.SetFamily( pMetric->meFamily );
    aMetric.SetPitch( pMetric->mePitch );
    aMetric.SetWeight( pMetric->meWeight );
    aMetric.SetItalic( pMetric->meItalic );
    aMetric.SetWidthType( pMetric->meWidthType );
    if ( pEntry->mnOwnOrientation )
        aMetric.SetOrientation( pEntry->mnOwnOrientation );
    else
        aMetric.SetOrientation( pMetric->mnOrientation );
    if( !pEntry->maMetric.mbKernableFont )
         aMetric.SetKerning( maFont.GetKerning() & ~KERNING_FONTSPECIFIC );

    // set remaining metric fields
    aMetric.mpImplMetric->mnMiscFlags   = 0;
    if( pMetric->mbDevice )
            aMetric.mpImplMetric->mnMiscFlags |= ImplFontMetric::DEVICE_FLAG;
    if( pMetric->mbScalableFont )
            aMetric.mpImplMetric->mnMiscFlags |= ImplFontMetric::SCALABLE_FLAG;
    aMetric.mpImplMetric->mnAscent      = ImplDevicePixelToLogicHeight( pMetric->mnAscent+mnEmphasisAscent );
    aMetric.mpImplMetric->mnDescent     = ImplDevicePixelToLogicHeight( pMetric->mnDescent+mnEmphasisDescent );
    aMetric.mpImplMetric->mnIntLeading  = ImplDevicePixelToLogicHeight( pMetric->mnIntLeading+mnEmphasisAscent );
    aMetric.mpImplMetric->mnExtLeading  = ImplDevicePixelToLogicHeight( pMetric->mnExtLeading );
    aMetric.mpImplMetric->mnLineHeight  = ImplDevicePixelToLogicHeight( pMetric->mnAscent+pMetric->mnDescent+mnEmphasisAscent+mnEmphasisDescent );
    aMetric.mpImplMetric->mnSlant       = ImplDevicePixelToLogicHeight( pMetric->mnSlant );

#ifdef UNX
    // backwards compatible line metrics after fixing #i60945#
    if( (meOutDevType == OUTDEV_VIRDEV)
    &&  static_cast<const VirtualDevice*>(this)->ForceZeroExtleadBug() )
        aMetric.mpImplMetric->mnExtLeading = 0;
#endif

    return aMetric;
}

// -----------------------------------------------------------------------

FontMetric OutputDevice::GetFontMetric( const Font& rFont ) const
{
    // select font, query metrics, select original font again
    Font aOldFont = GetFont();
    const_cast<OutputDevice*>(this)->SetFont( rFont );
    FontMetric aMetric( GetFontMetric() );
    const_cast<OutputDevice*>(this)->SetFont( aOldFont );
    return aMetric;
}

// -----------------------------------------------------------------------

/** OutputDevice::GetSysFontData
 *
 * @param nFallbacklevel Fallback font level (0 = best matching font)
 *
 * Retrieve detailed font information in platform independent structure
 *
 * @return SystemFontData
 **/
SystemFontData OutputDevice::GetSysFontData(int nFallbacklevel) const
{
    SystemFontData aSysFontData;
    aSysFontData.nSize = sizeof(aSysFontData);

    if (!mpGraphics) ImplGetGraphics();
    if (mpGraphics) aSysFontData = mpGraphics->GetSysFontData(nFallbacklevel);
    
    return aSysFontData;
}


// -----------------------------------------------------------------------

/** OutputDevice::GetSysTextLayoutData
 *
 * @param rStartPt Start point of the text
 * @param rStr Text string that will be transformed into layout of glyphs
 * @param nIndex Position in the string from where layout will be done
 * @param nLen Length of the string
 * @param pDXAry Custom layout adjustment data
 *
 * Export finalized glyph layout data as platform independent SystemTextLayoutData
 * (see vcl/inc/vcl/sysdata.hxx)
 * 
 * Only parameters rStartPt and rStr are mandatory, the rest is optional
 * (default values will be used)
 *
 * @return SystemTextLayoutData
 **/
SystemTextLayoutData OutputDevice::GetSysTextLayoutData(const Point& rStartPt, const XubString& rStr, xub_StrLen nIndex, xub_StrLen nLen,
                                                        const sal_Int32* pDXAry) const
{
    DBG_TRACE( "OutputDevice::GetSysTextLayoutData()" );
	DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    SystemTextLayoutData aSysLayoutData;
    aSysLayoutData.nSize = sizeof(aSysLayoutData);
    aSysLayoutData.rGlyphData.reserve( 256 );
    
	if ( mpMetaFile ) {
        if (pDXAry)
            mpMetaFile->AddAction( new MetaTextArrayAction( rStartPt, rStr, pDXAry, nIndex, nLen ) );
        else
            mpMetaFile->AddAction( new MetaTextAction( rStartPt, rStr, nIndex, nLen ) );
	}
	
	if ( !IsDeviceOutputNecessary() ) return aSysLayoutData;
    
	SalLayout* rLayout = ImplLayout( rStr, nIndex, nLen, rStartPt, 0, pDXAry, true );
    
    // setup glyphs
    Point aPos;
    sal_GlyphId aGlyphId;
    int nFallbacklevel = 0;
    for( int nStart = 0; rLayout->GetNextGlyphs( 1, &aGlyphId, aPos, nStart ); )
    {
        // NOTE: Windows backend is producing unicode chars (ucs4), so on windows,
        //       ETO_GLYPH_INDEX is unusable, unless extra glyph conversion is made.
            
        SystemGlyphData aGlyph;
        aGlyph.index = static_cast<unsigned long> (aGlyphId & GF_IDXMASK);
        aGlyph.x = aPos.X();
        aGlyph.y = aPos.Y();
        aSysLayoutData.rGlyphData.push_back(aGlyph);

        int nLevel = (aGlyphId & GF_FONTMASK) >> GF_FONTSHIFT;
        if (nLevel > nFallbacklevel && nLevel < MAX_FALLBACK)
            nFallbacklevel = nLevel;
    }

    // Get font data
    aSysLayoutData.aSysFontData = GetSysFontData(nFallbacklevel);
    aSysLayoutData.orientation = rLayout->GetOrientation();
    
    rLayout->Release();

    return aSysLayoutData;
}

// -----------------------------------------------------------------------


long OutputDevice::GetMinKashida() const
{
    DBG_TRACE( "OutputDevice::GetMinKashida()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );
    if( mbNewFont && !ImplNewFont() )
        return 0;

    ImplFontEntry*      pEntry = mpFontEntry;
    ImplFontMetricData* pMetric = &(pEntry->maMetric);
    return ImplDevicePixelToLogicWidth( pMetric->mnMinKashida );
}
// -----------------------------------------------------------------------

long OutputDevice::GetMinKashida( const Font& rFont ) const
{
    // select font, query Kashida, select original font again
    Font aOldFont = GetFont();
    const_cast<OutputDevice*>(this)->SetFont( rFont );
    long aKashida = GetMinKashida();
    const_cast<OutputDevice*>(this)->SetFont( aOldFont );
    return aKashida;
}

// -----------------------------------------------------------------------
xub_StrLen OutputDevice::ValidateKashidas ( const String& rTxt, 
											xub_StrLen nIdx, xub_StrLen nLen,
											xub_StrLen nKashCount, 
											const xub_StrLen* pKashidaPos,
											xub_StrLen* pKashidaPosDropped ) const
{
   // do layout
    SalLayout* pSalLayout = ImplLayout( rTxt, nIdx, nLen );
    if( !pSalLayout )
        return 0;
	xub_StrLen nDropped = 0;
	for( int i = 0; i < nKashCount; ++i )
	{
		if( !pSalLayout->IsKashidaPosValid( pKashidaPos[ i ] ))
		{
			pKashidaPosDropped[ nDropped ] = pKashidaPos [ i ];
			++nDropped;
		}
	}
	pSalLayout->Release();
	return nDropped;
}



// -----------------------------------------------------------------------


// TODO: best is to get rid of this method completely
ULONG OutputDevice::GetKerningPairCount() const
{
    DBG_TRACE( "OutputDevice::GetKerningPairCount()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if( mbNewFont && !ImplNewFont() )
        return 0;
    if( mbInitFont )
        ImplInitFont();

    if( mpPDFWriter && mpPDFWriter->isBuiltinFont( mpFontEntry->maFontSelData.mpFontData ) )
        return 0;

    // get the kerning pair count from the device layer
    int nKernPairs = mpGraphics->GetKernPairs( 0, NULL );
    return nKernPairs;
}

// -----------------------------------------------------------------------

inline bool CmpKernData( const KerningPair& a, const KerningPair& b )
{
    return (a.nChar1 < b.nChar1) || ((a.nChar1 == a.nChar2) && (a.nChar2 < a.nChar2));
}

// TODO: best is to get rid of this method completely
void OutputDevice::GetKerningPairs( ULONG nRequestedPairs, KerningPair* pKernPairs ) const
{
    DBG_TRACE( "OutputDevice::GetKerningPairs()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    if( mbNewFont && !ImplNewFont() )
        return;
    if( mbInitFont )
        ImplInitFont();

    if( mpPDFWriter && mpPDFWriter->isBuiltinFont( mpFontEntry->maFontSelData.mpFontData ) )
        return;

    // get the kerning pairs directly from the device layer
    int nKernPairs = mpGraphics->GetKernPairs( nRequestedPairs, (ImplKernPairData*)pKernPairs );

    // sort kerning pairs
    std::sort( pKernPairs, pKernPairs+nKernPairs, CmpKernData );
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetGlyphBoundRects( const Point& rOrigin, const String& rStr,
    int nIndex, int nLen, int nBase, MetricVector& rVector )
{
    DBG_TRACE( "OutputDevice::GetGlyphBoundRect_CTL()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    rVector.clear();

    if( nLen == STRING_LEN )
        nLen = rStr.Len() - nIndex;

    Rectangle aRect;
    for( int i = 0; i < nLen; i++ )
    {
        if( !GetTextBoundRect( aRect, rStr, sal::static_int_cast<xub_StrLen>(nBase), sal::static_int_cast<xub_StrLen>(nIndex+i), 1 ) )
            break;
        aRect.Move( rOrigin.X(), rOrigin.Y() );
        rVector.push_back( aRect );
    }

    return (nLen == (int)rVector.size());
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetTextBoundRect( Rectangle& rRect,
    const String& rStr, xub_StrLen nBase, xub_StrLen nIndex, xub_StrLen nLen,
	ULONG nLayoutWidth, const sal_Int32* pDXAry ) const
{
    DBG_TRACE( "OutputDevice::GetTextBoundRect()" );
    DBG_CHKTHIS( OutputDevice, ImplDbgCheckOutputDevice );

    BOOL bRet = FALSE;
    rRect.SetEmpty();

    SalLayout* pSalLayout = NULL;
	const Point aPoint;
    // calculate offset when nBase!=nIndex
    long nXOffset = 0;
    if( nBase != nIndex )
    {
        xub_StrLen nStart = Min( nBase, nIndex );
        xub_StrLen nOfsLen = Max( nBase, nIndex ) - nStart;
        pSalLayout = ImplLayout( rStr, nStart, nOfsLen, aPoint, nLayoutWidth, pDXAry );
        if( pSalLayout )
        {
            nXOffset = pSalLayout->GetTextWidth();
            nXOffset /= pSalLayout->GetUnitsPerPixel();
            pSalLayout->Release();
            // TODO: fix offset calculation for Bidi case
            if( nBase < nIndex)
                nXOffset = -nXOffset;
        }
    }

    pSalLayout = ImplLayout( rStr, nIndex, nLen, aPoint, nLayoutWidth, pDXAry );
    Rectangle aPixelRect;
    if( pSalLayout )
    {
        bRet = pSalLayout->GetBoundRect( *mpGraphics, aPixelRect );

        if( bRet )
        {
            int nWidthFactor = pSalLayout->GetUnitsPerPixel();

            if( nWidthFactor > 1 )
            {
                double fFactor = 1.0 / nWidthFactor;
                aPixelRect.Left()
                    = static_cast< long >(aPixelRect.Left() * fFactor);
                aPixelRect.Right()
                    = static_cast< long >(aPixelRect.Right() * fFactor);
                aPixelRect.Top()
                    = static_cast< long >(aPixelRect.Top() * fFactor);
                aPixelRect.Bottom()
                    = static_cast< long >(aPixelRect.Bottom() * fFactor);
            }

            Point aRotatedOfs( mnTextOffX, mnTextOffY );
            aRotatedOfs -= pSalLayout->GetDrawPosition( Point( nXOffset, 0 ) );
            aPixelRect += aRotatedOfs;
            rRect = PixelToLogic( aPixelRect );
            if( mbMap )
                rRect += Point( maMapRes.mnMapOfsX, maMapRes.mnMapOfsY );
        }

        pSalLayout->Release();
    }

    if( bRet || (OUTDEV_PRINTER == meOutDevType) || !mpFontEntry )
        return bRet;

    // fall back to bitmap method to get the bounding rectangle,
    // so we need a monochrome virtual device with matching font
    VirtualDevice aVDev( 1 );
    Font aFont( GetFont() );
    aFont.SetShadow( FALSE );
    aFont.SetOutline( FALSE );
    aFont.SetRelief( RELIEF_NONE );
    aFont.SetOrientation( 0 );
    aFont.SetSize( Size( mpFontEntry->maFontSelData.mnWidth, mpFontEntry->maFontSelData.mnHeight ) );
    aVDev.SetFont( aFont );
    aVDev.SetTextAlign( ALIGN_TOP );

    // layout the text on the virtual device
    pSalLayout = aVDev.ImplLayout( rStr, nIndex, nLen, aPoint, nLayoutWidth, pDXAry );
    if( !pSalLayout )
        return false;

    // make the bitmap big enough
    // TODO: use factors when it would get too big
    long nWidth = pSalLayout->GetTextWidth();
    long nHeight = mpFontEntry->mnLineHeight + mnEmphasisAscent + mnEmphasisDescent;
    Point aOffset( nWidth/2, 8 );
    Size aOutSize( nWidth + 2*aOffset.X(), nHeight + 2*aOffset.Y() );
    if( !nWidth || !aVDev.SetOutputSizePixel( aOutSize ) )
        return false;

    // draw text in black
    pSalLayout->DrawBase() = aOffset;
    aVDev.SetTextColor( Color( COL_BLACK ) );
    aVDev.SetTextFillColor();
    aVDev.ImplInitTextColor();
    aVDev.ImplDrawText( *pSalLayout );
    pSalLayout->Release();

    // find extents using the bitmap
    Bitmap aBmp = aVDev.GetBitmap( Point(), aOutSize );
    BitmapReadAccess* pAcc = aBmp.AcquireReadAccess();
    if( !pAcc )
        return FALSE;
    const BitmapColor aBlack( pAcc->GetBestMatchingColor( Color( COL_BLACK ) ) );
    const long nW = pAcc->Width();
    const long nH = pAcc->Height();
    long nLeft = 0;
    long nRight = 0;

    // find top left point
    long nTop = 0;
    for(; nTop < nH; ++nTop )
    {
        for( nLeft = 0; nLeft < nW; ++nLeft )
            if( pAcc->GetPixel( nTop, nLeft ) == aBlack )
                break;
        if( nLeft < nW )
            break;
    }

    // find bottom right point
    long nBottom = nH;
    while( --nBottom >= nTop )
    {
        for( nRight = nW; --nRight >= 0; )
            if( pAcc->GetPixel( nBottom, nRight ) == aBlack )
                break;
        if( nRight >= 0 )
            break;
    }
    if( nRight < nLeft )
    {
        long nX = nRight;
        nRight = nLeft;
        nLeft  = nX;
    }

    for( long nY = nTop; nY <= nBottom; ++nY )
    {
        // find leftmost point
        long nX;
        for( nX = 0; nX < nLeft; ++nX )
            if( pAcc->GetPixel( nY, nX ) == aBlack )
                break;
        nLeft = nX;

        // find rightmost point
        for( nX = nW; --nX > nRight; )
            if( pAcc->GetPixel( nY, nX ) == aBlack )
                break;
        nRight = nX;
    }

    aBmp.ReleaseAccess( pAcc );

    if( nTop <= nBottom )
    {
        Size aSize( nRight - nLeft + 1, nBottom - nTop + 1 );
        Point aTopLeft( nLeft, nTop );
        aTopLeft -= aOffset;
        // adjust to text alignment
        aTopLeft.Y()+= mnTextOffY - (mpFontEntry->maMetric.mnAscent + mnEmphasisAscent);
        // convert to logical coordinates
        aSize = PixelToLogic( aSize );
        aTopLeft.X() = ImplDevicePixelToLogicWidth( aTopLeft.X() );
        aTopLeft.Y() = ImplDevicePixelToLogicHeight( aTopLeft.Y() );
        rRect = Rectangle( aTopLeft, aSize );
        return TRUE;
    }

    return FALSE;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetTextOutlines( ::basegfx::B2DPolyPolygonVector& rVector,
    const String& rStr, xub_StrLen nBase, xub_StrLen nIndex, xub_StrLen nLen,
    BOOL bOptimize, ULONG nTWidth, const sal_Int32* pDXArray ) const
{
    // the fonts need to be initialized
    if( mbNewFont )
        ImplNewFont();
    if( mbInitFont )
        ImplInitFont();
    if( !mpFontEntry )
        return FALSE;

    BOOL bRet = FALSE;
    rVector.clear();
    if( nLen == STRING_LEN )
        nLen = rStr.Len() - nIndex;
    rVector.reserve( nLen );

    // we want to get the Rectangle in logical units, so to
    // avoid rounding errors we just size the font in logical units
    BOOL bOldMap = mbMap;
    if( bOldMap )
    {
        const_cast<OutputDevice&>(*this).mbMap = FALSE;
        const_cast<OutputDevice&>(*this).mbNewFont = TRUE;
    }

    SalLayout* pSalLayout = NULL;

    // calculate offset when nBase!=nIndex
    long nXOffset = 0;
    if( nBase != nIndex )
    {
        xub_StrLen nStart = Min( nBase, nIndex );
        xub_StrLen nOfsLen = Max( nBase, nIndex ) - nStart;
        pSalLayout = ImplLayout( rStr, nStart, nOfsLen, Point(0,0), nTWidth, pDXArray );
        if( pSalLayout )
        {
            nXOffset = pSalLayout->GetTextWidth();
            pSalLayout->Release();
            // TODO: fix offset calculation for Bidi case
            if( nBase > nIndex)
                nXOffset = -nXOffset;
        }
    }

    pSalLayout = ImplLayout( rStr, nIndex, nLen, Point(0,0), nTWidth, pDXArray );
    if( pSalLayout )
    {
        bRet = pSalLayout->GetOutline( *mpGraphics, rVector );
        if( bRet )
        {
            // transform polygon to pixel units
            ::basegfx::B2DHomMatrix aMatrix;

            int nWidthFactor = pSalLayout->GetUnitsPerPixel();
            if( nXOffset | mnTextOffX | mnTextOffY )
            {
                Point aRotatedOfs( mnTextOffX*nWidthFactor, mnTextOffY*nWidthFactor );
                aRotatedOfs -= pSalLayout->GetDrawPosition( Point( nXOffset, 0 ) );
                aMatrix.translate( aRotatedOfs.X(), aRotatedOfs.Y() );
            }

            if( nWidthFactor > 1 )
            {
                double fFactor = 1.0 / nWidthFactor;
                aMatrix.scale( fFactor, fFactor );
            }

            if( !aMatrix.isIdentity() )
            {
                ::basegfx::B2DPolyPolygonVector::iterator aIt = rVector.begin();
                for(; aIt != rVector.end(); ++aIt )
                    (*aIt).transform( aMatrix );
            }
        }

        pSalLayout->Release();
    }

    if( bOldMap )
    {
        // restore original font size and map mode
        const_cast<OutputDevice&>(*this).mbMap = bOldMap;
        const_cast<OutputDevice&>(*this).mbNewFont = TRUE;
    }

    if( bRet || (OUTDEV_PRINTER == meOutDevType) || !mpFontEntry )
        return bRet;

    // fall back to bitmap conversion ------------------------------------------

    // Here, we can savely assume that the mapping between characters and glyphs
    // is one-to-one. This is most probably valid for the old bitmap fonts.

    // fall back to bitmap method to get the bounding rectangle,
    // so we need a monochrome virtual device with matching font
    pSalLayout = ImplLayout( rStr, nIndex, nLen, Point(0,0), nTWidth, pDXArray );
    if (pSalLayout == 0)
        return false;
    long nOrgWidth = pSalLayout->GetTextWidth();
    long nOrgHeight = mpFontEntry->mnLineHeight + mnEmphasisAscent
        + mnEmphasisDescent;
    pSalLayout->Release();

    VirtualDevice aVDev(1);

    Font aFont(GetFont());
    aFont.SetShadow(false);
    aFont.SetOutline(false);
    aFont.SetRelief(RELIEF_NONE);
    aFont.SetOrientation(0);
    if( bOptimize )
    {
        aFont.SetSize( Size( 0, GLYPH_FONT_HEIGHT ) );
        aVDev.SetMapMode( MAP_PIXEL );
    }
    aVDev.SetFont( aFont );
    aVDev.SetTextAlign( ALIGN_TOP );
    aVDev.SetTextColor( Color(COL_BLACK) );
    aVDev.SetTextFillColor();

    pSalLayout = aVDev.ImplLayout( rStr, nIndex, nLen, Point(0,0), nTWidth, pDXArray );
    if (pSalLayout == 0)
        return false;
    long nWidth = pSalLayout->GetTextWidth();
    long nHeight = ((OutputDevice*)&aVDev)->mpFontEntry->mnLineHeight + ((OutputDevice*)&aVDev)->mnEmphasisAscent
        + ((OutputDevice*)&aVDev)->mnEmphasisDescent;
    pSalLayout->Release();

    if( !nWidth || !nHeight )
        return TRUE;
    double fScaleX = static_cast< double >(nOrgWidth) / nWidth;
    double fScaleY = static_cast< double >(nOrgHeight) / nHeight;

    // calculate offset when nBase!=nIndex
    // TODO: fix offset calculation for Bidi case
    nXOffset = 0;
    if( nBase != nIndex )
    {
        xub_StrLen nStart  = ((nBase < nIndex) ? nBase : nIndex);
        xub_StrLen nLength = ((nBase > nIndex) ? nBase : nIndex) - nStart;
        pSalLayout = aVDev.ImplLayout( rStr, nStart, nLength, Point(0,0), nTWidth, pDXArray );
        if( pSalLayout )
        {
            nXOffset = pSalLayout->GetTextWidth();
            pSalLayout->Release();
            if( nBase > nIndex)
                nXOffset = -nXOffset;
        }
    }

    bRet = true;
    bool bRTL = false;
    String aStr( rStr ); // prepare for e.g. localized digits
    ImplLayoutArgs aLayoutArgs = ImplPrepareLayoutArgs( aStr, nIndex, nLen, 0, NULL );
    for( int nCharPos = -1; aLayoutArgs.GetNextPos( &nCharPos, &bRTL);)
    {
        bool bSuccess = false;

        // draw character into virtual device
        pSalLayout = aVDev.ImplLayout( rStr, static_cast< xub_StrLen >(nCharPos), 1, Point(0,0), nTWidth, pDXArray );
        if (pSalLayout == 0)
            return false;
        long nCharWidth = pSalLayout->GetTextWidth();

        Point aOffset(nCharWidth / 2, 8);
        Size aSize(nCharWidth + 2 * aOffset.X(), nHeight + 2 * aOffset.Y());
        bSuccess = (bool)aVDev.SetOutputSizePixel(aSize);
        if( bSuccess )
        {
            // draw glyph into virtual device
            aVDev.Erase();
            pSalLayout->DrawBase() += aOffset;
            pSalLayout->DrawBase() += Point( ((OutputDevice*)&aVDev)->mnTextOffX, ((OutputDevice*)&aVDev)->mnTextOffY );
            pSalLayout->DrawText( *((OutputDevice*)&aVDev)->mpGraphics );
            pSalLayout->Release();

            // convert character image into outline
            Bitmap aBmp( aVDev.GetBitmap(Point(0, 0), aSize));

            PolyPolygon aPolyPoly;
            bool bVectorized = aBmp.Vectorize(aPolyPoly, BMP_VECTORIZE_OUTER | BMP_VECTORIZE_REDUCE_EDGES);
            if( !bVectorized )
                bSuccess = false;
            else
            {
                // convert units to logical width
                for (USHORT j = 0; j < aPolyPoly.Count(); ++j)
                {
                    Polygon& rPoly = aPolyPoly[j];
                    for (USHORT k = 0; k < rPoly.GetSize(); ++k)
                    {
                        Point& rPt = rPoly[k];
                        rPt -= aOffset;
                        int nPixelX = rPt.X() - ((OutputDevice&)aVDev).mnTextOffX + nXOffset;
                        int nPixelY = rPt.Y() - ((OutputDevice&)aVDev).mnTextOffY;
                        rPt.X() = ImplDevicePixelToLogicWidth( nPixelX );
                        rPt.Y() = ImplDevicePixelToLogicHeight( nPixelY );
                    }
                }


                // ignore "empty" glyphs:
                if( aPolyPoly.Count() > 0 )
                {
                    // convert	to B2DPolyPolygon
                    // TODO: get rid of intermediate tool's PolyPolygon
                    ::basegfx::B2DPolyPolygon aB2DPolyPoly = aPolyPoly.getB2DPolyPolygon();
                    ::basegfx::B2DHomMatrix aMatrix;
                    aMatrix.scale( fScaleX, fScaleY );
                    int nAngle = GetFont().GetOrientation();
                    if( nAngle )
                        aMatrix.rotate( nAngle * F_PI1800 );
                    aB2DPolyPoly.transform( aMatrix );
                    rVector.push_back( aB2DPolyPoly );
                }
            }
        }

        nXOffset += nCharWidth;
        bRet = bRet && bSuccess;
    }

    return bRet;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetTextOutlines( PolyPolyVector& rResultVector,
    const String& rStr, xub_StrLen nBase, xub_StrLen nIndex,
    xub_StrLen nLen, BOOL bOptimize, ULONG nTWidth, const sal_Int32* pDXArray ) const
{
    rResultVector.clear();

    // get the basegfx polypolygon vector
    ::basegfx::B2DPolyPolygonVector aB2DPolyPolyVector;
    if( !GetTextOutlines( aB2DPolyPolyVector, rStr, nBase, nIndex, nLen,
                         bOptimize, nTWidth, pDXArray ) )
    return FALSE;

    // convert to a tool polypolygon vector
    rResultVector.reserve( aB2DPolyPolyVector.size() );
    ::basegfx::B2DPolyPolygonVector::const_iterator aIt = aB2DPolyPolyVector.begin();
    for(; aIt != aB2DPolyPolyVector.end(); ++aIt )
        rResultVector.push_back(PolyPolygon(*aIt)); // #i76339#

    return TRUE;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetTextOutline( PolyPolygon& rPolyPoly,
    const String& rStr, xub_StrLen nBase, xub_StrLen nIndex, xub_StrLen nLen,
    BOOL bOptimize, ULONG nTWidth, const sal_Int32* pDXArray ) const
{
    rPolyPoly.Clear();

    // get the basegfx polypolygon vector
    ::basegfx::B2DPolyPolygonVector aB2DPolyPolyVector;
    if( !GetTextOutlines( aB2DPolyPolyVector, rStr, nBase, nIndex, nLen,
                         bOptimize, nTWidth, pDXArray ) )
    return FALSE;

    // convert and merge into a tool polypolygon
    ::basegfx::B2DPolyPolygonVector::const_iterator aIt = aB2DPolyPolyVector.begin();
    for(; aIt != aB2DPolyPolyVector.end(); ++aIt )
        for( unsigned int i = 0; i < aIt->count(); ++i )
            rPolyPoly.Insert(Polygon((*aIt).getB2DPolygon( i ))); // #i76339#

    return TRUE;
}

// -----------------------------------------------------------------------

BOOL OutputDevice::GetFontCharMap( FontCharMap& rFontCharMap ) const
{
    rFontCharMap.Reset();

    // we need a graphics
    if( !mpGraphics && !ImplGetGraphics() )
        return FALSE;

    if( mbNewFont )
        ImplNewFont();
    if( mbInitFont )
        ImplInitFont();
    if( !mpFontEntry )
        return FALSE;

    // a little font charmap cache helps considerably
    static const int NMAXITEMS = 16;
    static int nUsedItems = 0, nCurItem = 0;

    struct CharMapCacheItem { const ImplFontData* mpFontData; FontCharMap maCharMap; };
    static CharMapCacheItem aCache[ NMAXITEMS ];

    const ImplFontData* pFontData = mpFontEntry->maFontSelData.mpFontData;

    int i;
    for( i = nUsedItems; --i >= 0; )
        if( pFontData == aCache[i].mpFontData )
            break;
    if( i >= 0 )	// found in cache
    {
        rFontCharMap.Reset( aCache[i].maCharMap.mpImpl );
    }
    else            // need to cache
    {
        ImplFontCharMap* pNewMap = mpGraphics->GetImplFontCharMap();
        rFontCharMap.Reset( pNewMap );

        // manage cache round-robin and insert data
        CharMapCacheItem& rItem = aCache[ nCurItem ];
        rItem.mpFontData = pFontData;
        rItem.maCharMap.Reset( pNewMap );

        if( ++nCurItem >= NMAXITEMS )
            nCurItem = 0;

        if( ++nUsedItems >= NMAXITEMS )
            nUsedItems = NMAXITEMS;
    }

    if( rFontCharMap.IsDefaultMap() )
        return FALSE;
    return TRUE;
}

// -----------------------------------------------------------------------

xub_StrLen OutputDevice::HasGlyphs( const Font& rTempFont, const String& rStr,
    xub_StrLen nIndex, xub_StrLen nLen ) const
{
    if( nIndex >= rStr.Len() )
        return nIndex;
    xub_StrLen nEnd = nIndex + nLen;
    if( (ULONG)nIndex+nLen > rStr.Len() )
        nEnd = rStr.Len();

    DBG_ASSERT( nIndex < nEnd, "StartPos >= EndPos?" );
    DBG_ASSERT( nEnd <= rStr.Len(), "String too short" );

    // to get the map temporarily set font
    const Font aOrigFont = GetFont();
    const_cast<OutputDevice&>(*this).SetFont( rTempFont );
    FontCharMap aFontCharMap;
    BOOL bRet = GetFontCharMap( aFontCharMap );
    const_cast<OutputDevice&>(*this).SetFont( aOrigFont );

    // if fontmap is unknown assume it doesn't have the glyphs
    if( bRet == FALSE )
        return nIndex;

    const sal_Unicode* pStr = rStr.GetBuffer();
    for( pStr += nIndex; nIndex < nEnd; ++pStr, ++nIndex )
        if( ! aFontCharMap.HasChar( *pStr ) )
            return nIndex;

    return STRING_LEN;
}

// -----------------------------------------------------------------------
