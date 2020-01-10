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

#include <com/sun/star/drawing/XControlShape.hpp>
#include <com/sun/star/script/ScriptEventDescriptor.hpp>
#include <svx/unoapi.hxx>
#include "xestream.hxx"
#include "document.hxx"
#include "xistream.hxx"
#include "xlescher.hxx"

using ::rtl::OUString;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::drawing::XShape;
using ::com::sun::star::drawing::XControlShape;
using ::com::sun::star::awt::XControlModel;
using ::com::sun::star::script::ScriptEventDescriptor;

// Structs and classes ========================================================

XclObjId::XclObjId() :
    mnScTab( SCTAB_INVALID ),
    mnObjId( EXC_OBJ_INVALID_ID )
{
}

XclObjId::XclObjId( SCTAB nScTab, sal_uInt16 nObjId ) :
    mnScTab( nScTab ),
    mnObjId( nObjId )
{
}

bool operator==( const XclObjId& rL, const XclObjId& rR )
{
    return (rL.mnScTab == rR.mnScTab) && (rL.mnObjId == rR.mnObjId);
}

bool operator<( const XclObjId& rL, const XclObjId& rR )
{
    return (rL.mnScTab < rR.mnScTab) || ((rL.mnScTab == rR.mnScTab) && (rL.mnObjId < rR.mnObjId));
}

// ----------------------------------------------------------------------------

namespace {

/** Returns the scaling factor to calculate coordinates from twips. */
double lclGetTwipsScale( MapUnit eMapUnit )
{
    /*  #111027# We cannot use OutputDevice::LogicToLogic() or the XclTools
        conversion functions to calculate drawing layer coordinates due to
        Calc's strange definition of a point (1 inch == 72.27 points, instead
        of 72 points). */
    double fScale = 1.0;
    switch( eMapUnit )
    {
        case MAP_TWIP:      fScale = 72 / POINTS_PER_INCH;  break;  // Calc twips <-> real twips
        case MAP_100TH_MM:  fScale = HMM_PER_TWIPS;         break;  // Calc twips <-> 1/100mm
        default:            DBG_ERRORFILE( "lclGetTwipsScale - map unit not implemented" );
    }
    return fScale;
}

/** Calculates a drawing layer X position (in twips) from an object column position. */
long lclGetXFromCol( ScDocument& rDoc, SCTAB nScTab, sal_uInt16 nXclCol, sal_uInt16 nOffset, double fScale )
{
    SCCOL nScCol = static_cast< SCCOL >( nXclCol );
    return static_cast< long >( fScale * (rDoc.GetColOffset( nScCol, nScTab ) +
        ::std::min( nOffset / 1024.0, 1.0 ) * rDoc.GetColWidth( nScCol, nScTab )) + 0.5 );
}

/** Calculates a drawing layer Y position (in twips) from an object row position. */
long lclGetYFromRow( ScDocument& rDoc, SCTAB nScTab, sal_uInt16 nXclRow, sal_uInt16 nOffset, double fScale )
{
    SCROW nScRow = static_cast< SCROW >( nXclRow );
    return static_cast< long >( fScale * (rDoc.GetRowOffset( nScRow, nScTab ) +
        ::std::min( nOffset / 256.0, 1.0 ) * rDoc.GetRowHeight( nScRow, nScTab )) + 0.5 );
}

/** Calculates an object column position from a drawing layer X position (in twips). */
void lclGetColFromX(
        ScDocument& rDoc, SCTAB nScTab, sal_uInt16& rnXclCol,
        sal_uInt16& rnOffset, sal_uInt16 nXclStartCol,
        long& rnStartW, long nX, double fScale )
{
    // rnStartW in conjunction with nXclStartCol is used as buffer for previously calculated width
    long nTwipsX = static_cast< long >( nX / fScale + 0.5 );
    long nColW = 0;
    for( rnXclCol = nXclStartCol; rnXclCol <= MAXCOL; ++rnXclCol )
    {
        nColW = rDoc.GetColWidth( static_cast<SCCOL>(rnXclCol), nScTab );
        if( rnStartW + nColW > nTwipsX )
            break;
        rnStartW += nColW;
    }
    rnOffset = nColW ? static_cast< sal_uInt16 >( (nTwipsX - rnStartW) * 1024.0 / nColW + 0.5 ) : 0;
}

/** Calculates an object row position from a drawing layer Y position (in twips). */
void lclGetRowFromY(
        ScDocument& rDoc, SCTAB nScTab,
        sal_uInt16& rnXclRow, sal_uInt16& rnOffset, sal_uInt16 nXclStartRow,
        long& rnStartH, long nY, double fScale )
{
    // rnStartH in conjunction with nXclStartRow is used as buffer for previously calculated height
    long nTwipsY = static_cast< long >( nY / fScale + 0.5 );
    long nRowH = 0;
    ScCoupledCompressedArrayIterator< SCROW, BYTE, USHORT> aIter(
            rDoc.GetRowFlagsArray( nScTab), static_cast<SCROW>(nXclStartRow),
            MAXROW, CR_HIDDEN, 0, rDoc.GetRowHeightArray( nScTab));
    for ( ; aIter; ++aIter )
    {
        nRowH = *aIter;
        if( rnStartH + nRowH > nTwipsY )
        {
            rnXclRow = static_cast< sal_uInt16 >( aIter.GetPos() );
            break;
        }
        rnStartH += nRowH;
    }
    if (!aIter)
        rnXclRow = static_cast< sal_uInt16 >( aIter.GetIterEnd() );  // down to the bottom..
    rnOffset = static_cast< sal_uInt16 >( nRowH ? ((nTwipsY - rnStartH) * 256.0 / nRowH + 0.5) : 0 );
}

/** Mirrors a rectangle (from LTR to RTL layout or vice versa). */
void lclMirrorRectangle( Rectangle& rRect )
{
    long nLeft = rRect.Left();
    rRect.Left() = -rRect.Right();
    rRect.Right() = -nLeft;
}

} // namespace

