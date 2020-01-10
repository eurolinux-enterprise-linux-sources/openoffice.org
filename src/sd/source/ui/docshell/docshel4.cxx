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

#include "DrawDocShell.hxx"
#include <com/sun/star/document/PrinterIndependentLayout.hpp>
#include <tools/urlobj.hxx>
#include <sfx2/progress.hxx>
#include <vcl/waitobj.hxx>
#ifndef _SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#include <svx/flstitem.hxx>
#include <svx/eeitem.hxx>
#include <svtools/aeitem.hxx>
#include <svtools/flagitem.hxx>
#include <sot/storage.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#ifndef _DISPATCH_HXX //autogen
#include <sfx2/dispatch.hxx>
#endif
#include <svx/svdotext.hxx>
#include <svtools/style.hxx>
#include <sfx2/printer.hxx>
#include <svtools/ctrltool.hxx>
#ifndef _SFX_ECODE_HXX //autogen
#include <svtools/sfxecode.hxx>
#endif
#include <sot/clsids.hxx>
#include <sot/formats.hxx>
#include <sfx2/request.hxx>
#ifdef TF_STARONE
#include "unomodel.hxx"
#endif

#include <svtools/fltrcfg.hxx>
#include <sfx2/frame.hxx>
#include <sfx2/viewfrm.hxx>
#include <svx/svxmsbas.hxx>
#include <svtools/saveopt.hxx>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/drawing/XDrawView.hpp>
#include <comphelper/processfactory.hxx>

#include "app.hrc"
#include "glob.hrc"
#include "strings.hrc"
#include "strmname.h"
#ifndef SD_FRAMW_VIEW_HXX
#include "FrameView.hxx"
#endif
#include "optsitem.hxx"
#include "Outliner.hxx"
#include "sdattr.hxx"
#include "drawdoc.hxx"
#include "ViewShell.hxx"
#include "app.hxx"
#include "View.hxx"
#include "sdpage.hxx"
#include "sdresid.hxx"
#include "DrawViewShell.hxx"
#include "ViewShellBase.hxx"
#include "Window.hxx"
#include "sdmod.hxx"
#include "OutlineViewShell.hxx"
#include "sdxmlwrp.hxx"
#include "sdpptwrp.hxx"
#include "sdcgmfilter.hxx"
#include "sdgrffilter.hxx"
#include "sdhtmlfilter.hxx"
#include "framework/FrameworkHelper.hxx"

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using ::sd::framework::FrameworkHelper;


