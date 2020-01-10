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

#include "xiescher.hxx"

#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/XEmbedPersist.hpp>
#include <com/sun/star/awt/PushButtonType.hpp>
#include <com/sun/star/awt/ScrollBarOrientation.hpp>
#include <com/sun/star/awt/VisualEffect.hpp>
#include <com/sun/star/style/HorizontalAlignment.hpp>
#include <com/sun/star/style/VerticalAlignment.hpp>
#include <com/sun/star/drawing/XControlShape.hpp>
#include <com/sun/star/form/XForm.hpp>
#include <com/sun/star/form/XFormsSupplier.hpp>
#include <com/sun/star/form/binding/XBindableValue.hpp>
#include <com/sun/star/form/binding/XValueBinding.hpp>
#include <com/sun/star/form/binding/XListEntrySink.hpp>
#include <com/sun/star/form/binding/XListEntrySource.hpp>
#include <com/sun/star/script/ScriptEventDescriptor.hpp>
#include <com/sun/star/script/XEventAttacherManager.hpp>

#include <rtl/logfile.hxx>
#include <sfx2/objsh.hxx>
#include <svtools/moduleoptions.hxx>
#include <svtools/fltrcfg.hxx>
#include <svtools/wmf.hxx>
#include <comphelper/types.hxx>
#include <comphelper/classids.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>

#include <basic/sbstar.hxx>
#include <basic/sbmod.hxx>
#include <basic/sbmeth.hxx>

#include <svx/svdopath.hxx>
#include <svx/svdocirc.hxx>
#include <svx/svdoedge.hxx>
#include <svx/svdogrp.hxx>
#include <svx/svdoashp.hxx>
#include <svx/svdograf.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdocapt.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdpage.hxx>
#include <svx/editobj.hxx>
#include <svx/outliner.hxx>
#include <svx/outlobj.hxx>
#include <svx/unoapi.hxx>
#include <svx/svditer.hxx>
#include <svx/writingmodeitem.hxx>

#include "scitems.hxx"
#include <svx/eeitem.hxx>
#include <svx/colritem.hxx>
#include <svx/xflclit.hxx>
#include <svx/adjitem.hxx>
#include <svx/xlineit.hxx>
#include <svx/xlinjoit.hxx>
#include <svx/xlntrit.hxx>
#include <svx/xbtmpit.hxx>

#include "document.hxx"
#include "drwlayer.hxx"
#include "userdat.hxx"
#include "chartarr.hxx"
#include "detfunc.hxx"
#include "unonames.hxx"
#include "convuno.hxx"
#include "postit.hxx"
#include "globstr.hrc"

#include "fprogressbar.hxx"
#include "xltracer.hxx"
#include "xistream.hxx"
#include "xihelper.hxx"
#include "xiformula.hxx"
#include "xilink.hxx"
#include "xistyle.hxx"
#include "xipage.hxx"
#include "xichart.hxx"
#include "xicontent.hxx"
#include "namebuff.hxx"

using ::rtl::OUString;
using ::rtl::OUStringBuffer;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::XInterface;
using ::com::sun::star::uno::Exception;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::UNO_QUERY_THROW;
using ::com::sun::star::beans::NamedValue;
using ::com::sun::star::lang::XMultiServiceFactory;
using ::com::sun::star::container::XIndexContainer;
using ::com::sun::star::container::XNameContainer;
using ::com::sun::star::frame::XModel;
using ::com::sun::star::awt::XControlModel;
using ::com::sun::star::embed::XEmbeddedObject;
using ::com::sun::star::embed::XEmbedPersist;
using ::com::sun::star::drawing::XControlShape;
using ::com::sun::star::drawing::XShape;
using ::com::sun::star::form::XForm;
using ::com::sun::star::form::XFormComponent;
using ::com::sun::star::form::XFormsSupplier;
using ::com::sun::star::form::binding::XBindableValue;
using ::com::sun::star::form::binding::XValueBinding;
using ::com::sun::star::form::binding::XListEntrySink;
using ::com::sun::star::form::binding::XListEntrySource;
using ::com::sun::star::script::ScriptEventDescriptor;
using ::com::sun::star::script::XEventAttacherManager;
using ::com::sun::star::table::CellAddress;
using ::com::sun::star::table::CellRangeAddress;

// ============================================================================

namespace {

/** Helper class which mimics the auto_ptr< SdrObject > semantics, but calls
    SdrObject::Free instead of deleting the SdrObject directly. */
template< typename SdrObjType >
class TSdrObjectPtr
{
public:
    inline explicit     TSdrObjectPtr( SdrObjType* pObj = 0 ) : mpObj( pObj ) {}
    inline              ~TSdrObjectPtr() { free(); }

    inline const SdrObjType* operator->() const { return mpObj; }
    inline SdrObjType*  operator->() { return mpObj; }

    inline const SdrObjType* get() const { return mpObj; }
    inline SdrObjType*  get() { return mpObj; }

    inline const SdrObjType& operator*() const { return *mpObj; }
    inline SdrObjType& operator*() { return *mpObj; }

    inline bool         is() const { return mpObj != 0; }
    inline bool         operator!() const { return mpObj == 0; }

    inline void         reset( SdrObjType* pObj = 0 ) { free(); mpObj = pObj; }
    inline SdrObjType*  release() { SdrObjType* pObj = mpObj; mpObj = 0; return pObj; }

private:
                        TSdrObjectPtr( const TSdrObjectPtr& );    // not implemented
    TSdrObjectPtr&      operator=( TSdrObjectPtr& rxObj );        // not implemented

    inline void         free() { SdrObject* pObj = mpObj; mpObj = 0; SdrObject::Free( pObj ); }

private:
    SdrObjType*         mpObj;
};

typedef TSdrObjectPtr< SdrObject > SdrObjectPtr;

} // namespace

// Drawing objects ============================================================

XclImpDrawObjBase::XclImpDrawObjBase( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot ),
    maObjId( rRoot.GetCurrScTab(), EXC_OBJ_INVALID_ID ),
    mnObjType( EXC_OBJTYPE_UNKNOWN ),
    mnDffShapeId( 0 ),
    mnDffFlags( 0 ),
    mbHidden( false ),
    mbVisible( true ),
    mbPrintable( true ),
    mbAreaObj( false ),
    mbAutoMargin( true ),
    mbSimpleMacro( true ),
    mbProcessSdr( true ),
    mbInsertSdr( true ),
    mbCustomDff( false )
{
}

XclImpDrawObjBase::~XclImpDrawObjBase()
{
}

XclImpDrawObjRef XclImpDrawObjBase::ReadObj3( XclImpStream& rStrm )
{
    const XclImpRoot& rRoot = rStrm.GetRoot();
    XclImpDrawObjRef xDrawObj;

    if( rStrm.GetRecLeft() >= 30 )
    {
        sal_uInt16 nObjType;
        rStrm.Ignore( 4 );
        rStrm >> nObjType;
        switch( nObjType )
        {
            case EXC_OBJTYPE_GROUP:         xDrawObj.reset( new XclImpGroupObj( rRoot ) );          break;
            case EXC_OBJTYPE_LINE:          xDrawObj.reset( new XclImpLineObj( rRoot ) );           break;
            case EXC_OBJTYPE_RECTANGLE:     xDrawObj.reset( new XclImpRectObj( rRoot ) );           break;
            case EXC_OBJTYPE_OVAL:          xDrawObj.reset( new XclImpOvalObj( rRoot ) );           break;
            case EXC_OBJTYPE_ARC:           xDrawObj.reset( new XclImpArcObj( rRoot ) );            break;
            case EXC_OBJTYPE_CHART:         xDrawObj.reset( new XclImpChartObj( rRoot ) );          break;
            case EXC_OBJTYPE_TEXT:          xDrawObj.reset( new XclImpTextObj( rRoot ) );           break;
            case EXC_OBJTYPE_BUTTON:        xDrawObj.reset( new XclImpButtonObj( rRoot ) );         break;
            case EXC_OBJTYPE_PICTURE:       xDrawObj.reset( new XclImpPictureObj( rRoot ) );        break;
            default:
                DBG_ERROR1( "XclImpDrawObjBase::ReadObj3 - unknown object type 0x%04hX", nObjType );
                rRoot.GetTracer().TraceUnsupportedObjects();
                xDrawObj.reset( new XclImpPhObj( rRoot ) );
        }
    }

    xDrawObj->ImplReadObj3( rStrm );
    return xDrawObj;
}

XclImpDrawObjRef XclImpDrawObjBase::ReadObj4( XclImpStream& rStrm )
{
    const XclImpRoot& rRoot = rStrm.GetRoot();
    XclImpDrawObjRef xDrawObj;

    if( rStrm.GetRecLeft() >= 30 )
    {
        sal_uInt16 nObjType;
        rStrm.Ignore( 4 );
        rStrm >> nObjType;
        switch( nObjType )
        {
            case EXC_OBJTYPE_GROUP:         xDrawObj.reset( new XclImpGroupObj( rRoot ) );          break;
            case EXC_OBJTYPE_LINE:          xDrawObj.reset( new XclImpLineObj( rRoot ) );           break;
            case EXC_OBJTYPE_RECTANGLE:     xDrawObj.reset( new XclImpRectObj( rRoot ) );           break;
            case EXC_OBJTYPE_OVAL:          xDrawObj.reset( new XclImpOvalObj( rRoot ) );           break;
            case EXC_OBJTYPE_ARC:           xDrawObj.reset( new XclImpArcObj( rRoot ) );            break;
            case EXC_OBJTYPE_CHART:         xDrawObj.reset( new XclImpChartObj( rRoot ) );          break;
            case EXC_OBJTYPE_TEXT:          xDrawObj.reset( new XclImpTextObj( rRoot ) );           break;
            case EXC_OBJTYPE_BUTTON:        xDrawObj.reset( new XclImpButtonObj( rRoot ) );         break;
            case EXC_OBJTYPE_PICTURE:       xDrawObj.reset( new XclImpPictureObj( rRoot ) );        break;
            case EXC_OBJTYPE_POLYGON:       xDrawObj.reset( new XclImpPolygonObj( rRoot ) );        break;
            default:
                DBG_ERROR1( "XclImpDrawObjBase::ReadObj4 - unknown object type 0x%04hX", nObjType );
                rRoot.GetTracer().TraceUnsupportedObjects();
                xDrawObj.reset( new XclImpPhObj( rRoot ) );
        }
    }

    xDrawObj->ImplReadObj4( rStrm );
    return xDrawObj;
}

XclImpDrawObjRef XclImpDrawObjBase::ReadObj5( XclImpStream& rStrm )
{
    const XclImpRoot& rRoot = rStrm.GetRoot();
    XclImpDrawObjRef xDrawObj;

    if( rStrm.GetRecLeft() >= 34 )
    {
        sal_uInt16 nObjType;
        rStrm.Ignore( 4 );
        rStrm >> nObjType;
        switch( nObjType )
        {
            case EXC_OBJTYPE_GROUP:         xDrawObj.reset( new XclImpGroupObj( rRoot ) );          break;
            case EXC_OBJTYPE_LINE:          xDrawObj.reset( new XclImpLineObj( rRoot ) );           break;
            case EXC_OBJTYPE_RECTANGLE:     xDrawObj.reset( new XclImpRectObj( rRoot ) );           break;
            case EXC_OBJTYPE_OVAL:          xDrawObj.reset( new XclImpOvalObj( rRoot ) );           break;
            case EXC_OBJTYPE_ARC:           xDrawObj.reset( new XclImpArcObj( rRoot ) );            break;
            case EXC_OBJTYPE_CHART:         xDrawObj.reset( new XclImpChartObj( rRoot ) );          break;
            case EXC_OBJTYPE_TEXT:          xDrawObj.reset( new XclImpTextObj( rRoot ) );           break;
            case EXC_OBJTYPE_BUTTON:        xDrawObj.reset( new XclImpButtonObj( rRoot ) );         break;
            case EXC_OBJTYPE_PICTURE:       xDrawObj.reset( new XclImpPictureObj( rRoot ) );        break;
            case EXC_OBJTYPE_POLYGON:       xDrawObj.reset( new XclImpPolygonObj( rRoot ) );        break;
            case EXC_OBJTYPE_CHECKBOX:      xDrawObj.reset( new XclImpCheckBoxObj( rRoot ) );       break;
            case EXC_OBJTYPE_OPTIONBUTTON:  xDrawObj.reset( new XclImpOptionButtonObj( rRoot ) );   break;
            case EXC_OBJTYPE_EDIT:          xDrawObj.reset( new XclImpEditObj( rRoot ) );           break;
            case EXC_OBJTYPE_LABEL:         xDrawObj.reset( new XclImpLabelObj( rRoot ) );          break;
            case EXC_OBJTYPE_DIALOG:        xDrawObj.reset( new XclImpDialogObj( rRoot ) );         break;
            case EXC_OBJTYPE_SPIN:          xDrawObj.reset( new XclImpSpinButtonObj( rRoot ) );     break;
            case EXC_OBJTYPE_SCROLLBAR:     xDrawObj.reset( new XclImpScrollBarObj( rRoot ) );      break;
            case EXC_OBJTYPE_LISTBOX:       xDrawObj.reset( new XclImpListBoxObj( rRoot ) );        break;
            case EXC_OBJTYPE_GROUPBOX:      xDrawObj.reset( new XclImpGroupBoxObj( rRoot ) );       break;
            case EXC_OBJTYPE_DROPDOWN:      xDrawObj.reset( new XclImpDropDownObj( rRoot ) );       break;
            default:
                DBG_ERROR1( "XclImpDrawObjBase::ReadObj5 - unknown object type 0x%04hX", nObjType );
                rRoot.GetTracer().TraceUnsupportedObjects();
                xDrawObj.reset( new XclImpPhObj( rRoot ) );
        }
    }

    xDrawObj->ImplReadObj5( rStrm );
    return xDrawObj;
}

XclImpDrawObjRef XclImpDrawObjBase::ReadObj8( XclImpStream& rStrm )
{
    const XclImpRoot& rRoot = rStrm.GetRoot();
    XclImpDrawObjRef xDrawObj;

    if( rStrm.GetRecLeft() >= 10 )
    {
        sal_uInt16 nSubRecId, nSubRecSize, nObjType;
        rStrm >> nSubRecId >> nSubRecSize >> nObjType;
        DBG_ASSERT( nSubRecId == EXC_ID_OBJCMO, "XclImpDrawObjBase::ReadObj8 - OBJCMO subrecord expected" );
        if( (nSubRecId == EXC_ID_OBJCMO) && (nSubRecSize >= 6) )
        {
            switch( nObjType )
            {
                // in BIFF8, all simple objects support text
                case EXC_OBJTYPE_LINE:
                case EXC_OBJTYPE_ARC:
                    xDrawObj.reset( new XclImpTextObj( rRoot ) );
                    // lines and arcs may be 2-dimensional
                    xDrawObj->SetAreaObj( false );
                break;

                // in BIFF8, all simple objects support text
                case EXC_OBJTYPE_RECTANGLE:
                case EXC_OBJTYPE_OVAL:
                case EXC_OBJTYPE_POLYGON:
                case EXC_OBJTYPE_DRAWING:
                case EXC_OBJTYPE_TEXT:
                    xDrawObj.reset( new XclImpTextObj( rRoot ) );
                break;

                case EXC_OBJTYPE_GROUP:         xDrawObj.reset( new XclImpGroupObj( rRoot ) );          break;
                case EXC_OBJTYPE_CHART:         xDrawObj.reset( new XclImpChartObj( rRoot ) );          break;
                case EXC_OBJTYPE_BUTTON:        xDrawObj.reset( new XclImpButtonObj( rRoot ) );         break;
                case EXC_OBJTYPE_PICTURE:       xDrawObj.reset( new XclImpPictureObj( rRoot ) );        break;
                case EXC_OBJTYPE_CHECKBOX:      xDrawObj.reset( new XclImpCheckBoxObj( rRoot ) );       break;
                case EXC_OBJTYPE_OPTIONBUTTON:  xDrawObj.reset( new XclImpOptionButtonObj( rRoot ) );   break;
                case EXC_OBJTYPE_EDIT:          xDrawObj.reset( new XclImpEditObj( rRoot ) );           break;
                case EXC_OBJTYPE_LABEL:         xDrawObj.reset( new XclImpLabelObj( rRoot ) );          break;
                case EXC_OBJTYPE_DIALOG:        xDrawObj.reset( new XclImpDialogObj( rRoot ) );         break;
                case EXC_OBJTYPE_SPIN:          xDrawObj.reset( new XclImpSpinButtonObj( rRoot ) );     break;
                case EXC_OBJTYPE_SCROLLBAR:     xDrawObj.reset( new XclImpScrollBarObj( rRoot ) );      break;
                case EXC_OBJTYPE_LISTBOX:       xDrawObj.reset( new XclImpListBoxObj( rRoot ) );        break;
                case EXC_OBJTYPE_GROUPBOX:      xDrawObj.reset( new XclImpGroupBoxObj( rRoot ) );       break;
                case EXC_OBJTYPE_DROPDOWN:      xDrawObj.reset( new XclImpDropDownObj( rRoot ) );       break;
                case EXC_OBJTYPE_NOTE:          xDrawObj.reset( new XclImpNoteObj( rRoot ) );           break;

                default:
                    DBG_ERROR1( "XclImpDrawObjBase::ReadObj8 - unknown object type 0x%04hX", nObjType );
                    rRoot.GetTracer().TraceUnsupportedObjects();
                    xDrawObj.reset( new XclImpPhObj( rRoot ) );
            }
        }
    }

    xDrawObj->ImplReadObj8( rStrm );
    return xDrawObj;
}

void XclImpDrawObjBase::SetDffData( const DffObjData& rDffObjData, const String& rObjName, const String& rHyperlink, bool bVisible, bool bAutoMargin )
{
    mnDffShapeId = rDffObjData.nShapeId;
    mnDffFlags = rDffObjData.nSpFlags;
    maObjName = rObjName;
    maHyperlink = rHyperlink;
    mbVisible = bVisible;
    mbAutoMargin = bAutoMargin;
}

void XclImpDrawObjBase::SetAnchor( const XclObjAnchor& rAnchor )
{
    mxAnchor.reset( new XclObjAnchor( rAnchor ) );
}

String XclImpDrawObjBase::GetObjName() const
{
    /*  #118053# #i51348# Always return a non-empty name. Create English
        default names depending on the object type. This is not implemented as
        virtual functions in derived classes, as class type and object type may
        not match. */
    return (maObjName.Len() > 0) ? maObjName : GetObjectManager().GetDefaultObjName( *this );
}

bool XclImpDrawObjBase::IsValidSize( const Rectangle& rAnchorRect ) const
{
    // XclObjAnchor rounds up the width, width of 3 is the result of an Excel width of 0
    return mbAreaObj ?
        ((rAnchorRect.GetWidth() > 3) && (rAnchorRect.GetHeight() > 1)) :
        ((rAnchorRect.GetWidth() > 3) || (rAnchorRect.GetHeight() > 1));
}

ScRange XclImpDrawObjBase::GetUsedArea() const
{
    ScRange aScUsedArea( ScAddress::INITIALIZE_INVALID );
    if( mxAnchor.is() )
    {
        // #i44077# object inserted -> update used area for OLE object import
        if( GetAddressConverter().ConvertRange( aScUsedArea, *mxAnchor, GetScTab(), GetScTab(), false ) )
        {
            // reduce range, if object ends directly on borders between two columns or rows
            if( (mxAnchor->mnRX == 0) && (aScUsedArea.aStart.Col() < aScUsedArea.aEnd.Col()) )
                aScUsedArea.aEnd.IncCol( -1 );
            if( (mxAnchor->mnBY == 0) && (aScUsedArea.aStart.Row() < aScUsedArea.aEnd.Row()) )
                aScUsedArea.aEnd.IncRow( -1 );
        }
    }
    return aScUsedArea;
}

Rectangle XclImpDrawObjBase::GetAnchorRect() const
{
    Rectangle aAnchorRect;
    if( mxAnchor.is() )
        aAnchorRect = mxAnchor->GetRect( GetDoc(), MAP_100TH_MM );
    return aAnchorRect;
}

sal_Size XclImpDrawObjBase::GetProgressSize() const
{
    return DoGetProgressSize();
}

SdrObject* XclImpDrawObjBase::CreateSdrObject( const Rectangle& rAnchorRect, ScfProgressBar& rProgress, bool bDffImport ) const
{
    SdrObjectPtr xSdrObj;
    if( bDffImport && !mbCustomDff )
    {
        rProgress.Progress( GetProgressSize() );
    }
    else
    {
        xSdrObj.reset( DoCreateSdrObj( rAnchorRect, rProgress ) );
        if( xSdrObj.is() )
            xSdrObj->SetModel( GetDoc().GetDrawLayer() );
    }
    return xSdrObj.release();
}

