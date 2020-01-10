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
#include <svx/eeitem.hxx>
#include <svx/flditem.hxx>
#include <sfx2/printer.hxx>
#include <svtools/inethist.hxx>
#include <svtools/poolitem.hxx>
#include <svtools/flagitem.hxx>
#include <svtools/useroptions.hxx>
#include <sfx2/bindings.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/request.hxx>

#include <svx/svdfield.hxx>
#include <svx/editstat.hxx>
#include <svx/editeng.hxx>

#include <svx/dialogs.hrc>
#include <svx/svdotext.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdopage.hxx>

#include <sfx2/sfxdlg.hxx>


#include <svx/sdr/contact/displayinfo.hxx>

#define _SD_DLL                 // fuer SD_MOD()
#include "sdmod.hxx"
#include "sddll.hxx"
#include "app.hrc"
#include "glob.hrc"
#include "strings.hrc"
#include "res_bmp.hrc"
#include "ViewShell.hxx"
#ifndef SD_FRAMW_VIEW_HXX
#include "FrameView.hxx"
#endif
#include "sdattr.hxx"
#include "tpoption.hrc"
#include "optsitem.hxx"
#include "DrawDocShell.hxx"
#include "drawdoc.hxx"
#include "Outliner.hxx"
#include "sdresid.hxx"
#include "pres.hxx"
#include "DrawViewShell.hxx"
#include "OutlineViewShell.hxx"
#include "OutlineView.hxx"
#include "ViewShellBase.hxx"
#include "sdpage.hxx"
#include "sdxfer.hxx"
#include "sdabstdlg.hxx"
#include "tpoption.hrc"
#include "prntopts.hrc"
#include <svtools/intitem.hxx>

/** retrieves the page that is currently painted. This will only be the master page
	if the current drawn view only shows the master page*/
static SdPage* GetCurrentPage( sd::ViewShell* pViewSh, EditFieldInfo* pInfo, bool& bMasterView )
{
	if( !pInfo )
		return 0;

	bMasterView = false;
	SdPage* pPage = dynamic_cast< SdPage* >( pInfo->GetSdrPage() );
	SdrOutliner* pOutliner = dynamic_cast< SdrOutliner* >( pInfo->GetOutliner() );

	// special case, someone already set the current page on the EditFieldInfo
	// This is used from the svx::UnoGraphicsExporter f.e.
	if( pPage )
	{
		bMasterView = false;
		return pPage;
	}

	// first try to check if we are inside the outline view
	sd::OutlineView* pSdView = NULL;
	if( pViewSh && pViewSh->ISA(sd::OutlineViewShell))
		pSdView = static_cast<sd::OutlineView*> (static_cast<sd::OutlineViewShell*>(pViewSh)->GetView());

	if (pSdView != NULL && (pOutliner ==  pSdView->GetOutliner()))
    {
        // outline mode
        int nPgNum = 0;
        Outliner* pOutl = pSdView->GetOutliner();
        long nPos = pInfo->GetPara();
        ULONG nParaPos = 0;

        for( Paragraph* pPara = pOutl->GetParagraph( 0 ); pPara && nPos >= 0; pPara = pOutl->GetParagraph( ++nParaPos ), nPos-- )
        {
            if( pOutl->HasParaFlag( pPara, PARAFLAG_ISPAGE ) )
                nPgNum++;
        }

		pPage = pViewSh->GetDoc()->GetSdPage( (USHORT)nPgNum, PK_STANDARD );
	}
	else
	{
		// draw mode, slide mode and preview. Get the processed page from the outliner
		if(pOutliner)
		{
			pPage = dynamic_cast< SdPage* >(const_cast< SdrPage* >(pOutliner->getVisualizedPage()));
		}

		// The path using GetPaintingPageView() and GetCurrentPaintingDisplayInfo()
		// is no longer needed. I debugged and checked all usages of PageNumber decompositions
		// which all use the new possibility of setting the visualized page at the SdrOutliner.

		// if all else failed, geht the current page from the object that is
		// currently formated from the document
		if(!pPage)
		{
			const SdrTextObj* pTextObj = (pViewSh && pViewSh->GetDoc()) ? pViewSh->GetDoc()->GetFormattingTextObj() : NULL;

			if( pTextObj )
			{
				pPage = dynamic_cast< SdPage* >( pTextObj->GetPage() );
			}
		}

		if(pPage)
		{
			bMasterView = pPage && pPage->IsMasterPage();
		}
	}

	return pPage;
}