namespace sd {

/*************************************************************************
|*
|* SfxPrinter ggf. erzeugen und zurueckgeben
|*
\************************************************************************/

SfxPrinter* DrawDocShell::GetPrinter(BOOL bCreate)
{
	if (bCreate && !mpPrinter)
	{
		// ItemSet mit speziellem Poolbereich anlegen
		SfxItemSet* pSet = new SfxItemSet( GetPool(),
							SID_PRINTER_NOTFOUND_WARN,	SID_PRINTER_NOTFOUND_WARN,
							SID_PRINTER_CHANGESTODOC,	SID_PRINTER_CHANGESTODOC,
							ATTR_OPTIONS_PRINT, 		ATTR_OPTIONS_PRINT,
							0 );
		// PrintOptionsSet setzen
		SdOptionsPrintItem aPrintItem( ATTR_OPTIONS_PRINT,
							SD_MOD()->GetSdOptions(mpDoc->GetDocumentType()));
		SfxFlagItem aFlagItem( SID_PRINTER_CHANGESTODOC );
		USHORT		nFlags = 0;

		nFlags =  (aPrintItem.GetOptionsPrint().IsWarningSize() ? SFX_PRINTER_CHG_SIZE : 0) |
				(aPrintItem.GetOptionsPrint().IsWarningOrientation() ? SFX_PRINTER_CHG_ORIENTATION : 0);
		aFlagItem.SetValue( nFlags );

		pSet->Put( aPrintItem );
		pSet->Put( SfxBoolItem( SID_PRINTER_NOTFOUND_WARN, aPrintItem.GetOptionsPrint().IsWarningPrinter() ) );
		pSet->Put( aFlagItem );

		mpPrinter = new SfxPrinter(pSet);
		mbOwnPrinter = TRUE;

		// Ausgabequalitaet setzen
		UINT16 nQuality = aPrintItem.GetOptionsPrint().GetOutputQuality();

		ULONG nMode = DRAWMODE_DEFAULT;

        if( nQuality == 1 )
			nMode = DRAWMODE_GRAYLINE | DRAWMODE_GRAYFILL | DRAWMODE_BLACKTEXT | DRAWMODE_GRAYBITMAP | DRAWMODE_GRAYGRADIENT;
		else if( nQuality == 2 )
			nMode = DRAWMODE_BLACKLINE | DRAWMODE_BLACKTEXT | DRAWMODE_WHITEFILL | DRAWMODE_GRAYBITMAP | DRAWMODE_WHITEGRADIENT;

		mpPrinter->SetDrawMode( nMode );

		MapMode aMM (mpPrinter->GetMapMode());
		aMM.SetMapUnit(MAP_100TH_MM);
		mpPrinter->SetMapMode(aMM);
        UpdateRefDevice();
	}
	return mpPrinter;
}

/*************************************************************************
|*
|* neuen SfxPrinter setzen (Eigentuemeruebergang)
|*
\************************************************************************/

void DrawDocShell::SetPrinter(SfxPrinter *pNewPrinter)
{
	if ( mpViewShell )
	{
		::sd::View* pView = mpViewShell->GetView();
		if ( pView->IsTextEdit() )
			pView->SdrEndTextEdit();
	}

	if ( mpPrinter && mbOwnPrinter && (mpPrinter != pNewPrinter) )
	{
		delete mpPrinter;
	}

	mpPrinter = pNewPrinter;
	mbOwnPrinter = TRUE;
    if ( mpDoc->GetPrinterIndependentLayout() == ::com::sun::star::document::PrinterIndependentLayout::DISABLED )
		UpdateFontList();
    UpdateRefDevice();
}

void DrawDocShell::UpdateFontList()
{
	delete mpFontList;
    OutputDevice* pRefDevice = NULL;
    if ( mpDoc->GetPrinterIndependentLayout() == ::com::sun::star::document::PrinterIndependentLayout::DISABLED )
		pRefDevice = GetPrinter(TRUE);
	else
		pRefDevice = SD_MOD()->GetVirtualRefDevice();
	mpFontList = new FontList( pRefDevice, NULL, FALSE );
    SvxFontListItem aFontListItem( mpFontList, SID_ATTR_CHAR_FONTLIST );
	PutItem( aFontListItem );
}

/*************************************************************************
|*
|*
|*
\************************************************************************/
Printer* DrawDocShell::GetDocumentPrinter()
{
	return GetPrinter(FALSE);
}

/*************************************************************************
|*
|*
|*
\************************************************************************/
void DrawDocShell::OnDocumentPrinterChanged(Printer* pNewPrinter)
{
	// if we already have a printer, see if its the same
	if( mpPrinter )
	{
		// easy case
		if( mpPrinter == pNewPrinter )
			return;

		// compare if its the same printer with the same job setup
		if( (mpPrinter->GetName() == pNewPrinter->GetName()) &&
			(mpPrinter->GetJobSetup() == pNewPrinter->GetJobSetup()))
			return;
	}

	//	if (mpPrinter->IsA(SfxPrinter))
	{
		// Da kein RTTI verfuegbar, wird hart gecasted (...)
		SetPrinter((SfxPrinter*) pNewPrinter);

		// Printer gehoert dem Container
		mbOwnPrinter = FALSE;
	}
}

/*************************************************************************
|*
|*
|*
\************************************************************************/
void DrawDocShell::UpdateRefDevice()
{
	if( mpDoc )
	{
        // Determine the device for which the output will be formatted.
        OutputDevice* pRefDevice = NULL;
        switch (mpDoc->GetPrinterIndependentLayout())
        {
            case ::com::sun::star::document::PrinterIndependentLayout::DISABLED:
                pRefDevice = mpPrinter;
                break;

            case ::com::sun::star::document::PrinterIndependentLayout::ENABLED:
                pRefDevice = SD_MOD()->GetVirtualRefDevice();
                break;

            default:
                // We are confronted with an invalid or un-implemented
                // layout mode.  Use the printer as formatting device
                // as a fall-back.
                DBG_ASSERT(false, "DrawDocShell::UpdateRefDevice(): Unexpected printer layout mode");

                pRefDevice = mpPrinter;
                break;
        }
		mpDoc->SetRefDevice( pRefDevice );

		::sd::Outliner* pOutl = mpDoc->GetOutliner( FALSE );

		if( pOutl )
			pOutl->SetRefDevice( pRefDevice );

		::sd::Outliner* pInternalOutl = mpDoc->GetInternalOutliner( FALSE );

		if( pInternalOutl )
			pInternalOutl->SetRefDevice( pRefDevice );
	}
}

/*************************************************************************
|*
|* InitNew, (Dokument wird neu erzeugt): Streams oeffnen
|*
\************************************************************************/

BOOL DrawDocShell::InitNew( const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& xStorage )
{
	BOOL bRet = FALSE;

    bRet = SfxObjectShell::InitNew( xStorage );

	Rectangle aVisArea( Point(0, 0), Size(14100, 10000) );
	SetVisArea(aVisArea);

	if (bRet)
	{
		if( !mbSdDataObj )
			mpDoc->NewOrLoadCompleted(NEW_DOC);  // otherwise calling
			                                    // NewOrLoadCompleted(NEW_LOADED) in
												// SdDrawDocument::AllocModel()
	}
	return bRet;
}

/*************************************************************************
|*
|* Load: Pools und Dokument laden
|*
\************************************************************************/

sal_Bool DrawDocShell::IsNewDocument() const
{
    return( mbNewDocument &&
            ( !GetMedium() || GetMedium()->GetURLObject().GetProtocol() == INET_PROT_NOT_VALID ) );
}

/*************************************************************************
|*
|* Load: Pools und Dokument laden
|*
\************************************************************************/

BOOL DrawDocShell::Load( SfxMedium& rMedium )
{
    mbNewDocument = sal_False;

	BOOL	bRet = FALSE;
	bool	bStartPresentation = false;
    ErrCode nError = ERRCODE_NONE;

    SfxItemSet* pSet = rMedium.GetItemSet();


	if( pSet )
	{
		if( (  SFX_ITEM_SET == pSet->GetItemState(SID_PREVIEW ) ) && ( (SfxBoolItem&) ( pSet->Get( SID_PREVIEW ) ) ).GetValue() )
		{
			mpDoc->SetStarDrawPreviewMode( TRUE );
		}

		if( SFX_ITEM_SET == pSet->GetItemState(SID_DOC_STARTPRESENTATION)&&
			( (SfxBoolItem&) ( pSet->Get( SID_DOC_STARTPRESENTATION ) ) ).GetValue() )
		{
			bStartPresentation = true;
			mpDoc->SetStartWithPresentation( true );
		}
	}

    bRet = SfxObjectShell::Load( rMedium );
	if( bRet )
	{
        bRet = SdXMLFilter( rMedium, *this, sal_True, SDXMLMODE_Normal, SotStorage::GetVersion( rMedium.GetStorage() ) ).Import( nError );
	}

	if( bRet )
	{
		UpdateTablePointers();

        // #108451# If we're an embedded OLE object, use tight bounds
        // for our visArea. No point in showing the user lots of empty
        // space. Had to remove the check for empty VisArea below,
        // since XML load always sets a VisArea before.
        //TODO/LATER: looks a little bit strange!
        if( ( GetCreateMode() == SFX_CREATE_MODE_EMBEDDED ) && SfxObjectShell::GetVisArea( ASPECT_CONTENT ).IsEmpty() )
		{
			SdPage* pPage = mpDoc->GetSdPage( 0, PK_STANDARD );

			if( pPage )
				SetVisArea( Rectangle( pPage->GetAllObjBoundRect() ) );
		}

		FinishedLoading( SFX_LOADED_ALL );

        const INetURLObject aUrl;
        SfxObjectShell::SetAutoLoad( aUrl, 0, sal_False );
	}
	else
	{
        if( nError == ERRCODE_IO_BROKENPACKAGE )
            SetError( ERRCODE_IO_BROKENPACKAGE, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( OSL_LOG_PREFIX ) ) );

        // TODO/LATER: correct error handling?!
        //pStore->SetError( SVSTREAM_WRONGVERSION, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( OSL_LOG_PREFIX ) ) );
        else
            SetError( ERRCODE_ABORT, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( OSL_LOG_PREFIX ) ) );
	}

	// tell SFX to change viewshell when in preview mode
	if( IsPreview() || bStartPresentation )
	{
		SfxItemSet *pMediumSet = GetMedium()->GetItemSet();
		if( pMediumSet )
			pMediumSet->Put( SfxUInt16Item( SID_VIEW_ID, bStartPresentation ? 1 : 5 ) );
	}

	return bRet;
}