void XclImpDrawObjBase::ProcessSdrObject( SdrObject& rSdrObj ) const
{
    // default: front layer, derived classes may have to set other layer in DoProcessSdrObj()
    rSdrObj.NbcSetLayer( SC_LAYER_FRONT );

    // set object name (GetObjName() will always return a non-empty name)
    rSdrObj.SetName( GetObjName() );

    // #i39167# full width for all objects regardless of horizontal alignment
    rSdrObj.SetMergedItem( SdrTextHorzAdjustItem( SDRTEXTHORZADJUST_BLOCK ) );

    // automatic text margin
    if( mbAutoMargin )
    {
        sal_Int32 nMargin = GetObjectManager().GetDffManager().GetDefaultTextMargin();
        rSdrObj.SetMergedItem( SdrTextLeftDistItem( nMargin ) );
        rSdrObj.SetMergedItem( SdrTextRightDistItem( nMargin ) );
        rSdrObj.SetMergedItem( SdrTextUpperDistItem( nMargin ) );
        rSdrObj.SetMergedItem( SdrTextLowerDistItem( nMargin ) );
    }

    // macro and hyperlink
#ifdef ISSUE66550_HLINK_FOR_SHAPES
    if( mbSimpleMacro && ((maMacroName.Len() > 0) || (maHyperlink.getLength() > 0)) )
    {
        if( ScMacroInfo* pInfo = ScDrawLayer::GetMacroInfo( &rSdrObj, TRUE ) )
        {
            pInfo->SetMacro( XclControlHelper::GetScMacroName( maMacroName ) );
            pInfo->SetHlink( maHyperlink );
        }
    }
#else
    if( mbSimpleMacro && (maMacroName.Len() > 0) )
        if( ScMacroInfo* pInfo = ScDrawLayer::GetMacroInfo( &rSdrObj, TRUE ) )
            pInfo->SetMacro( XclControlHelper::GetScMacroName( maMacroName ) );
#endif

    // call virtual function for object type specific processing
    DoProcessSdrObj( rSdrObj );
}

// protected ------------------------------------------------------------------

void XclImpDrawObjBase::ReadName5( XclImpStream& rStrm, sal_uInt16 nNameLen )
{
    maObjName.Erase();
    if( nNameLen > 0 )
    {
        // name length field is repeated before the name
        maObjName = rStrm.ReadByteString( false );
        // skip padding byte for word boundaries
        if( rStrm.GetRecPos() & 1 ) rStrm.Ignore( 1 );
    }
}

void XclImpDrawObjBase::ReadMacro3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    maMacroName.Erase();
    rStrm.Ignore( nMacroSize );
    // skip padding byte for word boundaries, not contained in nMacroSize
    if( rStrm.GetRecPos() & 1 ) rStrm.Ignore( 1 );
}

void XclImpDrawObjBase::ReadMacro4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    maMacroName.Erase();
    rStrm.Ignore( nMacroSize );
}

void XclImpDrawObjBase::ReadMacro5( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    maMacroName.Erase();
    rStrm.Ignore( nMacroSize );
}

void XclImpDrawObjBase::ReadMacro8( XclImpStream& rStrm )
{
    maMacroName.Erase();
    if( rStrm.GetRecLeft() > 6 )
    {
        // macro is stored in a tNameXR token containing a link to a defined name
        sal_uInt16 nFmlaSize;
        rStrm >> nFmlaSize;
        rStrm.Ignore( 4 );
        DBG_ASSERT( nFmlaSize == 7, "XclImpDrawObjBase::ReadMacro - unexpected formula size" );
        if( nFmlaSize == 7 )
        {
            sal_uInt8 nTokenId;
            sal_uInt16 nExtSheet, nExtName;
            rStrm >> nTokenId >> nExtSheet >> nExtName;
            DBG_ASSERT( nTokenId == XclTokenArrayHelper::GetTokenId( EXC_TOKID_NAMEX, EXC_TOKCLASS_REF ),
                "XclImpDrawObjBase::ReadMacro - tNameXR token expected" );
            if( nTokenId == XclTokenArrayHelper::GetTokenId( EXC_TOKID_NAMEX, EXC_TOKCLASS_REF ) )
            {
                maMacroName = GetLinkManager().GetMacroName( nExtSheet, nExtName );
                // #i38718# missing module name - try to find the macro in the imported modules
                if( maMacroName.Len() && (maMacroName.Search( '.' ) == STRING_NOTFOUND) )
                    if( SfxObjectShell* pDocShell = GetDocShell() )
                        if( StarBASIC* pBasic = pDocShell->GetBasic() )
                            if( SbMethod* pMethod = dynamic_cast< SbMethod* >( pBasic->Find( maMacroName, SbxCLASS_METHOD ) ) )
                                if( SbModule* pModule = pMethod->GetModule() )
                                    maMacroName.Insert( '.', 0 ).Insert( pModule->GetName(), 0 );
            }
        }
    }
}

void XclImpDrawObjBase::ConvertLineStyle( SdrObject& rSdrObj, const XclObjLineData& rLineData ) const
{
    if( rLineData.IsAuto() )
    {
        XclObjLineData aAutoData;
        aAutoData.mnAuto = 0;
        ConvertLineStyle( rSdrObj, aAutoData );
    }
    else
    {
        long nLineWidth = 35 * ::std::min( rLineData.mnWidth, EXC_OBJ_LINE_THICK );
        rSdrObj.SetMergedItem( XLineWidthItem( nLineWidth ) );
        rSdrObj.SetMergedItem( XLineColorItem( EMPTY_STRING, GetPalette().GetColor( rLineData.mnColorIdx ) ) );
        rSdrObj.SetMergedItem( XLineJointItem( XLINEJOINT_MITER ) );

        ULONG nDotLen = ::std::max< ULONG >( 70 * rLineData.mnWidth, 35 );
        ULONG nDashLen = 3 * nDotLen;
        ULONG nDist = 2 * nDotLen;

        switch( rLineData.mnStyle )
        {
            default:
            case EXC_OBJ_LINE_SOLID:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_SOLID ) );
            break;
            case EXC_OBJ_LINE_DASH:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_DASH ) );
                rSdrObj.SetMergedItem( XLineDashItem( EMPTY_STRING, XDash( XDASH_RECT, 0, nDotLen, 1, nDashLen, nDist ) ) );
            break;
            case EXC_OBJ_LINE_DOT:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_DASH ) );
                rSdrObj.SetMergedItem( XLineDashItem( EMPTY_STRING, XDash( XDASH_RECT, 1, nDotLen, 0, nDashLen, nDist ) ) );
            break;
            case EXC_OBJ_LINE_DASHDOT:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_DASH ) );
                rSdrObj.SetMergedItem( XLineDashItem( EMPTY_STRING, XDash( XDASH_RECT, 1, nDotLen, 1, nDashLen, nDist ) ) );
            break;
            case EXC_OBJ_LINE_DASHDOTDOT:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_DASH ) );
                rSdrObj.SetMergedItem( XLineDashItem( EMPTY_STRING, XDash( XDASH_RECT, 2, nDotLen, 1, nDashLen, nDist ) ) );
            break;
            case EXC_OBJ_LINE_MEDTRANS:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_SOLID ) );
                rSdrObj.SetMergedItem( XLineTransparenceItem( 50 ) );
            break;
            case EXC_OBJ_LINE_DARKTRANS:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_SOLID ) );
                rSdrObj.SetMergedItem( XLineTransparenceItem( 25 ) );
            break;
            case EXC_OBJ_LINE_LIGHTTRANS:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_SOLID ) );
                rSdrObj.SetMergedItem( XLineTransparenceItem( 75 ) );
            break;
            case EXC_OBJ_LINE_NONE:
                rSdrObj.SetMergedItem( XLineStyleItem( XLINE_NONE ) );
            break;
        }
    }
}

void XclImpDrawObjBase::ConvertFillStyle( SdrObject& rSdrObj, const XclObjFillData& rFillData ) const
{
    if( rFillData.IsAuto() )
    {
        XclObjFillData aAutoData;
        aAutoData.mnAuto = 0;
        ConvertFillStyle( rSdrObj, aAutoData );
    }
    else if( rFillData.mnPattern == EXC_PATT_NONE )
    {
        rSdrObj.SetMergedItem( XFillStyleItem( XFILL_NONE ) );
    }
    else
    {
        Color aPattColor = GetPalette().GetColor( rFillData.mnPattColorIdx );
        Color aBackColor = GetPalette().GetColor( rFillData.mnBackColorIdx );
        if( (rFillData.mnPattern == EXC_PATT_SOLID) || (aPattColor == aBackColor) )
        {
            rSdrObj.SetMergedItem( XFillStyleItem( XFILL_SOLID ) );
            rSdrObj.SetMergedItem( XFillColorItem( EMPTY_STRING, aPattColor ) );
        }
        else
        {
            static const sal_uInt8 sppnPatterns[][ 8 ] =
            {
                { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 },
                { 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD },
                { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 },
                { 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 },
                { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
                { 0x33, 0x66, 0xCC, 0x99, 0x33, 0x66, 0xCC, 0x99 },
                { 0xCC, 0x66, 0x33, 0x99, 0xCC, 0x66, 0x33, 0x99 },
                { 0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33 },
                { 0xCC, 0xFF, 0x33, 0xFF, 0xCC, 0xFF, 0x33, 0xFF },
                { 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 },
                { 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88 },
                { 0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88 },
                { 0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11 },
                { 0xFF, 0x11, 0x11, 0x11, 0xFF, 0x11, 0x11, 0x11 },
                { 0xAA, 0x44, 0xAA, 0x11, 0xAA, 0x44, 0xAA, 0x11 },
                { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 },
                { 0x80, 0x00, 0x08, 0x00, 0x80, 0x00, 0x08, 0x00 }
            };
            const sal_uInt8* const pnPattern = sppnPatterns[ ::std::min< size_t >( rFillData.mnPattern - 2, STATIC_TABLE_SIZE( sppnPatterns ) ) ];
            // create 2-colored 8x8 DIB
            SvMemoryStream aMemStrm;
            aMemStrm << sal_uInt32( 12 ) << sal_Int16( 8 ) << sal_Int16( 8 ) << sal_uInt16( 1 ) << sal_uInt16( 1 );
            aMemStrm << sal_uInt8( 0xFF ) << sal_uInt8( 0xFF ) << sal_uInt8( 0xFF );
            aMemStrm << sal_uInt8( 0x00 ) << sal_uInt8( 0x00 ) << sal_uInt8( 0x00 );
            for( size_t nIdx = 0; nIdx < 8; ++nIdx )
                aMemStrm << sal_uInt32( pnPattern[ nIdx ] ); // 32-bit little-endian
            aMemStrm.Seek( STREAM_SEEK_TO_BEGIN );
            Bitmap aBitmap;
            aBitmap.Read( aMemStrm, FALSE );
            XOBitmap aXOBitmap( aBitmap );
            aXOBitmap.Bitmap2Array();
            aXOBitmap.SetBitmapType( XBITMAP_8X8 );
            if( aXOBitmap.GetBackgroundColor().GetColor() == COL_BLACK )
                ::std::swap( aPattColor, aBackColor );
            aXOBitmap.SetPixelColor( aPattColor );
            aXOBitmap.SetBackgroundColor( aBackColor );
            rSdrObj.SetMergedItem( XFillStyleItem( XFILL_BITMAP ) );
            rSdrObj.SetMergedItem( XFillBitmapItem( EMPTY_STRING, aXOBitmap ) );
        }
    }
}

void XclImpDrawObjBase::ConvertFrameStyle( SdrObject& rSdrObj, sal_uInt16 nFrameFlags ) const
{
    if( ::get_flag( nFrameFlags, EXC_OBJ_FRAME_SHADOW ) )
    {
        rSdrObj.SetMergedItem( SdrShadowItem( TRUE ) );
        rSdrObj.SetMergedItem( SdrShadowXDistItem( 35 ) );
        rSdrObj.SetMergedItem( SdrShadowYDistItem( 35 ) );
        rSdrObj.SetMergedItem( SdrShadowColorItem( EMPTY_STRING, GetPalette().GetColor( EXC_COLOR_WINDOWTEXT ) ) );
    }
}

Color XclImpDrawObjBase::GetSolidLineColor( const XclObjLineData& rLineData ) const
{
    Color aColor( COL_TRANSPARENT );
    if( rLineData.IsAuto() )
    {
        XclObjLineData aAutoData;
        aAutoData.mnAuto = 0;
        aColor = GetSolidLineColor( aAutoData );
    }
    else if( rLineData.mnStyle != EXC_OBJ_LINE_NONE )
    {
        aColor = GetPalette().GetColor( rLineData.mnColorIdx );
    }
    return aColor;
}

Color XclImpDrawObjBase::GetSolidFillColor( const XclObjFillData& rFillData ) const
{
    Color aColor( COL_TRANSPARENT );
    if( rFillData.IsAuto() )
    {
        XclObjFillData aAutoData;
        aAutoData.mnAuto = 0;
        aColor = GetSolidFillColor( aAutoData );
    }
    else if( rFillData.mnPattern != EXC_PATT_NONE )
    {
        Color aPattColor = GetPalette().GetColor( rFillData.mnPattColorIdx );
        Color aBackColor = GetPalette().GetColor( rFillData.mnBackColorIdx );
        aColor = XclTools::GetPatternColor( aPattColor, aBackColor, rFillData.mnPattern );
    }
    return aColor;
}

void XclImpDrawObjBase::DoReadObj3( XclImpStream&, sal_uInt16 )
{
}

void XclImpDrawObjBase::DoReadObj4( XclImpStream&, sal_uInt16 )
{
}

void XclImpDrawObjBase::DoReadObj5( XclImpStream&, sal_uInt16, sal_uInt16 )
{
}

void XclImpDrawObjBase::DoReadObj8SubRec( XclImpStream&, sal_uInt16, sal_uInt16 )
{
}

sal_Size XclImpDrawObjBase::DoGetProgressSize() const
{
    return 1;
}

SdrObject* XclImpDrawObjBase::DoCreateSdrObj( const Rectangle&, ScfProgressBar& rProgress ) const
{
    rProgress.Progress( GetProgressSize() );
    return 0;
}

void XclImpDrawObjBase::DoProcessSdrObj( SdrObject& /*rSdrObj*/ ) const
{
    // trace if object is not printable
    if( !IsPrintable() )
        GetTracer().TraceObjectNotPrintable();
}

void XclImpDrawObjBase::ImplReadObj3( XclImpStream& rStrm )
{
    sal_uInt16 nObjFlags, nMacroSize;
    XclObjAnchor aAnchor( GetCurrScTab() );

    // back to offset 4 (ignore object count field)
    rStrm.Seek( 4 );
    rStrm >> mnObjType >> maObjId.mnObjId >> nObjFlags >> aAnchor >> nMacroSize;
    rStrm.Ignore( 2 );

    mbHidden = ::get_flag( nObjFlags, EXC_OBJ_HIDDEN );
    mbVisible = ::get_flag( nObjFlags, EXC_OBJ_VISIBLE );
    SetAnchor( aAnchor );
    DoReadObj3( rStrm, nMacroSize );
}

void XclImpDrawObjBase::ImplReadObj4( XclImpStream& rStrm )
{
    sal_uInt16 nObjFlags, nMacroSize;
    XclObjAnchor aAnchor( GetCurrScTab() );

    // back to offset 4 (ignore object count field)
    rStrm.Seek( 4 );
    rStrm >> mnObjType >> maObjId.mnObjId >> nObjFlags >> aAnchor >> nMacroSize;
    rStrm.Ignore( 2 );

    mbHidden = ::get_flag( nObjFlags, EXC_OBJ_HIDDEN );
    mbVisible = ::get_flag( nObjFlags, EXC_OBJ_VISIBLE );
    mbPrintable = ::get_flag( nObjFlags, EXC_OBJ_PRINTABLE );
    SetAnchor( aAnchor );
    DoReadObj4( rStrm, nMacroSize );
}

void XclImpDrawObjBase::ImplReadObj5( XclImpStream& rStrm )
{
    sal_uInt16 nObjFlags, nMacroSize, nNameLen;
    XclObjAnchor aAnchor( GetCurrScTab() );

    // back to offset 4 (ignore object count field)
    rStrm.Seek( 4 );
    rStrm >> mnObjType >> maObjId.mnObjId >> nObjFlags >> aAnchor >> nMacroSize;
    rStrm.Ignore( 2 );
    rStrm >> nNameLen;
    rStrm.Ignore( 2 );

    mbHidden = ::get_flag( nObjFlags, EXC_OBJ_HIDDEN );
    mbVisible = ::get_flag( nObjFlags, EXC_OBJ_VISIBLE );
    mbPrintable = ::get_flag( nObjFlags, EXC_OBJ_PRINTABLE );
    SetAnchor( aAnchor );
    DoReadObj5( rStrm, nNameLen, nMacroSize );
}

void XclImpDrawObjBase::ImplReadObj8( XclImpStream& rStrm )
{
    // back to beginning
    rStrm.Seek( EXC_REC_SEEK_TO_BEGIN );

    bool bLoop = true;
    while( bLoop && (rStrm.GetRecLeft() >= 4) )
    {
        sal_uInt16 nSubRecId, nSubRecSize;
        rStrm >> nSubRecId >> nSubRecSize;
        rStrm.PushPosition();
        // sometimes the last subrecord has an invalid length (OBJLBSDATA) -> min()
        nSubRecSize = static_cast< sal_uInt16 >( ::std::min< sal_Size >( nSubRecSize, rStrm.GetRecLeft() ) );

        switch( nSubRecId )
        {
            case EXC_ID_OBJCMO:
                DBG_ASSERT( rStrm.GetRecPos() == 4, "XclImpDrawObjBase::ImplReadObj8 - unexpected OBJCMO subrecord" );
                if( (rStrm.GetRecPos() == 4) && (nSubRecSize >= 6) )
                {
                    sal_uInt16 nObjFlags;
                    rStrm >> mnObjType >> maObjId.mnObjId >> nObjFlags;
                    mbPrintable = ::get_flag( nObjFlags, EXC_OBJCMO_PRINTABLE );
                }
            break;
            case EXC_ID_OBJMACRO:
                ReadMacro8( rStrm );
            break;
            case EXC_ID_OBJEND:
                bLoop = false;
            break;
            default:
                DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
        }

        rStrm.PopPosition();
        rStrm.Ignore( nSubRecSize );
    }

    /*  Call DoReadObj8SubRec() with EXC_ID_OBJEND for further stream
        processing (e.g. charts), even if the OBJEND subrecord is missing. */
    DoReadObj8SubRec( rStrm, EXC_ID_OBJEND, 0 );

    /*  Pictures that Excel reads from BIFF5 and writes to BIFF8 still have the
        IMGDATA record following the OBJ record (but they use the image data
        stored in DFF). The IMGDATA record may be continued by several CONTINUE
        records. But the last CONTINUE record may be in fact an MSODRAWING
        record that contains the DFF data of the next drawing object! So we
        have to skip just enough CONTINUE records to look at the next
        MSODRAWING/CONTINUE record. */
    if( (rStrm.GetNextRecId() == EXC_ID3_IMGDATA) && rStrm.StartNextRecord() )
    {
        sal_uInt32 nDataSize;
        rStrm.Ignore( 4 );
        rStrm >> nDataSize;
        nDataSize -= rStrm.GetRecLeft();
        // skip following CONTINUE records until IMGDATA ends
        while( (nDataSize > 0) && (rStrm.GetNextRecId() == EXC_ID_CONT) && rStrm.StartNextRecord() )
        {
            DBG_ASSERT( nDataSize >= rStrm.GetRecLeft(), "XclImpDrawObjBase::ImplReadObj8 - CONTINUE too long" );
            nDataSize -= ::std::min< sal_uInt32 >( rStrm.GetRecLeft(), nDataSize );
        }
        DBG_ASSERT( nDataSize == 0, "XclImpDrawObjBase::ImplReadObj8 - missing CONTINUE records" );
        // next record may be MSODRAWING or CONTINUE or anything else
    }
}

// ----------------------------------------------------------------------------

void XclImpDrawObjVector::InsertGrouped( XclImpDrawObjRef xDrawObj )
{
    if( !empty() )
        if( XclImpGroupObj* pGroupObj = dynamic_cast< XclImpGroupObj* >( back().get() ) )
            if( pGroupObj->TryInsert( xDrawObj ) )
                return;
    push_back( xDrawObj );
}

sal_Size XclImpDrawObjVector::GetProgressSize() const
{
    sal_Size nProgressSize = 0;
    for( const_iterator aIt = begin(), aEnd = end(); aIt != aEnd; ++aIt )
        nProgressSize += (*aIt)->GetProgressSize();
    return nProgressSize;
}

// ----------------------------------------------------------------------------

XclImpPhObj::XclImpPhObj( const XclImpRoot& rRoot ) :
    XclImpDrawObjBase( rRoot )
{
    SetProcessSdrObj( false );
}

// ----------------------------------------------------------------------------

XclImpGroupObj::XclImpGroupObj( const XclImpRoot& rRoot ) :
    XclImpDrawObjBase( rRoot ),
    mnFirstUngrouped( 0 )
{
}

bool XclImpGroupObj::TryInsert( XclImpDrawObjRef xDrawObj )
{
    if( (xDrawObj->GetScTab() != GetScTab()) || (xDrawObj->GetObjId().mnObjId == mnFirstUngrouped) )
        return false;
    // insert into own list or into nested group
    maChildren.InsertGrouped( xDrawObj );
    return true;
}

void XclImpGroupObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    rStrm.Ignore( 4 );
    rStrm >> mnFirstUngrouped;
    rStrm.Ignore( 16 );
    ReadMacro3( rStrm, nMacroSize );
}

void XclImpGroupObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    rStrm.Ignore( 4 );
    rStrm >> mnFirstUngrouped;
    rStrm.Ignore( 16 );
    ReadMacro4( rStrm, nMacroSize );
}

void XclImpGroupObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    rStrm.Ignore( 4 );
    rStrm >> mnFirstUngrouped;
    rStrm.Ignore( 16 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
}

sal_Size XclImpGroupObj::DoGetProgressSize() const
{
    return XclImpDrawObjBase::DoGetProgressSize() + maChildren.GetProgressSize();
}

SdrObject* XclImpGroupObj::DoCreateSdrObj( const Rectangle& /*rAnchorRect*/, ScfProgressBar& rProgress ) const
{
    TSdrObjectPtr< SdrObjGroup > xSdrObj( new SdrObjGroup );
    // child objects in BIFF2-BIFF5 have absolute size, not needed to pass own anchor rectangle
    for( XclImpDrawObjVector::const_iterator aIt = maChildren.begin(), aEnd = maChildren.end(); aIt != aEnd; ++aIt )
        GetObjectManager().GetDffManager().ProcessObject( xSdrObj->GetSubList(), **aIt );
    rProgress.Progress();
    return xSdrObj.release();
}

// ----------------------------------------------------------------------------

XclImpLineObj::XclImpLineObj( const XclImpRoot& rRoot ) :
    XclImpDrawObjBase( rRoot ),
    mnArrows( 0 ),
    mnStartPoint( EXC_OBJ_LINE_TL )
{
    SetAreaObj( false );
}

void XclImpLineObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    rStrm >> maLineData >> mnArrows >> mnStartPoint;
    rStrm.Ignore( 1 );
    ReadMacro3( rStrm, nMacroSize );
}

void XclImpLineObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    rStrm >> maLineData >> mnArrows >> mnStartPoint;
    rStrm.Ignore( 1 );
    ReadMacro4( rStrm, nMacroSize );
}

void XclImpLineObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    rStrm >> maLineData >> mnArrows >> mnStartPoint;
    rStrm.Ignore( 1 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
}

SdrObject* XclImpLineObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    ::basegfx::B2DPolygon aB2DPolygon;
    switch( mnStartPoint )
    {
        default:
        case EXC_OBJ_LINE_TL:
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Left(), rAnchorRect.Top() ) );
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Right(), rAnchorRect.Bottom() ) );
        break;
        case EXC_OBJ_LINE_TR:
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Right(), rAnchorRect.Top() ) );
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Left(), rAnchorRect.Bottom() ) );
        break;
        case EXC_OBJ_LINE_BR:
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Right(), rAnchorRect.Bottom() ) );
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Left(), rAnchorRect.Top() ) );
        break;
        case EXC_OBJ_LINE_BL:
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Left(), rAnchorRect.Bottom() ) );
            aB2DPolygon.append( ::basegfx::B2DPoint( rAnchorRect.Right(), rAnchorRect.Top() ) );
        break;
    }
    SdrObjectPtr xSdrObj( new SdrPathObj( OBJ_LINE, ::basegfx::B2DPolyPolygon( aB2DPolygon ) ) );
    ConvertLineStyle( *xSdrObj, maLineData );

    // line ends
    sal_uInt8 nArrowType = ::extract_value< sal_uInt8 >( mnArrows, 0, 4 );
    bool bLineStart = false;
    bool bLineEnd = false;
    bool bFilled = false;
    switch( nArrowType )
    {
        case EXC_OBJ_ARROW_OPEN:        bLineStart = false; bLineEnd = true;  bFilled = false;  break;
        case EXC_OBJ_ARROW_OPENBOTH:    bLineStart = true;  bLineEnd = true;  bFilled = false;  break;
        case EXC_OBJ_ARROW_FILLED:      bLineStart = false; bLineEnd = true;  bFilled = true;   break;
        case EXC_OBJ_ARROW_FILLEDBOTH:  bLineStart = true;  bLineEnd = true;  bFilled = true;   break;
    }
    if( bLineStart || bLineEnd )
    {
        sal_uInt8 nArrowWidth = ::extract_value< sal_uInt8 >( mnArrows, 4, 4 );
        double fArrowWidth = 3.0;
        switch( nArrowWidth )
        {
            case EXC_OBJ_ARROW_NARROW:  fArrowWidth = 2.0;  break;
            case EXC_OBJ_ARROW_MEDIUM:  fArrowWidth = 3.0;  break;
            case EXC_OBJ_ARROW_WIDE:    fArrowWidth = 5.0;  break;
        }

        sal_uInt8 nArrowLength = ::extract_value< sal_uInt8 >( mnArrows, 8, 4 );
        double fArrowLength = 3.0;
        switch( nArrowLength )
        {
            case EXC_OBJ_ARROW_NARROW:  fArrowLength = 2.5; break;
            case EXC_OBJ_ARROW_MEDIUM:  fArrowLength = 3.5; break;
            case EXC_OBJ_ARROW_WIDE:    fArrowLength = 6.0; break;
        }

        ::basegfx::B2DPolygon aArrowPoly;
#define EXC_ARROW_POINT( x, y ) ::basegfx::B2DPoint( fArrowWidth * (x), fArrowLength * (y) )
        if( bFilled )
        {
            aArrowPoly.append( EXC_ARROW_POINT(   0, 100 ) );
            aArrowPoly.append( EXC_ARROW_POINT(  50,   0 ) );
            aArrowPoly.append( EXC_ARROW_POINT( 100, 100 ) );
        }
        else
        {
            sal_uInt8 nLineWidth = ::limit_cast< sal_uInt8 >( maLineData.mnWidth, EXC_OBJ_LINE_THIN, EXC_OBJ_LINE_THICK );
            aArrowPoly.append( EXC_ARROW_POINT( 50, 0 ) );
            aArrowPoly.append( EXC_ARROW_POINT( 100, 100 - 3 * nLineWidth ) );
            aArrowPoly.append( EXC_ARROW_POINT( 100 - 5 * nLineWidth, 100 ) );
            aArrowPoly.append( EXC_ARROW_POINT( 50, 12 * nLineWidth ) );
            aArrowPoly.append( EXC_ARROW_POINT( 5 * nLineWidth, 100 ) );
            aArrowPoly.append( EXC_ARROW_POINT( 0, 100 - 3 * nLineWidth ) );
        }
#undef EXC_ARROW_POINT

        ::basegfx::B2DPolyPolygon aArrowPolyPoly( aArrowPoly );
        long nWidth = static_cast< long >( 125 * fArrowWidth );
        if( bLineStart )
        {
            xSdrObj->SetMergedItem( XLineStartItem( EMPTY_STRING, aArrowPolyPoly ) );
            xSdrObj->SetMergedItem( XLineStartWidthItem( nWidth ) );
            xSdrObj->SetMergedItem( XLineStartCenterItem( FALSE ) );
        }
        if( bLineEnd )
        {
            xSdrObj->SetMergedItem( XLineEndItem( EMPTY_STRING, aArrowPolyPoly ) );
            xSdrObj->SetMergedItem( XLineEndWidthItem( nWidth ) );
            xSdrObj->SetMergedItem( XLineEndCenterItem( FALSE ) );
        }
    }
    rProgress.Progress();
    return xSdrObj.release();
}

// ----------------------------------------------------------------------------

XclImpRectObj::XclImpRectObj( const XclImpRoot& rRoot ) :
    XclImpDrawObjBase( rRoot ),
    mnFrameFlags( 0 )
{
    SetAreaObj( true );
}

void XclImpRectObj::ReadFrameData( XclImpStream& rStrm )
{
    rStrm >> maFillData >> maLineData >> mnFrameFlags;
}

void XclImpRectObj::ConvertRectStyle( SdrObject& rSdrObj ) const
{
    ConvertLineStyle( rSdrObj, maLineData );
    ConvertFillStyle( rSdrObj, maFillData );
    ConvertFrameStyle( rSdrObj, mnFrameFlags );
}

void XclImpRectObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    ReadMacro3( rStrm, nMacroSize );
}

void XclImpRectObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    ReadMacro4( rStrm, nMacroSize );
}

void XclImpRectObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
}

SdrObject* XclImpRectObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    SdrObjectPtr xSdrObj( new SdrRectObj( rAnchorRect ) );
    ConvertRectStyle( *xSdrObj );
    rProgress.Progress();
    return xSdrObj.release();
}

// ----------------------------------------------------------------------------

XclImpOvalObj::XclImpOvalObj( const XclImpRoot& rRoot ) :
    XclImpRectObj( rRoot )
{
}

SdrObject* XclImpOvalObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    SdrObjectPtr xSdrObj( new SdrCircObj( OBJ_CIRC, rAnchorRect ) );
    ConvertRectStyle( *xSdrObj );
    rProgress.Progress();
    return xSdrObj.release();
}

// ----------------------------------------------------------------------------

XclImpArcObj::XclImpArcObj( const XclImpRoot& rRoot ) :
    XclImpDrawObjBase( rRoot ),
    mnQuadrant( EXC_OBJ_ARC_TR )
{
    SetAreaObj( false );    // arc may be 2-dimensional
}

void XclImpArcObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    rStrm >> maFillData >> maLineData >> mnQuadrant;
    rStrm.Ignore( 1 );
    ReadMacro3( rStrm, nMacroSize );
}

void XclImpArcObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    rStrm >> maFillData >> maLineData >> mnQuadrant;
    rStrm.Ignore( 1 );
    ReadMacro4( rStrm, nMacroSize );
}

void XclImpArcObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    rStrm >> maFillData >> maLineData >> mnQuadrant;
    rStrm.Ignore( 1 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
}

SdrObject* XclImpArcObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    Rectangle aNewRect = rAnchorRect;
    long nStartAngle = 0;
    long nEndAngle = 0;
    switch( mnQuadrant )
    {
        default:
        case EXC_OBJ_ARC_TR:
            nStartAngle = 0;
            nEndAngle = 9000;
            aNewRect.Left() -= rAnchorRect.GetWidth();
            aNewRect.Bottom() += rAnchorRect.GetHeight();
        break;
        case EXC_OBJ_ARC_TL:
            nStartAngle = 9000;
            nEndAngle = 18000;
            aNewRect.Right() += rAnchorRect.GetWidth();
            aNewRect.Bottom() += rAnchorRect.GetHeight();
        break;
        case EXC_OBJ_ARC_BL:
            nStartAngle = 18000;
            nEndAngle = 27000;
            aNewRect.Right() += rAnchorRect.GetWidth();
            aNewRect.Top() -= rAnchorRect.GetHeight();
        break;
        case EXC_OBJ_ARC_BR:
            nStartAngle = 27000;
            nEndAngle = 0;
            aNewRect.Left() -= rAnchorRect.GetWidth();
            aNewRect.Top() -= rAnchorRect.GetHeight();
        break;
    }
    SdrObjKind eObjKind = maFillData.IsFilled() ? OBJ_SECT : OBJ_CARC;
    SdrObjectPtr xSdrObj( new SdrCircObj( eObjKind, aNewRect, nStartAngle, nEndAngle ) );
    ConvertFillStyle( *xSdrObj, maFillData );
    ConvertLineStyle( *xSdrObj, maLineData );
    rProgress.Progress();
    return xSdrObj.release();
}

// ----------------------------------------------------------------------------

XclImpPolygonObj::XclImpPolygonObj( const XclImpRoot& rRoot ) :
    XclImpRectObj( rRoot ),
    mnPolyFlags( 0 ),
    mnPointCount( 0 )
{
    SetAreaObj( false );    // polygon may be 2-dimensional
}

void XclImpPolygonObj::ReadCoordList( XclImpStream& rStrm )
{
    if( (rStrm.GetNextRecId() == EXC_ID_COORDLIST) && rStrm.StartNextRecord() )
    {
        DBG_ASSERT( rStrm.GetRecLeft() / 4 == mnPointCount, "XclImpPolygonObj::ReadCoordList - wrong polygon point count" );
        while( rStrm.GetRecLeft() >= 4 )
        {
            sal_uInt16 nX, nY;
            rStrm >> nX >> nY;
            maCoords.push_back( Point( nX, nY ) );
        }
    }
}

void XclImpPolygonObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    rStrm >> mnPolyFlags;
    rStrm.Ignore( 10 );
    rStrm >> mnPointCount;
    rStrm.Ignore( 8 );
    ReadMacro4( rStrm, nMacroSize );
    ReadCoordList( rStrm );
}

void XclImpPolygonObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    rStrm >> mnPolyFlags;
    rStrm.Ignore( 10 );
    rStrm >> mnPointCount;
    rStrm.Ignore( 8 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
    ReadCoordList( rStrm );
}

namespace {

::basegfx::B2DPoint lclGetPolyPoint( const Rectangle& rAnchorRect, const Point& rPoint )
{
    return ::basegfx::B2DPoint(
        rAnchorRect.Left() + static_cast< sal_Int32 >( ::std::min< double >( rPoint.X(), 16384.0 ) / 16384.0 * rAnchorRect.GetWidth() + 0.5 ),
        rAnchorRect.Top() + static_cast< sal_Int32 >( ::std::min< double >( rPoint.Y(), 16384.0 ) / 16384.0 * rAnchorRect.GetHeight() + 0.5 ) );
}

} // namespace

SdrObject* XclImpPolygonObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    SdrObjectPtr xSdrObj;
    if( maCoords.size() >= 2 )
    {
        // create the polygon
        ::basegfx::B2DPolygon aB2DPolygon;
        for( PointVector::const_iterator aIt = maCoords.begin(), aEnd = maCoords.end(); aIt != aEnd; ++aIt )
            aB2DPolygon.append( lclGetPolyPoint( rAnchorRect, *aIt ) );
        // close polygon if specified
        if( ::get_flag( mnPolyFlags, EXC_OBJ_POLY_CLOSED ) && (maCoords.front() != maCoords.back()) )
            aB2DPolygon.append( lclGetPolyPoint( rAnchorRect, maCoords.front() ) );
        // create the SdrObject
        SdrObjKind eObjKind = maFillData.IsFilled() ? OBJ_PATHPOLY : OBJ_PATHPLIN;
        xSdrObj.reset( new SdrPathObj( eObjKind, ::basegfx::B2DPolyPolygon( aB2DPolygon ) ) );
        ConvertRectStyle( *xSdrObj );
    }
    rProgress.Progress();
    return xSdrObj.release();
}

// ----------------------------------------------------------------------------

void XclImpObjTextData::ReadByteString( XclImpStream& rStrm )
{
    mxString.reset();
    if( maData.mnTextLen > 0 )
    {
        mxString.reset( new XclImpString( rStrm.ReadRawByteString( maData.mnTextLen ) ) );
        // skip padding byte for word boundaries
        if( rStrm.GetRecPos() & 1 ) rStrm.Ignore( 1 );
    }
}

void XclImpObjTextData::ReadFormats( XclImpStream& rStrm )
{
    if( mxString.is() )
        mxString->ReadObjFormats( rStrm, maData.mnFormatSize );
    else
        rStrm.Ignore( maData.mnFormatSize );
}

// ----------------------------------------------------------------------------

XclImpTextObj::XclImpTextObj( const XclImpRoot& rRoot ) :
    XclImpRectObj( rRoot )
{
}

void XclImpTextObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    maTextData.maData.ReadObj3( rStrm );
    ReadMacro3( rStrm, nMacroSize );
    maTextData.ReadByteString( rStrm );
    maTextData.ReadFormats( rStrm );
}

void XclImpTextObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    maTextData.maData.ReadObj3( rStrm );
    ReadMacro4( rStrm, nMacroSize );
    maTextData.ReadByteString( rStrm );
    maTextData.ReadFormats( rStrm );
}

void XclImpTextObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    ReadFrameData( rStrm );
    maTextData.maData.ReadObj5( rStrm );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
    maTextData.ReadByteString( rStrm );
    rStrm.Ignore( maTextData.maData.mnLinkSize );   // ignore text link formula
    maTextData.ReadFormats( rStrm );
}

SdrObject* XclImpTextObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    TSdrObjectPtr< SdrObjCustomShape > xSdrObj( new SdrObjCustomShape );
    xSdrObj->NbcSetSnapRect( rAnchorRect );
    OUString aRectType = CREATE_OUSTRING( "rectangle" );
    xSdrObj->MergeDefaultAttributes( &aRectType );
    ConvertRectStyle( *xSdrObj );
    BOOL bAutoSize = ::get_flag( maTextData.maData.mnFlags, EXC_OBJ_TEXT_AUTOSIZE );
    xSdrObj->SetMergedItem( SdrTextAutoGrowWidthItem( bAutoSize ) );
    xSdrObj->SetMergedItem( SdrTextAutoGrowHeightItem( bAutoSize ) );
    xSdrObj->SetMergedItem( SdrTextWordWrapItem( TRUE ) );
    rProgress.Progress();
    return xSdrObj.release();
}

void XclImpTextObj::DoProcessSdrObj( SdrObject& rSdrObj ) const
{
    // set text data
    if( SdrTextObj* pTextObj = dynamic_cast< SdrTextObj* >( &rSdrObj ) )
    {
        if( maTextData.mxString.is() )
        {
            if( maTextData.mxString->IsRich() )
            {
                // rich text
                ::std::auto_ptr< EditTextObject > xEditObj(
                    XclImpStringHelper::CreateTextObject( GetRoot(), *maTextData.mxString ) );
                OutlinerParaObject* pOutlineObj = new OutlinerParaObject( *xEditObj );
                pOutlineObj->SetOutlinerMode( OUTLINERMODE_TEXTOBJECT );
                // text object takes ownership of the outliner object
                pTextObj->NbcSetOutlinerParaObject( pOutlineObj );
            }
            else
            {
                // plain text
                pTextObj->NbcSetText( maTextData.mxString->GetText() );
            }

            /*  #i96858# Do not apply any formatting if there is no text.
                SdrObjCustomShape::SetVerticalWriting (initiated from
                SetMergedItem) calls SdrTextObj::ForceOutlinerParaObject which
                ensures that we can erroneously write a ClientTextbox record
                (with no content) while exporting to XLS, which can cause a
                corrupted exported document. */

            // horizontal text alignment
            SvxAdjust eHorAlign = SVX_ADJUST_LEFT;
            switch( maTextData.maData.GetHorAlign() )
            {
                case EXC_OBJ_HOR_LEFT:      eHorAlign = SVX_ADJUST_LEFT;    break;
                case EXC_OBJ_HOR_CENTER:    eHorAlign = SVX_ADJUST_CENTER;  break;
                case EXC_OBJ_HOR_RIGHT:     eHorAlign = SVX_ADJUST_RIGHT;   break;
                case EXC_OBJ_HOR_JUSTIFY:   eHorAlign = SVX_ADJUST_BLOCK;   break;
            }
            rSdrObj.SetMergedItem( SvxAdjustItem( eHorAlign, EE_PARA_JUST ) );

            // vertical text alignment
            SdrTextVertAdjust eVerAlign = SDRTEXTVERTADJUST_TOP;
            switch( maTextData.maData.GetVerAlign() )
            {
                case EXC_OBJ_VER_TOP:       eVerAlign = SDRTEXTVERTADJUST_TOP;      break;
                case EXC_OBJ_VER_CENTER:    eVerAlign = SDRTEXTVERTADJUST_CENTER;   break;
                case EXC_OBJ_VER_BOTTOM:    eVerAlign = SDRTEXTVERTADJUST_BOTTOM;   break;
                case EXC_OBJ_VER_JUSTIFY:   eVerAlign = SDRTEXTVERTADJUST_BLOCK;    break;
            }
            rSdrObj.SetMergedItem( SdrTextVertAdjustItem( eVerAlign ) );

            // orientation (this is only a fake, drawing does not support real text orientation)
            namespace csst = ::com::sun::star::text;
            csst::WritingMode eWriteMode = csst::WritingMode_LR_TB;
            switch( maTextData.maData.mnOrient )
            {
                case EXC_OBJ_ORIENT_NONE:       eWriteMode = csst::WritingMode_LR_TB;   break;
                case EXC_OBJ_ORIENT_STACKED:    eWriteMode = csst::WritingMode_TB_RL;   break;
                case EXC_OBJ_ORIENT_90CCW:      eWriteMode = csst::WritingMode_TB_RL;   break;
                case EXC_OBJ_ORIENT_90CW:       eWriteMode = csst::WritingMode_TB_RL;   break;
            }
            rSdrObj.SetMergedItem( SvxWritingModeItem( eWriteMode, SDRATTR_TEXTDIRECTION ) );
        }
    }
    // base class processing
    XclImpRectObj::DoProcessSdrObj( rSdrObj );
}