/*************************************************************************
|*
|* Link fuer CalcFieldValue des Outliners
|*
\************************************************************************/

IMPL_LINK(SdModule, CalcFieldValueHdl, EditFieldInfo*, pInfo)
{
	if (pInfo)
	{
		const SvxFieldData* pField = pInfo->GetField().GetField();
        ::sd::DrawDocShell*     pDocShell = NULL;
		SdDrawDocument* pDoc = 0;

        if( pInfo->GetOutliner() )
        {
            const SdrTextObj* pTextObj = static_cast< SdrOutliner* >( pInfo->GetOutliner() )->GetTextObj();

			if( pTextObj )
				pDoc = dynamic_cast< SdDrawDocument* >( pTextObj->GetModel() );

			if( pDoc )
                pDocShell = pDoc->GetDocSh();
        }

        if( !pDocShell )
            pDocShell = dynamic_cast< ::sd::DrawDocShell *>( SfxObjectShell::Current() );

		const SvxDateField* pDateField = 0;
		const SvxExtTimeField* pExtTimeField = 0;
		const SvxExtFileField* pExtFileField = 0;
		const SvxAuthorField* pAuthorField = 0;
		const SvxURLField* pURLField = 0;

		if( (pDateField = dynamic_cast< const SvxDateField* >(pField)) != 0 )
		{
			LanguageType eLang = pInfo->GetOutliner()->GetLanguage( pInfo->GetPara(), pInfo->GetPos() );
			pInfo->SetRepresentation( pDateField->GetFormatted( *GetNumberFormatter(), eLang ) );
		}
		else if( (pExtTimeField = dynamic_cast< const SvxExtTimeField *>(pField)) != 0 )
		{
			LanguageType eLang = pInfo->GetOutliner()->GetLanguage( pInfo->GetPara(), pInfo->GetPos() );
			pInfo->SetRepresentation( pExtTimeField->GetFormatted( *GetNumberFormatter(), eLang ) );
		}
		else if( (pExtFileField = dynamic_cast< const SvxExtFileField * >(pField)) != 0 )
		{
			if( pDocShell && (pExtFileField->GetType() != SVXFILETYPE_FIX) )
			{
				String aName;
				if( pDocShell->HasName() )
					aName = pDocShell->GetMedium()->GetName();
				else
					aName = pDocShell->GetName();

				const_cast< SvxExtFileField* >(pExtFileField)->SetFile( aName );
			}
			pInfo->SetRepresentation( pExtFileField->GetFormatted() );

		}
		else if( (pAuthorField = dynamic_cast< const SvxAuthorField* >( pField )) != 0  )
		{
			if( pAuthorField->GetType() != SVXAUTHORTYPE_FIX )
			{
                SvtUserOptions aUserOptions;
                SvxAuthorField aAuthorField(
                        aUserOptions.GetFirstName(), aUserOptions.GetLastName(), aUserOptions.GetID(),
                        pAuthorField->GetType(), pAuthorField->GetFormat() );

                *(const_cast< SvxAuthorField* >(pAuthorField)) = aAuthorField;
			}
			pInfo->SetRepresentation( pAuthorField->GetFormatted() );

		}
		else if( dynamic_cast< const SvxPageField*  >(pField) )
		{
			String aRepresentation;
			aRepresentation += sal_Unicode( ' ' );

			::sd::ViewShell* pViewSh = pDocShell ? pDocShell->GetViewShell() : NULL;
			if(pViewSh == NULL)
            {
                ::sd::ViewShellBase* pBase = PTR_CAST(::sd::ViewShellBase, SfxViewShell::Current());
                if(pBase)
                    pViewSh = pBase->GetMainViewShell().get();
            }
			if( !pDoc && pViewSh )
				pDoc = pViewSh->GetDoc();

			bool bMasterView;
			SdPage* pPage = GetCurrentPage( pViewSh, pInfo, bMasterView );

			if( pPage && pDoc && !bMasterView )
			{
				int nPgNum;

				if( (pPage->GetPageKind() == PK_HANDOUT) && pViewSh )
				{
					nPgNum = pViewSh->GetPrintedHandoutPageNum();
				}
				else
				{
					nPgNum = (pPage->GetPageNum() - 1) / 2 + 1;
				}
				aRepresentation = pDoc->CreatePageNumValue((USHORT)nPgNum);
			}
			else
			{
				static String aNumberText( SdResId( STR_FIELD_PLACEHOLDER_NUMBER ) );
				aRepresentation = aNumberText;
			}

			pInfo->SetRepresentation( aRepresentation );
		}
		else if( dynamic_cast< const SvxPagesField*  >(pField) )
		{
			String aRepresentation;
			aRepresentation += sal_Unicode( ' ' );

			::sd::ViewShell* pViewSh = pDocShell ? pDocShell->GetViewShell() : NULL;
			if(pViewSh == NULL)
            {
                ::sd::ViewShellBase* pBase = PTR_CAST(::sd::ViewShellBase, SfxViewShell::Current());
                if(pBase)
                    pViewSh = pBase->GetMainViewShell().get();
            }
			if( !pDoc && pViewSh )
				pDoc = pViewSh->GetDoc();

			bool bMasterView;
			SdPage* pPage = GetCurrentPage( pViewSh, pInfo, bMasterView );

            USHORT nPageCount = 0;

            if( !bMasterView )
            {
			    if( pPage && (pPage->GetPageKind() == PK_HANDOUT) && pViewSh )
			    {
				    nPageCount = pViewSh->GetPrintedHandoutPageCount();
			    }
			    else if( pDoc )
			    {
                    nPageCount = (USHORT)pDoc->GetSdPageCount(PK_STANDARD);
                }
            }

            if( nPageCount > 0 )
            {
				aRepresentation = pDoc->CreatePageNumValue(nPageCount);
			}
			else
			{
				static String aNumberText( SdResId( STR_FIELD_PLACEHOLDER_COUNT ) );
				aRepresentation = aNumberText;
			}

			pInfo->SetRepresentation( aRepresentation );
		}
		else if( (pURLField = dynamic_cast< const SvxURLField* >(pField)) != 0 )
		{
			switch ( pURLField->GetFormat() )
			{
				case SVXURLFORMAT_APPDEFAULT: //!!! einstellbar an App???
				case SVXURLFORMAT_REPR:
					pInfo->SetRepresentation( pURLField->GetRepresentation() );
					break;

				case SVXURLFORMAT_URL:
					pInfo->SetRepresentation( pURLField->GetURL() );
					break;
			}

			String aURL = pURLField->GetURL();

            svtools::ColorConfig aConfig;
            svtools::ColorConfigEntry eEntry =
                INetURLHistory::GetOrCreate()->QueryUrl( aURL ) ? svtools::LINKSVISITED : svtools::LINKS;
			pInfo->SetTxtColor( aConfig.GetColorValue(eEntry).nColor );
		}
		else if ( dynamic_cast< const SdrMeasureField* >(pField))
		{
			pInfo->ClearFldColor();
		}
		else
		{
			String aRepresentation;

			bool bHeaderField = dynamic_cast< const SvxHeaderField* >( pField ) != 0;
			bool bFooterField = !bHeaderField && (dynamic_cast< const SvxFooterField* >( pField ) != 0 );
			bool bDateTimeField = !bHeaderField && !bFooterField && (dynamic_cast< const SvxDateTimeField* >( pField ) != 0);

			if( bHeaderField || bFooterField || bDateTimeField )
			{
				sd::ViewShell* pViewSh = pDocShell ? pDocShell->GetViewShell() : NULL;
				bool bMasterView = false;
				SdPage* pPage = GetCurrentPage( pViewSh, pInfo, bMasterView );

				if( (pPage == NULL) || bMasterView )
				{
					if( bHeaderField )
					{
						static String aHeaderStr( SdResId( STR_FIELD_PLACEHOLDER_HEADER ) );
						aRepresentation = aHeaderStr;
					}
					else if (bFooterField )
					{
						static String aFooterStr( SdResId( STR_FIELD_PLACEHOLDER_FOOTER ) );
						aRepresentation = aFooterStr;
					}
					else if (bDateTimeField )
					{
						static String aDateTimeStr( SdResId( STR_FIELD_PLACEHOLDER_DATETIME ) );
						aRepresentation = aDateTimeStr;
					}
				}
				else
				{
					const sd::HeaderFooterSettings &rSettings = pPage->getHeaderFooterSettings();

					if( bHeaderField )
					{
						aRepresentation = rSettings.maHeaderText;
					}
					else if( bFooterField )
					{
						aRepresentation = rSettings.maFooterText;
					}
					else if( bDateTimeField )
					{
						if( rSettings.mbDateTimeIsFixed )
						{
							aRepresentation = rSettings.maDateTimeText;
						}
						else
						{
							Date aDate;
							Time aTime;
							LanguageType eLang = pInfo->GetOutliner()->GetLanguage( pInfo->GetPara(), pInfo->GetPos() );
							aRepresentation = SvxDateTimeField::GetFormatted( aDate, aTime, (SvxDateFormat)rSettings.meDateTimeFormat, *GetNumberFormatter(), eLang );
						}
					}
				}
			}
			else
			{
				DBG_ERROR("sd::SdModule::CalcFieldValueHdl(), unknown field type!");
			}

			if( aRepresentation.Len() == 0 )				// TODO: Edit engine doesn't handle empty fields?
				aRepresentation += sal_Unicode( ' ' );
			pInfo->SetRepresentation( aRepresentation );
		}
	}

	return(0);
}



