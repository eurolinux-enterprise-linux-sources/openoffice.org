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



// INCLUDE ---------------------------------------------------------------

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>

#include "scitems.hxx"
#include "rangelst.hxx"
#include <svx/flstitem.hxx>
#include <svx/pageitem.hxx>
#include <svx/paperinf.hxx>
#include <svx/postattr.hxx>
//#include <svx/postdlg.hxx>
#include <svx/sizeitem.hxx>

#include <sfx2/viewfrm.hxx>
#include <sfx2/app.hxx>
#include <sfx2/docfile.hxx>
#include <svtools/misccfg.hxx>
#include <sfx2/printer.hxx>
#include <svtools/ctrltool.hxx>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>
#include <vcl/msgbox.hxx>
#include <unotools/localedatawrapper.hxx>

#include "docsh.hxx"
#include "docshimp.hxx"
#include "scmod.hxx"
#include "tabvwsh.hxx"
#include "viewdata.hxx"
#include "docpool.hxx"
#include "stlpool.hxx"
#include "patattr.hxx"
#include "uiitems.hxx"
#include "hints.hxx"
#include "docoptio.hxx"
#include "viewopti.hxx"
#include "pntlock.hxx"
#include "chgtrack.hxx"
#include "docfunc.hxx"
#include "cell.hxx"
#include "chgviset.hxx"
#include "progress.hxx"
#include "redcom.hxx"
#include "sc.hrc"
#include "inputopt.hxx"
#include "drwlayer.hxx"
#include "inputhdl.hxx"
#include "conflictsdlg.hxx"
#include "globstr.hrc"

#if DEBUG_CHANGETRACK
#include <stdio.h>
#endif // DEBUG_CHANGETRACK


//------------------------------------------------------------------

//
//			Redraw - Benachrichtigungen
//


void ScDocShell::PostEditView( ScEditEngineDefaulter* pEditEngine, const ScAddress& rCursorPos )
{
//	Broadcast( ScEditViewHint( pEditEngine, rCursorPos ) );

		//	Test: nur aktive ViewShell

	ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell();
	if (pViewSh && pViewSh->GetViewData()->GetDocShell() == this)
	{
		ScEditViewHint aHint( pEditEngine, rCursorPos );
		pViewSh->Notify( *this, aHint );
	}
}

void ScDocShell::PostDataChanged()
{
	Broadcast( SfxSimpleHint( FID_DATACHANGED ) );
	aDocument.ResetChanged( ScRange(0,0,0,MAXCOL,MAXROW,MAXTAB) );

	SFX_APP()->Broadcast(SfxSimpleHint( FID_ANYDATACHANGED ));		// Navigator
	//!	Navigator direkt benachrichtigen!
}

void ScDocShell::PostPaint( SCCOL nStartCol, SCROW nStartRow, SCTAB nStartTab,
							SCCOL nEndCol, SCROW nEndRow, SCTAB nEndTab, USHORT nPart,
							USHORT nExtFlags )
{
	if (!ValidCol(nStartCol)) nStartCol = MAXCOL;
	if (!ValidRow(nStartRow)) nStartRow = MAXROW;
	if (!ValidCol(nEndCol)) nEndCol = MAXCOL;
	if (!ValidRow(nEndRow)) nEndRow = MAXROW;

	if ( pPaintLockData )
	{
        // #i54081# PAINT_EXTRAS still has to be brodcast because it changes the
        // current sheet if it's invalid. All other flags added to pPaintLockData.
        USHORT nLockPart = nPart & ~PAINT_EXTRAS;
        if ( nLockPart )
        {
            //! nExtFlags ???
            pPaintLockData->AddRange( ScRange( nStartCol, nStartRow, nStartTab,
                                               nEndCol, nEndRow, nEndTab ), nLockPart );
        }

        nPart &= PAINT_EXTRAS;  // for broadcasting
        if ( !nPart )
            return;
	}


    if (nExtFlags & SC_PF_LINES)            // Platz fuer Linien beruecksichtigen
	{
											//! Abfrage auf versteckte Spalten/Zeilen!
		if (nStartCol>0) --nStartCol;
		if (nEndCol<MAXCOL) ++nEndCol;
		if (nStartRow>0) --nStartRow;
		if (nEndRow<MAXROW) ++nEndRow;
	}

											// um zusammengefasste erweitern
	if (nExtFlags & SC_PF_TESTMERGE)
		aDocument.ExtendMerge( nStartCol, nStartRow, nEndCol, nEndRow, nStartTab );

	if ( nStartCol != 0 || nEndCol != MAXCOL )
	{
		//	Extend to whole rows if SC_PF_WHOLEROWS is set, or rotated or non-left
		//	aligned cells are contained (see UpdatePaintExt).
		//	Special handling for RTL text (#i9731#) is unnecessary now with full
		//	support of right-aligned text.

		if ( ( nExtFlags & SC_PF_WHOLEROWS ) ||
			 aDocument.HasAttrib( nStartCol,nStartRow,nStartTab,
								  MAXCOL,nEndRow,nEndTab, HASATTR_ROTATE | HASATTR_RIGHTORCENTER ) )
		{
			nStartCol = 0;
			nEndCol = MAXCOL;
		}
	}

	Broadcast( ScPaintHint( ScRange( nStartCol, nStartRow, nStartTab,
									 nEndCol, nEndRow, nEndTab ), nPart ) );

	if ( nPart & PAINT_GRID )
		aDocument.ResetChanged( ScRange(nStartCol,nStartRow,nStartTab,nEndCol,nEndRow,nEndTab) );
}

void ScDocShell::PostPaint( const ScRange& rRange, USHORT nPart, USHORT nExtFlags )
{
	PostPaint( rRange.aStart.Col(), rRange.aStart.Row(), rRange.aStart.Tab(),
			   rRange.aEnd.Col(),   rRange.aEnd.Row(),   rRange.aEnd.Tab(),
			   nPart, nExtFlags );
}

void ScDocShell::PostPaintGridAll()
{
	PostPaint( 0,0,0, MAXCOL,MAXROW,MAXTAB, PAINT_GRID );
}

void ScDocShell::PostPaintCell( SCCOL nCol, SCROW nRow, SCTAB nTab )
{
	PostPaint( nCol,nRow,nTab, nCol,nRow,nTab, PAINT_GRID, SC_PF_TESTMERGE );
}

void ScDocShell::PostPaintCell( const ScAddress& rPos )
{
	PostPaintCell( rPos.Col(), rPos.Row(), rPos.Tab() );
}

void ScDocShell::PostPaintExtras()
{
	PostPaint( 0,0,0, MAXCOL,MAXROW,MAXTAB, PAINT_EXTRAS );
}

