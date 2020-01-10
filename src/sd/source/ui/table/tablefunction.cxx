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
#include "precompiled_sd.hxx"

#include <sal/config.h>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/drawing/XSelectionFunction.hpp>
#include <com/sun/star/awt/KeyModifier.hpp>
#include <com/sun/star/lang/XInitialization.hpp>

#include <cppuhelper/compbase2.hxx>
#include <cppuhelper/basemutex.hxx>

#include <vcl/svapp.hxx>

#include <svx/svdotable.hxx>
#include <svx/sdr/overlay/overlayobjectcell.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>
#include <svx/svxids.hrc>
#include <svx/outlobj.hxx>
#include <svx/svdoutl.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdetc.hxx>
#include <svx/editstat.hxx>
#include <svx/unolingu.hxx>
#include <svx/sdrpagewindow.hxx>
#include <svx/sdr/table/tabledesign.hxx>
#include <svx/svxdlg.hxx>
#include <vcl/msgbox.hxx>

#include <svtools/itempool.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/request.hxx>
#include <svtools/style.hxx>

#include "framework/FrameworkHelper.hxx"
#include "app.hrc"
#include "glob.hrc"
#include "DrawViewShell.hxx"
#include "drawdoc.hxx"
#include "DrawDocShell.hxx"
#include "Window.hxx"
#include "drawview.hxx"
#include "sdresid.hxx"

using ::rtl::OUString;
using namespace ::sd;
using namespace ::sdr::table;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::linguistic2;

namespace css = ::com::sun::star;

namespace sd
{
extern void showTableDesignDialog( ::Window*, ViewShellBase& );

static void apply_table_style( SdrTableObj* pObj, SdrModel* pModel, const OUString& sTableStyle )
{
	if( pModel && pObj )
	{
		Reference< XNameAccess > xPool( dynamic_cast< XNameAccess* >( pModel->GetStyleSheetPool() ) );
		if( xPool.is() ) try
		{
			const OUString sFamilyName( RTL_CONSTASCII_USTRINGPARAM( "table" ) );
			Reference< XNameContainer > xTableFamily( xPool->getByName( sFamilyName ), UNO_QUERY_THROW );
			OUString aStdName( RTL_CONSTASCII_USTRINGPARAM("default") );
			if( sTableStyle.getLength() )
				aStdName = sTableStyle;
			Reference< XIndexAccess > xStyle( xTableFamily->getByName( aStdName ), UNO_QUERY_THROW );
			pObj->setTableStyle( xStyle );
		}
		catch( Exception& )
		{
			DBG_ERROR("sd::apply_default_table_style(), exception caught!");
		}
	}
}

void DrawViewShell::FuTable(SfxRequest& rReq)
{
	switch( rReq.GetSlot() )
	{
	case SID_INSERT_TABLE:
	{
		sal_Int32 nColumns = 0;
		sal_Int32 nRows = 0;
		OUString sTableStyle;

		SFX_REQUEST_ARG( rReq, pCols, SfxUInt16Item, SID_ATTR_TABLE_COLUMN, sal_False );
		SFX_REQUEST_ARG( rReq, pRows, SfxUInt16Item, SID_ATTR_TABLE_ROW, sal_False );
		SFX_REQUEST_ARG( rReq, pStyle, SfxStringItem, SID_TABLE_STYLE, sal_False );

		if( pCols )
			nColumns = pCols->GetValue();

		if( pRows )
			nRows = pRows->GetValue();

		if( pStyle )
			sTableStyle = pStyle->GetValue();

		if( (nColumns == 0) || (nRows == 0) )
		{
			SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
			::std::auto_ptr<SvxAbstractNewTableDialog> pDlg( pFact ? pFact->CreateSvxNewTableDialog( NULL ) : 0);

			if( !pDlg.get() || (pDlg->Execute() != RET_OK) )
				break;

			nColumns = pDlg->getColumns();
			nRows = pDlg->getRows();
		}

		Size aSize( 14100, 200 );

		Point aPos;
		Rectangle aWinRect(aPos, GetActiveWindow()->GetOutputSizePixel() );
		aPos = aWinRect.Center();
		aPos = GetActiveWindow()->PixelToLogic(aPos);
		aPos.X() -= aSize.Width() / 2;
		aPos.Y() -= aSize.Height() / 2;
		Rectangle aRect (aPos, aSize);

		::sdr::table::SdrTableObj* pObj = new ::sdr::table::SdrTableObj( GetDoc(), aRect, nColumns, nRows );
		pObj->NbcSetStyleSheet( GetDoc()->GetDefaultStyleSheet(), sal_True );
		apply_table_style( pObj, GetDoc(), sTableStyle );
		SdrPageView* pPV = mpView->GetSdrPageView();
		mpView->InsertObjectAtView(pObj, *pPV, SDRINSERT_SETDEFLAYER);
		Invalidate(SID_DRAWTBX_INSERT);
		rReq.Ignore();
		break;
	}
	case SID_TABLEDESIGN:
	{
		if( GetDoc() && (GetDoc()->GetDocumentType() == DOCUMENT_TYPE_DRAW) )
		{
			// in draw open a modal dialog since we have no tool pane yet
			showTableDesignDialog( GetActiveWindow(), GetViewShellBase() );
		}
		else
		{
			// Make the slide transition panel visible (expand it) in the
	        // tool pane.
		    framework::FrameworkHelper::Instance(GetViewShellBase())->RequestTaskPanel(
			    framework::FrameworkHelper::msTableDesignPanelURL);
		}

		Cancel();
		rReq.Done ();
	}
	default:
		break;
	}
}

// --------------------------------------------------------------------

void DrawViewShell::GetTableMenuState( SfxItemSet &rSet )
{
	bool bIsUIActive = GetDocSh()->IsUIActive();
	if( bIsUIActive )
	{
		rSet.DisableItem( SID_INSERT_TABLE );
	}
	else
	{
		String aActiveLayer = mpDrawView->GetActiveLayer();
		SdrPageView* pPV = mpDrawView->GetSdrPageView();

		if( bIsUIActive ||
			( aActiveLayer.Len() != 0 && pPV && ( pPV->IsLayerLocked(aActiveLayer) ||
			!pPV->IsLayerVisible(aActiveLayer) ) ) || 
			SD_MOD()->GetWaterCan() )
		{
			rSet.DisableItem( SID_INSERT_TABLE );
		}
	}
}

// --------------------------------------------------------------------

void CreateTableFromRTF( SvStream& rStream, SdDrawDocument* pModel )
{
	rStream.Seek( 0 );

	if( pModel )
	{
		SdrPage* pPage = pModel->GetPage(0);
		if( pPage )
		{
			Size aSize( 200, 200 );
			Point aPos;
			Rectangle aRect (aPos, aSize);
			::sdr::table::SdrTableObj* pObj = new ::sdr::table::SdrTableObj( pModel, aRect, 1, 1 );
			pObj->NbcSetStyleSheet( pModel->GetDefaultStyleSheet(), sal_True );
			OUString sTableStyle;
			apply_table_style( pObj, pModel, sTableStyle );

			pPage->NbcInsertObject( pObj );

			sdr::table::SdrTableObj::ImportAsRTF( rStream, *pObj );
		}
	}
}

}