/*************************************************************************
|*
|* virt. Methoden fuer Optionendialog
|*
\************************************************************************/
SfxItemSet*	 SdModule::CreateItemSet( USHORT nSlot )
{
	::sd::FrameView* pFrameView = NULL;
	::sd::DrawDocShell* pDocSh = PTR_CAST(::sd::DrawDocShell, SfxObjectShell::Current() );
	SdDrawDocument* pDoc = NULL;

	// Hier wird der DocType vom Optionsdialog gesetzt (nicht Dokument!)
	DocumentType eDocType = DOCUMENT_TYPE_IMPRESS;
	if( nSlot == SID_SD_GRAPHIC_OPTIONS )
		eDocType = DOCUMENT_TYPE_DRAW;

	::sd::ViewShell* pViewShell = NULL;

	if (pDocSh)
	{
		pDoc = pDocSh->GetDoc();

		// Wenn der Optionsdialog zum Dokumenttyp identisch ist,
		// kann auch die FrameView mit uebergeben werden:
		if( pDoc && eDocType == pDoc->GetDocumentType() )
			pFrameView = pDocSh->GetFrameView();

		pViewShell = pDocSh->GetViewShell();
		if (pViewShell != NULL)
            pViewShell->WriteFrameViewData();
	}

	SdOptions* pOptions = GetSdOptions(eDocType);

	// Pool hat standardmaessig MapUnit Twips (Baeh!)
	SfxItemPool& rPool = GetPool();
	rPool.SetDefaultMetric( SFX_MAPUNIT_100TH_MM );

	SfxItemSet*	 pRet = new SfxItemSet( rPool,
						SID_ATTR_METRIC, SID_ATTR_METRIC,
						SID_ATTR_DEFTABSTOP, SID_ATTR_DEFTABSTOP,

						ATTR_OPTIONS_LAYOUT, ATTR_OPTIONS_LAYOUT,
						ATTR_OPTIONS_CONTENTS, ATTR_OPTIONS_CONTENTS,
						ATTR_OPTIONS_MISC, ATTR_OPTIONS_MISC,

						ATTR_OPTIONS_SNAP, ATTR_OPTIONS_SNAP,

						ATTR_OPTIONS_SCALE_START, ATTR_OPTIONS_SCALE_END,

						ATTR_OPTIONS_PRINT, ATTR_OPTIONS_PRINT,

						SID_ATTR_GRID_OPTIONS, SID_ATTR_GRID_OPTIONS,
						0 );

	// TP_OPTIONS_LAYOUT:
	pRet->Put( SdOptionsLayoutItem( ATTR_OPTIONS_LAYOUT, pOptions, pFrameView ) );

	UINT16 nDefTab = 0;
	if( pFrameView)
		nDefTab = pDoc->GetDefaultTabulator();
	else
		nDefTab = pOptions->GetDefTab();
	pRet->Put( SfxUInt16Item( SID_ATTR_DEFTABSTOP, nDefTab ) );

	FieldUnit nMetric = (FieldUnit)0xffff;
	if( pFrameView)
		nMetric = pDoc->GetUIUnit();
	else
		nMetric = (FieldUnit)pOptions->GetMetric();

	if( nMetric == (FieldUnit)0xffff )
		nMetric = GetModuleFieldUnit();

	pRet->Put( SfxUInt16Item( SID_ATTR_METRIC, (UINT16)nMetric ) );

	// TP_OPTIONS_CONTENTS:
	pRet->Put( SdOptionsContentsItem( ATTR_OPTIONS_CONTENTS, pOptions, pFrameView ) );

	// TP_OPTIONS_MISC:
    SdOptionsMiscItem aSdOptionsMiscItem( ATTR_OPTIONS_MISC, pOptions, pFrameView );
    if ( pFrameView )
    {
        aSdOptionsMiscItem.GetOptionsMisc().SetSummationOfParagraphs( pDoc->IsSummationOfParagraphs() );
        aSdOptionsMiscItem.GetOptionsMisc().SetPrinterIndependentLayout (
            (USHORT)pDoc->GetPrinterIndependentLayout());
    }
	pRet->Put( aSdOptionsMiscItem );


	// TP_OPTIONS_SNAP:
	pRet->Put( SdOptionsSnapItem( ATTR_OPTIONS_SNAP, pOptions, pFrameView ) );

	// TP_SCALE:
	UINT32 nW = 10L;
	UINT32 nH = 10L;
	INT32  nX;
	INT32  nY;
	if( pDocSh )
	{
		SdrPage* pPage = (SdrPage*) pDoc->GetSdPage(0, PK_STANDARD);
		Size aSize(pPage->GetSize());
		nW = aSize.Width();
		nH = aSize.Height();
	}

	if(pFrameView)
	{
		const Fraction& rFraction =  pDoc->GetUIScale();
		nX=rFraction.GetNumerator();
		nY=rFraction.GetDenominator();
	}
	else
	{
		// Optionen aus Configdatei holen
		pOptions->GetScale( nX, nY );
	}

	pRet->Put( SfxInt32Item( ATTR_OPTIONS_SCALE_X, nX ) );
	pRet->Put( SfxInt32Item( ATTR_OPTIONS_SCALE_Y, nY ) );
	pRet->Put( SfxUInt32Item( ATTR_OPTIONS_SCALE_WIDTH, nW ) );
	pRet->Put( SfxUInt32Item( ATTR_OPTIONS_SCALE_HEIGHT, nH ) );


	// TP_OPTIONS_PRINT:
	pRet->Put( SdOptionsPrintItem( ATTR_OPTIONS_PRINT, pOptions ) );

	// RID_SVXPAGE_GRID:
	pRet->Put( SdOptionsGridItem( SID_ATTR_GRID_OPTIONS, pOptions ) );

	return pRet;
}
void SdModule::ApplyItemSet( USHORT nSlot, const SfxItemSet& rSet )
{
	const SfxPoolItem*	pItem = NULL;
	BOOL bNewDefTab = FALSE;
	BOOL bNewPrintOptions = FALSE;
	BOOL bMiscOptions = FALSE;

	::sd::FrameView* pFrameView = NULL;
	::sd::DrawDocShell* pDocSh = PTR_CAST(::sd::DrawDocShell, SfxObjectShell::Current() );
	SdDrawDocument* pDoc = NULL;
	// Hier wird der DocType vom Optionsdialog gesetzt (nicht Dokument!)
	DocumentType eDocType = DOCUMENT_TYPE_IMPRESS;
	if( nSlot == SID_SD_GRAPHIC_OPTIONS )
		eDocType = DOCUMENT_TYPE_DRAW;

	::sd::ViewShell* pViewShell = NULL;

	if (pDocSh)
	{
		pDoc = pDocSh->GetDoc();

		// Wenn der Optionsdialog zum Dokumenttyp identisch ist,
		// kann auch die FrameView mit uebergeben werden:
		if( pDoc && eDocType == pDoc->GetDocumentType() )
			pFrameView = pDocSh->GetFrameView();

		pViewShell = pDocSh->GetViewShell();
        if (pViewShell != NULL)
            pViewShell->WriteFrameViewData();
	}
	SdOptions* pOptions = GetSdOptions(eDocType);
	// Raster
	if( SFX_ITEM_SET == rSet.GetItemState( SID_ATTR_GRID_OPTIONS ,
							FALSE, (const SfxPoolItem**) &pItem ))
	{
		const SdOptionsGridItem* pGridItem = (SdOptionsGridItem*) pItem;
		pGridItem->SetOptions( pOptions );
	}

	// Layout
	const SdOptionsLayoutItem* pLayoutItem = NULL;
	if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_LAYOUT,
							FALSE, (const SfxPoolItem**) &pLayoutItem ))
	{
		pLayoutItem->SetOptions( pOptions );
	}

	// Metric
	if( SFX_ITEM_SET == rSet.GetItemState( SID_ATTR_METRIC, FALSE, &pItem ) )
	{
		if( pDoc && eDocType == pDoc->GetDocumentType() )
			PutItem( *pItem );
		pOptions->SetMetric( ( (SfxUInt16Item*) pItem )->GetValue() );
	}
	UINT16 nDefTab = pOptions->GetDefTab();
	// Default-Tabulator
	if( SFX_ITEM_SET == rSet.GetItemState( SID_ATTR_DEFTABSTOP, FALSE, &pItem ) )
	{
		nDefTab = ( (SfxUInt16Item*) pItem )->GetValue();
		pOptions->SetDefTab( nDefTab );

		bNewDefTab = TRUE;
	}

    // Massstab
    if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_SCALE_X, FALSE, &pItem ) )
    {
        INT32 nX = ( (SfxInt32Item*) pItem )->GetValue();
        if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_SCALE_Y, FALSE, &pItem ) )
        {
            INT32 nY = ( (SfxInt32Item*) pItem )->GetValue();
            pOptions->SetScale( nX, nY );

            // #92067# Apply to document only if doc type match
            if( pDocSh && pDoc && eDocType == pDoc->GetDocumentType() )
            {
                pDoc->SetUIScale( Fraction( nX, nY ) );
                if( pViewShell )
                    pViewShell->SetRuler( pViewShell->HasRuler() );
            }
        }
    }

	// Contents (Inhalte)
	const SdOptionsContentsItem* pContentsItem = NULL;
	if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_CONTENTS,
							FALSE, (const SfxPoolItem**) &pContentsItem ))
	{
		pContentsItem->SetOptions( pOptions );
	}

	// Misc (Sonstiges)
	const SdOptionsMiscItem* pMiscItem = NULL;
	if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_MISC,
							FALSE, (const SfxPoolItem**) &pMiscItem ))
	{
		pMiscItem->SetOptions( pOptions );
		bMiscOptions = TRUE;
	}

	// Fangen/Einrasten
	const SdOptionsSnapItem* pSnapItem = NULL;
	if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_SNAP,
							FALSE, (const SfxPoolItem**) &pSnapItem ))
	{
		pSnapItem->SetOptions( pOptions );
	}

	SfxItemSet aPrintSet( GetPool(),
					SID_PRINTER_NOTFOUND_WARN,	SID_PRINTER_NOTFOUND_WARN,
					SID_PRINTER_CHANGESTODOC,	SID_PRINTER_CHANGESTODOC,
					ATTR_OPTIONS_PRINT, 		ATTR_OPTIONS_PRINT,
					0 );

	// Drucken
	const SdOptionsPrintItem* pPrintItem = NULL;
	if( SFX_ITEM_SET == rSet.GetItemState( ATTR_OPTIONS_PRINT,
							FALSE, (const SfxPoolItem**) &pPrintItem ))
	{
		pPrintItem->SetOptions( pOptions );

		// PrintOptionsSet setzen
		SdOptionsPrintItem aPrintItem( ATTR_OPTIONS_PRINT, pOptions );
		SfxFlagItem aFlagItem( SID_PRINTER_CHANGESTODOC );
		USHORT		nFlags = 0;

		nFlags =  (aPrintItem.GetOptionsPrint().IsWarningSize() ? SFX_PRINTER_CHG_SIZE : 0) |
				(aPrintItem.GetOptionsPrint().IsWarningOrientation() ? SFX_PRINTER_CHG_ORIENTATION : 0);
		aFlagItem.SetValue( nFlags );

		aPrintSet.Put( aPrintItem );
		aPrintSet.Put( SfxBoolItem( SID_PRINTER_NOTFOUND_WARN, aPrintItem.GetOptionsPrint().IsWarningPrinter() ) );
		aPrintSet.Put( aFlagItem );

		bNewPrintOptions = TRUE;
	}

	// Nur, wenn auch der Dokumenttyp uebereinstimmt...
	if( pDocSh && pDoc && eDocType == pDoc->GetDocumentType() )
	{
		if( bNewPrintOptions )
		{
			pDocSh->GetPrinter(TRUE)->SetOptions( aPrintSet );
		}

		// Am Model den DefTab setzen
		if( bNewDefTab )
		{
			SdDrawDocument* pDocument = pDocSh->GetDoc();
			pDocument->SetDefaultTabulator( nDefTab );

			::sd::Outliner* pOutl = pDocument->GetOutliner( FALSE );
			if( pOutl )
				pOutl->SetDefTab( nDefTab );

			::sd::Outliner* pInternalOutl = pDocument->GetInternalOutliner( FALSE );
			if( pInternalOutl )
				pInternalOutl->SetDefTab( nDefTab );
		}
		if ( bMiscOptions )
		{
            pDoc->SetSummationOfParagraphs( pMiscItem->GetOptionsMisc().IsSummationOfParagraphs() );
			sal_uInt32 nSum = pMiscItem->GetOptionsMisc().IsSummationOfParagraphs() ? EE_CNTRL_ULSPACESUMMATION : 0;
			sal_uInt32 nCntrl;

			SdDrawDocument* pDocument = pDocSh->GetDoc();
			SdrOutliner& rOutl = pDocument->GetDrawOutliner( FALSE );
			nCntrl = rOutl.GetControlWord() &~ EE_CNTRL_ULSPACESUMMATION;
			rOutl.SetControlWord( nCntrl | nSum );
			::sd::Outliner* pOutl = pDocument->GetOutliner( FALSE );
			if( pOutl )
			{
				nCntrl = pOutl->GetControlWord() &~ EE_CNTRL_ULSPACESUMMATION;
				pOutl->SetControlWord( nCntrl | nSum );
			}
			pOutl = pDocument->GetInternalOutliner( FALSE );
			if( pOutl )
			{
				nCntrl = pOutl->GetControlWord() &~ EE_CNTRL_ULSPACESUMMATION;
				pOutl->SetControlWord( nCntrl | nSum );
			}

            // Set printer independent layout mode.
			if( pDoc->GetPrinterIndependentLayout() != pMiscItem->GetOptionsMisc().GetPrinterIndependentLayout() )
	            pDoc->SetPrinterIndependentLayout (pMiscItem->GetOptionsMisc().GetPrinterIndependentLayout());
		}
	}

	pOptions->StoreConfig();

	// Nur, wenn auch der Dokumenttyp uebereinstimmt...
	if( pDocSh && pDoc && eDocType == pDoc->GetDocumentType() )
	{
		FieldUnit eUIUnit = (FieldUnit) pOptions->GetMetric();
		pDoc->SetUIUnit(eUIUnit);

		if (pViewShell)
		{
			// #74495# make sure no one is in text edit mode, cause there
			// are some pointers remembered else (!)
			if(pViewShell->GetView())
				pViewShell->GetView()->SdrEndTextEdit();

			::sd::FrameView* pFrame = pViewShell->GetFrameView();
			pFrame->Update(pOptions);
			pViewShell->ReadFrameViewData(pFrame);
			pViewShell->SetUIUnit(eUIUnit);
			pViewShell->SetDefTabHRuler( nDefTab );
		}
	}

	if( pViewShell && pViewShell->GetViewFrame() )
		pViewShell->GetViewFrame()->GetBindings().InvalidateAll( TRUE );
}