/*************************************************************************
|*
|* LoadFrom: Inhalte fuer Organizer laden
|*
\************************************************************************/

BOOL DrawDocShell::LoadFrom( SfxMedium& rMedium )
{
    mbNewDocument = sal_False;

	WaitObject* pWait = NULL;
	if( mpViewShell )
		pWait = new WaitObject( (Window*) mpViewShell->GetActiveWindow() );

	BOOL bRet = FALSE;

        /*
        // #90691# return to old behaviour (before #80365#): construct own medium
        SfxMedium aMedium(xStorage);

		// #90691# for having a progress bar nonetheless for XML copy it
		// from the local DocShell medium (GetMedium()) to the constructed one
		SfxMedium* pLocalMedium = GetMedium();
		if(pLocalMedium)
		{
			SfxItemSet* pLocalItemSet = pLocalMedium->GetItemSet();
			SfxItemSet* pDestItemSet = aMedium.GetItemSet();

			if(pLocalItemSet && pDestItemSet)
			{
				const SfxUnoAnyItem* pItem = static_cast<
                    const SfxUnoAnyItem*>(
                        pLocalItemSet->GetItem(SID_PROGRESS_STATUSBAR_CONTROL));

				if(pItem)
				{
					pDestItemSet->Put(*pItem);
				}
			}
        }                           */

		mpDoc->NewOrLoadCompleted( NEW_DOC );
		mpDoc->CreateFirstPages();
		mpDoc->StopWorkStartupDelay();

        // TODO/LATER: nobody is interested in the error code?!
        ErrCode nError = ERRCODE_NONE;
        bRet = SdXMLFilter( rMedium, *this, sal_True, SDXMLMODE_Organizer, SotStorage::GetVersion( rMedium.GetStorage() ) ).Import( nError );


	// tell SFX to change viewshell when in preview mode
	if( IsPreview() )
	{
		SfxItemSet *pSet = GetMedium()->GetItemSet();

		if( pSet )
			pSet->Put( SfxUInt16Item( SID_VIEW_ID, 5 ) );
	}

	delete pWait;

	return bRet;
}