void ScDocShell::UpdatePaintExt( USHORT& rExtFlags, const ScRange& rRange )
{
	if ( ( rExtFlags & SC_PF_LINES ) == 0 && aDocument.HasAttrib( rRange, HASATTR_PAINTEXT ) )
	{
		//	If the range contains lines, shadow or conditional formats,
		//	set SC_PF_LINES to include one extra cell in all directions.

		rExtFlags |= SC_PF_LINES;
	}

	if ( ( rExtFlags & SC_PF_WHOLEROWS ) == 0 &&
		 ( rRange.aStart.Col() != 0 || rRange.aEnd.Col() != MAXCOL ) &&
		 aDocument.HasAttrib( rRange, HASATTR_ROTATE | HASATTR_RIGHTORCENTER ) )
	{
		//	If the range contains (logically) right- or center-aligned cells,
		//	or rotated cells, set SC_PF_WHOLEROWS to paint the whole rows.
		//	This test isn't needed after the cell changes, because it's also
		//	tested in PostPaint. UpdatePaintExt may later be changed to do this
		//	only if called before the changes.

		rExtFlags |= SC_PF_WHOLEROWS;
	}
}

void ScDocShell::UpdatePaintExt( USHORT& rExtFlags, SCCOL nStartCol, SCROW nStartRow, SCTAB nStartTab,
												   SCCOL nEndCol, SCROW nEndRow, SCTAB nEndTab )
{
	UpdatePaintExt( rExtFlags, ScRange( nStartCol, nStartRow, nStartTab, nEndCol, nEndRow, nEndTab ) );
}

//------------------------------------------------------------------

void ScDocShell::LockPaint_Impl(BOOL bDoc)
{
	if ( !pPaintLockData )
		pPaintLockData = new ScPaintLockData(0);	//! Modus...
    pPaintLockData->IncLevel(bDoc);
}

void ScDocShell::UnlockPaint_Impl(BOOL bDoc)
{
	if ( pPaintLockData )
	{
		if ( pPaintLockData->GetLevel(bDoc) )
			pPaintLockData->DecLevel(bDoc);
		if (!pPaintLockData->GetLevel(!bDoc) && !pPaintLockData->GetLevel(bDoc))
		{
			//		Paint jetzt ausfuehren

			ScPaintLockData* pPaint = pPaintLockData;
			pPaintLockData = NULL;						// nicht weitersammeln

			ScRangeListRef xRangeList = pPaint->GetRangeList();
			if (xRangeList)
			{
				USHORT nParts = pPaint->GetParts();
				ULONG nCount = xRangeList->Count();
				for ( ULONG i=0; i<nCount; i++ )
				{
					//!	nExtFlags ???
					ScRange aRange = *xRangeList->GetObject(i);
					PostPaint( aRange.aStart.Col(), aRange.aStart.Row(), aRange.aStart.Tab(),
								aRange.aEnd.Col(), aRange.aEnd.Row(), aRange.aEnd.Tab(),
								nParts );
				}
			}

			if ( pPaint->GetModified() )
				SetDocumentModified();

			delete pPaint;
		}
	}
	else
	{
		DBG_ERROR("UnlockPaint ohne LockPaint");
	}
}

void ScDocShell::LockDocument_Impl(USHORT nNew)
{
	if (!nDocumentLock)
	{
		ScDrawLayer* pDrawLayer = aDocument.GetDrawLayer();
		if (pDrawLayer)
			pDrawLayer->setLock(TRUE);
	}
	nDocumentLock = nNew;
}

void ScDocShell::UnlockDocument_Impl(USHORT nNew)
{
	nDocumentLock = nNew;
	if (!nDocumentLock)
	{
		ScDrawLayer* pDrawLayer = aDocument.GetDrawLayer();
		if (pDrawLayer)
			pDrawLayer->setLock(FALSE);
	}
}

USHORT ScDocShell::GetLockCount() const
{
	return nDocumentLock;
}

void ScDocShell::SetLockCount(USHORT nNew)
{
	if (nNew)					// setzen
	{
		if ( !pPaintLockData )
			pPaintLockData = new ScPaintLockData(0);	//! Modus...
		pPaintLockData->SetLevel(nNew-1, TRUE);
		LockDocument_Impl(nNew);
	}
	else if (pPaintLockData)	// loeschen
	{
		pPaintLockData->SetLevel(0, TRUE);	// bei Unlock sofort ausfuehren
		UnlockPaint_Impl(TRUE);					// jetzt
		UnlockDocument_Impl(0);
	}
}

void ScDocShell::LockPaint()
{
	LockPaint_Impl(FALSE);
}

void ScDocShell::UnlockPaint()
{
	UnlockPaint_Impl(FALSE);
}

void ScDocShell::LockDocument()
{
	LockPaint_Impl(TRUE);
	LockDocument_Impl(nDocumentLock + 1);
}

void ScDocShell::UnlockDocument()
{
	if (nDocumentLock)
	{
		UnlockPaint_Impl(TRUE);
		UnlockDocument_Impl(nDocumentLock - 1);
	}
	else
	{
		DBG_ERROR("UnlockDocument without LockDocument");
	}
}

//------------------------------------------------------------------

void ScDocShell::SetInplace( BOOL bInplace )
{
	if (bIsInplace != bInplace)
	{
		bIsInplace = bInplace;
		CalcOutputFactor();
	}
}