// ----------------------------------------------------------------------------

XclObjAnchor::XclObjAnchor( SCTAB nScTab ) :
    mnScTab( nScTab ),
    mnLX( 0 ),
    mnTY( 0 ),
    mnRX( 0 ),
    mnBY( 0 )
{
}

Rectangle XclObjAnchor::GetRect( ScDocument& rDoc, MapUnit eMapUnit ) const
{
    double fScale = lclGetTwipsScale( eMapUnit );
    Rectangle aRect(
        lclGetXFromCol( rDoc, mnScTab, maFirst.mnCol, mnLX, fScale ),
        lclGetYFromRow( rDoc, mnScTab, maFirst.mnRow, mnTY, fScale ),
        lclGetXFromCol( rDoc, mnScTab, maLast.mnCol,  mnRX + 1, fScale ),
        lclGetYFromRow( rDoc, mnScTab, maLast.mnRow,  mnBY, fScale ) );

    // #106948# adjust coordinates in mirrored sheets
    if( rDoc.IsLayoutRTL( mnScTab ) )
        lclMirrorRectangle( aRect );
    return aRect;
}

void XclObjAnchor::SetRect( ScDocument& rDoc, const Rectangle& rRect, MapUnit eMapUnit )
{
    Rectangle aRect( rRect );
    // #106948# adjust coordinates in mirrored sheets
    if( rDoc.IsLayoutRTL( mnScTab ) )
        lclMirrorRectangle( aRect );

    double fScale = lclGetTwipsScale( eMapUnit );
    long nDummy = 0;
    lclGetColFromX( rDoc, mnScTab, maFirst.mnCol, mnLX, 0,             nDummy, aRect.Left(),   fScale );
    lclGetColFromX( rDoc, mnScTab, maLast.mnCol,  mnRX, maFirst.mnCol, nDummy, aRect.Right(),  fScale );
    nDummy = 0;
    lclGetRowFromY( rDoc, mnScTab, maFirst.mnRow, mnTY, 0,             nDummy, aRect.Top(),    fScale );
    lclGetRowFromY( rDoc, mnScTab, maLast.mnRow,  mnBY, maFirst.mnRow, nDummy, aRect.Bottom(), fScale );
}

// ----------------------------------------------------------------------------

XclObjLineData::XclObjLineData() :
    mnColorIdx( EXC_OBJ_LINE_AUTOCOLOR ),
    mnStyle( EXC_OBJ_LINE_SOLID ),
    mnWidth( EXC_OBJ_LINE_HAIR ),
    mnAuto( EXC_OBJ_LINE_AUTO )
{
}

XclImpStream& operator>>( XclImpStream& rStrm, XclObjLineData& rLineData )
{
    return rStrm
        >> rLineData.mnColorIdx
        >> rLineData.mnStyle
        >> rLineData.mnWidth
        >> rLineData.mnAuto;
}

// ----------------------------------------------------------------------------

XclObjFillData::XclObjFillData() :
    mnBackColorIdx( EXC_OBJ_LINE_AUTOCOLOR ),
    mnPattColorIdx( EXC_OBJ_FILL_AUTOCOLOR ),
    mnPattern( EXC_PATT_SOLID ),
    mnAuto( EXC_OBJ_FILL_AUTO )
{
}

XclImpStream& operator>>( XclImpStream& rStrm, XclObjFillData& rFillData )
{
    return rStrm
        >> rFillData.mnBackColorIdx
        >> rFillData.mnPattColorIdx
        >> rFillData.mnPattern
        >> rFillData.mnAuto;
}

// ----------------------------------------------------------------------------

XclObjTextData::XclObjTextData() :
    mnTextLen( 0 ),
    mnFormatSize( 0 ),
    mnLinkSize( 0 ),
    mnDefFontIdx( EXC_FONT_APP ),
    mnFlags( 0 ),
    mnOrient( EXC_OBJ_ORIENT_NONE ),
    mnButtonFlags( 0 ),
    mnShortcut( 0 ),
    mnShortcutEA( 0 )
{
}