/*************************************************************************
|*
|* ConvertFrom: aus Fremdformat laden
|*
\************************************************************************/

BOOL DrawDocShell::ConvertFrom( SfxMedium& rMedium )
{
    mbNewDocument = sal_False;

	const String	aFilterName( rMedium.GetFilter()->GetFilterName() );
	BOOL			bRet = FALSE;
	bool	bStartPresentation = false;

	SetWaitCursor( TRUE );

    SfxItemSet* pSet = rMedium.GetItemSet();
	if( pSet )
	{
		if( (  SFX_ITEM_SET == pSet->GetItemState(SID_PREVIEW ) ) && ( (SfxBoolItem&) ( pSet->Get( SID_PREVIEW ) ) ).GetValue() )
		{
			mpDoc->SetStarDrawPreviewMode( TRUE );
		}

		if( SFX_ITEM_SET == pSet->GetItemState(SID_DOC_STARTPRESENTATION)&&
			( (SfxBoolItem&) ( pSet->Get( SID_DOC_STARTPRESENTATION ) ) ).GetValue() )
		{
			bStartPresentation = true;
			mpDoc->SetStartWithPresentation( true );
		}
	}

	if( aFilterName == pFilterPowerPoint97 || aFilterName == pFilterPowerPoint97Template)
	{
		mpDoc->StopWorkStartupDelay();
        bRet = SdPPTFilter( rMedium, *this, sal_True ).Import();
	}
	else if (aFilterName.SearchAscii("impress8" )  != STRING_NOTFOUND ||
		     aFilterName.SearchAscii("draw8")  != STRING_NOTFOUND )
	{
        // TODO/LATER: nobody is interested in the error code?!
		mpDoc->CreateFirstPages();
		mpDoc->StopWorkStartupDelay();
        ErrCode nError = ERRCODE_NONE;
        bRet = SdXMLFilter( rMedium, *this, sal_True ).Import( nError );

	}
	else if (aFilterName.SearchAscii("StarOffice XML (Draw)" )  != STRING_NOTFOUND || aFilterName.SearchAscii("StarOffice XML (Impress)")  != STRING_NOTFOUND )
	{
        // TODO/LATER: nobody is interested in the error code?!
		mpDoc->CreateFirstPages();
		mpDoc->StopWorkStartupDelay();
        ErrCode nError = ERRCODE_NONE;
        bRet = SdXMLFilter( rMedium, *this, sal_True, SDXMLMODE_Normal, SOFFICE_FILEFORMAT_60 ).Import( nError );
	}
	else if( aFilterName.EqualsAscii( "CGM - Computer Graphics Metafile" ) )
	{
		mpDoc->CreateFirstPages();
		mpDoc->StopWorkStartupDelay();
        bRet = SdCGMFilter( rMedium, *this, sal_True ).Import();
	}
	else
	{
		mpDoc->CreateFirstPages();
		mpDoc->StopWorkStartupDelay();
        bRet = SdGRFFilter( rMedium, *this ).Import();
	}

	FinishedLoading( SFX_LOADED_MAINDOCUMENT | SFX_LOADED_IMAGES );

	// tell SFX to change viewshell when in preview mode
	if( IsPreview() )
	{
		SfxItemSet *pMediumSet = GetMedium()->GetItemSet();

		if( pMediumSet )
			pMediumSet->Put( SfxUInt16Item( SID_VIEW_ID, 5 ) );
	}
	SetWaitCursor( FALSE );

	// tell SFX to change viewshell when in preview mode
	if( IsPreview() || bStartPresentation )
	{
		SfxItemSet *pMediumSet = GetMedium()->GetItemSet();
		if( pMediumSet )
			pMediumSet->Put( SfxUInt16Item( SID_VIEW_ID, bStartPresentation ? 1 : 5 ) );
	}

	return bRet;
}