void ScDocShell::CalcOutputFactor()
{
	if (bIsInplace)
	{
		nPrtToScreenFactor = 1.0;			// passt sonst nicht zur inaktiven Darstellung
		return;
	}

	BOOL bTextWysiwyg = SC_MOD()->GetInputOptions().GetTextWysiwyg();
	if (bTextWysiwyg)
	{
		nPrtToScreenFactor = 1.0;
		return;
	}

	String aTestString = String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM(
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567890123456789" ));
	long nPrinterWidth = 0;
	long nWindowWidth = 0;
	const ScPatternAttr* pPattern = (const ScPatternAttr*)&aDocument.GetPool()->
											GetDefaultItem(ATTR_PATTERN);

	Font aDefFont;
	OutputDevice* pRefDev = GetRefDevice();
	MapMode aOldMode = pRefDev->GetMapMode();
	Font	aOldFont = pRefDev->GetFont();

	pRefDev->SetMapMode(MAP_PIXEL);
	pPattern->GetFont(aDefFont, SC_AUTOCOL_BLACK, pRefDev);	// font color doesn't matter here
	pRefDev->SetFont(aDefFont);
	nPrinterWidth = pRefDev->PixelToLogic( Size( pRefDev->GetTextWidth(aTestString), 0 ), MAP_100TH_MM ).Width();
	pRefDev->SetFont(aOldFont);
	pRefDev->SetMapMode(aOldMode);

	VirtualDevice aVirtWindow( *Application::GetDefaultDevice() );
	aVirtWindow.SetMapMode(MAP_PIXEL);
	pPattern->GetFont(aDefFont, SC_AUTOCOL_BLACK, &aVirtWindow);	// font color doesn't matter here
	aVirtWindow.SetFont(aDefFont);
	nWindowWidth = aVirtWindow.GetTextWidth(aTestString);
	nWindowWidth = (long) ( nWindowWidth / ScGlobal::nScreenPPTX * HMM_PER_TWIPS );

	if (nPrinterWidth && nWindowWidth)
		nPrtToScreenFactor = nPrinterWidth / (double) nWindowWidth;
	else
	{
		DBG_ERROR("GetTextSize gibt 0 ??");
		nPrtToScreenFactor = 1.0;
	}
}

double ScDocShell::GetOutputFactor() const
{
	return nPrtToScreenFactor;
}

//---------------------------------------------------------------------

void ScDocShell::InitOptions()			// Fortsetzung von InitNew (CLOOKs)
{
	//	Einstellungen aus dem SpellCheckCfg kommen in Doc- und ViewOptions

	USHORT nDefLang, nCjkLang, nCtlLang;
    BOOL bAutoSpell;
    ScModule::GetSpellSettings( nDefLang, nCjkLang, nCtlLang, bAutoSpell );
	ScModule* pScMod = SC_MOD();

	ScDocOptions  aDocOpt  = pScMod->GetDocOptions();
	ScViewOptions aViewOpt = pScMod->GetViewOptions();
	aDocOpt.SetAutoSpell( bAutoSpell );

	// zweistellige Jahreszahleneingabe aus Extras->Optionen->Allgemein->Sonstiges
    aDocOpt.SetYear2000( sal::static_int_cast<USHORT>( SFX_APP()->GetMiscConfig()->GetYear2000() ) );

	aDocument.SetDocOptions( aDocOpt );
	aDocument.SetViewOptions( aViewOpt );

	//	Druck-Optionen werden jetzt direkt vor dem Drucken gesetzt

	aDocument.SetLanguage( (LanguageType) nDefLang, (LanguageType) nCjkLang, (LanguageType) nCtlLang );
}

//---------------------------------------------------------------------

Printer* ScDocShell::GetDocumentPrinter()		// fuer OLE
{
	return aDocument.GetPrinter();
}

SfxPrinter* ScDocShell::GetPrinter(BOOL bCreateIfNotExist)
{
	return aDocument.GetPrinter(bCreateIfNotExist);
}

void ScDocShell::UpdateFontList()
{
    delete pImpl->pFontList;
    // pImpl->pFontList = new FontList( GetPrinter(), Application::GetDefaultDevice() );
    pImpl->pFontList = new FontList( GetRefDevice(), NULL, FALSE ); // FALSE or TRUE???
    SvxFontListItem aFontListItem( pImpl->pFontList, SID_ATTR_CHAR_FONTLIST );
	PutItem( aFontListItem );

	CalcOutputFactor();
}

OutputDevice* ScDocShell::GetRefDevice()
{
	return aDocument.GetRefDevice();
}

USHORT ScDocShell::SetPrinter( SfxPrinter* pNewPrinter, USHORT nDiffFlags )
{
    SfxPrinter *pOld = aDocument.GetPrinter( FALSE );
    if ( pOld && pOld->IsPrinting() )
        return SFX_PRINTERROR_BUSY;

	if (nDiffFlags & SFX_PRINTER_PRINTER)
	{
		if ( aDocument.GetPrinter() != pNewPrinter )
		{
			aDocument.SetPrinter( pNewPrinter );
			aDocument.SetPrintOptions();

			// MT: Use UpdateFontList: Will use Printer fonts only if needed!
			/*
            delete pImpl->pFontList;
            pImpl->pFontList = new FontList( pNewPrinter, Application::GetDefaultDevice() );
            SvxFontListItem aFontListItem( pImpl->pFontList, SID_ATTR_CHAR_FONTLIST );
			PutItem( aFontListItem );

			CalcOutputFactor();
			*/
			if ( SC_MOD()->GetInputOptions().GetTextWysiwyg() )
				UpdateFontList();

			ScModule* pScMod = SC_MOD();
			SfxViewFrame *pFrame = SfxViewFrame::GetFirst( this );
			while (pFrame)
			{
				SfxViewShell* pSh = pFrame->GetViewShell();
				if (pSh && pSh->ISA(ScTabViewShell))
				{
					ScTabViewShell* pViewSh	= (ScTabViewShell*)pSh;
					ScInputHandler* pInputHdl = pScMod->GetInputHdl(pViewSh);
					if (pInputHdl)
						pInputHdl->UpdateRefDevice();
				}
				pFrame = SfxViewFrame::GetNext( *pFrame, this );
			}
		}
	}
	else if (nDiffFlags & SFX_PRINTER_JOBSETUP)
	{
		SfxPrinter* pOldPrinter = aDocument.GetPrinter();
		if (pOldPrinter)
		{
			pOldPrinter->SetJobSetup( pNewPrinter->GetJobSetup() );

			//	#i6706# Call SetPrinter with the old printer again, so the drawing layer
			//	RefDevice is set (calling ReformatAllTextObjects and rebuilding charts),
			//	because the JobSetup (printer device settings) may affect text layout.
			aDocument.SetPrinter( pOldPrinter );
			CalcOutputFactor();							// also with the new settings
		}
	}

	if (nDiffFlags & SFX_PRINTER_OPTIONS)
	{
		aDocument.SetPrintOptions();		//! aus neuem Printer ???
	}

	if (nDiffFlags & (SFX_PRINTER_CHG_ORIENTATION | SFX_PRINTER_CHG_SIZE))
	{
		String aStyle = aDocument.GetPageStyle( GetCurTab() );
		ScStyleSheetPool* pStPl = aDocument.GetStyleSheetPool();
		SfxStyleSheet* pStyleSheet = (SfxStyleSheet*)pStPl->Find(aStyle, SFX_STYLE_FAMILY_PAGE);
		if (pStyleSheet)
		{
			SfxItemSet& rSet = pStyleSheet->GetItemSet();

			if (nDiffFlags & SFX_PRINTER_CHG_ORIENTATION)
			{
				const SvxPageItem& rOldItem = (const SvxPageItem&)rSet.Get(ATTR_PAGE);
				BOOL bWasLand = rOldItem.IsLandscape();
				BOOL bNewLand = ( pNewPrinter->GetOrientation() == ORIENTATION_LANDSCAPE );
				if (bNewLand != bWasLand)
				{
					SvxPageItem aNewItem( rOldItem );
					aNewItem.SetLandscape( bNewLand );
					rSet.Put( aNewItem );

					//	Groesse umdrehen
					Size aOldSize = ((const SvxSizeItem&)rSet.Get(ATTR_PAGE_SIZE)).GetSize();
					Size aNewSize(aOldSize.Height(),aOldSize.Width());
					SvxSizeItem aNewSItem(ATTR_PAGE_SIZE,aNewSize);
					rSet.Put( aNewSItem );
				}
			}
			if (nDiffFlags & SFX_PRINTER_CHG_SIZE)
			{
				SvxSizeItem	aPaperSizeItem( ATTR_PAGE_SIZE, SvxPaperInfo::GetPaperSize(pNewPrinter) );
				rSet.Put( aPaperSizeItem );
			}
		}
	}

	PostPaint(0,0,0,MAXCOL,MAXROW,MAXTAB,PAINT_ALL);

	return 0;
}

//---------------------------------------------------------------------

ScChangeAction* ScDocShell::GetChangeAction( const ScAddress& rPos )
{
	ScChangeTrack* pTrack = GetDocument()->GetChangeTrack();
	if (!pTrack)
		return NULL;

	SCTAB nTab = rPos.Tab();

	const ScChangeAction* pFound = NULL;
	const ScChangeAction* pFoundContent = NULL;
	const ScChangeAction* pFoundMove = NULL;
	long nModified = 0;
	const ScChangeAction* pAction = pTrack->GetFirst();
	while (pAction)
	{
		ScChangeActionType eType = pAction->GetType();
		//!	ScViewUtil::IsActionShown( *pAction, *pSettings, *pDoc )...
		if ( pAction->IsVisible() && eType != SC_CAT_DELETE_TABS )
		{
			const ScBigRange& rBig = pAction->GetBigRange();
			if ( rBig.aStart.Tab() == nTab )
			{
				ScRange aRange = rBig.MakeRange();

				if ( eType == SC_CAT_DELETE_ROWS )
					aRange.aEnd.SetRow( aRange.aStart.Row() );
				else if ( eType == SC_CAT_DELETE_COLS )
					aRange.aEnd.SetCol( aRange.aStart.Col() );

				if ( aRange.In( rPos ) )
				{
					pFound = pAction;		// der letzte gewinnt
					switch ( pAction->GetType() )
					{
						case SC_CAT_CONTENT :
							pFoundContent = pAction;
						break;
						case SC_CAT_MOVE :
							pFoundMove = pAction;
						break;
                        default:
                        {
                            // added to avoid warnings
                        }
					}
					++nModified;
				}
			}
			if ( pAction->GetType() == SC_CAT_MOVE )
			{
				ScRange aRange =
					((const ScChangeActionMove*)pAction)->
					GetFromRange().MakeRange();
				if ( aRange.In( rPos ) )
				{
					pFound = pAction;
					++nModified;
				}
			}
		}
		pAction = pAction->GetNext();
	}

	return (ScChangeAction*)pFound;
}

void ScDocShell::SetChangeComment( ScChangeAction* pAction, const String& rComment )
{
	if (pAction)
	{
		pAction->SetComment( rComment );
		//!	Undo ???
		SetDocumentModified();

		//	Dialog-Notify
		ScChangeTrack* pTrack = GetDocument()->GetChangeTrack();
		if (pTrack)
		{
			ULONG nNumber = pAction->GetActionNumber();
			pTrack->NotifyModified( SC_CTM_CHANGE, nNumber, nNumber );
		}
	}
}

void ScDocShell::ExecuteChangeCommentDialog( ScChangeAction* pAction, Window* pParent,BOOL bPrevNext)
{
	if (!pAction) return;			// ohne Aktion ist nichts..

	String aComment = pAction->GetComment();
	String aAuthor = pAction->GetUser();

	DateTime aDT = pAction->GetDateTime();
    String aDate = ScGlobal::pLocaleData->getDate( aDT );
	aDate += ' ';
    aDate += ScGlobal::pLocaleData->getTime( aDT, FALSE, FALSE );

	SfxItemSet aSet( GetPool(),
					  SID_ATTR_POSTIT_AUTHOR, SID_ATTR_POSTIT_AUTHOR,
					  SID_ATTR_POSTIT_DATE,   SID_ATTR_POSTIT_DATE,
					  SID_ATTR_POSTIT_TEXT,   SID_ATTR_POSTIT_TEXT,
					  0 );

	aSet.Put( SvxPostItTextItem  ( aComment, SID_ATTR_POSTIT_TEXT ) );
	aSet.Put( SvxPostItAuthorItem( aAuthor,  SID_ATTR_POSTIT_AUTHOR ) );
	aSet.Put( SvxPostItDateItem  ( aDate,    SID_ATTR_POSTIT_DATE ) );

	ScRedComDialog* pDlg = new ScRedComDialog( pParent, aSet,this,pAction,bPrevNext);

	pDlg->Execute();

	delete pDlg;
}

//---------------------------------------------------------------------

void ScDocShell::CompareDocument( ScDocument& rOtherDoc )
{
	ScChangeTrack* pTrack = aDocument.GetChangeTrack();
	if ( pTrack && pTrack->GetFirst() )
	{
		//!	Changes vorhanden -> Nachfrage ob geloescht werden soll
	}

	aDocument.EndChangeTracking();
	aDocument.StartChangeTracking();

	String aOldUser;
	pTrack = aDocument.GetChangeTrack();
	if ( pTrack )
	{
		aOldUser = pTrack->GetUser();

		//	check if comparing to same document

		String aThisFile;
		const SfxMedium* pThisMed = GetMedium();
		if (pThisMed)
			aThisFile = pThisMed->GetName();
		String aOtherFile;
		SfxObjectShell* pOtherSh = rOtherDoc.GetDocumentShell();
		if (pOtherSh)
		{
			const SfxMedium* pOtherMed = pOtherSh->GetMedium();
			if (pOtherMed)
				aOtherFile = pOtherMed->GetName();
		}
		BOOL bSameDoc = ( aThisFile == aOtherFile && aThisFile.Len() );
		if ( !bSameDoc )
		{
			//	create change actions from comparing with the name of the user
			//	who last saved the document
			//	(only if comparing different documents)

            using namespace ::com::sun::star;
            uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                GetModel(), uno::UNO_QUERY_THROW);
            uno::Reference<document::XDocumentProperties> xDocProps(
                xDPS->getDocumentProperties());
            DBG_ASSERT(xDocProps.is(), "no DocumentProperties");
            String aDocUser = xDocProps->getModifiedBy();

			if ( aDocUser.Len() )
				pTrack->SetUser( aDocUser );
		}
	}

	aDocument.CompareDocument( rOtherDoc );

	pTrack = aDocument.GetChangeTrack();
	if ( pTrack )
		pTrack->SetUser( aOldUser );

	PostPaintGridAll();
	SetDocumentModified();
}