// ----------------------------------------------------------------------------

XclImpChartObj::XclImpChartObj( const XclImpRoot& rRoot, bool bOwnTab ) :
    XclImpRectObj( rRoot ),
    mbOwnTab( bOwnTab )
{
    SetSimpleMacro( false );
    SetCustomDffObj( true );
}

void XclImpChartObj::ReadChartSubStream( XclImpStream& rStrm )
{
    if( mbOwnTab ? (rStrm.GetRecId() == EXC_ID5_BOF) : ((rStrm.GetNextRecId() == EXC_ID5_BOF) && rStrm.StartNextRecord()) )
    {
        sal_uInt16 nBofType;
        rStrm.Seek( 2 );
        rStrm >> nBofType;
        DBG_ASSERT( nBofType == EXC_BOF_CHART, "XclImpChartObj::ReadChartSubStream - no chart BOF record" );

        // read chart, even if BOF record contains wrong substream identifier
        mxChart.reset( new XclImpChart( GetRoot(), mbOwnTab ) );
        mxChart->ReadChartSubStream( rStrm );
        if( mbOwnTab )
            FinalizeTabChart();
    }
    else
    {
        DBG_ERRORFILE( "XclImpChartObj::ReadChartSubStream - missing chart substream" );
    }
}

void XclImpChartObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    // read OBJ record and the following chart substream
    ReadFrameData( rStrm );
    rStrm.Ignore( 18 );
    ReadMacro3( rStrm, nMacroSize );
#if 0
    ReadChartSubStream( rStrm );
#endif
    // set frame format from OBJ record, it is used if chart itself is transparent
    if( mxChart.is() )
        mxChart->UpdateObjFrame( maLineData, maFillData );
}

void XclImpChartObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    // read OBJ record and the following chart substream
    ReadFrameData( rStrm );
    rStrm.Ignore( 18 );
    ReadMacro4( rStrm, nMacroSize );
#if 0
    ReadChartSubStream( rStrm );
#endif
    // set frame format from OBJ record, it is used if chart itself is transparent
    if( mxChart.is() )
        mxChart->UpdateObjFrame( maLineData, maFillData );
}

void XclImpChartObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    // read OBJ record and the following chart substream
    ReadFrameData( rStrm );
    rStrm.Ignore( 18 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
    ReadChartSubStream( rStrm );
    // set frame format from OBJ record, it is used if chart itself is transparent
    if( mxChart.is() )
        mxChart->UpdateObjFrame( maLineData, maFillData );
}

void XclImpChartObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 /*nSubRecSize*/ )
{
    // read the following chart substream
    if( nSubRecId == EXC_ID_OBJEND )
    {
        // enable CONTINUE handling for the entire chart substream
        rStrm.ResetRecord( true );
        ReadChartSubStream( rStrm );
        /*  #90118# disable CONTINUE handling again to be able to read
            following CONTINUE records as MSODRAWING records. */
        rStrm.ResetRecord( false );
    }
}

sal_Size XclImpChartObj::DoGetProgressSize() const
{
    return mxChart.is() ? mxChart->GetProgressSize() : 1;
}

SdrObject* XclImpChartObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    SdrObjectPtr xSdrObj;
    SfxObjectShell* pDocShell = GetDocShell();
    if( SvtModuleOptions().IsChart() && pDocShell && mxChart.is() && !mxChart->IsPivotChart() )
    {
        // create embedded chart object
        OUString aEmbObjName;
        Reference< XEmbeddedObject > xEmbObj = pDocShell->GetEmbeddedObjectContainer().
                CreateEmbeddedObject( SvGlobalName( SO3_SCH_CLASSID ).GetByteSequence(), aEmbObjName );

        /*  Set the size to the embedded object, this prevents that font sizes
            of text objects are changed in the chart when the object is
            inserted into the draw page. */
        sal_Int64 nAspect = ::com::sun::star::embed::Aspects::MSOLE_CONTENT;
        MapUnit aUnit = VCLUnoHelper::UnoEmbed2VCLMapUnit( xEmbObj->getMapUnit( nAspect ) );
        Size aSize( Window::LogicToLogic( rAnchorRect.GetSize(), MapMode( MAP_100TH_MM ), MapMode( aUnit ) ) );
        ::com::sun::star::awt::Size aAwtSize( aSize.Width(), aSize.Height() );
        xEmbObj->setVisualAreaSize( nAspect, aAwtSize );

        // create the container OLE object
        xSdrObj.reset( new SdrOle2Obj( svt::EmbeddedObjectRef( xEmbObj, nAspect ), aEmbObjName, rAnchorRect ) );

        // convert Excel chart to OOo Chart
        if( svt::EmbeddedObjectRef::TryRunningState( xEmbObj ) )
        {
            Reference< XModel > xModel( xEmbObj->getComponent(), UNO_QUERY );
            mxChart->Convert( xModel, rProgress );

            Reference< XEmbedPersist > xPers( xEmbObj, UNO_QUERY );
            if( xPers.is() )
                xPers->storeOwn();
        }
    }

    return xSdrObj.release();
}

void XclImpChartObj::FinalizeTabChart()
{
    /*  #i44077# Calculate and store DFF anchor for sheet charts.
        Needed to get used area if this chart is inserted as OLE object. */
    DBG_ASSERT( mbOwnTab, "XclImpChartObj::FinalizeTabChart - not allowed for embedded chart objects" );

    // set uninitialized page to landscape
    if( !GetPageSettings().GetPageData().mbValid )
        GetPageSettings().SetPaperSize( EXC_PAPERSIZE_DEFAULT, false );

    // calculate size of the chart object
    const XclPageData& rPageData = GetPageSettings().GetPageData();
    Size aPaperSize( rPageData.GetScPaperSize() );

    long nWidth = XclTools::GetHmmFromTwips( aPaperSize.Width() );
    long nHeight = XclTools::GetHmmFromTwips( aPaperSize.Height() );

    // subtract page margins, give 1cm extra space
    nWidth -= (XclTools::GetHmmFromInch( rPageData.mfLeftMargin + rPageData.mfRightMargin ) + 2000);
    nHeight -= (XclTools::GetHmmFromInch( rPageData.mfTopMargin + rPageData.mfBottomMargin ) + 1000);

    // print column/row headers?
    if( rPageData.mbPrintHeadings )
    {
        nWidth -= 2000;
        nHeight -= 1000;
    }

    // create the object anchor
    XclObjAnchor aAnchor( GetScTab() );
    aAnchor.SetRect( GetDoc(), Rectangle( 1000, 500, nWidth, nHeight ), MAP_100TH_MM );
    SetAnchor( aAnchor );
}

// ----------------------------------------------------------------------------

XclImpNoteObj::XclImpNoteObj( const XclImpRoot& rRoot ) :
    XclImpTextObj( rRoot ),
    maScPos( ScAddress::INITIALIZE_INVALID ),
    mnNoteFlags( 0 )
{
    SetSimpleMacro( false );
    // caption object will be created manually
    SetInsertSdrObj( false );
}

void XclImpNoteObj::SetNoteData( const ScAddress& rScPos, sal_uInt16 nNoteFlags )
{
    maScPos = rScPos;
    mnNoteFlags = nNoteFlags;
}

void XclImpNoteObj::DoProcessSdrObj( SdrObject& rSdrObj ) const
{
    // create formatted text
    XclImpTextObj::DoProcessSdrObj( rSdrObj );
    OutlinerParaObject* pOutlinerObj = rSdrObj.GetOutlinerParaObject();
    if( maScPos.IsValid() && pOutlinerObj )
    {
        // create cell note with all data from drawing object
        ScNoteUtil::CreateNoteFromObjectData(
            GetDoc(), maScPos,
            rSdrObj.GetMergedItemSet().Clone(),             // new object on heap expected
            new OutlinerParaObject( *pOutlinerObj ),        // new object on heap expected
            rSdrObj.GetLogicRect(),
            ::get_flag( mnNoteFlags, EXC_NOTE_VISIBLE ),
            false );
    }
}

// ----------------------------------------------------------------------------

XclImpControlHelper::XclImpControlHelper( const XclImpRoot& rRoot, XclCtrlBindMode eBindMode ) :
    mrRoot( rRoot ),
    meBindMode( eBindMode )
{
}

XclImpControlHelper::~XclImpControlHelper()
{
}

SdrObject* XclImpControlHelper::CreateSdrObjectFromShape(
        const Reference< XShape >& rxShape, const Rectangle& rAnchorRect ) const
{
    mxShape = rxShape;
    SdrObjectPtr xSdrObj( SdrObject::getSdrObjectFromXShape( rxShape ) );
    if( xSdrObj.is() )
    {
        xSdrObj->NbcSetSnapRect( rAnchorRect );
        // #i30543# insert into control layer
        xSdrObj->NbcSetLayer( SC_LAYER_CONTROLS );
    }
    return xSdrObj.release();
}

void XclImpControlHelper::ProcessControl( const XclImpDrawObjBase& rDrawObj ) const
{
    Reference< XControlModel > xCtrlModel = XclControlHelper::GetControlModel( mxShape );
    if( !xCtrlModel.is() )
        return;

    ScfPropertySet aPropSet( xCtrlModel );

    // #118053# #i51348# set object name at control model
    aPropSet.SetStringProperty( CREATE_OUSTRING( "Name" ), rDrawObj.GetObjName() );

    // control visible and printable?
    aPropSet.SetBoolProperty( CREATE_OUSTRING( "EnableVisible" ), rDrawObj.IsVisible() );
    aPropSet.SetBoolProperty( CREATE_OUSTRING( "Printable" ), rDrawObj.IsPrintable() );

    // sheet links
    if( SfxObjectShell* pDocShell = mrRoot.GetDocShell() )
    {
        Reference< XMultiServiceFactory > xFactory( pDocShell->GetModel(), UNO_QUERY );
        if( xFactory.is() )
        {
            // cell link
            if( mxCellLink.is() ) try
            {
                Reference< XBindableValue > xBindable( xCtrlModel, UNO_QUERY_THROW );

                // create argument sequence for createInstanceWithArguments()
                CellAddress aApiAddress;
                ScUnoConversion::FillApiAddress( aApiAddress, *mxCellLink );

                NamedValue aValue;
                aValue.Name = CREATE_OUSTRING( SC_UNONAME_BOUNDCELL );
                aValue.Value <<= aApiAddress;

                Sequence< Any > aArgs( 1 );
                aArgs[ 0 ] <<= aValue;

                // create the CellValueBinding instance and set at the control model
                OUString aServiceName;
                switch( meBindMode )
                {
                    case EXC_CTRL_BINDCONTENT:  aServiceName = CREATE_OUSTRING( SC_SERVICENAME_VALBIND );       break;
                    case EXC_CTRL_BINDPOSITION: aServiceName = CREATE_OUSTRING( SC_SERVICENAME_LISTCELLBIND );  break;
                }
                Reference< XValueBinding > xBinding(
                    xFactory->createInstanceWithArguments( aServiceName, aArgs ), UNO_QUERY_THROW );
                xBindable->setValueBinding( xBinding );
            }
            catch( const Exception& )
            {
            }

            // source range
            if( mxSrcRange.is() ) try
            {
                Reference< XListEntrySink > xEntrySink( xCtrlModel, UNO_QUERY_THROW );

                // create argument sequence for createInstanceWithArguments()
                CellRangeAddress aApiRange;
                ScUnoConversion::FillApiRange( aApiRange, *mxSrcRange );

                NamedValue aValue;
                aValue.Name = CREATE_OUSTRING( SC_UNONAME_CELLRANGE );
                aValue.Value <<= aApiRange;

                Sequence< Any > aArgs( 1 );
                aArgs[ 0 ] <<= aValue;

                // create the EntrySource instance and set at the control model
                Reference< XListEntrySource > xEntrySource( xFactory->createInstanceWithArguments(
                    CREATE_OUSTRING( SC_SERVICENAME_LISTSOURCE ), aArgs ), UNO_QUERY_THROW );
                xEntrySink->setListEntrySource( xEntrySource );
            }
            catch( const Exception& )
            {
            }
        }
    }

    // virtual call for type specific processing
    DoProcessControl( aPropSet );
}

void XclImpControlHelper::ReadCellLinkFormula( XclImpStream& rStrm, bool bWithBoundSize )
{
    ScRangeList aScRanges;
    ReadRangeList( aScRanges, rStrm, bWithBoundSize );
    // Use first cell of first range
    if( const ScRange* pScRange = aScRanges.GetObject( 0 ) )
        mxCellLink.reset( new ScAddress( pScRange->aStart ) );
}

void XclImpControlHelper::ReadSourceRangeFormula( XclImpStream& rStrm, bool bWithBoundSize )
{
    ScRangeList aScRanges;
    ReadRangeList( aScRanges, rStrm, bWithBoundSize );
    // Use first range
    if( const ScRange* pScRange = aScRanges.GetObject( 0 ) )
        mxSrcRange.reset( new ScRange( *pScRange ) );
}

void XclImpControlHelper::DoProcessControl( ScfPropertySet& ) const
{
}

void XclImpControlHelper::ReadRangeList( ScRangeList& rScRanges, XclImpStream& rStrm )
{
    XclTokenArray aXclTokArr;
    aXclTokArr.ReadSize( rStrm );
    rStrm.Ignore( 4 );
    aXclTokArr.ReadArray( rStrm );
    mrRoot.GetFormulaCompiler().CreateRangeList( rScRanges, EXC_FMLATYPE_CONTROL, aXclTokArr, rStrm );
}

void XclImpControlHelper::ReadRangeList( ScRangeList& rScRanges, XclImpStream& rStrm, bool bWithBoundSize )
{
    if( bWithBoundSize )
    {
        sal_uInt16 nSize;
        rStrm >> nSize;
        if( nSize > 0 )
        {
            rStrm.PushPosition();
            ReadRangeList( rScRanges, rStrm );
            rStrm.PopPosition();
            rStrm.Ignore( nSize );
        }
    }
    else
    {
        ReadRangeList( rScRanges, rStrm );
    }
}

// ----------------------------------------------------------------------------

XclImpTbxObjBase::XclImpTbxObjBase( const XclImpRoot& rRoot ) :
    XclImpTextObj( rRoot ),
    XclImpControlHelper( rRoot, EXC_CTRL_BINDPOSITION )
{
    SetSimpleMacro( false );
    SetCustomDffObj( true );
}

namespace {

void lclExtractColor( sal_uInt8& rnColorIdx, const DffPropSet& rDffPropSet, sal_uInt32 nPropId )
{
    if( rDffPropSet.IsProperty( nPropId ) )
    {
        sal_uInt32 nColor = rDffPropSet.GetPropertyValue( nPropId );
        if( (nColor & 0xFF000000) == 0x08000000 )
            rnColorIdx = ::extract_value< sal_uInt8 >( nColor, 0, 8 );
    }
}

} // namespace

void XclImpTbxObjBase::SetDffProperties( const DffPropSet& rDffPropSet )
{
    maFillData.mnPattern = rDffPropSet.GetPropertyBool( DFF_Prop_fFilled ) ? EXC_PATT_SOLID : EXC_PATT_NONE;
    lclExtractColor( maFillData.mnBackColorIdx, rDffPropSet, DFF_Prop_fillBackColor );
    lclExtractColor( maFillData.mnPattColorIdx, rDffPropSet, DFF_Prop_fillColor );
    ::set_flag( maFillData.mnAuto, EXC_OBJ_LINE_AUTO, false );

    maLineData.mnStyle = rDffPropSet.GetPropertyBool( DFF_Prop_fLine ) ? EXC_OBJ_LINE_SOLID : EXC_OBJ_LINE_NONE;
    lclExtractColor( maLineData.mnColorIdx, rDffPropSet, DFF_Prop_lineColor );
    ::set_flag( maLineData.mnAuto, EXC_OBJ_FILL_AUTO, false );
}

bool XclImpTbxObjBase::FillMacroDescriptor( ScriptEventDescriptor& rDescriptor ) const
{
    return XclControlHelper::FillMacroDescriptor( rDescriptor, DoGetEventType(), GetMacroName() );
}

void XclImpTbxObjBase::ConvertFont( ScfPropertySet& rPropSet ) const
{
    if( maTextData.mxString.is() )
    {
        const XclFormatRunVec& rFormatRuns = maTextData.mxString->GetFormats();
        if( rFormatRuns.empty() )
            GetFontBuffer().WriteDefaultCtrlFontProperties( rPropSet );
        else
            GetFontBuffer().WriteFontProperties( rPropSet, EXC_FONTPROPSET_CONTROL, rFormatRuns.front().mnFontIdx );
    }
}

void XclImpTbxObjBase::ConvertLabel( ScfPropertySet& rPropSet ) const
{
    if( maTextData.mxString.is() )
    {
        String aLabel = maTextData.mxString->GetText();
        if( maTextData.maData.mnShortcut > 0 )
        {
            xub_StrLen nPos = aLabel.Search( static_cast< sal_Unicode >( maTextData.maData.mnShortcut ) );
            if( nPos != STRING_NOTFOUND )
                aLabel.Insert( '~', nPos );
        }
        rPropSet.SetStringProperty( CREATE_OUSTRING( "Label" ), aLabel );
    }
    ConvertFont( rPropSet );
}

SdrObject* XclImpTbxObjBase::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    SdrObjectPtr xSdrObj( GetObjectManager().GetDffManager().CreateSdrObject( *this, rAnchorRect ) );
    rProgress.Progress();
    return xSdrObj.release();
}

void XclImpTbxObjBase::DoProcessSdrObj( SdrObject& /*rSdrObj*/ ) const
{
    // do not call DoProcessSdrObj() from base class (to skip text processing)
    ProcessControl( *this );
}

// ----------------------------------------------------------------------------

XclImpButtonObj::XclImpButtonObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot )
{
}

void XclImpButtonObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // label and text formatting
    ConvertLabel( rPropSet );

    /*  Horizontal text alignment. For unknown reason, the property type is a
        simple sal_Int16 and not a com.sun.star.style.HorizontalAlignment. */
    sal_Int16 nHorAlign = 1;
    switch( maTextData.maData.GetHorAlign() )
    {
        case EXC_OBJ_HOR_LEFT:      nHorAlign = 0;  break;
        case EXC_OBJ_HOR_CENTER:    nHorAlign = 1;  break;
        case EXC_OBJ_HOR_RIGHT:     nHorAlign = 2;  break;
    }
    rPropSet.SetProperty( CREATE_OUSTRING( "Align" ), nHorAlign );

    // vertical text alignment
    namespace csss = ::com::sun::star::style;
    csss::VerticalAlignment eVerAlign = csss::VerticalAlignment_MIDDLE;
    switch( maTextData.maData.GetVerAlign() )
    {
        case EXC_OBJ_VER_TOP:       eVerAlign = csss::VerticalAlignment_TOP;    break;
        case EXC_OBJ_VER_CENTER:    eVerAlign = csss::VerticalAlignment_MIDDLE; break;
        case EXC_OBJ_VER_BOTTOM:    eVerAlign = csss::VerticalAlignment_BOTTOM; break;
    }
    rPropSet.SetProperty( CREATE_OUSTRING( "VerticalAlign" ), eVerAlign );

    // always wrap text automatically
    rPropSet.SetBoolProperty( CREATE_OUSTRING( "MultiLine" ), true );

    // default button
    bool bDefButton = ::get_flag( maTextData.maData.mnButtonFlags, EXC_OBJ_BUTTON_DEFAULT );
    rPropSet.SetBoolProperty( CREATE_OUSTRING( "DefaultButton" ), bDefButton );

    // button type (flags cannot be combined in OOo)
    namespace cssa = ::com::sun::star::awt;
    cssa::PushButtonType eButtonType = cssa::PushButtonType_STANDARD;
    if( ::get_flag( maTextData.maData.mnButtonFlags, EXC_OBJ_BUTTON_CLOSE ) )
        eButtonType = cssa::PushButtonType_OK;
    else if( ::get_flag( maTextData.maData.mnButtonFlags, EXC_OBJ_BUTTON_CANCEL ) )
        eButtonType = cssa::PushButtonType_CANCEL;
    else if( ::get_flag( maTextData.maData.mnButtonFlags, EXC_OBJ_BUTTON_HELP ) )
        eButtonType = cssa::PushButtonType_HELP;
    // property type is short, not enum
    rPropSet.SetProperty( CREATE_OUSTRING( "PushButtonType" ), sal_Int16( eButtonType ) );
}

OUString XclImpButtonObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.CommandButton" );
}

XclTbxEventType XclImpButtonObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_ACTION;
}

// ----------------------------------------------------------------------------

XclImpCheckBoxObj::XclImpCheckBoxObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot ),
    mnState( EXC_OBJ_CHECKBOX_UNCHECKED ),
    mnCheckBoxFlags( 0 )
{
}

void XclImpCheckBoxObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    rStrm.Ignore( 10 );
    rStrm >> maTextData.maData.mnFlags;
    rStrm.Ignore( 20 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    ReadCellLinkFormula( rStrm, true );
    rStrm >> maTextData.maData.mnTextLen;
    maTextData.ReadByteString( rStrm );
    rStrm >> mnState >> maTextData.maData.mnShortcut >> maTextData.maData.mnShortcutEA >> mnCheckBoxFlags;
}

void XclImpCheckBoxObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJCBLS:
            // do not read EXC_ID_OBJCBLSDATA, not written by OOo Excel export
            rStrm >> mnState;
            rStrm.Ignore( 4 );
            rStrm >> maTextData.maData.mnShortcut >> maTextData.maData.mnShortcutEA >> mnCheckBoxFlags;
        break;
        case EXC_ID_OBJCBLSFMLA:
            ReadCellLinkFormula( rStrm, false );
        break;
        default:
            XclImpTbxObjBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

void XclImpCheckBoxObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // label and text formatting
    ConvertLabel( rPropSet );

    // state
    bool bSupportsTristate = GetObjType() == EXC_OBJTYPE_CHECKBOX;
    sal_Int16 nApiState = 0;
    switch( mnState )
    {
        case EXC_OBJ_CHECKBOX_UNCHECKED:    nApiState = 0;                          break;
        case EXC_OBJ_CHECKBOX_CHECKED:      nApiState = 1;                          break;
        case EXC_OBJ_CHECKBOX_TRISTATE:     nApiState = bSupportsTristate ? 2 : 1;  break;
    }
    if( bSupportsTristate )
        rPropSet.SetBoolProperty( CREATE_OUSTRING( "TriState" ), nApiState == 2 );
    rPropSet.SetProperty( CREATE_OUSTRING( "DefaultState" ), nApiState );

    // box style
    namespace AwtVisualEffect = ::com::sun::star::awt::VisualEffect;
    sal_Int16 nEffect = ::get_flagvalue( mnCheckBoxFlags, EXC_OBJ_CHECKBOX_FLAT, AwtVisualEffect::FLAT, AwtVisualEffect::LOOK3D );
    rPropSet.SetProperty( CREATE_OUSTRING( "VisualEffect" ), nEffect );

    // do not wrap text automatically
    rPropSet.SetBoolProperty( CREATE_OUSTRING( "MultiLine" ), false );

    // #i40279# always centered vertically
    namespace csss = ::com::sun::star::style;
    rPropSet.SetProperty( CREATE_OUSTRING( "VerticalAlign" ), csss::VerticalAlignment_MIDDLE );

    // background color
    if( maFillData.IsFilled() )
    {
        sal_Int32 nColor = static_cast< sal_Int32 >( GetSolidFillColor( maFillData ).GetColor() );
        rPropSet.SetProperty( CREATE_OUSTRING( "BackgroundColor" ), nColor );
    }
}

OUString XclImpCheckBoxObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.CheckBox" );
}

XclTbxEventType XclImpCheckBoxObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_ACTION;
}

// ----------------------------------------------------------------------------

XclImpOptionButtonObj::XclImpOptionButtonObj( const XclImpRoot& rRoot ) :
    XclImpCheckBoxObj( rRoot ),
    mnNextInGroup( 0 ),
    mnFirstInGroup( 1 )
{
}

void XclImpOptionButtonObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    rStrm.Ignore( 10 );
    rStrm >> maTextData.maData.mnFlags;
    rStrm.Ignore( 32 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    ReadCellLinkFormula( rStrm, true );
    rStrm >> maTextData.maData.mnTextLen;
    maTextData.ReadByteString( rStrm );
    rStrm >> mnState >> maTextData.maData.mnShortcut >> maTextData.maData.mnShortcutEA;
    rStrm >> mnCheckBoxFlags >> mnNextInGroup >> mnFirstInGroup;
}

void XclImpOptionButtonObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJRBODATA:
            rStrm >> mnNextInGroup >> mnFirstInGroup;
        break;
        default:
            XclImpCheckBoxObj::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

void XclImpOptionButtonObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    XclImpCheckBoxObj::DoProcessControl( rPropSet );
    // TODO: grouping
}

OUString XclImpOptionButtonObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.RadioButton" );
}

XclTbxEventType XclImpOptionButtonObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_ACTION;
}

// ----------------------------------------------------------------------------

XclImpLabelObj::XclImpLabelObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot )
{
}

void XclImpLabelObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // label and text formatting
    ConvertLabel( rPropSet );

    // text alignment (always top/left aligned)
    rPropSet.SetProperty( CREATE_OUSTRING( "Align" ), sal_Int16( 0 ) );
    namespace csss = ::com::sun::star::style;
    rPropSet.SetProperty( CREATE_OUSTRING( "VerticalAlign" ), csss::VerticalAlignment_TOP );

    // always wrap text automatically
    rPropSet.SetBoolProperty( CREATE_OUSTRING( "MultiLine" ), true );
}

OUString XclImpLabelObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.FixedText" );
}

XclTbxEventType XclImpLabelObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_MOUSE;
}

// ----------------------------------------------------------------------------

XclImpGroupBoxObj::XclImpGroupBoxObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot ),
    mnGroupBoxFlags( 0 )
{
}

void XclImpGroupBoxObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    rStrm.Ignore( 10 );
    rStrm >> maTextData.maData.mnFlags;
    rStrm.Ignore( 26 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    rStrm >> maTextData.maData.mnTextLen;
    maTextData.ReadByteString( rStrm );
    rStrm >> maTextData.maData.mnShortcut >> maTextData.maData.mnShortcutEA >> mnGroupBoxFlags;
}

void XclImpGroupBoxObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJGBODATA:
            rStrm >> maTextData.maData.mnShortcut >> maTextData.maData.mnShortcutEA >> mnGroupBoxFlags;
        break;
        default:
            XclImpTbxObjBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

void XclImpGroupBoxObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // label and text formatting
    ConvertLabel( rPropSet );
}

OUString XclImpGroupBoxObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.GroupBox" );
}

XclTbxEventType XclImpGroupBoxObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_MOUSE;
}

// ----------------------------------------------------------------------------

XclImpDialogObj::XclImpDialogObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot )
{
}

void XclImpDialogObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // label and text formatting
    ConvertLabel( rPropSet );
}

OUString XclImpDialogObj::DoGetServiceName() const
{
    // dialog frame faked by a groupbox
    return CREATE_OUSTRING( "com.sun.star.form.component.GroupBox" );
}

XclTbxEventType XclImpDialogObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_MOUSE;
}

// ----------------------------------------------------------------------------

XclImpEditObj::XclImpEditObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot ),
    mnContentType( EXC_OBJ_EDIT_TEXT ),
    mnMultiLine( 0 ),
    mnScrollBar( 0 ),
    mnListBoxObjId( 0 )
{
}

bool XclImpEditObj::IsNumeric() const
{
    return (mnContentType == EXC_OBJ_EDIT_INTEGER) || (mnContentType == EXC_OBJ_EDIT_DOUBLE);
}

void XclImpEditObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    rStrm.Ignore( 10 );
    rStrm >> maTextData.maData.mnFlags;
    rStrm.Ignore( 14 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    rStrm >> maTextData.maData.mnTextLen;
    maTextData.ReadByteString( rStrm );
    rStrm >> mnContentType >> mnMultiLine >> mnScrollBar >> mnListBoxObjId;
}

void XclImpEditObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJEDODATA:
            rStrm >> mnContentType >> mnMultiLine >> mnScrollBar >> mnListBoxObjId;
        break;
        default:
            XclImpTbxObjBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

void XclImpEditObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    if( maTextData.mxString.is() )
    {
        OUString aText = maTextData.mxString->GetText();
        if( IsNumeric() )
        {
            // TODO: OUString::toDouble() does not handle local decimal separator
            rPropSet.SetProperty( CREATE_OUSTRING( "DefaultValue" ), aText.toDouble() );
            rPropSet.SetBoolProperty( CREATE_OUSTRING( "Spin" ), mnScrollBar != 0 );
        }
        else
        {
            rPropSet.SetProperty( CREATE_OUSTRING( "DefaultText" ), aText );
            rPropSet.SetBoolProperty( CREATE_OUSTRING( "MultiLine" ), mnMultiLine != 0 );
            rPropSet.SetBoolProperty( CREATE_OUSTRING( "VScroll" ), mnScrollBar != 0 );
        }
    }
    ConvertFont( rPropSet );
}

OUString XclImpEditObj::DoGetServiceName() const
{
    return IsNumeric() ?
        CREATE_OUSTRING( "com.sun.star.form.component.NumericField" ) :
        CREATE_OUSTRING( "com.sun.star.form.component.TextField" );
}

XclTbxEventType XclImpEditObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_TEXT;
}

// ----------------------------------------------------------------------------

XclImpTbxObjScrollableBase::XclImpTbxObjScrollableBase( const XclImpRoot& rRoot ) :
    XclImpTbxObjBase( rRoot ),
    mnValue( 0 ),
    mnMin( 0 ),
    mnMax( 100 ),
    mnStep( 1 ),
    mnPageStep( 10 ),
    mnOrient( 0 ),
    mnThumbWidth( 1 ),
    mnScrollFlags( 0 )
{
}

void XclImpTbxObjScrollableBase::ReadSbs( XclImpStream& rStrm )
{
    rStrm.Ignore( 4 );
    rStrm >> mnValue >> mnMin >> mnMax >> mnStep >> mnPageStep >> mnOrient >> mnThumbWidth >> mnScrollFlags;
}

void XclImpTbxObjScrollableBase::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJSBS:
            ReadSbs( rStrm );
        break;
        case EXC_ID_OBJSBSFMLA:
            ReadCellLinkFormula( rStrm, false );
        break;
        default:
            XclImpTbxObjBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

// ----------------------------------------------------------------------------

XclImpSpinButtonObj::XclImpSpinButtonObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjScrollableBase( rRoot )
{
}

void XclImpSpinButtonObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    ReadSbs( rStrm );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    ReadCellLinkFormula( rStrm, true );
}

void XclImpSpinButtonObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // Calc's "Border" property is not the 3D/flat style effect in Excel (#i34712#)
    rPropSet.SetProperty( CREATE_OUSTRING( "Border" ), ::com::sun::star::awt::VisualEffect::NONE );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "DefaultSpinValue" ), mnValue );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "SpinValueMin" ), mnMin );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "SpinValueMax" ), mnMax );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "SpinIncrement" ), mnStep );

    // Excel spin buttons always vertical
    rPropSet.SetProperty( CREATE_OUSTRING( "Orientation" ), ::com::sun::star::awt::ScrollBarOrientation::VERTICAL );
}

OUString XclImpSpinButtonObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.SpinButton" );
}

XclTbxEventType XclImpSpinButtonObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_VALUE;
}

// ----------------------------------------------------------------------------

XclImpScrollBarObj::XclImpScrollBarObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjScrollableBase( rRoot )
{
}

void XclImpScrollBarObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    ReadSbs( rStrm );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    ReadCellLinkFormula( rStrm, true );
}

void XclImpScrollBarObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // Calc's "Border" property is not the 3D/flat style effect in Excel (#i34712#)
    rPropSet.SetProperty( CREATE_OUSTRING( "Border" ), ::com::sun::star::awt::VisualEffect::NONE );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "DefaultScrollValue" ), mnValue );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "ScrollValueMin" ), mnMin );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "ScrollValueMax" ), mnMax );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "LineIncrement" ), mnStep );
    rPropSet.SetProperty< sal_Int32 >( CREATE_OUSTRING( "BlockIncrement" ), mnPageStep );
    rPropSet.SetProperty( CREATE_OUSTRING( "VisibleSize" ), ::std::min< sal_Int32 >( mnPageStep, 1 ) );

    namespace AwtScrollOrient = ::com::sun::star::awt::ScrollBarOrientation;
    sal_Int32 nApiOrient = ::get_flagvalue( mnOrient, EXC_OBJ_SCROLLBAR_HOR, AwtScrollOrient::HORIZONTAL, AwtScrollOrient::VERTICAL );
    rPropSet.SetProperty( CREATE_OUSTRING( "Orientation" ), nApiOrient );
}

OUString XclImpScrollBarObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.ScrollBar" );
}

XclTbxEventType XclImpScrollBarObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_VALUE;
}

// ----------------------------------------------------------------------------

XclImpTbxObjListBase::XclImpTbxObjListBase( const XclImpRoot& rRoot ) :
    XclImpTbxObjScrollableBase( rRoot ),
    mnEntryCount( 0 ),
    mnSelEntry( 0 ),
    mnListFlags( 0 ),
    mnEditObjId( 0 ),
    mbHasDefFontIdx( false )
{
}

void XclImpTbxObjListBase::ReadLbsData( XclImpStream& rStrm )
{
    ReadSourceRangeFormula( rStrm, true );
    rStrm >> mnEntryCount >> mnSelEntry >> mnListFlags >> mnEditObjId;
}

void XclImpTbxObjListBase::SetBoxFormatting( ScfPropertySet& rPropSet ) const
{
    // border style
    namespace AwtVisualEffect = ::com::sun::star::awt::VisualEffect;
    sal_Int16 nApiBorder = ::get_flagvalue( mnListFlags, EXC_OBJ_LISTBOX_FLAT, AwtVisualEffect::FLAT, AwtVisualEffect::LOOK3D );
    rPropSet.SetProperty( CREATE_OUSTRING( "Border" ), nApiBorder );

    // font formatting
    if( mbHasDefFontIdx )
        GetFontBuffer().WriteFontProperties( rPropSet, EXC_FONTPROPSET_CONTROL, maTextData.maData.mnDefFontIdx );
    else
        GetFontBuffer().WriteDefaultCtrlFontProperties( rPropSet );
}

// ----------------------------------------------------------------------------

XclImpListBoxObj::XclImpListBoxObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjListBase( rRoot )
{
}

void XclImpListBoxObj::ReadFullLbsData( XclImpStream& rStrm, sal_Size nRecLeft )
{
    sal_Size nRecEnd = rStrm.GetRecPos() + nRecLeft;
    ReadLbsData( rStrm );
    DBG_ASSERT( (rStrm.GetRecPos() == nRecEnd) || (rStrm.GetRecPos() + mnEntryCount == nRecEnd),
        "XclImpListBoxObj::ReadFullLbsData - invalid size of OBJLBSDATA record" );
    while( rStrm.IsValid() && (rStrm.GetRecPos() < nRecEnd) )
        maSelection.push_back( rStrm.ReaduInt8() );
}

void XclImpListBoxObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    ReadSbs( rStrm );
    rStrm.Ignore( 18 );
    rStrm >> maTextData.maData.mnDefFontIdx;
    rStrm.Ignore( 4 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    ReadCellLinkFormula( rStrm, true );
    ReadFullLbsData( rStrm, rStrm.GetRecLeft() );
    mbHasDefFontIdx = true;
}

void XclImpListBoxObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJLBSDATA:
            ReadFullLbsData( rStrm, nSubRecSize );
        break;
        default:
            XclImpTbxObjListBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

void XclImpListBoxObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // listbox formatting
    SetBoxFormatting( rPropSet );

    // selection type
    sal_uInt8 nSelType = ::extract_value< sal_uInt8 >( mnListFlags, 4, 2 );
    bool bMultiSel = nSelType != EXC_OBJ_LISTBOX_SINGLE;
    rPropSet.SetBoolProperty( CREATE_OUSTRING( "MultiSelection" ), bMultiSel );

    // selection (do not set, if listbox is linked to a cell)
    if( !HasCellLink() )
    {
        ScfInt16Vec aSelVec;

        // multi selection: API expects sequence of list entry indexes
        if( bMultiSel )
            for( ScfUInt8Vec::const_iterator aBeg = maSelection.begin(), aIt = aBeg, aEnd = maSelection.end(); aIt != aEnd; ++aIt )
                if( *aIt != 0 )
                    aSelVec.push_back( static_cast< sal_Int16 >( aIt - aBeg ) );
        // single selection: mnSelEntry is one-based, API expects zero-based
        else if( mnSelEntry > 0 )
            aSelVec.push_back( static_cast< sal_Int16 >( mnSelEntry - 1 ) );

        if( !aSelVec.empty() )
        {
            Sequence< sal_Int16 > aSelSeq( &aSelVec.front(), static_cast< sal_Int32 >( aSelVec.size() ) );
            rPropSet.SetProperty( CREATE_OUSTRING( "DefaultSelection" ), aSelSeq );
        }
    }
}

OUString XclImpListBoxObj::DoGetServiceName() const
{
    return CREATE_OUSTRING( "com.sun.star.form.component.ListBox" );
}

XclTbxEventType XclImpListBoxObj::DoGetEventType() const
{
    return EXC_TBX_EVENT_CHANGE;
}

// ----------------------------------------------------------------------------

XclImpDropDownObj::XclImpDropDownObj( const XclImpRoot& rRoot ) :
    XclImpTbxObjListBase( rRoot ),
    mnLeft( 0 ),
    mnTop( 0 ),
    mnRight( 0 ),
    mnBottom( 0 ),
    mnDropDownFlags( 0 ),
    mnLineCount( 0 ),
    mnMinWidth( 0 )
{
}

sal_uInt16 XclImpDropDownObj::GetDropDownType() const
{
    return ::extract_value< sal_uInt8 >( mnDropDownFlags, 0, 2 );
}

void XclImpDropDownObj::ReadFullLbsData( XclImpStream& rStrm )
{
    ReadLbsData( rStrm );
    rStrm >> mnDropDownFlags >> mnLineCount >> mnMinWidth >> maTextData.maData.mnTextLen;
    maTextData.ReadByteString( rStrm );
    // dropdowns of auto-filters have 'simple' style, they don't have a text area
    if( GetDropDownType() == EXC_OBJ_DROPDOWN_SIMPLE )
        SetProcessSdrObj( false );
}

void XclImpDropDownObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 /*nMacroSize*/ )
{
    ReadFrameData( rStrm );
    ReadSbs( rStrm );
    rStrm.Ignore( 18 );
    rStrm >> maTextData.maData.mnDefFontIdx;
    rStrm.Ignore( 14 );
    rStrm >> mnLeft >> mnTop >> mnRight >> mnBottom;
    rStrm.Ignore( 4 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, rStrm.ReaduInt16() );   // fist macro size invalid and unused
    ReadCellLinkFormula( rStrm, true );
    ReadFullLbsData( rStrm );
    mbHasDefFontIdx = true;
}

void XclImpDropDownObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJLBSDATA:
            ReadFullLbsData( rStrm );
        break;
        default:
            XclImpTbxObjListBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