/*************************************************************************
|*
|* Save: Pools und Dokument in die offenen Streams schreiben
|*
\************************************************************************/

BOOL DrawDocShell::Save()
{
	mpDoc->StopWorkStartupDelay();

    //TODO/LATER: why this?!
	if( GetCreateMode() == SFX_CREATE_MODE_STANDARD )
        SfxObjectShell::SetVisArea( Rectangle() );

    BOOL bRet = SfxObjectShell::Save();

	if( bRet )
	{
		// #86834# Call UpdateDocInfoForSave() before export
		UpdateDocInfoForSave();

        bRet = SdXMLFilter( *GetMedium(), *this, sal_True, SDXMLMODE_Normal, SotStorage::GetVersion( GetMedium()->GetStorage() ) ).Export();
	}

	return bRet;
}

/*************************************************************************
|*
|* SaveAs: Pools und Dokument in den angegebenen Storage sichern
|*
\************************************************************************/

BOOL DrawDocShell::SaveAs( SfxMedium& rMedium )
{
	mpDoc->StopWorkStartupDelay();

    //TODO/LATER: why this?!
	if( GetCreateMode() == SFX_CREATE_MODE_STANDARD )
        SfxObjectShell::SetVisArea( Rectangle() );

	UINT32	nVBWarning = ERRCODE_NONE;
    BOOL    bRet = SfxObjectShell::SaveAs( rMedium );

	if( bRet )
	{
        // #86834# Call UpdateDocInfoForSave() before export
        UpdateDocInfoForSave();
        bRet = SdXMLFilter( rMedium, *this, sal_True, SDXMLMODE_Normal, SotStorage::GetVersion( rMedium.GetStorage() ) ).Export();
    }

	if( GetError() == ERRCODE_NONE )
		SetError( nVBWarning, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( OSL_LOG_PREFIX ) ) );

	return bRet;
}

/*************************************************************************
|*
|* ConvertTo: im Fremdformat speichern
|*
\************************************************************************/

BOOL DrawDocShell::ConvertTo( SfxMedium& rMedium )
{
	BOOL bRet = FALSE;

	if( mpDoc->GetPageCount() )
	{
		const SfxFilter*	pMediumFilter = rMedium.GetFilter();
		const String		aTypeName( pMediumFilter->GetTypeName() );
		SdFilter*			pFilter = NULL;

		if( aTypeName.SearchAscii( "graphic_HTML" ) != STRING_NOTFOUND )
		{
			pFilter = new SdHTMLFilter( rMedium, *this, sal_True );
		}
		else if( aTypeName.SearchAscii( "MS_PowerPoint_97" ) != STRING_NOTFOUND )
		{
			pFilter = new SdPPTFilter( rMedium, *this, sal_True );
			((SdPPTFilter*)pFilter)->PreSaveBasic();
		}
		else if ( aTypeName.SearchAscii( "CGM_Computer_Graphics_Metafile" ) != STRING_NOTFOUND )
		{
			pFilter = new SdCGMFilter( rMedium, *this, sal_True );
		}
		else if( ( aTypeName.SearchAscii( "draw8" ) != STRING_NOTFOUND ) ||
				 ( aTypeName.SearchAscii( "impress8" ) != STRING_NOTFOUND ) )
		{
			pFilter = new SdXMLFilter( rMedium, *this, sal_True );
			UpdateDocInfoForSave();
		}
		else if( ( aTypeName.SearchAscii( "StarOffice_XML_Impress" ) != STRING_NOTFOUND ) ||
				 ( aTypeName.SearchAscii( "StarOffice_XML_Draw" ) != STRING_NOTFOUND ) )
		{
			pFilter = new SdXMLFilter( rMedium, *this, sal_True, SDXMLMODE_Normal, SOFFICE_FILEFORMAT_60 );
			UpdateDocInfoForSave();
		}
		else
		{
			pFilter = new SdGRFFilter( rMedium, *this );
		}

		if( pFilter )
		{
			const ULONG	nOldSwapMode = mpDoc->GetSwapGraphicsMode();

			mpDoc->SetSwapGraphicsMode( SDR_SWAPGRAPHICSMODE_TEMP );

			bRet = pFilter->Export();
			if( !bRet )
				mpDoc->SetSwapGraphicsMode( nOldSwapMode );

			delete pFilter;
		}
	}

	return  bRet;
}