//---------------------------------------------------------------------
//
//				Merge (Aenderungen zusammenfuehren)
//
//---------------------------------------------------------------------

inline BOOL lcl_Equal( const ScChangeAction* pA, const ScChangeAction* pB, BOOL bIgnore100Sec )
{
	return pA && pB &&
        pA->GetActionNumber() == pB->GetActionNumber() &&
        pA->GetType()		  == pB->GetType() &&
        pA->GetUser()		  == pB->GetUser() &&
        (bIgnore100Sec ?
         pA->GetDateTimeUTC().IsEqualIgnore100Sec( pB->GetDateTimeUTC() ) :
         pA->GetDateTimeUTC() == pB->GetDateTimeUTC());
	//	State nicht vergleichen, falls eine alte Aenderung akzeptiert wurde
}

bool lcl_FindAction( ScDocument* pDoc, const ScChangeAction* pAction, ScDocument* pSearchDoc, const ScChangeAction* pFirstSearchAction, const ScChangeAction* pLastSearchAction, BOOL bIgnore100Sec )
{
    if ( !pDoc || !pAction || !pSearchDoc || !pFirstSearchAction || !pLastSearchAction )
    {
        return false;
    }

    ULONG nLastSearchAction = pLastSearchAction->GetActionNumber();
    const ScChangeAction* pA = pFirstSearchAction;
    while ( pA && pA->GetActionNumber() <= nLastSearchAction )
    {
	    if ( pAction->GetType() == pA->GetType() &&
             pAction->GetUser() == pA->GetUser() &&
             (bIgnore100Sec ?
                pAction->GetDateTimeUTC().IsEqualIgnore100Sec( pA->GetDateTimeUTC() ) :
                pAction->GetDateTimeUTC() == pA->GetDateTimeUTC() ) &&
             pAction->GetBigRange() == pA->GetBigRange() )
        {
            String aActionDesc;
            pAction->GetDescription( aActionDesc, pDoc, TRUE );
            String aADesc;
            pA->GetDescription( aADesc, pSearchDoc, TRUE );
            if ( aActionDesc.Equals( aADesc ) )
            {
                DBG_ERROR( "lcl_FindAction(): found equal action!" );
                return true;
            }
        }
        pA = pA->GetNext();
    }

    return false;
}