void XclObjTextData::ReadObj3( XclImpStream& rStrm )
{
    rStrm >> mnTextLen;
    rStrm.Ignore( 2 );
    rStrm >> mnFormatSize >> mnDefFontIdx;
    rStrm.Ignore( 2 );
    rStrm >> mnFlags >> mnOrient;
    rStrm.Ignore( 8 );
}

void XclObjTextData::ReadObj5( XclImpStream& rStrm )
{
    rStrm >> mnTextLen;
    rStrm.Ignore( 2 );
    rStrm >> mnFormatSize >> mnDefFontIdx;
    rStrm.Ignore( 2 );
    rStrm >> mnFlags >> mnOrient;
    rStrm.Ignore( 2 );
    rStrm >> mnLinkSize;
    rStrm.Ignore( 2 );
    rStrm >> mnButtonFlags >> mnShortcut >> mnShortcutEA;
}

void XclObjTextData::ReadTxo8( XclImpStream& rStrm )
{
    rStrm >> mnFlags >> mnOrient >> mnButtonFlags >> mnShortcut >> mnShortcutEA >> mnTextLen >> mnFormatSize;
}

// ============================================================================

Reference< XControlModel > XclControlHelper::GetControlModel( Reference< XShape > xShape )
{
    Reference< XControlModel > xCtrlModel;
    Reference< XControlShape > xCtrlShape( xShape, UNO_QUERY );
    if( xCtrlShape.is() )
        xCtrlModel = xCtrlShape->getControl();
    return xCtrlModel;
}

#define EXC_MACRONAME_PRE "vnd.sun.star.script:Standard."
#define EXC_MACRONAME_SUF "?language=Basic&location=document"

OUString XclControlHelper::GetScMacroName( const String& rXclMacroName )
{
    if( rXclMacroName.Len() > 0 )
        return CREATE_OUSTRING( EXC_MACRONAME_PRE ) + rXclMacroName + CREATE_OUSTRING( EXC_MACRONAME_SUF );
    return OUString();
}

String XclControlHelper::GetXclMacroName( const OUString& rScMacroName )
{
    const OUString saMacroNamePre = CREATE_OUSTRING( EXC_MACRONAME_PRE );
    const OUString saMacroNameSuf = CREATE_OUSTRING( EXC_MACRONAME_SUF );
    sal_Int32 snScMacroNameLen = rScMacroName.getLength();
    sal_Int32 snXclMacroNameLen = snScMacroNameLen - saMacroNamePre.getLength() - saMacroNameSuf.getLength();
    if( (snXclMacroNameLen > 0) && rScMacroName.matchIgnoreAsciiCase( saMacroNamePre, 0 ) &&
            rScMacroName.matchIgnoreAsciiCase( saMacroNameSuf, snScMacroNameLen - saMacroNameSuf.getLength() ) )
        return rScMacroName.copy( saMacroNamePre.getLength(), snXclMacroNameLen );
    return String::EmptyString();
}

static const struct
{
    const sal_Char*     mpcListenerType;
    const sal_Char*     mpcEventMethod;
}
spTbxListenerData[] =
{
    // Attention: MUST be in order of the XclTbxEventType enum!
    /*EXC_TBX_EVENT_ACTION*/    { "XActionListener",     "actionPerformed"        },
    /*EXC_TBX_EVENT_MOUSE*/     { "XMouseListener",      "mouseReleased"          },
    /*EXC_TBX_EVENT_TEXT*/      { "XTextListener",       "textChanged"            },
    /*EXC_TBX_EVENT_VALUE*/     { "XAdjustmentListener", "adjustmentValueChanged" },
    /*EXC_TBX_EVENT_CHANGE*/    { "XChangeListener",     "changed"                }
};

#define EXC_MACROSCRIPT "Script"

bool XclControlHelper::FillMacroDescriptor( ScriptEventDescriptor& rDescriptor,
        XclTbxEventType eEventType, const String& rXclMacroName )
{
    if( rXclMacroName.Len() > 0 )
    {
        rDescriptor.ListenerType = OUString::createFromAscii( spTbxListenerData[ eEventType ].mpcListenerType );
        rDescriptor.EventMethod = OUString::createFromAscii( spTbxListenerData[ eEventType ].mpcEventMethod );
        rDescriptor.ScriptType = CREATE_OUSTRING( EXC_MACROSCRIPT );
        rDescriptor.ScriptCode = GetScMacroName( rXclMacroName );
        return true;
    }
    return false;
}

String XclControlHelper::ExtractFromMacroDescriptor(
        const ScriptEventDescriptor& rDescriptor, XclTbxEventType eEventType )
{
    if( (rDescriptor.ScriptCode.getLength() > 0) &&
            rDescriptor.ScriptType.equalsIgnoreAsciiCaseAscii( EXC_MACROSCRIPT ) &&
            rDescriptor.ListenerType.equalsAscii( spTbxListenerData[ eEventType ].mpcListenerType ) &&
            rDescriptor.EventMethod.equalsAscii( spTbxListenerData[ eEventType ].mpcEventMethod ) )
        return GetXclMacroName( rDescriptor.ScriptCode );
    return String::EmptyString();
}

// ============================================================================