/*************************************************************************
|*
|* SaveCompleted: die eigenen Streams wieder oeffnen, damit kein anderer
|*								  sie "besetzt"
|*
\************************************************************************/

BOOL DrawDocShell::SaveCompleted( const ::com::sun::star::uno::Reference< ::com::sun::star::embed::XStorage >& xStorage )
{
	BOOL bRet = FALSE;

    if( SfxObjectShell::SaveCompleted(xStorage) )
	{
		mpDoc->NbcSetChanged( FALSE );

		if( mpViewShell )
		{
			if( mpViewShell->ISA( OutlineViewShell ) )
				static_cast<OutlineView*>(mpViewShell->GetView())
                    ->GetOutliner()->ClearModifyFlag();

			SdrOutliner* pOutl = mpViewShell->GetView()->GetTextEditOutliner();
			if( pOutl )
			{
				SdrObject* pObj = mpViewShell->GetView()->GetTextEditObject();
				if( pObj )
					pObj->NbcSetOutlinerParaObject( pOutl->CreateParaObject() );

				pOutl->ClearModifyFlag();
			}
		}

		bRet = TRUE;

		SfxViewFrame* pFrame = ( mpViewShell && mpViewShell->GetViewFrame() ) ?
							   mpViewShell->GetViewFrame() :
							   SfxViewFrame::Current();

		if( pFrame )
			pFrame->GetBindings().Invalidate( SID_NAVIGATOR_STATE, TRUE, FALSE );
	}
	return bRet;
}

/*************************************************************************
|*
|* Referenz auf Dokument
|*
\************************************************************************/

SdDrawDocument* DrawDocShell::GetDoc()
{
	return mpDoc;
}

/*************************************************************************
|*
|* Referenz auf Dokument
|*
\************************************************************************/

SfxStyleSheetBasePool* DrawDocShell::GetStyleSheetPool()
{
	return( (SfxStyleSheetBasePool*) mpDoc->GetStyleSheetPool() );
}

/*************************************************************************
|*
|* Sprung zu Bookmark
|*
\************************************************************************/