SfxTabPage* SdModule::CreateTabPage( USHORT nId, Window* pParent, const SfxItemSet& rSet )
{
	SfxTabPage* pRet = NULL;
	SfxAllItemSet aSet(*(rSet.GetPool()));
	SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
	if( pFact )
	{
		switch(nId)
		{
			case SID_SD_TP_CONTENTS:
			case SID_SI_TP_CONTENTS:
			{	::CreateTabPage fnCreatePage = pFact->GetSdOptionsContentsTabPageCreatorFunc();
				if( fnCreatePage )
					pRet = (*fnCreatePage)( pParent, rSet );
			}
			break;
			case SID_SD_TP_SNAP:
			case SID_SI_TP_SNAP:
			{	::CreateTabPage fnCreatePage = pFact->GetSdOptionsSnapTabPageCreatorFunc();
				if( fnCreatePage )
					pRet = (*fnCreatePage)( pParent, rSet );
			}
			break;
			case SID_SD_TP_PRINT:
			case SID_SI_TP_PRINT:
			{
				::CreateTabPage fnCreatePage = pFact->GetSdPrintOptionsTabPageCreatorFunc();
				if( fnCreatePage )
				{
					pRet = (*fnCreatePage)( pParent, rSet );
					if(SID_SD_TP_PRINT == nId)
						aSet.Put (SfxUInt32Item(SID_SDMODE_FLAG,SD_DRAW_MODE));
					pRet->PageCreated(aSet);
				}
			}
			break;
			case SID_SI_TP_MISC:
			case SID_SD_TP_MISC:
			{
				::CreateTabPage fnCreatePage = pFact->GetSdOptionsMiscTabPageCreatorFunc();
				if( fnCreatePage )
				{
					pRet = (*fnCreatePage)( pParent, rSet );
					if(SID_SD_TP_MISC == nId)
						aSet.Put (SfxUInt32Item(SID_SDMODE_FLAG,SD_DRAW_MODE));
					else
						aSet.Put (SfxUInt32Item(SID_SDMODE_FLAG,SD_IMPRESS_MODE));
					pRet->PageCreated(aSet);
				}
			}
			break;
			case RID_OFA_TP_INTERNATIONAL_SD:
			case RID_OFA_TP_INTERNATIONAL_IMPR:
			case RID_SVXPAGE_TEXTANIMATION :
			{
				SfxAbstractDialogFactory* pSfxFact = SfxAbstractDialogFactory::Create();
				if ( pSfxFact )
				{
					::CreateTabPage fnCreatePage = pSfxFact->GetTabPageCreatorFunc( nId );
					if ( fnCreatePage )
						pRet = (*fnCreatePage)( pParent, rSet );
				}
			}
			break;
		}
		DBG_ASSERT( pRet, "SdModule::CreateTabPage(): no valid ID for TabPage!" );
	}

	return pRet;
}