void XclImpDropDownObj::DoProcessControl( ScfPropertySet& rPropSet ) const
{
    // dropdown listbox formatting
    SetBoxFormatting( rPropSet );
    // enable dropdown button
    rPropSet.SetBoolProperty( CREATE_OUSTRING( "Dropdown" ), true );
    // dropdown line count
    rPropSet.SetProperty( CREATE_OUSTRING( "LineCount" ), mnLineCount );

    if( GetDropDownType() == EXC_OBJ_DROPDOWN_COMBOBOX )
    {
        // text of editable combobox
        if( maTextData.mxString.is() )
            rPropSet.SetStringProperty( CREATE_OUSTRING( "DefaultText" ), maTextData.mxString->GetText() );
    }
    else
    {
        // selection (do not set, if dropdown is linked to a cell)
        if( !HasCellLink() && (mnSelEntry > 0) )
        {
            Sequence< sal_Int16 > aSelSeq( 1 );
            aSelSeq[ 0 ] = mnSelEntry - 1;
            rPropSet.SetProperty( CREATE_OUSTRING( "DefaultSelection" ), aSelSeq );
        }
    }
}

OUString XclImpDropDownObj::DoGetServiceName() const
{
    return (GetDropDownType() == EXC_OBJ_DROPDOWN_COMBOBOX) ?
        CREATE_OUSTRING( "com.sun.star.form.component.ComboBox" ) :
        CREATE_OUSTRING( "com.sun.star.form.component.ListBox" );
}

XclTbxEventType XclImpDropDownObj::DoGetEventType() const
{
    return (GetDropDownType() == EXC_OBJ_DROPDOWN_COMBOBOX) ? EXC_TBX_EVENT_TEXT : EXC_TBX_EVENT_CHANGE;
}

// ----------------------------------------------------------------------------

XclImpPictureObj::XclImpPictureObj( const XclImpRoot& rRoot ) :
    XclImpRectObj( rRoot ),
    XclImpControlHelper( rRoot, EXC_CTRL_BINDCONTENT ),
    mnStorageId( 0 ),
    mnCtlsStrmPos( 0 ),
    mnCtlsStrmSize( 0 ),
    mbEmbedded( false ),
    mbLinked( false ),
    mbSymbol( false ),
    mbControl( false ),
    mbUseCtlsStrm( false )
{
    SetAreaObj( true );
    SetSimpleMacro( false );
    SetCustomDffObj( true );
}

String XclImpPictureObj::GetOleStorageName() const
{
    String aStrgName;
    if( (mbEmbedded || mbLinked) && !mbControl && (mnStorageId > 0) )
    {
        aStrgName = mbEmbedded ? EXC_STORAGE_OLE_EMBEDDED : EXC_STORAGE_OLE_LINKED;
        static const sal_Char spcHexChars[] = "0123456789ABCDEF";
        for( sal_uInt8 nIndex = 32; nIndex > 0; nIndex -= 4 )
            aStrgName.Append( sal_Unicode( spcHexChars[ ::extract_value< sal_uInt8 >( mnStorageId, nIndex - 4, 4 ) ] ) );
    }
    return aStrgName;
}

void XclImpPictureObj::DoReadObj3( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    sal_uInt16 nLinkSize;
    ReadFrameData( rStrm );
    rStrm.Ignore( 6 );
    rStrm >> nLinkSize;
    rStrm.Ignore( 2 );
    ReadFlags3( rStrm );
    ReadMacro3( rStrm, nMacroSize );
    ReadPictFmla( rStrm, nLinkSize );

    if( (rStrm.GetNextRecId() == EXC_ID3_IMGDATA) && rStrm.StartNextRecord() )
        maGraphic = XclImpObjectManager::ReadImgData( rStrm );
}

void XclImpPictureObj::DoReadObj4( XclImpStream& rStrm, sal_uInt16 nMacroSize )
{
    sal_uInt16 nLinkSize;
    ReadFrameData( rStrm );
    rStrm.Ignore( 6 );
    rStrm >> nLinkSize;
    rStrm.Ignore( 2 );
    ReadFlags3( rStrm );
    ReadMacro4( rStrm, nMacroSize );
    ReadPictFmla( rStrm, nLinkSize );

    if( (rStrm.GetNextRecId() == EXC_ID3_IMGDATA) && rStrm.StartNextRecord() )
        maGraphic = XclImpObjectManager::ReadImgData( rStrm );
}

void XclImpPictureObj::DoReadObj5( XclImpStream& rStrm, sal_uInt16 nNameLen, sal_uInt16 nMacroSize )
{
    sal_uInt16 nLinkSize;
    ReadFrameData( rStrm );
    rStrm.Ignore( 6 );
    rStrm >> nLinkSize;
    rStrm.Ignore( 2 );
    ReadFlags3( rStrm );
    rStrm.Ignore( 4 );
    ReadName5( rStrm, nNameLen );
    ReadMacro5( rStrm, nMacroSize );
    ReadPictFmla( rStrm, nLinkSize );

    if( (rStrm.GetNextRecId() == EXC_ID3_IMGDATA) && rStrm.StartNextRecord() )
    {
        // page background is stored as hidden picture with name "__BkgndObj"
        if( IsHidden() && (GetObjName() == CREATE_STRING( "__BkgndObj" )) )
            GetPageSettings().ReadImgData( rStrm );
        else
            maGraphic = XclImpObjectManager::ReadImgData( rStrm );
    }
}

void XclImpPictureObj::DoReadObj8SubRec( XclImpStream& rStrm, sal_uInt16 nSubRecId, sal_uInt16 nSubRecSize )
{
    switch( nSubRecId )
    {
        case EXC_ID_OBJFLAGS:
            ReadFlags8( rStrm );
        break;
        case EXC_ID_OBJPICTFMLA:
            ReadPictFmla( rStrm, rStrm.ReaduInt16() );
        break;
        default:
            XclImpDrawObjBase::DoReadObj8SubRec( rStrm, nSubRecId, nSubRecSize );
    }
}

SdrObject* XclImpPictureObj::DoCreateSdrObj( const Rectangle& rAnchorRect, ScfProgressBar& rProgress ) const
{
    // try to create an OLE object or form control
    SdrObjectPtr xSdrObj( GetObjectManager().GetDffManager().CreateSdrObject( *this, rAnchorRect ) );

    // no OLE - create a plain picture from IMGDATA record data
    if( !xSdrObj && (maGraphic.GetType() != GRAPHIC_NONE) )
    {
        xSdrObj.reset( new SdrGrafObj( maGraphic, rAnchorRect ) );
        ConvertRectStyle( *xSdrObj );
    }

    rProgress.Progress();
    return xSdrObj.release();
}

void XclImpPictureObj::DoProcessSdrObj( SdrObject& rSdrObj ) const
{
    if( IsOcxControl() )
    {
        // do not call XclImpRectObj::DoProcessSdrObj(), it would trace missing "printable" feature
        ProcessControl( *this );
    }
    else if( mbEmbedded || mbLinked )
    {
        // trace missing "printable" feature
        XclImpRectObj::DoProcessSdrObj( rSdrObj );

        SfxObjectShell* pDocShell = GetDocShell();
        SdrOle2Obj* pOleSdrObj = dynamic_cast< SdrOle2Obj* >( &rSdrObj );
        if( pOleSdrObj && pDocShell )
        {
            comphelper::EmbeddedObjectContainer& rEmbObjCont = pDocShell->GetEmbeddedObjectContainer();
            Reference< XEmbeddedObject > xEmbObj = pOleSdrObj->GetObjRef();
            OUString aOldName( pOleSdrObj->GetPersistName() );

            /*  The object persistence should be already in the storage, but
                the object still might not be inserted into the container. */
            if( rEmbObjCont.HasEmbeddedObject( aOldName ) )
            {
                if( !rEmbObjCont.HasEmbeddedObject( xEmbObj ) )
                    // filter code is allowed to call the following method
                    rEmbObjCont.AddEmbeddedObject( xEmbObj, aOldName );
            }
            else
            {
                /*  If the object is still not in container it must be inserted
                    there, the name must be generated in this case. */
                OUString aNewName;
                rEmbObjCont.InsertEmbeddedObject( xEmbObj, aNewName );
                if( aOldName != aNewName )
                    // #95381# SetPersistName, not SetName
                    pOleSdrObj->SetPersistName( aNewName );
            }
        }
    }
}

void XclImpPictureObj::ReadFlags3( XclImpStream& rStrm )
{
    sal_uInt16 nFlags;
    rStrm >> nFlags;
    mbSymbol = ::get_flag( nFlags, EXC_OBJ_PIC_SYMBOL );
}

void XclImpPictureObj::ReadFlags8( XclImpStream& rStrm )
{
    sal_uInt16 nFlags;
    rStrm >> nFlags;
    mbSymbol      = ::get_flag( nFlags, EXC_OBJ_PIC_SYMBOL );
    mbControl     = ::get_flag( nFlags, EXC_OBJ_PIC_CONTROL );
    mbUseCtlsStrm = ::get_flag( nFlags, EXC_OBJ_PIC_CTLSSTREAM );
    DBG_ASSERT( mbControl || !mbUseCtlsStrm, "XclImpPictureObj::ReadFlags8 - CTLS stream for controls only" );
    SetProcessSdrObj( mbControl || !mbUseCtlsStrm );
}

void XclImpPictureObj::ReadPictFmla( XclImpStream& rStrm, sal_uInt16 nLinkSize )
{
    sal_Size nLinkEnd = rStrm.GetRecPos() + nLinkSize;
    if( nLinkSize >= 6 )
    {
        sal_uInt16 nFmlaSize;
        rStrm >> nFmlaSize;
        DBG_ASSERT( nFmlaSize > 0, "XclImpPictureObj::ReadPictFmla - missing link formula" );
        // BIFF3/BIFF4 do not support storages, nothing to do here
        if( (nFmlaSize > 0) && (GetBiff() >= EXC_BIFF5) )
        {
            rStrm.Ignore( 4 );
            sal_uInt8 nToken;
            rStrm >> nToken;

            // different processing for linked vs. embedded OLE objects
            if( nToken == XclTokenArrayHelper::GetTokenId( EXC_TOKID_NAMEX, EXC_TOKCLASS_REF ) )
            {
                mbLinked = true;
                switch( GetBiff() )
                {
                    case EXC_BIFF5:
                    {
                        sal_Int16 nRefIdx;
                        sal_uInt16 nNameIdx;
                        rStrm >> nRefIdx;
                        rStrm.Ignore( 8 );
                        rStrm >> nNameIdx;
                        rStrm.Ignore( 12 );
                        const ExtName* pExtName = GetOldRoot().pExtNameBuff->GetNameByIndex( nRefIdx, nNameIdx );
                        if( pExtName && pExtName->IsOLE() )
                            mnStorageId = pExtName->nStorageId;
                    }
                    break;
                    case EXC_BIFF8:
                    {
                        sal_uInt16 nXti, nExtName;
                        rStrm >> nXti >> nExtName;
                        const XclImpExtName* pExtName = GetLinkManager().GetExternName( nXti, nExtName );
                        if( pExtName && (pExtName->GetType() == xlExtOLE) )
                            mnStorageId = pExtName->GetStorageId();
                    }
                    break;
                    default:
                        DBG_ERROR_BIFF();
                }
            }
            else if( nToken == XclTokenArrayHelper::GetTokenId( EXC_TOKID_TBL, EXC_TOKCLASS_NONE ) )
            {
                mbEmbedded = true;
                DBG_ASSERT( nFmlaSize == 5, "XclImpPictureObj::ReadPictFmla - unexpected formula size" );
                rStrm.Ignore( nFmlaSize - 1 );      // token ID already read
                if( nFmlaSize & 1 )
                    rStrm.Ignore( 1 );              // padding byte

                // a class name may follow inside the picture link
                if( rStrm.GetRecPos() + 2 <= nLinkEnd )
                {
                    sal_uInt16 nLen;
                    rStrm >> nLen;
                    if( nLen > 0 )
                        maClassName = (GetBiff() == EXC_BIFF8) ? rStrm.ReadUniString( nLen ) : rStrm.ReadRawByteString( nLen );
                }
            }
            // else: ignore other formulas, e.g. pictures linked to cell ranges
        }
    }

    // seek behind picture link data
    rStrm.Seek( nLinkEnd );

    // read additional data for embedded OLE objects following the picture link
    if( IsOcxControl() )
    {
        // #i26521# form controls to be ignored
        if( maClassName.EqualsAscii( "Forms.HTML:Hidden.1" ) )
        {
            SetProcessSdrObj( false );
            return;
        }

        if( rStrm.GetRecLeft() <= 8 ) return;

        // position and size of control data in 'Ctls' stream
        mnCtlsStrmPos = static_cast< sal_Size >( rStrm.ReaduInt32() );
        mnCtlsStrmSize = static_cast< sal_Size >( rStrm.ReaduInt32() );

        if( rStrm.GetRecLeft() <= 8 ) return;

        // additional string (16-bit characters), e.g. for progress bar control
        sal_uInt32 nAddStrSize;
        rStrm >> nAddStrSize;
        DBG_ASSERT( rStrm.GetRecLeft() >= nAddStrSize + 4, "XclImpPictureObj::ReadPictFmla - missing data" );
        if( rStrm.GetRecLeft() >= nAddStrSize + 4 )
        {
            rStrm.Ignore( nAddStrSize );
            // cell link and source range
            ReadCellLinkFormula( rStrm, true );
            ReadSourceRangeFormula( rStrm, true );
        }
    }
    else if( mbEmbedded && (rStrm.GetRecLeft() >= 4) )
    {
        rStrm >> mnStorageId;
    }
}

// DFF stream conversion ======================================================

//UNUSED2009-05 void XclImpSolverContainer::ReadSolverContainer( SvStream& rDffStrm )
//UNUSED2009-05 {
//UNUSED2009-05     rDffStrm >> *this;
//UNUSED2009-05 }

void XclImpSolverContainer::InsertSdrObjectInfo( SdrObject& rSdrObj, sal_uInt32 nDffShapeId, sal_uInt32 nDffFlags )
{
    if( nDffShapeId > 0 )
    {
        maSdrInfoMap[ nDffShapeId ].Set( &rSdrObj, nDffFlags );
        maSdrObjMap[ &rSdrObj ] = nDffShapeId;
    }
}

void XclImpSolverContainer::RemoveSdrObjectInfo( SdrObject& rSdrObj )
{
    // remove info of passed object from the maps
    XclImpSdrObjMap::iterator aIt = maSdrObjMap.find( &rSdrObj );
    if( aIt != maSdrObjMap.end() )
    {
        maSdrInfoMap.erase( aIt->second );
        maSdrObjMap.erase( aIt );
    }

    // remove info of all child objects of a group object
    if( SdrObjGroup* pGroupObj = dynamic_cast< SdrObjGroup* >( &rSdrObj ) )
    {
        if( SdrObjList* pSubList = pGroupObj->GetSubList() )
        {
            // iterate flat over the list because this function already works recursively
            SdrObjListIter aObjIt( *pSubList, IM_FLAT );
            for( SdrObject* pChildObj = aObjIt.Next(); pChildObj; pChildObj = aObjIt.Next() )
                RemoveSdrObjectInfo( *pChildObj );
        }
    }
}

void XclImpSolverContainer::UpdateConnectorRules()
{
    for( SvxMSDffConnectorRule* pRule = GetFirstRule(); pRule; pRule = GetNextRule() )
    {
        UpdateConnection( pRule->nShapeA, pRule->pAObj, &pRule->nSpFlagsA );
        UpdateConnection( pRule->nShapeB, pRule->pBObj, &pRule->nSpFlagsB );
        UpdateConnection( pRule->nShapeC, pRule->pCObj );
    }
}

void XclImpSolverContainer::RemoveConnectorRules()
{
    // base class from SVX uses plain untyped tools/List
    for( SvxMSDffConnectorRule* pRule = GetFirstRule(); pRule; pRule = GetNextRule() )
        delete pRule;
    aCList.Clear();

    maSdrInfoMap.clear();
    maSdrObjMap.clear();
}

SvxMSDffConnectorRule* XclImpSolverContainer::GetFirstRule()
{
    return static_cast< SvxMSDffConnectorRule* >( aCList.First() );
}

SvxMSDffConnectorRule* XclImpSolverContainer::GetNextRule()
{
    return static_cast< SvxMSDffConnectorRule* >( aCList.Next() );
}

void XclImpSolverContainer::UpdateConnection( sal_uInt32 nDffShapeId, SdrObject*& rpSdrObj, sal_uInt32* pnDffFlags )
{
    XclImpSdrInfoMap::const_iterator aIt = maSdrInfoMap.find( nDffShapeId );
    if( aIt != maSdrInfoMap.end() )
    {
        rpSdrObj = aIt->second.mpSdrObj;
        if( pnDffFlags )
            *pnDffFlags = aIt->second.mnDffFlags;
    }
}

// ----------------------------------------------------------------------------

XclImpSimpleDffManager::XclImpSimpleDffManager( const XclImpRoot& rRoot, SvStream& rDffStrm ) :
    SvxMSDffManager( rDffStrm, rRoot.GetBasePath(), 0, 0, rRoot.GetDoc().GetDrawLayer(), 1440, COL_DEFAULT, 24, 0, &rRoot.GetTracer().GetBaseTracer() ),
    XclImpRoot( rRoot )
{
    SetSvxMSDffSettings( SVXMSDFF_SETTINGS_CROP_BITMAPS | SVXMSDFF_SETTINGS_IMPORT_EXCEL );
}

XclImpSimpleDffManager::~XclImpSimpleDffManager()
{
}

FASTBOOL XclImpSimpleDffManager::GetColorFromPalette( USHORT nIndex, Color& rColor ) const
{
    ColorData nColor = GetPalette().GetColorData( static_cast< sal_uInt16 >( nIndex ) );

    if( nColor == COL_AUTO )
        return FALSE;

    rColor.SetColor( nColor );
    return TRUE;
}

// ----------------------------------------------------------------------------

XclImpDffManager::XclImpDffManager(
        const XclImpRoot& rRoot, XclImpObjectManager& rObjManager, SvStream& rDffStrm ) :
    XclImpSimpleDffManager( rRoot, rDffStrm ),
    SvxMSConvertOCXControls( rRoot.GetDocShell(), 0 ),
    mrObjManager( rObjManager ),
    mnOleImpFlags( 0 ),
    mnLastCtrlIndex( -1 ),
    mnCurrFormScTab( -1 )
{
    if( SvtFilterOptions* pFilterOpt = SvtFilterOptions::Get() )
    {
        if( pFilterOpt->IsMathType2Math() )
            mnOleImpFlags |= OLE_MATHTYPE_2_STARMATH;
        if( pFilterOpt->IsWinWord2Writer() )
            mnOleImpFlags |= OLE_WINWORD_2_STARWRITER;
        if( pFilterOpt->IsPowerPoint2Impress() )
            mnOleImpFlags |= OLE_POWERPOINT_2_STARIMPRESS;
    }

    // try to open the 'Ctls' storage stream containing OCX control properties
    mxCtlsStrm = OpenStream( EXC_STREAM_CTLS );

    // default text margin (convert EMU to drawing layer units)
    mnDefTextMargin = EXC_OBJ_TEXT_MARGIN;
    ScaleEmu( mnDefTextMargin );
}

XclImpDffManager::~XclImpDffManager()
{
}

void XclImpDffManager::StartProgressBar( sal_Size nProgressSize )
{
    mxProgress.reset( new ScfProgressBar( GetDocShell(), STR_PROGRESS_CALCULATING ) );
    mxProgress->AddSegment( nProgressSize );
    mxProgress->Activate();
}

void XclImpDffManager::ProcessObject( SdrObjList* pObjList, const XclImpDrawObjBase& rDrawObj )
{
    Rectangle aAnchorRect = rDrawObj.GetAnchorRect();
    if( rDrawObj.IsProcessSdrObj() && rDrawObj.IsValidSize( aAnchorRect ) )
    {
        // CreateSdrObject() recursively creates embedded child objects
        SdrObjectPtr xSdrObj( rDrawObj.CreateSdrObject( aAnchorRect, *mxProgress, false ) );
        if( xSdrObj.is() )
            rDrawObj.ProcessSdrObject( *xSdrObj );
        // call InsertSdrObject() also, if SdrObject is missing
        InsertSdrObject( pObjList, rDrawObj, xSdrObj.release() );
        UpdateUsedArea( rDrawObj );
    }
}