BOOL DrawDocShell::GotoBookmark(const String& rBookmark)
{
	BOOL bFound = FALSE;

	if (mpViewShell && mpViewShell->ISA(DrawViewShell))
	{
		DrawViewShell* pDrawViewShell = static_cast<DrawViewShell*>(mpViewShell);
        ViewShellBase& rBase (mpViewShell->GetViewShellBase());

		BOOL bIsMasterPage = sal_False;
		USHORT nPageNumber = SDRPAGE_NOTFOUND;
		SdrObject* pObj = NULL;

		rtl::OUString sBookmark( rBookmark );
		const rtl::OUString sInteraction( RTL_CONSTASCII_USTRINGPARAM( "action?" ) );
		if ( sBookmark.match( sInteraction ) )
		{
			const rtl::OUString sJump( RTL_CONSTASCII_USTRINGPARAM( "jump=" ) );
			if ( sBookmark.match( sJump, sInteraction.getLength() ) )
			{
				rtl::OUString aDestination( sBookmark.copy( sInteraction.getLength() + sJump.getLength() ) );
				if ( aDestination.match( String( RTL_CONSTASCII_USTRINGPARAM( "firstslide" ) ) ) )
				{
					nPageNumber = 1;
				}
				else if ( aDestination.match( String( RTL_CONSTASCII_USTRINGPARAM( "lastslide" ) ) ) )
				{
					nPageNumber = mpDoc->GetPageCount() - 2;
				}
				else if ( aDestination.match( String( RTL_CONSTASCII_USTRINGPARAM( "previousslide" ) ) ) )
				{
                    SdPage* pPage = pDrawViewShell->GetActualPage();
                    nPageNumber = pPage->GetPageNum();
					nPageNumber = nPageNumber > 2 ? nPageNumber - 2 : SDRPAGE_NOTFOUND; 
				}
				else if ( aDestination.match( String( RTL_CONSTASCII_USTRINGPARAM( "nextslide" ) ) ) )
				{
                    SdPage* pPage = pDrawViewShell->GetActualPage();
                    nPageNumber = pPage->GetPageNum() + 2;
					if ( nPageNumber >= mpDoc->GetPageCount() )
						nPageNumber = SDRPAGE_NOTFOUND; 
				}
			}
		}
		else
		{
			String aBookmark( rBookmark );

			// Ist das Bookmark eine Seite?
			nPageNumber = mpDoc->GetPageByName( aBookmark, bIsMasterPage );

			if (nPageNumber == SDRPAGE_NOTFOUND)
			{
				// Ist das Bookmark ein Objekt?
				pObj = mpDoc->GetObj(aBookmark);

				if (pObj)
				{
					nPageNumber = pObj->GetPage()->GetPageNum();
				}
			}
		}
		if (nPageNumber != SDRPAGE_NOTFOUND)
		{
			// Jump to the bookmarked page.  This is done in three steps.

			bFound = TRUE;
			SdPage* pPage;
            if (bIsMasterPage)
                pPage = (SdPage*) mpDoc->GetMasterPage(nPageNumber);
            else
                pPage = (SdPage*) mpDoc->GetPage(nPageNumber);

            // 1.) Change the view shell to the edit view, the notes view,
            // or the handout view.
			PageKind eNewPageKind = pPage->GetPageKind();

			if( (eNewPageKind != PK_STANDARD) && (mpDoc->GetDocumentType() == DOCUMENT_TYPE_DRAW) )
				return FALSE;

			if (eNewPageKind != pDrawViewShell->GetPageKind())
			{
				// Arbeitsbereich wechseln
				GetFrameView()->SetPageKind(eNewPageKind);
                ::rtl::OUString sViewURL;
                switch (eNewPageKind)
                {
                    case PK_STANDARD: 
                        sViewURL = FrameworkHelper::msImpressViewURL;
                        break;
                    case PK_NOTES:
                        sViewURL = FrameworkHelper::msNotesViewURL;
                        break;
                    case PK_HANDOUT:
                        sViewURL = FrameworkHelper::msHandoutViewURL;
                        break;
                    default:
                        break;
                }
                if (sViewURL.getLength() > 0)
                {
                    ::boost::shared_ptr<FrameworkHelper> pHelper (
                        FrameworkHelper::Instance(rBase));
                    pHelper->RequestView(
                        sViewURL,
                        FrameworkHelper::msCenterPaneURL);
                    pHelper->WaitForUpdate();

                    // Get the new DrawViewShell.
                    mpViewShell = pHelper->GetViewShell(FrameworkHelper::msCenterPaneURL).get();
                    pDrawViewShell = dynamic_cast<sd::DrawViewShell*>(mpViewShell);
                }
                else
                {
                    pDrawViewShell = NULL;
                }
            }

            if (pDrawViewShell != NULL)
            {
                // Set the edit mode to either the normal edit mode or the
                // master page mode.
                EditMode eNewEditMode = EM_PAGE;
                if (bIsMasterPage)
                {
                    eNewEditMode = EM_MASTERPAGE;
                }

                if (eNewEditMode != pDrawViewShell->GetEditMode())
                {
                    // EditMode setzen
                    pDrawViewShell->ChangeEditMode(eNewEditMode, FALSE);
                }

                // Make the bookmarked page the current page.  This is done
                // by using the API because this takes care of all the
                // little things to be done.  Especially writing the view
                // data to the frame view (see bug #107803#).
                USHORT nSdPgNum = (nPageNumber - 1) / 2;
                Reference<drawing::XDrawView> xController (rBase.GetController(), UNO_QUERY);
                if (xController.is())
                {
                    Reference<drawing::XDrawPage> xDrawPage (pPage->getUnoPage(), UNO_QUERY);
                    xController->setCurrentPage (xDrawPage);
                }
                else
                {
                    // As a fall back switch to the page via the core.
                    DBG_ASSERT (xController.is(),
                        "DrawDocShell::GotoBookmark: can't switch page via API");
                    pDrawViewShell->SwitchPage(nSdPgNum);
                }

                if (pObj != NULL)
                {
                    // Objekt einblenden und selektieren
                    pDrawViewShell->MakeVisible(pObj->GetLogicRect(),
                        *pDrawViewShell->GetActiveWindow());
                    pDrawViewShell->GetView()->UnmarkAll();
                    pDrawViewShell->GetView()->MarkObj(
                        pObj,
                        pDrawViewShell->GetView()->GetSdrPageView(), FALSE);
                }
            }
        }
        
        SfxBindings& rBindings = (pDrawViewShell->GetViewFrame()!=NULL
            ? pDrawViewShell->GetViewFrame()
            : SfxViewFrame::Current() )->GetBindings();
                
        rBindings.Invalidate(SID_NAVIGATOR_STATE, TRUE, FALSE);
        rBindings.Invalidate(SID_NAVIGATOR_PAGENAME);
    }

	return (bFound);
}

/*************************************************************************
|*
|* SaveAsOwnFormat: wenn es eine Dokumentvorlage werden soll,
|*
\************************************************************************/
#include <tools/urlobj.hxx>