void ScDocShell::MergeDocument( ScDocument& rOtherDoc, bool bShared, bool bCheckDuplicates, ULONG nOffset, ScChangeActionMergeMap* pMergeMap, bool bInverseMap )
{
    ScTabViewShell* pViewSh = GetBestViewShell( FALSE );    //! Funktionen an die DocShell
	if (!pViewSh)
		return;

	ScChangeTrack* pSourceTrack = rOtherDoc.GetChangeTrack();
	if (!pSourceTrack)
		return;				//!	nichts zu tun - Fehlermeldung?

	ScChangeTrack* pThisTrack = aDocument.GetChangeTrack();
	if ( !pThisTrack )
	{	// anschalten
		aDocument.StartChangeTracking();
		pThisTrack = aDocument.GetChangeTrack();
		DBG_ASSERT(pThisTrack,"ChangeTracking nicht angeschaltet?");
        if ( !bShared )
        {
            // #51138# visuelles RedLining einschalten
            ScChangeViewSettings aChangeViewSet;
            aChangeViewSet.SetShowChanges(TRUE);
            aDocument.SetChangeViewSettings(aChangeViewSet);
        }
	}

    // #97286# include 100th seconds in compare?
    BOOL bIgnore100Sec = !pSourceTrack->IsTime100thSeconds() ||
            !pThisTrack->IsTime100thSeconds();

	//	gemeinsame Ausgangsposition suchen
	ULONG nFirstNewNumber = 0;
	const ScChangeAction* pSourceAction = pSourceTrack->GetFirst();
	const ScChangeAction* pThisAction = pThisTrack->GetFirst();
    // skip identical actions
    while ( lcl_Equal( pSourceAction, pThisAction, bIgnore100Sec ) )
	{
		nFirstNewNumber = pSourceAction->GetActionNumber() + 1;
		pSourceAction = pSourceAction->GetNext();
		pThisAction = pThisAction->GetNext();
	}
	//	pSourceAction und pThisAction zeigen jetzt auf die ersten "eigenen" Aktionen
	//	Die gemeinsamen Aktionen davor interessieren ueberhaupt nicht

	//!	Abfrage, ob die Dokumente vor dem Change-Tracking gleich waren !!!


	const ScChangeAction* pFirstMergeAction = pSourceAction;
    const ScChangeAction* pFirstSearchAction = pThisAction;

    // #i94841# [Collaboration] When deleting rows is rejected, the content is sometimes wrong
    const ScChangeAction* pLastSearchAction = pThisTrack->GetLast();

	//	MergeChangeData aus den folgenden Aktionen erzeugen
	ULONG nNewActionCount = 0;
	const ScChangeAction* pCount = pSourceAction;
	while ( pCount )
	{
        if ( bShared || !ScChangeTrack::MergeIgnore( *pCount, nFirstNewNumber ) )
			++nNewActionCount;
		pCount = pCount->GetNext();
	}
	if (!nNewActionCount)
		return;				//!	nichts zu tun - Fehlermeldung?
							//	ab hier kein return mehr

	ScProgress aProgress( this,
					String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("...")),
					nNewActionCount );

	ULONG nLastMergeAction = pSourceTrack->GetLast()->GetActionNumber();
	// UpdateReference-Undo, gueltige Referenzen fuer den letzten gemeinsamen Zustand
    pSourceTrack->MergePrepare( (ScChangeAction*) pFirstMergeAction, bShared );

	//	MergeChangeData an alle noch folgenden Aktionen in diesem Dokument anpassen
	//	-> Referenzen gueltig fuer dieses Dokument
	while ( pThisAction )
	{
        // #i87049# [Collaboration] Conflict between delete row and insert content is not merged correctly
        if ( !bShared || !ScChangeTrack::MergeIgnore( *pThisAction, nFirstNewNumber ) )
        {
            ScChangeActionType eType = pThisAction->GetType();
		    switch ( eType )
		    {
			    case SC_CAT_INSERT_COLS :
			    case SC_CAT_INSERT_ROWS :
			    case SC_CAT_INSERT_TABS :
				    pSourceTrack->AppendInsert( pThisAction->GetBigRange().MakeRange() );
			    break;
			    case SC_CAT_DELETE_COLS :
			    case SC_CAT_DELETE_ROWS :
			    case SC_CAT_DELETE_TABS :
			    {
				    const ScChangeActionDel* pDel = (const ScChangeActionDel*) pThisAction;
				    if ( pDel->IsTopDelete() && !pDel->IsTabDeleteCol() )
				    {	// deleted Table enthaelt deleted Cols, die nicht
					    ULONG nStart, nEnd;
					    pSourceTrack->AppendDeleteRange(
						    pDel->GetOverAllRange().MakeRange(), NULL, nStart, nEnd );
				    }
			    }
			    break;
			    case SC_CAT_MOVE :
			    {
				    const ScChangeActionMove* pMove = (const ScChangeActionMove*) pThisAction;
				    pSourceTrack->AppendMove( pMove->GetFromRange().MakeRange(),
					    pMove->GetBigRange().MakeRange(), NULL );
			    }
			    break;
                default:
                {
                    // added to avoid warnings
                }
		    }
        }
		pThisAction = pThisAction->GetNext();
	}

    LockPaint();    // #i73877# no repainting after each action

	//	MergeChangeData in das aktuelle Dokument uebernehmen
	BOOL bHasRejected = FALSE;
	String aOldUser = pThisTrack->GetUser();
	pThisTrack->SetUseFixDateTime( TRUE );
	ScMarkData& rMarkData = pViewSh->GetViewData()->GetMarkData();
	ScMarkData aOldMarkData( rMarkData );
	pSourceAction = pFirstMergeAction;
	while ( pSourceAction && pSourceAction->GetActionNumber() <= nLastMergeAction )
	{
        bool bMergeAction = false;
        if ( bShared )
        {
            if ( !bCheckDuplicates || !lcl_FindAction( &rOtherDoc, pSourceAction, &aDocument, pFirstSearchAction, pLastSearchAction, bIgnore100Sec ) )
            {
                bMergeAction = true;
            }
        }
        else
        {
            if ( !ScChangeTrack::MergeIgnore( *pSourceAction, nFirstNewNumber ) )
            {
                bMergeAction = true;
            }
        }

        if ( bMergeAction )
		{
			ScChangeActionType eSourceType = pSourceAction->GetType();
            if ( !bShared && pSourceAction->IsDeletedIn() )
			{
				//! muss hier noch festgestellt werden, ob wirklich in
				//! _diesem_ Dokument geloescht?

				//	liegt in einem Bereich, der in diesem Dokument geloescht wurde
				//	-> wird weggelassen
				//!	??? Loesch-Aktion rueckgaengig machen ???
				//!	??? Aktion irgendwo anders speichern  ???
#ifndef PRODUCT
				String aValue;
				if ( eSourceType == SC_CAT_CONTENT )
					((const ScChangeActionContent*)pSourceAction)->GetNewString( aValue );
				ByteString aError( aValue, gsl_getSystemTextEncoding() );
				aError += " weggelassen";
				DBG_ERROR( aError.GetBuffer() );
#endif
			}
			else
			{
				//!	Datum/Autor/Kommentar der Source-Aktion uebernehmen!

				pThisTrack->SetUser( pSourceAction->GetUser() );
				pThisTrack->SetFixDateTimeUTC( pSourceAction->GetDateTimeUTC() );
                ULONG nOldActionMax = pThisTrack->GetActionMax();

                bool bExecute = true;
				ULONG nReject = pSourceAction->GetRejectAction();
                if ( nReject )
				{
                    if ( bShared )
                    {
                        if ( nReject >= nFirstNewNumber )
                        {
                            nReject += nOffset;
                        }
                        ScChangeAction* pOldAction = pThisTrack->GetAction( nReject );
                        if ( pOldAction && pOldAction->IsVirgin() )
                        {
                            pThisTrack->Reject( pOldAction );
                            bHasRejected = TRUE;
                            bExecute = false;
                        }
                    }
                    else
                    {
					    //	alte Aktion (aus den gemeinsamen) ablehnen
					    ScChangeAction* pOldAction = pThisTrack->GetAction( nReject );
					    if (pOldAction && pOldAction->GetState() == SC_CAS_VIRGIN)
					    {
						    //!	was passiert bei Aktionen, die in diesem Dokument accepted worden sind???
						    //!	Fehlermeldung oder was???
						    //!	oder Reject-Aenderung normal ausfuehren

						    pThisTrack->Reject(pOldAction);
						    bHasRejected = TRUE;				// fuer Paint
					    }
                        bExecute = false;
                    }
				}

                if ( bExecute )
				{
					//	normal ausfuehren
					ScRange aSourceRange = pSourceAction->GetBigRange().MakeRange();
					rMarkData.SelectOneTable( aSourceRange.aStart.Tab() );
					switch ( eSourceType )
					{
						case SC_CAT_CONTENT:
						{
							//!	Test, ob es ganz unten im Dokument war, dann automatisches
							//!	Zeilen-Einfuegen ???

							DBG_ASSERT( aSourceRange.aStart == aSourceRange.aEnd, "huch?" );
							ScAddress aPos = aSourceRange.aStart;
							String aValue;
							((const ScChangeActionContent*)pSourceAction)->GetNewString( aValue );
							BYTE eMatrix = MM_NONE;
							const ScBaseCell* pCell = ((const ScChangeActionContent*)pSourceAction)->GetNewCell();
                            if ( pCell && pCell->GetCellType() == CELLTYPE_FORMULA )
								eMatrix = ((const ScFormulaCell*)pCell)->GetMatrixFlag();
							switch ( eMatrix )
							{
								case MM_NONE :
									pViewSh->EnterData( aPos.Col(), aPos.Row(), aPos.Tab(), aValue );
								break;
								case MM_FORMULA :
								{
                                    SCCOL nCols;
                                    SCROW nRows;
                                    ((const ScFormulaCell*)pCell)->GetMatColsRows( nCols, nRows );
									aSourceRange.aEnd.SetCol( aPos.Col() + nCols - 1 );
									aSourceRange.aEnd.SetRow( aPos.Row() + nRows - 1 );
									aValue.Erase( 0, 1 );
									aValue.Erase( aValue.Len()-1, 1 );
                                    GetDocFunc().EnterMatrix( aSourceRange,
                                        NULL, NULL, aValue, FALSE, FALSE,
                                        EMPTY_STRING, formula::FormulaGrammar::GRAM_DEFAULT );
								}
								break;
								case MM_REFERENCE :		// do nothing
								break;
								case MM_FAKE :
									DBG_WARNING( "MergeDocument: MatrixFlag MM_FAKE" );
									pViewSh->EnterData( aPos.Col(), aPos.Row(), aPos.Tab(), aValue );
								break;
								default:
									DBG_ERROR( "MergeDocument: unknown MatrixFlag" );
							}
						}
						break;
						case SC_CAT_INSERT_TABS :
						{
							String aName;
							aDocument.CreateValidTabName( aName );
							GetDocFunc().InsertTable( aSourceRange.aStart.Tab(), aName, TRUE, FALSE );
						}
						break;
						case SC_CAT_INSERT_ROWS:
							GetDocFunc().InsertCells( aSourceRange, NULL, INS_INSROWS, TRUE, FALSE );
						break;
						case SC_CAT_INSERT_COLS:
							GetDocFunc().InsertCells( aSourceRange, NULL, INS_INSCOLS, TRUE, FALSE );
						break;
						case SC_CAT_DELETE_TABS :
							GetDocFunc().DeleteTable( aSourceRange.aStart.Tab(), TRUE, FALSE );
						break;
						case SC_CAT_DELETE_ROWS:
						{
							const ScChangeActionDel* pDel = (const ScChangeActionDel*) pSourceAction;
							if ( pDel->IsTopDelete() )
							{
								aSourceRange = pDel->GetOverAllRange().MakeRange();
								GetDocFunc().DeleteCells( aSourceRange, NULL, DEL_DELROWS, TRUE, FALSE );
							}
						}
						break;
						case SC_CAT_DELETE_COLS:
						{
							const ScChangeActionDel* pDel = (const ScChangeActionDel*) pSourceAction;
							if ( pDel->IsTopDelete() && !pDel->IsTabDeleteCol() )
							{	// deleted Table enthaelt deleted Cols, die nicht
								aSourceRange = pDel->GetOverAllRange().MakeRange();
								GetDocFunc().DeleteCells( aSourceRange, NULL, DEL_DELCOLS, TRUE, FALSE );
							}
						}
						break;
						case SC_CAT_MOVE :
						{
							const ScChangeActionMove* pMove = (const ScChangeActionMove*) pSourceAction;
							ScRange aFromRange( pMove->GetFromRange().MakeRange() );
							GetDocFunc().MoveBlock( aFromRange,
								aSourceRange.aStart, TRUE, TRUE, FALSE, FALSE );
						}
						break;
                        default:
                        {
                            // added to avoid warnings
                        }
					}
				}
				const String& rComment = pSourceAction->GetComment();
				if ( rComment.Len() )
				{
					ScChangeAction* pAct = pThisTrack->GetLast();
                    if ( pAct && pAct->GetActionNumber() > nOldActionMax )
						pAct->SetComment( rComment );
#ifndef PRODUCT
					else
						DBG_ERROR( "MergeDocument: wohin mit dem Kommentar?!?" );
#endif
				}

				// Referenzen anpassen
				pSourceTrack->MergeOwn( (ScChangeAction*) pSourceAction, nFirstNewNumber, bShared );

                // merge action state
                if ( bShared && !pSourceAction->IsRejected() )
                {
                    ScChangeAction* pAct = pThisTrack->GetLast();
                    if ( pAct && pAct->GetActionNumber() > nOldActionMax )
                    {
                        pThisTrack->MergeActionState( pAct, pSourceAction );
                    }
                }

                // fill merge map
                if ( bShared && pMergeMap )
                {
                    ScChangeAction* pAct = pThisTrack->GetLast();
                    if ( pAct && pAct->GetActionNumber() > nOldActionMax )
                    {
                        ULONG nActionMax = pAct->GetActionNumber();
                        ULONG nActionCount = nActionMax - nOldActionMax;
                        ULONG nAction = nActionMax - nActionCount + 1;
                        ULONG nSourceAction = pSourceAction->GetActionNumber() - nActionCount + 1;
                        while ( nAction <= nActionMax )
                        {
                            if ( bInverseMap )
                            {
                                (*pMergeMap)[ nAction++ ] = nSourceAction++;
                            }
                            else
                            {
                                (*pMergeMap)[ nSourceAction++ ] = nAction++;
                            }
                        }
                    }
                }
			}
			aProgress.SetStateCountDown( --nNewActionCount );
		}
		pSourceAction = pSourceAction->GetNext();
	}

	rMarkData = aOldMarkData;
	pThisTrack->SetUser(aOldUser);
	pThisTrack->SetUseFixDateTime( FALSE );

	pSourceTrack->Clear();		//! der ist jetzt verhunzt

	if (bHasRejected)
		PostPaintGridAll();			// Reject() paintet nicht selber

    UnlockPaint();
}