void XclImpDffManager::ProcessDrawingGroup( SvStream& rDffStrm )
{
    rDffStrm.Seek( STREAM_SEEK_TO_BEGIN );
    DffRecordHeader aHeader;
    rDffStrm >> aHeader;
    if( aHeader.nRecType == DFF_msofbtDggContainer )
        ProcessDggContainer( rDffStrm, aHeader );
    else
    {
        DBG_ERRORFILE( "XclImpDffManager::ProcessDrawingGroup - unexpected record" );
    }
}

void XclImpDffManager::ProcessDrawing( SvStream& rDffStrm, sal_Size nStrmPos )
{
    rDffStrm.Seek( nStrmPos );
    DffRecordHeader aHeader;
    rDffStrm >> aHeader;
    if( aHeader.nRecType == DFF_msofbtDgContainer )
        ProcessDgContainer( rDffStrm, aHeader );
    else
    {
        DBG_ERRORFILE( "XclImpDffManager::ProcessDrawing - unexpected record" );
    }
}

SdrObject* XclImpDffManager::CreateSdrObject( const XclImpTbxObjBase& rTbxObj, const Rectangle& rAnchorRect )
{
    SdrObjectPtr xSdrObj;

    OUString aServiceName = rTbxObj.GetServiceName();
    if( aServiceName.getLength() > 0 ) try
    {
        // create the form control from scratch
        Reference< XFormComponent > xFormComp( ScfApiHelper::CreateInstance( GetDocShell(), aServiceName ), UNO_QUERY_THROW );
        // set current controls form, needed in virtual function InsertControl()
        SetCurrentForm( rTbxObj.GetScTab() );
        // try to insert the control into the form
        ::com::sun::star::awt::Size aDummySize;
        Reference< XShape > xShape;
        if( mxCurrForm.is() && InsertControl( xFormComp, aDummySize, &xShape, TRUE ) )
        {
            xSdrObj.reset( rTbxObj.CreateSdrObjectFromShape( xShape, rAnchorRect ) );
            // try to attach a macro to the control
            ScriptEventDescriptor aDescriptor;
            if( (mnLastCtrlIndex >= 0) && rTbxObj.FillMacroDescriptor( aDescriptor ) )
            {
                Reference< XEventAttacherManager > xEventMgr( mxCurrForm, UNO_QUERY_THROW );
                xEventMgr->registerScriptEvent( mnLastCtrlIndex, aDescriptor );
            }
        }
    }
    catch( Exception& )
    {
    }

    return xSdrObj.release();
}

SdrObject* XclImpDffManager::CreateSdrObject( const XclImpPictureObj& rPicObj, const Rectangle& rAnchorRect )
{
    SdrObjectPtr xSdrObj;

    if( rPicObj.IsOcxControl() )
    {
        if( mxCtlsStrm.Is() ) try
        {
            /*  set current controls form, needed in virtual function InsertControl()
                called from ReadOCXExcelKludgeStream() */
            SetCurrentForm( rPicObj.GetScTab() );
            // seek to stream position of the extra data for this control
            mxCtlsStrm->Seek( rPicObj.GetCtlsStreamPos() );
            // read from mxCtlsStrm into xShape, insert the control model into the form
            Reference< XShape > xShape;
            if( mxCurrForm.is() && ReadOCXExcelKludgeStream( mxCtlsStrm, &xShape, TRUE ) )
                xSdrObj.reset( rPicObj.CreateSdrObjectFromShape( xShape, rAnchorRect ) );
        }
        catch( Exception& )
        {
        }
    }
    else
    {
        SfxObjectShell* pDocShell = GetDocShell();
        SotStorageRef xSrcStrg = GetRootStorage();
        String aStrgName = rPicObj.GetOleStorageName();
        if( pDocShell && xSrcStrg.Is() && (aStrgName.Len() > 0) )
        {
            // first try to resolve graphic from DFF storage
            Graphic aGraphic;
            Rectangle aVisArea;
            if( !GetBLIP( GetPropertyValue( DFF_Prop_pib ), aGraphic, &aVisArea ) )
            {
                // if not found, use graphic from object (imported from IMGDATA record)
                aGraphic = rPicObj.GetGraphic();
                aVisArea = rPicObj.GetVisArea();
            }
            if( aGraphic.GetType() != GRAPHIC_NONE )
            {
                ErrCode nError = ERRCODE_NONE;
                namespace cssea = ::com::sun::star::embed::Aspects;
                sal_Int64 nAspects = rPicObj.IsSymbol() ? cssea::MSOLE_ICON : cssea::MSOLE_CONTENT;
                xSdrObj.reset( CreateSdrOLEFromStorage(
                    aStrgName, xSrcStrg, pDocShell->GetStorage(), aGraphic,
                    rAnchorRect, aVisArea, 0, nError, mnOleImpFlags, nAspects ) );
            }
        }
    }

    return xSdrObj.release();
}

ScRange XclImpDffManager::GetUsedArea( SCTAB nScTab ) const
{
    ScRange aScUsedArea( ScAddress::INITIALIZE_INVALID );
    ScRangeMap::const_iterator aIt = maUsedAreaMap.find( nScTab );
    if( aIt != maUsedAreaMap.end() )
        aScUsedArea = aIt->second;
    return aScUsedArea;
}

// virtual functions ----------------------------------------------------------

void XclImpDffManager::ProcessClientAnchor2( SvStream& rDffStrm,
        DffRecordHeader& rHeader, void* /*pClientData*/, DffObjData& rObjData )
{
    // find the OBJ record data related to the processed shape
    if( XclImpDrawObjBase* pDrawObj = mrObjManager.FindDrawObj( rObjData.rSpHd ).get() )
    {
        DBG_ASSERT( rHeader.nRecType == DFF_msofbtClientAnchor, "XclImpDffManager::ProcessClientAnchor2 - no client anchor record" );
        XclObjAnchor aAnchor( pDrawObj->GetScTab() );
        rHeader.SeekToContent( rDffStrm );
        rDffStrm.SeekRel( 2 );  // flags
        rDffStrm >> aAnchor;    // anchor format equal to BIFF5 OBJ records
        pDrawObj->SetAnchor( aAnchor );
        rObjData.aChildAnchor = pDrawObj->GetAnchorRect();
        rObjData.bChildAnchor = sal_True;
    }
}

SdrObject* XclImpDffManager::ProcessObj( SvStream& rDffStrm,
        DffObjData& rDffObjData, void* pClientData, Rectangle& /*rTextRect*/, SdrObject* pOldSdrObj )
{
    /*  pOldSdrObj passes a generated SdrObject. This function owns this object
        and can modify it. The function has either to return it back to caller
        or to delete it by itself. */
    SdrObjectPtr xSdrObj( pOldSdrObj );

    // find the OBJ record data related to the processed shape
    XclImpDrawObjRef xDrawObj = mrObjManager.FindDrawObj( rDffObjData.rSpHd );
    const Rectangle& rAnchorRect = rDffObjData.aChildAnchor;

    // #102378# Do not process the global page group shape (flag SP_FPATRIARCH)
    bool bGlobalPageGroup = ::get_flag< sal_uInt32 >( rDffObjData.nSpFlags, SP_FPATRIARCH );
    if( !xDrawObj || !xDrawObj->IsProcessSdrObj() || bGlobalPageGroup )
        return 0;   // simply return, xSdrObj will be destroyed

    /*  Pass pointer to top-level object back to caller. If the processed
        object is embedded in a group, the pointer is already set to the
        top-level parent object. */
    XclImpDrawObjBase** ppTopLevelObj = reinterpret_cast< XclImpDrawObjBase** >( pClientData );
    bool bIsTopLevel = !ppTopLevelObj || !*ppTopLevelObj;
    if( ppTopLevelObj && bIsTopLevel )
        *ppTopLevelObj = xDrawObj.get();

    // #119010# connectors don't have to be area objects
    if( dynamic_cast< SdrEdgeObj* >( xSdrObj.get() ) )
        xDrawObj->SetAreaObj( false );

    /*  Check for valid size for all objects. Needed to ignore lots of invisible
        phantom objects from deleted rows or columns (for performance reasons).
        #i30816# Include objects embedded in groups.
        #i58780# Ignore group shapes, size is not initialized. */
    bool bEmbeddedGroup = !bIsTopLevel && dynamic_cast< SdrObjGroup* >( xSdrObj.get() );
    if( !bEmbeddedGroup && !xDrawObj->IsValidSize( rAnchorRect ) )
        return 0;   // simply return, xSdrObj will be destroyed

    // set shape information from DFF stream
    String aObjName = GetPropertyString( DFF_Prop_wzName, rDffStrm );
    String aHyperlink = ReadHlinkProperty( rDffStrm );
    bool bVisible = !GetPropertyBool( DFF_Prop_fHidden );
    bool bAutoMargin = GetPropertyBool( DFF_Prop_AutoTextMargin );
    xDrawObj->SetDffData( rDffObjData, aObjName, aHyperlink, bVisible, bAutoMargin );

    /*  Connect textbox data (string, alignment, text orientation) to object.
        #98132# don't ask for a text-ID, DFF export doesn't set one. */
    if( XclImpTextObj* pTextObj = dynamic_cast< XclImpTextObj* >( xDrawObj.get() ) )
        if( const XclImpObjTextData* pTextData = mrObjManager.FindTextData( rDffObjData.rSpHd ) )
            pTextObj->SetTextData( *pTextData );

    // copy line and fill formatting of TBX form controls from DFF properties
    if( XclImpTbxObjBase* pTbxObj = dynamic_cast< XclImpTbxObjBase* >( xDrawObj.get() ) )
        pTbxObj->SetDffProperties( *this );

    // try to create a custom SdrObject that overwrites the passed object
    SdrObjectPtr xNewSdrObj( xDrawObj->CreateSdrObject( rAnchorRect, *mxProgress, true ) );
    if( xNewSdrObj.is() )
        xSdrObj.reset( xNewSdrObj.release() );

    // process the SdrObject
    if( xSdrObj.is() )
    {
        // filled without color -> set system window color
        if( GetPropertyBool( DFF_Prop_fFilled ) && !IsProperty( DFF_Prop_fillColor ) )
            xSdrObj->SetMergedItem( XFillColorItem( EMPTY_STRING, GetPalette().GetColor( EXC_COLOR_WINDOWBACK ) ) );

        // additional processing on the SdrObject
        xDrawObj->ProcessSdrObject( *xSdrObj );

        // add the area used by this object to the internal map of used areas
        UpdateUsedArea( *xDrawObj );

        /*  If the SdrObject will not be inserted into the draw page, delete it
            here. Happens e.g. for notes: The ProcessSdrObject() call above has
            inserted the note into the document, and the SdrObject is not
            needed anymore. */
        if( !xDrawObj->IsInsertSdrObj() )
            xSdrObj.reset();
    }

    /*  Store the relation between shape ID and SdrObject for connectors. Must
        be done here (and not in InsertSdrObject() function), otherwise all
        SdrObjects embedded in groups would be lost. */
    if( xSdrObj.is() )
        maSolverCont.InsertSdrObjectInfo( *xSdrObj, xDrawObj->GetDffShapeId(), xDrawObj->GetDffFlags() );

    return xSdrObj.release();
}

ULONG XclImpDffManager::Calc_nBLIPPos( ULONG /*nOrgVal*/, ULONG nStreamPos ) const
{
    return nStreamPos + 4;
}

sal_Bool XclImpDffManager::InsertControl( const Reference< XFormComponent >& rxFormComp,
        const ::com::sun::star::awt::Size& /*rSize*/, Reference< XShape >* pxShape,
        BOOL /*bFloatingCtrl*/ )
{
    if( GetDocShell() ) try
    {
        Reference< XIndexContainer > xFormIC( mxCurrForm, UNO_QUERY_THROW );
        Reference< XControlModel > xCtrlModel( rxFormComp, UNO_QUERY_THROW );

        // create the control shape
        Reference< XShape > xShape( ScfApiHelper::CreateInstance( GetDocShell(), CREATE_OUSTRING( "com.sun.star.drawing.ControlShape" ) ), UNO_QUERY_THROW );
        Reference< XControlShape > xCtrlShape( xShape, UNO_QUERY_THROW );

        // insert the new control into the form
        sal_Int32 nNewIndex = xFormIC->getCount();
        xFormIC->insertByIndex( nNewIndex, Any( rxFormComp ) );
        // on success: store new index of the control for later use (macro events)
        mnLastCtrlIndex = nNewIndex;

        // set control model at control shape and pass back shape to caller
        xCtrlShape->setControl( xCtrlModel );
        if( pxShape ) *pxShape = xShape;
        return sal_True;
    }
    catch( Exception& )
    {
        DBG_ERRORFILE( "XclImpDffManager::InsertControl - cannot create form control" );
    }

    return sal_False;
}

// private --------------------------------------------------------------------

String XclImpDffManager::ReadHlinkProperty( SvStream& rDffStrm ) const
{
    /*  Reads hyperlink data from a complex DFF property. Contents of this
        property are equal to the HLINK record, import of this record is
        implemented in class XclImpHyperlink. This function has to create an
        instance of the XclImpStream class to be able to reuse the
        functionality of XclImpHyperlink. */
    String aString;
    sal_uInt32 nBufferSize = GetPropertyValue( DFF_Prop_pihlShape );
    if( (0 < nBufferSize) && (nBufferSize <= 0xFFFF) && SeekToContent( DFF_Prop_pihlShape, rDffStrm ) )
    {
        // create a faked BIFF record that can be read by XclImpStream class
        SvMemoryStream aMemStream;
        aMemStream << sal_uInt16( 0 ) << static_cast< sal_uInt16 >( nBufferSize );

        // copy from DFF stream to memory stream
        ::std::vector< sal_uInt8 > aBuffer( nBufferSize );
        sal_uInt8* pnData = &aBuffer.front();
        if( rDffStrm.Read( pnData, nBufferSize ) == nBufferSize )
        {
            aMemStream.Write( pnData, nBufferSize );

            // create BIFF import stream to be able to use XclImpHyperlink class
            XclImpStream aXclStrm( aMemStream, GetRoot() );
            if( aXclStrm.StartNextRecord() )
                aString = XclImpHyperlink::ReadEmbeddedData( aXclStrm );
        }
    }
    return aString;
}

void XclImpDffManager::ProcessDggContainer( SvStream& rDffStrm, const DffRecordHeader& rDggHeader )
{
    // seek to end of drawing group container
    rDggHeader.SeekToEndOfRecord( rDffStrm );
}

void XclImpDffManager::ProcessDgContainer( SvStream& rDffStrm, const DffRecordHeader& rDgHeader )
{
    sal_Size nEndPos = rDgHeader.GetRecEndFilePos();
    while( rDffStrm.Tell() < nEndPos )
    {
        DffRecordHeader aHeader;
        rDffStrm >> aHeader;
        switch( aHeader.nRecType )
        {
            case DFF_msofbtSolverContainer:
                ProcessSolverContainer( rDffStrm, aHeader );
            break;
            case DFF_msofbtSpgrContainer:
                ProcessShGrContainer( rDffStrm, aHeader );
            break;
            default:
                aHeader.SeekToEndOfRecord( rDffStrm );
        }
    }
    // seek to end of drawing page container
    rDgHeader.SeekToEndOfRecord( rDffStrm );

    // #i12638# #i37900# connector rules
    maSolverCont.UpdateConnectorRules();
    SolveSolver( maSolverCont );
    maSolverCont.RemoveConnectorRules();
}

void XclImpDffManager::ProcessShGrContainer( SvStream& rDffStrm, const DffRecordHeader& rShGrHeader )
{
    sal_Size nEndPos = rShGrHeader.GetRecEndFilePos();
    while( rDffStrm.Tell() < nEndPos )
    {
        DffRecordHeader aHeader;
        rDffStrm >> aHeader;
        switch( aHeader.nRecType )
        {
            case DFF_msofbtSpgrContainer:
            case DFF_msofbtSpContainer:
                ProcessShContainer( rDffStrm, aHeader );
            break;
            default:
                aHeader.SeekToEndOfRecord( rDffStrm );
        }
    }
    // seek to end of shape group container
    rShGrHeader.SeekToEndOfRecord( rDffStrm );
}

void XclImpDffManager::ProcessSolverContainer( SvStream& rDffStrm, const DffRecordHeader& rSolverHeader )
{
    // solver container wants to read the solver container header again
    rSolverHeader.SeekToBegOfRecord( rDffStrm );
    // read the entire solver container
    rDffStrm >> maSolverCont;
    // seek to end of solver container
    rSolverHeader.SeekToEndOfRecord( rDffStrm );
}

void XclImpDffManager::ProcessShContainer( SvStream& rDffStrm, const DffRecordHeader& rShHeader )
{
    rShHeader.SeekToBegOfRecord( rDffStrm );
    Rectangle aDummy;
    const XclImpDrawObjBase* pDrawObj = 0;
    /*  The call to ImportObj() creates and returns a new SdrObject for the
        processed shape. We take ownership of the returned object here. If the
        shape is a group object, all embedded objects are created recursively,
        and the returned group object contains them all. ImportObj() calls the
        virtual functions ProcessClientAnchor2() and ProcessObj() and writes
        the pointer to the related draw object data (OBJ record) into pDrawObj. */
    SdrObjectPtr xSdrObj( ImportObj( rDffStrm, &pDrawObj, aDummy, aDummy, 0, 0 ) );
    if( pDrawObj && xSdrObj.is() )
        InsertSdrObject( GetSdrPage( pDrawObj->GetScTab() ), *pDrawObj, xSdrObj.release() );
    rShHeader.SeekToEndOfRecord( rDffStrm );
}

void XclImpDffManager::InsertSdrObject( SdrObjList* pObjList, const XclImpDrawObjBase& rDrawObj, SdrObject* pSdrObj )
{
    /*  Take ownership of the passed object. If insertion fails (e.g. rDrawObj
        states to skip insertion, or missing draw page), the object is
        automatically deleted. */
    SdrObjectPtr xSdrObj( pSdrObj );
    if( pObjList && xSdrObj.is() && rDrawObj.IsInsertSdrObj() )
        pObjList->NbcInsertObject( xSdrObj.release() );
    // SdrObject still here? Insertion failed, remove data from shape ID map.
    if( xSdrObj.is() )
        maSolverCont.RemoveSdrObjectInfo( *xSdrObj );
}

void XclImpDffManager::SetCurrentForm( SCTAB nScTab )
{
    if( nScTab != mnCurrFormScTab )
    {
        mxCurrForm.clear();
        mnCurrFormScTab = nScTab;

        SdrPage* pSdrPage = GetSdrPage( nScTab );
        if( GetDocShell() && pSdrPage ) try
        {
            Reference< XFormsSupplier > xFormsSupplier( pSdrPage->getUnoPage(), UNO_QUERY_THROW );
            Reference< XNameContainer > xFormsNC = xFormsSupplier->getForms();
            if( xFormsNC.is() )
            {
                // find or create the Standard form used to insert the imported controls
                OUString aFormName = CREATE_OUSTRING( "Standard" );
                if( xFormsNC->hasByName( aFormName ) )
                {
                    xFormsNC->getByName( aFormName ) >>= mxCurrForm;
                }
                else
                {
                    mxCurrForm.set( ScfApiHelper::CreateInstance( GetDocShell(), CREATE_OUSTRING( "com.sun.star.form.component.Form" ) ), UNO_QUERY_THROW );
                    xFormsNC->insertByName( aFormName, Any( mxCurrForm ) );
                }
            }
        }
        catch( Exception& )
        {
        }
    }
}

void XclImpDffManager::UpdateUsedArea( const XclImpDrawObjBase& rDrawObj )
{
    ScRange aScObjArea = rDrawObj.GetUsedArea();
    if( aScObjArea.IsValid() )
    {
        ScRange* pScTabArea = 0;
        ScRangeMap::iterator aIt = maUsedAreaMap.find( rDrawObj.GetScTab() );
        if( aIt == maUsedAreaMap.end() )
        {
            pScTabArea = &maUsedAreaMap[ rDrawObj.GetScTab() ];
            pScTabArea->SetInvalid();
        }
        else
            pScTabArea = &aIt->second;

        if( pScTabArea )
            pScTabArea->ExtendTo( aScObjArea );
    }
}

// The object manager =========================================================