BOOL DrawDocShell::SaveAsOwnFormat( SfxMedium& rMedium )
{

	const SfxFilter* pFilter = rMedium.GetFilter();

	if (pFilter->IsOwnTemplateFormat())
	{
		// jetzt die StarDraw-Spezialitaeten:
		// die Layoutvorlagen der ersten Seite werden mit dem jetzt
		// bekannten Layoutnamen versehen, die Layoutnamen der betroffenen
		// Masterpages und Seiten werden gesetzt;
		// alle Textobjekte der betroffenen Standard-, Notiz- und
		// Masterpages werden ueber die Namensaenderung informiert

		String aLayoutName;

		SfxStringItem* pLayoutItem;
		if( rMedium.GetItemSet()->GetItemState(SID_TEMPLATE_NAME, FALSE, (const SfxPoolItem**) & pLayoutItem ) == SFX_ITEM_SET )
		{
			aLayoutName = pLayoutItem->GetValue();
		}
		else
		{
			INetURLObject aURL( rMedium.GetName() );
			aURL.removeExtension();
			aLayoutName = aURL.getName();
		}

		if( aLayoutName.Len() )
		{
			String aOldPageLayoutName = mpDoc->GetSdPage(0, PK_STANDARD)->GetLayoutName();
			mpDoc->RenameLayoutTemplate(aOldPageLayoutName, aLayoutName);
		}
	}

	return SfxObjectShell::SaveAsOwnFormat(rMedium);
}

/*************************************************************************
|*
|* FillClass
|*
\************************************************************************/

void DrawDocShell::FillClass(SvGlobalName* pClassName,
										sal_uInt32*  pFormat,
										String* ,
										String* pFullTypeName,
                                        String* pShortTypeName,
										sal_Int32 nFileFormat,
										sal_Bool bTemplate /* = sal_False */) const
{
	if (nFileFormat == SOFFICE_FILEFORMAT_60)
	{
        if ( meDocType == DOCUMENT_TYPE_DRAW )
        {
                *pClassName = SvGlobalName(SO3_SDRAW_CLASSID_60);
                *pFormat = SOT_FORMATSTR_ID_STARDRAW_60;
                *pFullTypeName = String(SdResId(STR_GRAPHIC_DOCUMENT_FULLTYPE_60));
        }
        else
        {
                *pClassName = SvGlobalName(SO3_SIMPRESS_CLASSID_60);
                *pFormat = SOT_FORMATSTR_ID_STARIMPRESS_60;
                *pFullTypeName = String(SdResId(STR_IMPRESS_DOCUMENT_FULLTYPE_60));
        }
	}
	else if (nFileFormat == SOFFICE_FILEFORMAT_8)
	{
        if ( meDocType == DOCUMENT_TYPE_DRAW )
        {
                *pClassName = SvGlobalName(SO3_SDRAW_CLASSID_60);
                *pFormat = bTemplate ? SOT_FORMATSTR_ID_STARDRAW_8_TEMPLATE : SOT_FORMATSTR_ID_STARDRAW_8;
                *pFullTypeName = String(RTL_CONSTASCII_USTRINGPARAM("Draw 8"));	// HACK: method will be removed with new storage API
        }
        else
        {
                *pClassName = SvGlobalName(SO3_SIMPRESS_CLASSID_60);
                *pFormat = bTemplate ? SOT_FORMATSTR_ID_STARIMPRESS_8_TEMPLATE : SOT_FORMATSTR_ID_STARIMPRESS_8;
                *pFullTypeName = String(RTL_CONSTASCII_USTRINGPARAM("Impress 8")); // HACK: method will be removed with new storage API
        }
	}

	*pShortTypeName = String(SdResId( (meDocType == DOCUMENT_TYPE_DRAW) ?
									  STR_GRAPHIC_DOCUMENT : STR_IMPRESS_DOCUMENT ));
}

OutputDevice* DrawDocShell::GetDocumentRefDev (void)
{
    OutputDevice* pReferenceDevice = SfxObjectShell::GetDocumentRefDev ();
    // Only when our parent does not have a reference device then we return
    // our own.
    if (pReferenceDevice == NULL && mpDoc != NULL)
        pReferenceDevice = mpDoc->GetRefDevice ();
    return pReferenceDevice;
}

/** executes the SID_OPENDOC slot to let the framework open a document
	with the given URL and this document as a referer */
void DrawDocShell::OpenBookmark( const String& rBookmarkURL )
{
    SfxStringItem   aStrItem( SID_FILE_NAME, rBookmarkURL );
    SfxStringItem   aReferer( SID_REFERER, GetMedium()->GetName() );
	const SfxPoolItem* ppArgs[] = { &aStrItem, &aReferer, 0 };
	( mpViewShell ? mpViewShell->GetViewFrame() : SfxViewFrame::Current() )->GetBindings().Execute( SID_OPENHYPERLINK, ppArgs );
}

} // end of namespace sd