bool ScDocShell::MergeSharedDocument( ScDocShell* pSharedDocShell )
{
    if ( !pSharedDocShell )
    {
        return false;
    }

    ScChangeTrack* pThisTrack = aDocument.GetChangeTrack();
    if ( !pThisTrack )
    {
        return false;
    }

    ScDocument& rSharedDoc = *( pSharedDocShell->GetDocument() );
    ScChangeTrack* pSharedTrack = rSharedDoc.GetChangeTrack();
    if ( !pSharedTrack )
    {
        return false;
    }

#if DEBUG_CHANGETRACK
    ::rtl::OUString aMessage = ::rtl::OUString::createFromAscii( "\nbefore merge:\n" );
    aMessage += pThisTrack->ToString();
    ::rtl::OString aMsg = ::rtl::OUStringToOString( aMessage, RTL_TEXTENCODING_UTF8 );
    OSL_ENSURE( false, aMsg.getStr() );
    //fprintf( stdout, "%s ", aMsg.getStr() );
    //fflush( stdout );
#endif // DEBUG_CHANGETRACK

    // reset show changes
    ScChangeViewSettings aChangeViewSet;
    aChangeViewSet.SetShowChanges( FALSE );
    aDocument.SetChangeViewSettings( aChangeViewSet );

    // find first merge action in this document
    BOOL bIgnore100Sec = !pThisTrack->IsTime100thSeconds() || !pSharedTrack->IsTime100thSeconds();
    ScChangeAction* pThisAction = pThisTrack->GetFirst();
    ScChangeAction* pSharedAction = pSharedTrack->GetFirst();
    while ( lcl_Equal( pThisAction, pSharedAction, bIgnore100Sec ) )
    {
        pThisAction = pThisAction->GetNext();
        pSharedAction = pSharedAction->GetNext();
    }

    if ( pSharedAction )
    {
        if ( pThisAction )
        {
            // merge own changes into shared document
            ULONG nActStartShared = pSharedAction->GetActionNumber();
            ULONG nActEndShared = pSharedTrack->GetActionMax();
            ScDocument* pTmpDoc = new ScDocument;
            for ( sal_Int32 nIndex = 0; nIndex < aDocument.GetTableCount(); ++nIndex )
            {
                String sTabName;
                pTmpDoc->CreateValidTabName( sTabName );
                pTmpDoc->InsertTab( SC_TAB_APPEND, sTabName );
            }
            aDocument.GetChangeTrack()->Clone( pTmpDoc );
            ScChangeActionMergeMap aOwnInverseMergeMap;
            pSharedDocShell->MergeDocument( *pTmpDoc, true, true, 0, &aOwnInverseMergeMap, true );
            delete pTmpDoc;
            ULONG nActStartOwn = nActEndShared + 1;
            ULONG nActEndOwn = pSharedTrack->GetActionMax();

            // find conflicts
            ScConflictsList aConflictsList;
            ScConflictsFinder aFinder( pSharedTrack, nActStartShared, nActEndShared, nActStartOwn, nActEndOwn, aConflictsList );
            if ( aFinder.Find() )
            {
                ScConflictsListHelper::TransformConflictsList( aConflictsList, NULL, &aOwnInverseMergeMap );
                bool bLoop = true;
                while ( bLoop )
                {
                    bLoop = false;
                    ScConflictsDlg aDlg( GetActiveDialogParent(), GetViewData(), &rSharedDoc, aConflictsList );
                    if ( aDlg.Execute() == RET_CANCEL )
                    {
                        QueryBox aBox( GetActiveDialogParent(), WinBits( WB_YES_NO | WB_DEF_YES ),
                            ScGlobal::GetRscString( STR_DOC_WILLNOTBESAVED ) );
                        if ( aBox.Execute() == RET_YES )
                        {
                            return false;
                        }
                        else
                        {
                            bLoop = true;
                        }
                    }
                }
            }

            // undo own changes in shared document
            pSharedTrack->Undo( nActStartOwn, nActEndOwn );

            // clone change track for merging into own document
            pTmpDoc = new ScDocument;
            for ( sal_Int32 nIndex = 0; nIndex < aDocument.GetTableCount(); ++nIndex )
            {
                String sTabName;
                pTmpDoc->CreateValidTabName( sTabName );
                pTmpDoc->InsertTab( SC_TAB_APPEND, sTabName );
            }
            pThisTrack->Clone( pTmpDoc );

            // undo own changes since last save in own document
            ULONG nStartShared = pThisAction->GetActionNumber();
            ScChangeAction* pAction = pThisTrack->GetLast();
            while ( pAction && pAction->GetActionNumber() >= nStartShared )
            {
                pThisTrack->Reject( pAction, true );
                pAction = pAction->GetPrev();
            }

            // #i94841# [Collaboration] When deleting rows is rejected, the content is sometimes wrong
            pThisTrack->Undo( nStartShared, pThisTrack->GetActionMax(), true );

            // merge shared changes into own document
            ScChangeActionMergeMap aSharedMergeMap;
            MergeDocument( rSharedDoc, true, true, 0, &aSharedMergeMap );
            ULONG nEndShared = pThisTrack->GetActionMax();

            // resolve conflicts for shared non-content actions
            if ( !aConflictsList.empty() )
            {
                ScConflictsListHelper::TransformConflictsList( aConflictsList, &aSharedMergeMap, NULL );
                ScConflictsResolver aResolver( pThisTrack, aConflictsList );
                pAction = pThisTrack->GetAction( nEndShared );
                while ( pAction && pAction->GetActionNumber() >= nStartShared )
                {
                    aResolver.HandleAction( pAction, true /*bIsSharedAction*/,
                        false /*bHandleContentAction*/, true /*bHandleNonContentAction*/ );
                    pAction = pAction->GetPrev();
                }
            }
            nEndShared = pThisTrack->GetActionMax();

            // only show changes from shared document
            aChangeViewSet.SetShowChanges( TRUE );
            aChangeViewSet.SetShowAccepted( TRUE );
            aChangeViewSet.SetHasActionRange( true );
            aChangeViewSet.SetTheActionRange( nStartShared, nEndShared );
            aDocument.SetChangeViewSettings( aChangeViewSet );

            // merge own changes back into own document
            ULONG nStartOwn = nEndShared + 1;
            ScChangeActionMergeMap aOwnMergeMap;
            MergeDocument( *pTmpDoc, true, true, nEndShared - nStartShared + 1, &aOwnMergeMap );
            delete pTmpDoc;
            ULONG nEndOwn = pThisTrack->GetActionMax();

            // resolve conflicts for shared content actions and own actions
            if ( !aConflictsList.empty() )
            {
                ScConflictsListHelper::TransformConflictsList( aConflictsList, NULL, &aOwnMergeMap );
                ScConflictsResolver aResolver( pThisTrack, aConflictsList );
                pAction = pThisTrack->GetAction( nEndShared );
                while ( pAction && pAction->GetActionNumber() >= nStartShared )
                {
                    aResolver.HandleAction( pAction, true /*bIsSharedAction*/,
                        true /*bHandleContentAction*/, false /*bHandleNonContentAction*/ );
                    pAction = pAction->GetPrev();
                }

                pAction = pThisTrack->GetAction( nEndOwn );
                while ( pAction && pAction->GetActionNumber() >= nStartOwn )
                {
                    aResolver.HandleAction( pAction, false /*bIsSharedAction*/,
                        true /*bHandleContentAction*/, true /*bHandleNonContentAction*/ );
                    pAction = pAction->GetPrev();
                }
            }
            nEndOwn = pThisTrack->GetActionMax();
        }
        else
        {
            // merge shared changes into own document
            ULONG nStartShared = pThisTrack->GetActionMax() + 1;
            MergeDocument( rSharedDoc, true, true );
            ULONG nEndShared = pThisTrack->GetActionMax();

            // only show changes from shared document
            aChangeViewSet.SetShowChanges( TRUE );
            aChangeViewSet.SetShowAccepted( TRUE );
            aChangeViewSet.SetHasActionRange( true );
            aChangeViewSet.SetTheActionRange( nStartShared, nEndShared );
            aDocument.SetChangeViewSettings( aChangeViewSet );
        }

        // update view
        PostPaintExtras();
        PostPaintGridAll();

        InfoBox aInfoBox( GetActiveDialogParent(), ScGlobal::GetRscString( STR_DOC_UPDATED ) );
        aInfoBox.Execute();
    }

#if DEBUG_CHANGETRACK
    aMessage = ::rtl::OUString::createFromAscii( "\nafter merge:\n" );
    aMessage += pThisTrack->ToString();
    aMsg = ::rtl::OUStringToOString( aMessage, RTL_TEXTENCODING_UTF8 );
    OSL_ENSURE( false, aMsg.getStr() );
    //fprintf( stdout, "%s ", aMsg.getStr() );
    //fflush( stdout );
#endif // DEBUG_CHANGETRACK

    return ( pThisAction != NULL );
}