XclImpObjectManager::XclImpObjectManager( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot )
{
    maDefObjNames[ EXC_OBJTYPE_GROUP ]          = CREATE_STRING( "Group" );
    maDefObjNames[ EXC_OBJTYPE_LINE ]           = CREATE_STRING( "Line" );
    maDefObjNames[ EXC_OBJTYPE_RECTANGLE ]      = CREATE_STRING( "Rectangle" );
    maDefObjNames[ EXC_OBJTYPE_OVAL ]           = CREATE_STRING( "Oval" );
    maDefObjNames[ EXC_OBJTYPE_ARC ]            = CREATE_STRING( "Arc" );
    maDefObjNames[ EXC_OBJTYPE_CHART ]          = CREATE_STRING( "Chart" );
    maDefObjNames[ EXC_OBJTYPE_TEXT ]           = CREATE_STRING( "Text" );
    maDefObjNames[ EXC_OBJTYPE_BUTTON ]         = CREATE_STRING( "Button" );
    maDefObjNames[ EXC_OBJTYPE_PICTURE ]        = CREATE_STRING( "Picture" );
    maDefObjNames[ EXC_OBJTYPE_POLYGON ]        = CREATE_STRING( "Freeform" );
    maDefObjNames[ EXC_OBJTYPE_CHECKBOX ]       = CREATE_STRING( "Check Box" );
    maDefObjNames[ EXC_OBJTYPE_OPTIONBUTTON ]   = CREATE_STRING( "Option Button" );
    maDefObjNames[ EXC_OBJTYPE_EDIT ]           = CREATE_STRING( "Edit Box" );
    maDefObjNames[ EXC_OBJTYPE_LABEL ]          = CREATE_STRING( "Label" );
    maDefObjNames[ EXC_OBJTYPE_DIALOG ]         = CREATE_STRING( "Dialog Frame" );
    maDefObjNames[ EXC_OBJTYPE_SPIN ]           = CREATE_STRING( "Spinner" );
    maDefObjNames[ EXC_OBJTYPE_SCROLLBAR ]      = CREATE_STRING( "Scroll Bar" );
    maDefObjNames[ EXC_OBJTYPE_LISTBOX ]        = CREATE_STRING( "List Box" );
    maDefObjNames[ EXC_OBJTYPE_GROUPBOX ]       = CREATE_STRING( "Group Box" );
    maDefObjNames[ EXC_OBJTYPE_DROPDOWN ]       = CREATE_STRING( "Drop Down" );
    maDefObjNames[ EXC_OBJTYPE_NOTE ]           = CREATE_STRING( "Comment" );
    maDefObjNames[ EXC_OBJTYPE_DRAWING ]        = CREATE_STRING( "AutoShape" );
}

XclImpObjectManager::~XclImpObjectManager()
{
}

// *** Read Excel records *** -------------------------------------------------

Graphic XclImpObjectManager::ReadImgData( XclImpStream& rStrm ) // static helper
{
    Graphic aGraphic;
    sal_uInt16 nFormat, nEnv;
    sal_uInt32 nDataSize;
    rStrm >> nFormat >> nEnv >> nDataSize;
    if( nDataSize <= rStrm.GetRecLeft() )
    {
        switch( nFormat )
        {
            case EXC_IMGDATA_WMF:   ReadWmf( aGraphic, rStrm ); break;
            case EXC_IMGDATA_BMP:   ReadBmp( aGraphic, rStrm ); break;
            default:    DBG_ERRORFILE( "XclImpObjectManager::ReadImgData - unknown image format" );
        }
    }
    return aGraphic;
}

void XclImpObjectManager::ReadObj( XclImpStream& rStrm )
{
    XclImpDrawObjRef xDrawObj;

    /*  #i61786# In BIFF8 streams, OBJ records may occur without MSODRAWING
        records. In this case, the OBJ records are in BIFF5 format. Do a sanity
        check here that there is no DFF data loaded before. */
    DBG_ASSERT( maDffStrm.Tell() == 0, "XclImpObjectManager::ReadObj - unexpected DFF stream data, OBJ will be ignored" );
    if( maDffStrm.Tell() == 0 ) switch( GetBiff() )
    {
        case EXC_BIFF3:
            xDrawObj = XclImpDrawObjBase::ReadObj3( rStrm );
        break;
        case EXC_BIFF4:
            xDrawObj = XclImpDrawObjBase::ReadObj4( rStrm );
        break;
        case EXC_BIFF5:
        case EXC_BIFF8:
            xDrawObj = XclImpDrawObjBase::ReadObj5( rStrm );
        break;
        default:
            DBG_ERROR_BIFF();
    }

    if( xDrawObj.is() )
    {
        // insert into maRawObjs or into the last open group object
        maRawObjs.InsertGrouped( xDrawObj );
        // to be able to find objects by ID
        maObjMapId[ xDrawObj->GetObjId() ] = xDrawObj;
    }
}

void XclImpObjectManager::ReadMsoDrawingGroup( XclImpStream& rStrm )
{
    DBG_ASSERT_BIFF( GetBiff() == EXC_BIFF8 );
    // Excel continues this record with MSODRAWINGGROUP and CONTINUE records, hmm.
    rStrm.ResetRecord( true, EXC_ID_MSODRAWINGGROUP );
    ReadDffRecord( rStrm );
}

void XclImpObjectManager::ReadMsoDrawing( XclImpStream& rStrm )
{
    DBG_ASSERT_BIFF( GetBiff() == EXC_BIFF8 );
    // disable internal CONTINUE handling
    rStrm.ResetRecord( false );
    /*  #i60510# real life: MSODRAWINGSELECTION record may contain garbage -
        this makes it impossible to process the DFF stream in one run.
        Store stream start position for every sheet separately, will be used
        to seek the stream to these positions later, when processing the next
        sheet. */
    size_t nTabIdx = static_cast< size_t >( GetCurrScTab() );
    if( nTabIdx >= maTabStrmPos.size() )
    {
        maTabStrmPos.resize( nTabIdx, STREAM_SEEK_TO_END );
        maTabStrmPos.push_back( maDffStrm.Tell() );
    }
    // read leading MSODRAWING record
    ReadDffRecord( rStrm );

    // read following drawing records, but do not start following unrelated record
    bool bLoop = true;
    while( bLoop ) switch( rStrm.GetNextRecId() )
    {
        case EXC_ID_MSODRAWING:
        case EXC_ID_MSODRAWINGSEL:
        case EXC_ID_CONT:
            rStrm.StartNextRecord();
            ReadDffRecord( rStrm );
        break;
        case EXC_ID_OBJ:
            rStrm.StartNextRecord();
            ReadObj8( rStrm );
        break;
        case EXC_ID_TXO:
            rStrm.StartNextRecord();
            ReadTxo( rStrm );
        break;
        default:
            bLoop = false;
    }

    // re-enable internal CONTINUE handling
    rStrm.ResetRecord( true );
}

void XclImpObjectManager::ReadNote( XclImpStream& rStrm )
{
    switch( GetBiff() )
    {
        case EXC_BIFF2:
        case EXC_BIFF3:
        case EXC_BIFF4:
        case EXC_BIFF5:
            ReadNote3( rStrm );
        break;
        case EXC_BIFF8:
            ReadNote8( rStrm );
        break;
        default:
            DBG_ERROR_BIFF();
    }
}

void XclImpObjectManager::ReadTabChart( XclImpStream& rStrm )
{
    DBG_ASSERT_BIFF( GetBiff() >= EXC_BIFF5 );
    ScfRef< XclImpChartObj > xChartObj( new XclImpChartObj( GetRoot(), true ) );
    xChartObj->ReadChartSubStream( rStrm );
    // insert the chart as raw object without connected DFF data
    maRawObjs.push_back( xChartObj );
}

// *** Drawing objects *** ----------------------------------------------------

XclImpDrawObjRef XclImpObjectManager::FindDrawObj( const DffRecordHeader& rHeader ) const
{
    /*  maObjMap stores objects by position of the client data (OBJ record) in
        the DFF stream, which is always behind shape start position of the
        passed header. The function upper_bound() finds the first element in
        the map whose key is greater than the start position of the header. Its
        end position is used to test whether the found object is really related
        to the shape. */
    XclImpDrawObjRef xDrawObj;
    XclImpObjMap::const_iterator aIt = maObjMap.upper_bound( rHeader.GetRecBegFilePos() );
    if( (aIt != maObjMap.end()) && (aIt->first <= rHeader.GetRecEndFilePos()) )
        xDrawObj = aIt->second;
    return xDrawObj;
}

XclImpDrawObjRef XclImpObjectManager::FindDrawObj( const XclObjId& rObjId ) const
{
    XclImpDrawObjRef xDrawObj;
    XclImpObjMapById::const_iterator aIt = maObjMapId.find( rObjId );
    if( aIt != maObjMapId.end() )
        xDrawObj = aIt->second;
    return xDrawObj;
}

const XclImpObjTextData* XclImpObjectManager::FindTextData( const DffRecordHeader& rHeader ) const
{
    /*  maTextMap stores textbox data by position of the client data (TXO
        record) in the DFF stream, which is always behind shape start position
        of the passed header. The function upper_bound() finds the first
        element in the map whose key is greater than the start position of the
        header. Its end position is used to test whether the found object is
        really related to the shape. */
    XclImpObjTextMap::const_iterator aIt = maTextMap.upper_bound( rHeader.GetRecBegFilePos() );
    if( (aIt != maTextMap.end()) && (aIt->first <= rHeader.GetRecEndFilePos()) )
        return aIt->second.get();
    return 0;
}

void XclImpObjectManager::SetSkipObj( SCTAB nScTab, sal_uInt16 nObjId )
{
    maSkipObjs.push_back( XclObjId( nScTab, nObjId ) );
}

// *** Drawing object conversion *** ------------------------------------------

XclImpDffManager& XclImpObjectManager::GetDffManager()
{
    if( !mxDffManager )
        mxDffManager.reset( new XclImpDffManager( GetRoot(), *this, maDffStrm ) );
    return *mxDffManager;
}

void XclImpObjectManager::ConvertObjects()
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aLog, "sc", "dr104026", "XclImpObjectManager::ConvertObjects" );

    // do nothing if the document does not contain a drawing layer
    if( GetDoc().GetDrawLayer() )
    {
        // process list of identifiers of objects to be skipped
        for( XclObjIdVec::const_iterator aVIt = maSkipObjs.begin(), aVEnd = maSkipObjs.end(); aVIt != aVEnd; ++aVIt )
            if( XclImpDrawObjBase* pDrawObj = FindDrawObj( *aVIt ).get() )
                pDrawObj->SetProcessSdrObj( false );

        // get progress bar size for all valid objects
        sal_Size nProgressSize = GetProgressSize();
        if( nProgressSize > 0 )
        {
            XclImpDffManager& rDffManager = GetDffManager();
            rDffManager.StartProgressBar( nProgressSize );
            // process drawing objects without DFF data
            for( XclImpDrawObjVector::const_iterator aVIt = maRawObjs.begin(), aVEnd = maRawObjs.end(); aVIt != aVEnd; ++aVIt )
                rDffManager.ProcessObject( GetSdrPage( (*aVIt)->GetScTab() ), **aVIt );
            // process the global DFF container, contains pictures
            if( !maTabStrmPos.empty() && (maTabStrmPos.front() > 0) )
                rDffManager.ProcessDrawingGroup( maDffStrm );
            // process the sheet records, this inserts the objects into the drawing layer
            for( StreamPosVec::const_iterator aPIt = maTabStrmPos.begin(), aPEnd = maTabStrmPos.end(); aPIt != aPEnd; ++aPIt )
                if( *aPIt != STREAM_SEEK_TO_END )
                    rDffManager.ProcessDrawing( maDffStrm, *aPIt );
        }
    }
}

String XclImpObjectManager::GetDefaultObjName( const XclImpDrawObjBase& rDrawObj ) const
{
    String aDefName;
    DefObjNameMap::const_iterator aIt = maDefObjNames.find( rDrawObj.GetObjType() );
    if( aIt != maDefObjNames.end() )
        aDefName.Append( aIt->second );
    return aDefName.Append( sal_Unicode( ' ' ) ).Append( String::CreateFromInt32( rDrawObj.GetObjId().mnObjId ) );
}

ScRange XclImpObjectManager::GetUsedArea( SCTAB nScTab ) const
{
    ScRange aScUsedArea( ScAddress::INITIALIZE_INVALID );
    if( mxDffManager.is() )
        aScUsedArea = mxDffManager->GetUsedArea( nScTab );
    return aScUsedArea;
}

// private --------------------------------------------------------------------

void XclImpObjectManager::ReadWmf( Graphic& rGraphic, XclImpStream& rStrm ) // static helper
{
    // extract graphic data from IMGDATA and following CONTINUE records
    rStrm.Ignore( 8 );
    SvMemoryStream aMemStrm;
    rStrm.CopyToStream( aMemStrm, rStrm.GetRecLeft() );
    aMemStrm.Seek( STREAM_SEEK_TO_BEGIN );
    // import the graphic from memory stream
    GDIMetaFile aGDIMetaFile;
    if( ::ReadWindowMetafile( aMemStrm, aGDIMetaFile, 0 ) )
        rGraphic = aGDIMetaFile;
}

void XclImpObjectManager::ReadBmp( Graphic& rGraphic, XclImpStream& rStrm ) // static helper
{
    // extract graphic data from IMGDATA and following CONTINUE records
    SvMemoryStream aMemStrm;

    /*  Excel 3 and 4 seem to write broken BMP data. Usually they write a
        DIBCOREHEADER (12 bytes) containing width, height, planes = 1, and
        pixel depth = 32 bit. After that, 3 unused bytes are added before the
        actual pixel data. This does even confuse Excel 5 and later, which
        cannot read the image data correctly. */
    if( rStrm.GetRoot().GetBiff() <= EXC_BIFF4 )
    {
        rStrm.PushPosition();
        sal_uInt32 nHdrSize;
        sal_uInt16 nWidth, nHeight, nPlanes, nDepth;
        rStrm >> nHdrSize >> nWidth >> nHeight >> nPlanes >> nDepth;
        if( (nHdrSize == 12) && (nPlanes == 1) && (nDepth == 32) )
        {
            rStrm.Ignore( 3 );
            aMemStrm.SetNumberFormatInt( NUMBERFORMAT_INT_LITTLEENDIAN );
            aMemStrm << nHdrSize << nWidth << nHeight << nPlanes << nDepth;
            rStrm.CopyToStream( aMemStrm, rStrm.GetRecLeft() );
        }
        rStrm.PopPosition();
    }

    // no special handling above -> just copy the remaining record data
    if( aMemStrm.Tell() == 0 )
        rStrm.CopyToStream( aMemStrm, rStrm.GetRecLeft() );

    // import the graphic from memory stream
    aMemStrm.Seek( STREAM_SEEK_TO_BEGIN );
    Bitmap aBitmap;
    if( aBitmap.Read( aMemStrm, FALSE ) )   // read DIB without file header
        rGraphic = aBitmap;
}

void XclImpObjectManager::ReadDffRecord( XclImpStream& rStrm )
{
    maDffStrm.Seek( STREAM_SEEK_TO_END );
    rStrm.CopyRecordToStream( maDffStrm );
}

void XclImpObjectManager::ReadObj8( XclImpStream& rStrm )
{
    XclImpDrawObjRef xDrawObj = XclImpDrawObjBase::ReadObj8( rStrm );
    // store the new object in the internal containers
    maObjMap[ maDffStrm.Tell() ] = xDrawObj;
    maObjMapId[ xDrawObj->GetObjId() ] = xDrawObj;
}

void XclImpObjectManager::ReadTxo( XclImpStream& rStrm )
{
    XclImpObjTextRef xTextData( new XclImpObjTextData );
    maTextMap[ maDffStrm.Tell() ] = xTextData;

    // 1) read the TXO record
    xTextData->maData.ReadTxo8( rStrm );

    // 2) first CONTINUE with string
    xTextData->mxString.reset();
    bool bValid = true;
    if( xTextData->maData.mnTextLen > 0 )
    {
        bValid = (rStrm.GetNextRecId() == EXC_ID_CONT) && rStrm.StartNextRecord();
        DBG_ASSERT( bValid, "XclImpObjectManager::ReadTxo - missing CONTINUE record" );
        if( bValid )
            xTextData->mxString.reset( new XclImpString( rStrm.ReadUniString( xTextData->maData.mnTextLen ) ) );
    }

    // 3) second CONTINUE with formatting runs
    if( xTextData->maData.mnFormatSize > 0 )
    {
        bValid = (rStrm.GetNextRecId() == EXC_ID_CONT) && rStrm.StartNextRecord();
        DBG_ASSERT( bValid, "XclImpObjectManager::ReadTxo - missing CONTINUE record" );
        if( bValid )
            xTextData->ReadFormats( rStrm );
    }
}

void XclImpObjectManager::ReadNote3( XclImpStream& rStrm )
{
    XclAddress aXclPos;
    sal_uInt16 nTotalLen;
    rStrm >> aXclPos >> nTotalLen;

    SCTAB nScTab = GetCurrScTab();
    ScAddress aScNotePos( ScAddress::UNINITIALIZED );
    if( GetAddressConverter().ConvertAddress( aScNotePos, aXclPos, nScTab, true ) )
    {
        sal_uInt16 nPartLen = ::std::min( nTotalLen, static_cast< sal_uInt16 >( rStrm.GetRecLeft() ) );
        String aNoteText = rStrm.ReadRawByteString( nPartLen );
        nTotalLen = nTotalLen - nPartLen;
        while( (nTotalLen > 0) && (rStrm.GetNextRecId() == EXC_ID_NOTE) && rStrm.StartNextRecord() )
        {
            rStrm >> aXclPos >> nPartLen;
            DBG_ASSERT( aXclPos.mnRow == 0xFFFF, "XclImpObjectManager::ReadNote3 - missing continuation NOTE record" );
            if( aXclPos.mnRow == 0xFFFF )
            {
                DBG_ASSERT( nPartLen <= nTotalLen, "XclImpObjectManager::ReadNote3 - string too long" );
                aNoteText.Append( rStrm.ReadRawByteString( nPartLen ) );
                nTotalLen = nTotalLen - ::std::min( nTotalLen, nPartLen );
            }
            else
            {
                // seems to be a new note, record already started -> load the note
                rStrm.Seek( EXC_REC_SEEK_TO_BEGIN );
                ReadNote( rStrm );
                nTotalLen = 0;
            }
        }
        ScNoteUtil::CreateNoteFromString( GetDoc(), aScNotePos, aNoteText, false, false );
    }
}

void XclImpObjectManager::ReadNote8( XclImpStream& rStrm )
{
    XclAddress aXclPos;
    sal_uInt16 nFlags, nObjId;
    rStrm >> aXclPos >> nFlags >> nObjId;

    SCTAB nScTab = GetCurrScTab();
    ScAddress aScNotePos( ScAddress::UNINITIALIZED );
    if( GetAddressConverter().ConvertAddress( aScNotePos, aXclPos, nScTab, true ) )
        if( nObjId != EXC_OBJ_INVALID_ID )
            if( XclImpNoteObj* pNoteObj = dynamic_cast< XclImpNoteObj* >( FindDrawObj( XclObjId( nScTab, nObjId ) ).get() ) )
                pNoteObj->SetNoteData( aScNotePos, nFlags );
}

sal_Size XclImpObjectManager::GetProgressSize() const
{
    sal_Size nProgressSize = maRawObjs.GetProgressSize();
    for( XclImpObjMap::const_iterator aMIt = maObjMap.begin(), aMEnd = maObjMap.end(); aMIt != aMEnd; ++aMIt )
        nProgressSize += aMIt->second->GetProgressSize();
    return nProgressSize;
}

// DFF property set helper ====================================================

XclImpDffPropSet::XclImpDffPropSet( const XclImpRoot& rRoot ) :
    XclImpRoot( rRoot ),
    maDffManager( rRoot, maDummyStrm )
{
}

void XclImpDffPropSet::Read( XclImpStream& rStrm )
{
    sal_uInt32 nPropSetSize;

    rStrm.PushPosition();
    rStrm.Ignore( 4 );
    rStrm >> nPropSetSize;
    rStrm.PopPosition();

    mxMemStrm.reset( new SvMemoryStream );
    rStrm.CopyToStream( *mxMemStrm, 8 + nPropSetSize );
    mxMemStrm->Seek( STREAM_SEEK_TO_BEGIN );
    maDffManager.ReadPropSet( *mxMemStrm, 0 );
}

sal_uInt32 XclImpDffPropSet::GetPropertyValue( sal_uInt16 nPropId, sal_uInt32 nDefault ) const
{
    return maDffManager.GetPropertyValue( nPropId, nDefault );
}

void XclImpDffPropSet::FillToItemSet( SfxItemSet& rItemSet ) const
{
    if( mxMemStrm.get() )
        maDffManager.ApplyAttributes( *mxMemStrm, rItemSet );
}

XclImpStream& operator>>( XclImpStream& rStrm, XclImpDffPropSet& rPropSet )
{
    rPropSet.Read( rStrm );
    return rStrm;
}

// ============================================================================

