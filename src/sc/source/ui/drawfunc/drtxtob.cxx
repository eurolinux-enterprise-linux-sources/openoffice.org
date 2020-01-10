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



//-------------------------------------------------------------------------

#include "scitems.hxx"

#include <svx/adjitem.hxx>
#include <svx/clipfmtitem.hxx>
#include <svx/cntritem.hxx>
#include <svx/crsditem.hxx>
#include <svx/editeng.hxx>
#include <svx/escpitem.hxx>
#include <svx/flditem.hxx>
#include <svx/fontitem.hxx>
#include <svx/frmdiritem.hxx>
#include <svx/hlnkitem.hxx>
#include <svx/lspcitem.hxx>
#include <svx/svdoutl.hxx>
#include <svx/outlobj.hxx>
#include <svx/postitem.hxx>
#include <svx/scripttypeitem.hxx>
#include <svx/shdditem.hxx>
#include <svx/srchitem.hxx>
#include <svx/udlnitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/writingmodeitem.hxx>
#include <sfx2/app.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <svtools/cliplistener.hxx>
#include <svtools/transfer.hxx>
#include <svtools/whiter.hxx>
#include <svtools/languageoptions.hxx>
#include <vcl/msgbox.hxx>

#include <svx/svxdlg.hxx>
#include <svx/dialogs.hrc>

#include "sc.hrc"
#include "globstr.hrc"
#include "drtxtob.hxx"
#include "fudraw.hxx"
#include "viewdata.hxx"
#include "document.hxx"
#include "drawview.hxx"
#include "viewutil.hxx"
#include "scresid.hxx"
#include "tabvwsh.hxx"

#define ScDrawTextObjectBar
#include "scslots.hxx"

SFX_IMPL_INTERFACE( ScDrawTextObjectBar, SfxShell, ScResId(SCSTR_DRAWTEXTSHELL) )
{
	SFX_OBJECTBAR_REGISTRATION( SFX_OBJECTBAR_OBJECT|SFX_VISIBILITY_STANDARD|SFX_VISIBILITY_SERVER,
								ScResId(RID_TEXT_TOOLBOX) );
	SFX_POPUPMENU_REGISTRATION( ScResId(RID_POPUP_DRAWTEXT) );
	SFX_CHILDWINDOW_REGISTRATION( ScGetFontWorkId() );
}

TYPEINIT1( ScDrawTextObjectBar, SfxShell );



// abschalten der nicht erwuenschten Acceleratoren:

void ScDrawTextObjectBar::StateDisableItems( SfxItemSet &rSet )
{
	SfxWhichIter aIter(rSet);
	USHORT nWhich = aIter.FirstWhich();

	while (nWhich)
	{
		rSet.DisableItem( nWhich );
		nWhich = aIter.NextWhich();
	}
}

ScDrawTextObjectBar::ScDrawTextObjectBar(ScViewData* pData) :
	SfxShell(pData->GetViewShell()),
	pViewData(pData),
	pClipEvtLstnr(NULL),
	bPastePossible(FALSE)
{
	SetPool( pViewData->GetScDrawView()->GetDefaultAttr().GetPool() );

	//	UndoManager wird beim Umschalten in den Edit-Modus umgesetzt...
    SfxUndoManager* pMgr = pViewData->GetSfxDocShell()->GetUndoManager();
    SetUndoManager( pMgr );
    if ( !pViewData->GetDocument()->IsUndoEnabled() )
    {
        pMgr->SetMaxUndoActionCount( 0 );
    }

	SetHelpId( HID_SCSHELL_DRTXTOB );
	SetName(String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("DrawText")));
}

__EXPORT ScDrawTextObjectBar::~ScDrawTextObjectBar()
{
	if ( pClipEvtLstnr )
	{
		pClipEvtLstnr->AddRemoveListener( pViewData->GetActiveWin(), FALSE );

        //  #122057# The listener may just now be waiting for the SolarMutex and call the link
        //  afterwards, in spite of RemoveListener. So the link has to be reset, too.
        pClipEvtLstnr->ClearCallbackLink();

		pClipEvtLstnr->release();
	}
}

//========================================================================
//
//			Funktionen
//
//========================================================================

void __EXPORT ScDrawTextObjectBar::Execute( SfxRequest &rReq )
{
	ScDrawView* pView = pViewData->GetScDrawView();
	OutlinerView* pOutView = pView->GetTextEditOutlinerView();
	Outliner* pOutliner = pView->GetTextEditOutliner();

	if (!pOutView || !pOutliner)
	{
		ExecuteGlobal( rReq );				// auf ganze Objekte
		return;
	}

	const SfxItemSet* pReqArgs = rReq.GetArgs();
	USHORT nSlot = rReq.GetSlot();
	switch ( nSlot )
	{
		case SID_COPY:
			pOutView->Copy();
			break;

		case SID_CUT:
			pOutView->Cut();
			break;

		case SID_PASTE:
			pOutView->PasteSpecial();
			break;

		case SID_CLIPBOARD_FORMAT_ITEMS:
			{
				ULONG nFormat = 0;
				const SfxPoolItem* pItem;
				if ( pReqArgs &&
					 pReqArgs->GetItemState(nSlot, TRUE, &pItem) == SFX_ITEM_SET &&
					 pItem->ISA(SfxUInt32Item) )
				{
					nFormat = ((const SfxUInt32Item*)pItem)->GetValue();
				}

				if ( nFormat )
				{
					if (nFormat == SOT_FORMAT_STRING)
						pOutView->Paste();
					else
						pOutView->PasteSpecial();
				}
			}
			break;

        case SID_PASTE_SPECIAL:
			ExecutePasteContents( rReq );
			break;

		case SID_SELECTALL:
			{
				ULONG nCount = pOutliner->GetParagraphCount();
				ESelection aSel( 0,0,(USHORT)nCount,0 );
				pOutView->SetSelection( aSel );
			}
			break;

		case SID_CHARMAP:
			{
				const SvxFontItem& rItem = (const SvxFontItem&)
							pOutView->GetAttribs().Get(EE_CHAR_FONTINFO);

				String aString;
				SvxFontItem aNewItem( EE_CHAR_FONTINFO );

				const SfxItemSet *pArgs = rReq.GetArgs();
				const SfxPoolItem* pItem = 0;
				if( pArgs )
					pArgs->GetItemState(GetPool().GetWhich(SID_CHARMAP), FALSE, &pItem);

				if ( pItem )
				{
					aString = ((const SfxStringItem*)pItem)->GetValue();
					const SfxPoolItem* pFtItem = NULL;
					pArgs->GetItemState( GetPool().GetWhich(SID_ATTR_SPECIALCHAR), FALSE, &pFtItem);
					const SfxStringItem* pFontItem = PTR_CAST( SfxStringItem, pFtItem );
					if ( pFontItem )
					{
						String aFontName(pFontItem->GetValue());
					    Font aFont(aFontName, Size(1,1)); // Size nur wg. CTOR
						aNewItem = SvxFontItem( aFont.GetFamily(), aFont.GetName(),
									aFont.GetStyleName(), aFont.GetPitch(),
                                    aFont.GetCharSet(), ATTR_FONT  );
					}
					else
						aNewItem = rItem;
				}
				else 
					ScViewUtil::ExecuteCharMap( rItem, *pViewData->GetViewShell()->GetViewFrame(), aNewItem, aString );

				if ( aString.Len() )
				{
					SfxItemSet aSet( pOutliner->GetEmptyItemSet() );
					aSet.Put( aNewItem );
					//	SetAttribs an der View selektiert ein Wort, wenn nichts selektiert ist
					pOutView->GetOutliner()->QuickSetAttribs( aSet, pOutView->GetSelection() );
					pOutView->InsertText(aString);
				}

				Invalidate( SID_ATTR_CHAR_FONT );
			}
			break;

		case SID_HYPERLINK_SETLINK:
			if( pReqArgs )
			{
				const SfxPoolItem* pItem;
				if ( pReqArgs->GetItemState( SID_HYPERLINK_SETLINK, TRUE, &pItem ) == SFX_ITEM_SET )
				{
					const SvxHyperlinkItem* pHyper = (const SvxHyperlinkItem*) pItem;
					const String& rName		= pHyper->GetName();
					const String& rURL		= pHyper->GetURL();
					const String& rTarget	= pHyper->GetTargetFrame();
					SvxLinkInsertMode eMode = pHyper->GetInsertMode();

					BOOL bDone = FALSE;
					if ( pOutView && ( eMode == HLINK_DEFAULT || eMode == HLINK_FIELD ) )
					{
						const SvxFieldItem*	pFieldItem = pOutView->GetFieldAtSelection();
						if (pFieldItem)
						{
							const SvxFieldData* pField = pFieldItem->GetField();
							if ( pField && pField->ISA(SvxURLField) )
							{
								//	altes Feld selektieren

								ESelection aSel = pOutView->GetSelection();
								aSel.Adjust();
								aSel.nEndPara = aSel.nStartPara;
								aSel.nEndPos = aSel.nStartPos + 1;
								pOutView->SetSelection( aSel );
							}
						}

						//	neues Feld einfuegen

						SvxURLField aURLField( rURL, rName, SVXURLFORMAT_REPR );
						aURLField.SetTargetFrame( rTarget );
                        SvxFieldItem aURLItem( aURLField, EE_FEATURE_FIELD );
						pOutView->InsertField( aURLItem );

						//	select new field

						ESelection aSel = pOutView->GetSelection();
						if ( aSel.nStartPos == aSel.nEndPos && aSel.nStartPos > 0 )
						{
							//	Cursor is behind the inserted field -> extend selection to the left

							--aSel.nStartPos;
							pOutView->SetSelection( aSel );
						}

						bDone = TRUE;
					}

					if (!bDone)
						ExecuteGlobal( rReq );		// normal an der View

					//	InsertURL an der ViewShell schaltet bei "Text" die DrawShell ab !!!
				}
			}
			break;

        case SID_OPEN_HYPERLINK:
            {
                if ( pOutView )
                {
                    const SvxFieldItem* pFieldItem = pOutView->GetFieldAtSelection();
                    if ( pFieldItem )
                    {
                        const SvxFieldData* pField = pFieldItem->GetField();
                        if( pField && pField->ISA( SvxURLField ) )
                        {
                            const SvxURLField* pURLField = static_cast< const SvxURLField* >( pField );
                            ScGlobal::OpenURL( pURLField->GetURL(), pURLField->GetTargetFrame() );
                        }
                    }
                }
            }
            break;

		case SID_ENABLE_HYPHENATION:
		case SID_TEXTDIRECTION_LEFT_TO_RIGHT:
		case SID_TEXTDIRECTION_TOP_TO_BOTTOM:
#if 0 // DR
		    if (IsNoteEdit())
		    {
		        pView->CaptionTextDirection( rReq.GetSlot());     // process Notes before we end the text edit.
		        ExecuteGlobal( rReq );
		        pViewData->GetDispatcher().Execute(pViewData->GetView()->GetDrawFuncPtr()->GetSlotID(), SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD);
		    }
		    else
#endif
		    {
		        pView->ScEndTextEdit();	// end text edit before switching direction
		        ExecuteGlobal( rReq );
		        // restore consistent state between shells and functions:
		        pViewData->GetDispatcher().Execute(SID_OBJECT_SELECT, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD);
		    }
            break;

#if 0
		// Hyphenation is handled above - text edit is ended
		case SID_ENABLE_HYPHENATION:
			// force loading of hyphenator (object is skipped in repaint)
			((ScDrawLayer*)pView->GetModel())->UseHyphenator();
			pOutliner->SetHyphenator( LinguMgr::GetHyphenator() );
			ExecuteGlobal( rReq );
			break;
#endif
	}
}

void __EXPORT ScDrawTextObjectBar::GetState( SfxItemSet& rSet )
{
	SfxViewFrame* pViewFrm = pViewData->GetViewShell()->GetViewFrame();
	BOOL bHasFontWork = pViewFrm->HasChildWindow(SID_FONTWORK);
	BOOL bDisableFontWork = FALSE;

	if (IsNoteEdit())
	{
        // #i21255# notes now support rich text formatting (#i74140# but not fontwork)
        bDisableFontWork = TRUE;
	}

	if ( bDisableFontWork )
		rSet.DisableItem( SID_FONTWORK	);
	else
		rSet.Put(SfxBoolItem(SID_FONTWORK, bHasFontWork));

	if ( rSet.GetItemState( SID_HYPERLINK_GETLINK ) != SFX_ITEM_UNKNOWN )
	{
		SvxHyperlinkItem aHLinkItem;
		SdrView* pView = pViewData->GetScDrawView();
		OutlinerView* pOutView = pView->GetTextEditOutlinerView();
		if ( pOutView )
		{
			BOOL bField = FALSE;
			const SvxFieldItem*	pFieldItem = pOutView->GetFieldAtSelection();
			if (pFieldItem)
			{
				const SvxFieldData* pField = pFieldItem->GetField();
				if ( pField && pField->ISA(SvxURLField) )
				{
					const SvxURLField* pURLField = (const SvxURLField*) pField;
					aHLinkItem.SetName( pURLField->GetRepresentation() );
					aHLinkItem.SetURL( pURLField->GetURL() );
					aHLinkItem.SetTargetFrame( pURLField->GetTargetFrame() );
					bField = TRUE;
				}
			}
			if (!bField)
			{
				// use selected text as name for urls
				String sReturn = pOutView->GetSelected();
				sReturn.Erase(255);
				sReturn.EraseTrailingChars();
				aHLinkItem.SetName(sReturn);
			}
		}
		rSet.Put(aHLinkItem);
	}

    if ( rSet.GetItemState( SID_OPEN_HYPERLINK ) != SFX_ITEM_UNKNOWN )
    {
        SdrView* pView = pViewData->GetScDrawView();
        OutlinerView* pOutView = pView->GetTextEditOutlinerView();
        bool bEnable = false;
        if ( pOutView )
        {
            const SvxFieldItem* pFieldItem = pOutView->GetFieldAtSelection();
            if ( pFieldItem )
            {
                const SvxFieldData* pField = pFieldItem->GetField();
                bEnable = pField && pField->ISA( SvxURLField );
            }
        }
        if( !bEnable )
            rSet.DisableItem( SID_OPEN_HYPERLINK );
    }

    if( rSet.GetItemState( SID_TRANSLITERATE_HALFWIDTH ) != SFX_ITEM_UNKNOWN )
        ScViewUtil::HideDisabledSlot( rSet, pViewFrm->GetBindings(), SID_TRANSLITERATE_HALFWIDTH );
    if( rSet.GetItemState( SID_TRANSLITERATE_FULLWIDTH ) != SFX_ITEM_UNKNOWN )
        ScViewUtil::HideDisabledSlot( rSet, pViewFrm->GetBindings(), SID_TRANSLITERATE_FULLWIDTH );
    if( rSet.GetItemState( SID_TRANSLITERATE_HIRAGANA ) != SFX_ITEM_UNKNOWN )
        ScViewUtil::HideDisabledSlot( rSet, pViewFrm->GetBindings(), SID_TRANSLITERATE_HIRAGANA );
    if( rSet.GetItemState( SID_TRANSLITERATE_KATAGANA ) != SFX_ITEM_UNKNOWN )
        ScViewUtil::HideDisabledSlot( rSet, pViewFrm->GetBindings(), SID_TRANSLITERATE_KATAGANA );

	if ( rSet.GetItemState( SID_ENABLE_HYPHENATION ) != SFX_ITEM_UNKNOWN )
	{
		SdrView* pView = pViewData->GetScDrawView();
		SfxItemSet aAttrs( pView->GetModel()->GetItemPool() );
		pView->GetAttributes( aAttrs );
		if( aAttrs.GetItemState( EE_PARA_HYPHENATE ) >= SFX_ITEM_AVAILABLE )
		{
			BOOL bValue = ( (const SfxBoolItem&) aAttrs.Get( EE_PARA_HYPHENATE ) ).GetValue();
			rSet.Put( SfxBoolItem( SID_ENABLE_HYPHENATION, bValue ) );
		}
	}
}

IMPL_LINK( ScDrawTextObjectBar, ClipboardChanged, TransferableDataHelper*, pDataHelper )
{
	if ( pDataHelper )
	{
		bPastePossible = ( pDataHelper->HasFormat( SOT_FORMAT_STRING ) || pDataHelper->HasFormat( SOT_FORMAT_RTF ) );

		SfxBindings& rBindings = pViewData->GetBindings();
		rBindings.Invalidate( SID_PASTE );
        rBindings.Invalidate( SID_PASTE_SPECIAL );
		rBindings.Invalidate( SID_CLIPBOARD_FORMAT_ITEMS );
	}
	return 0;
}

void __EXPORT ScDrawTextObjectBar::GetClipState( SfxItemSet& rSet )
{
	SdrView* pView = pViewData->GetScDrawView();
	if ( !pView->GetTextEditOutlinerView() )
	{
		GetGlobalClipState( rSet );
		return;
	}

	if ( !pClipEvtLstnr )
	{
		// create listener
		pClipEvtLstnr = new TransferableClipboardListener( LINK( this, ScDrawTextObjectBar, ClipboardChanged ) );
		pClipEvtLstnr->acquire();
		Window* pWin = pViewData->GetActiveWin();
		pClipEvtLstnr->AddRemoveListener( pWin, TRUE );

		// get initial state
		TransferableDataHelper aDataHelper( TransferableDataHelper::CreateFromSystemClipboard( pViewData->GetActiveWin() ) );
		bPastePossible = ( aDataHelper.HasFormat( SOT_FORMAT_STRING ) || aDataHelper.HasFormat( SOT_FORMAT_RTF ) );
	}

	SfxWhichIter aIter( rSet );
	USHORT nWhich = aIter.FirstWhich();
	while (nWhich)
	{
		switch (nWhich)
		{
			case SID_PASTE:
            case SID_PASTE_SPECIAL:
				if( !bPastePossible )
					rSet.DisableItem( nWhich );
				break;
			case SID_CLIPBOARD_FORMAT_ITEMS:
				if ( bPastePossible )
				{
					SvxClipboardFmtItem aFormats( SID_CLIPBOARD_FORMAT_ITEMS );
					TransferableDataHelper aDataHelper(
							TransferableDataHelper::CreateFromSystemClipboard( pViewData->GetActiveWin() ) );

					if ( aDataHelper.HasFormat( SOT_FORMAT_STRING ) )
						aFormats.AddClipbrdFormat( SOT_FORMAT_STRING );
					if ( aDataHelper.HasFormat( SOT_FORMAT_RTF ) )
						aFormats.AddClipbrdFormat( SOT_FORMAT_RTF );

					rSet.Put( aFormats );
				}
				else
					rSet.DisableItem( nWhich );
				break;
		}
		nWhich = aIter.NextWhich();
	}
}

//========================================================================
//
//			Attribute
//
//========================================================================

void __EXPORT ScDrawTextObjectBar::ExecuteToggle( SfxRequest &rReq )
{
	//	Unterstreichung

	SdrView* pView = pViewData->GetScDrawView();

	USHORT nSlot = rReq.GetSlot();

	SfxItemSet aSet( pView->GetDefaultAttr() );

	SfxItemSet aViewAttr(pView->GetModel()->GetItemPool());
	pView->GetAttributes(aViewAttr);

	//	Unterstreichung
	FontUnderline eOld = ((const SvxUnderlineItem&) aViewAttr.
										Get(EE_CHAR_UNDERLINE)).GetLineStyle();
	FontUnderline eNew = eOld;
	switch (nSlot)
	{
		case SID_ULINE_VAL_NONE:
			eNew = UNDERLINE_NONE;
			break;
		case SID_ULINE_VAL_SINGLE:
			eNew = ( eOld == UNDERLINE_SINGLE ) ? UNDERLINE_NONE : UNDERLINE_SINGLE;
			break;
		case SID_ULINE_VAL_DOUBLE:
			eNew = ( eOld == UNDERLINE_DOUBLE ) ? UNDERLINE_NONE : UNDERLINE_DOUBLE;
			break;
		case SID_ULINE_VAL_DOTTED:
			eNew = ( eOld == UNDERLINE_DOTTED ) ? UNDERLINE_NONE : UNDERLINE_DOTTED;
			break;
		default:
			break;
	}
	aSet.Put( SvxUnderlineItem( eNew, EE_CHAR_UNDERLINE ) );

	pView->SetAttributes( aSet );
	rReq.Done();
	pViewData->GetScDrawView()->InvalidateDrawTextAttrs();
}

void lcl_RemoveFields( OutlinerView& rOutView )
{
	//!	Outliner should have RemoveFields with a selection

	Outliner* pOutliner = rOutView.GetOutliner();
	if (!pOutliner) return;

	ESelection aOldSel = rOutView.GetSelection();
	ESelection aSel = aOldSel;
	aSel.Adjust();
	xub_StrLen nNewEnd = aSel.nEndPos;

	BOOL bUpdate = pOutliner->GetUpdateMode();
	BOOL bChanged = FALSE;

	//!	GetPortions and GetAttribs should be const!
	EditEngine& rEditEng = (EditEngine&)pOutliner->GetEditEngine();

	ULONG nParCount = pOutliner->GetParagraphCount();
	for (ULONG nPar=0; nPar<nParCount; nPar++)
		if ( nPar >= aSel.nStartPara && nPar <= aSel.nEndPara )
		{
			SvUShorts aPortions;
			rEditEng.GetPortions( (USHORT)nPar, aPortions );
			//!	GetPortions should use xub_StrLen instead of USHORT

			for ( USHORT nPos = aPortions.Count(); nPos; )
			{
				--nPos;
				USHORT nEnd = aPortions.GetObject( nPos );
				USHORT nStart = nPos ? aPortions.GetObject( nPos - 1 ) : 0;
				// fields are single characters
				if ( nEnd == nStart+1 &&
					 ( nPar > aSel.nStartPara || nStart >= aSel.nStartPos ) &&
					 ( nPar < aSel.nEndPara   || nEnd   <= aSel.nEndPos ) )
				{
					ESelection aFieldSel( (USHORT)nPar, nStart, (USHORT)nPar, nEnd );
					SfxItemSet aSet = rEditEng.GetAttribs( aFieldSel );
					if ( aSet.GetItemState( EE_FEATURE_FIELD ) == SFX_ITEM_ON )
					{
						if (!bChanged)
						{
							if (bUpdate)
								pOutliner->SetUpdateMode( FALSE );
							String aName = ScGlobal::GetRscString( STR_UNDO_DELETECONTENTS );
							pOutliner->GetUndoManager().EnterListAction( aName, aName );
							bChanged = TRUE;
						}

						String aFieldText = rEditEng.GetText( aFieldSel );
						pOutliner->QuickInsertText( aFieldText, aFieldSel );
						if ( nPar == aSel.nEndPara )
						{
                            nNewEnd = sal::static_int_cast<xub_StrLen>( nNewEnd + aFieldText.Len() );
							--nNewEnd;
						}
					}
				}
			}
		}

	if (bUpdate && bChanged)
	{
		pOutliner->GetUndoManager().LeaveListAction();
		pOutliner->SetUpdateMode( TRUE );
	}

	if ( aOldSel.IsEqual( aSel ) )			// aSel is adjusted
		aOldSel.nEndPos = nNewEnd;
	else
		aOldSel.nStartPos = nNewEnd;		// if aOldSel is backwards
	rOutView.SetSelection( aOldSel );
}

void __EXPORT ScDrawTextObjectBar::ExecuteAttr( SfxRequest &rReq )
{
	SdrView*			pView = pViewData->GetScDrawView();
	const SfxItemSet*	pArgs = rReq.GetArgs();
	USHORT				nSlot = rReq.GetSlot();

	BOOL bArgsInReq = ( pArgs != NULL );
	if ( !bArgsInReq )
	{
		SfxItemSet aEditAttr(pView->GetModel()->GetItemPool());
		pView->GetAttributes(aEditAttr);
		SfxItemSet	aNewAttr( *aEditAttr.GetPool(), aEditAttr.GetRanges() );
		BOOL		bDone = TRUE;

		switch ( nSlot )
		{
			case SID_TEXT_STANDARD:	// Harte Textattributierung loeschen
			{
				OutlinerView* pOutView = pView->IsTextEdit() ?
								pView->GetTextEditOutlinerView() : NULL;
				if ( pOutView )
					pOutView->Paint( Rectangle() );

				SfxItemSet aEmptyAttr( *aEditAttr.GetPool(), EE_ITEMS_START, EE_ITEMS_END );
				pView->SetAttributes( aEmptyAttr, TRUE );

				if ( pOutView )
				{
					lcl_RemoveFields( *pOutView );
					pOutView->ShowCursor();
				}

				rReq.Done( aEmptyAttr );
				pViewData->GetScDrawView()->InvalidateDrawTextAttrs();
				bDone = FALSE; // bereits hier passiert
			}
			break;

			case SID_CHAR_DLG:						// Dialog-Button
			case SID_ATTR_CHAR_FONT:				// Controller nicht angezeigt
			case SID_ATTR_CHAR_FONTHEIGHT:
				bDone = ExecuteCharDlg( aEditAttr, aNewAttr );
				break;

			case SID_PARA_DLG:
				bDone = ExecuteParaDlg( aEditAttr, aNewAttr );
				break;

			case SID_ATTR_CHAR_WEIGHT:
				aNewAttr.Put( (const SvxWeightItem&)aEditAttr.Get( EE_CHAR_WEIGHT ) );
				break;

			case SID_ATTR_CHAR_POSTURE:
				aNewAttr.Put( (const SvxPostureItem&)aEditAttr.Get( EE_CHAR_ITALIC ) );
				break;

			case SID_ATTR_CHAR_UNDERLINE:
				aNewAttr.Put( (const SvxUnderlineItem&)aEditAttr.Get( EE_CHAR_UNDERLINE ) );
				break;

			case SID_ATTR_CHAR_OVERLINE:
				aNewAttr.Put( (const SvxOverlineItem&)aEditAttr.Get( EE_CHAR_OVERLINE ) );
				break;

			case SID_ATTR_CHAR_CONTOUR:
				aNewAttr.Put( (const SvxContourItem&)aEditAttr.Get( EE_CHAR_OUTLINE ) );
				break;

			case SID_ATTR_CHAR_SHADOWED:
				aNewAttr.Put( (const SvxShadowedItem&)aEditAttr.Get( EE_CHAR_SHADOW ) );
				break;

			case SID_ATTR_CHAR_STRIKEOUT:
				aNewAttr.Put( (const SvxCrossedOutItem&)aEditAttr.Get( EE_CHAR_STRIKEOUT ) );
				break;

			case SID_ALIGNLEFT:
            case SID_ALIGN_ANY_LEFT:
				aNewAttr.Put( SvxAdjustItem( SVX_ADJUST_LEFT, EE_PARA_JUST ) );
				break;

			case SID_ALIGNCENTERHOR:
            case SID_ALIGN_ANY_HCENTER:
				aNewAttr.Put( SvxAdjustItem( SVX_ADJUST_CENTER, EE_PARA_JUST ) );
				break;

			case SID_ALIGNRIGHT:
            case SID_ALIGN_ANY_RIGHT:
				aNewAttr.Put( SvxAdjustItem( SVX_ADJUST_RIGHT, EE_PARA_JUST ) );
				break;

			case SID_ALIGNBLOCK:
            case SID_ALIGN_ANY_JUSTIFIED:
				aNewAttr.Put( SvxAdjustItem( SVX_ADJUST_BLOCK, EE_PARA_JUST ) );
				break;

			case SID_ATTR_PARA_LINESPACE_10:
				{
					SvxLineSpacingItem aItem( SVX_LINESPACE_ONE_LINE, EE_PARA_SBL );
					aItem.SetPropLineSpace( 100 );
					aNewAttr.Put( aItem );
				}
				break;

			case SID_ATTR_PARA_LINESPACE_15:
				{
					SvxLineSpacingItem aItem( SVX_LINESPACE_ONE_POINT_FIVE_LINES, EE_PARA_SBL );
					aItem.SetPropLineSpace( 150 );
					aNewAttr.Put( aItem );
				}
				break;

			case SID_ATTR_PARA_LINESPACE_20:
				{
					SvxLineSpacingItem aItem( SVX_LINESPACE_TWO_LINES, EE_PARA_SBL );
					aItem.SetPropLineSpace( 200 );
					aNewAttr.Put( aItem );
				}
				break;

			case SID_SET_SUPER_SCRIPT:
				{
					SvxEscapementItem aItem(EE_CHAR_ESCAPEMENT);
					SvxEscapement eEsc = (SvxEscapement) ( (const SvxEscapementItem&)
									aEditAttr.Get( EE_CHAR_ESCAPEMENT ) ).GetEnumValue();

					if( eEsc == SVX_ESCAPEMENT_SUPERSCRIPT )
						aItem.SetEscapement( SVX_ESCAPEMENT_OFF );
					else
						aItem.SetEscapement( SVX_ESCAPEMENT_SUPERSCRIPT );
					aNewAttr.Put( aItem );
				}
				break;
			case SID_SET_SUB_SCRIPT:
				{
					SvxEscapementItem aItem(EE_CHAR_ESCAPEMENT);
					SvxEscapement eEsc = (SvxEscapement) ( (const SvxEscapementItem&)
									aEditAttr.Get( EE_CHAR_ESCAPEMENT ) ).GetEnumValue();

					if( eEsc == SVX_ESCAPEMENT_SUBSCRIPT )
						aItem.SetEscapement( SVX_ESCAPEMENT_OFF );
					else
						aItem.SetEscapement( SVX_ESCAPEMENT_SUBSCRIPT );
					aNewAttr.Put( aItem );
				}
				break;

			case SID_DRAWTEXT_ATTR_DLG:
				{
					SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
					SfxAbstractTabDialog *pDlg = pFact->CreateTextTabDialog( pViewData->GetDialogParent(), &aEditAttr, RID_SVXDLG_TEXT, pView );

					bDone = ( RET_OK == pDlg->Execute() );

					if ( bDone )
						aNewAttr.Put( *pDlg->GetOutputItemSet() );

					delete pDlg;
				}
				break;
		}

		if ( bDone ) // wurden Attribute geaendert?
		{
			rReq.Done( aNewAttr );
			pArgs = rReq.GetArgs();
		}
	}

	if ( pArgs )
	{
		if ( bArgsInReq &&
			( nSlot == SID_ATTR_CHAR_FONT || nSlot == SID_ATTR_CHAR_FONTHEIGHT ||
			  nSlot == SID_ATTR_CHAR_WEIGHT || nSlot == SID_ATTR_CHAR_POSTURE ) )
		{
			// font items from toolbox controller have to be applied for the right script type

			// #i78017 establish the same behaviour as in Writer
			USHORT nScript = SCRIPTTYPE_LATIN | SCRIPTTYPE_ASIAN | SCRIPTTYPE_COMPLEX;
			if (nSlot == SID_ATTR_CHAR_FONT)
				nScript = pView->GetScriptType();

			SfxItemPool& rPool = GetPool();
			SvxScriptSetItem aSetItem( nSlot, rPool );
			USHORT nWhich = rPool.GetWhich( nSlot );
			aSetItem.PutItemForScriptType( nScript, pArgs->Get( nWhich ) );

			pView->SetAttributes( aSetItem.GetItemSet() );
		}
		else
		{
			// use args directly

			pView->SetAttributes( *pArgs );
		}
		pViewData->GetScDrawView()->InvalidateDrawTextAttrs();
	}
}

void __EXPORT ScDrawTextObjectBar::GetAttrState( SfxItemSet& rDestSet )
{
	if ( IsNoteEdit() )
	{
	    // issue 21255 - Notes now support rich text formatting.
	}

	SvtLanguageOptions	aLangOpt;
	BOOL bDisableCTLFont = !aLangOpt.IsCTLFontEnabled();
	BOOL bDisableVerticalText = !aLangOpt.IsVerticalTextEnabled();

	SdrView* pView = pViewData->GetScDrawView();
	SfxItemSet aAttrSet(pView->GetModel()->GetItemPool());
	pView->GetAttributes(aAttrSet);

	//	direkte Attribute

	rDestSet.Put( aAttrSet );

	//	choose font info according to selection script type

	USHORT nScript = pView->GetScriptType();

    // #i55929# input-language-dependent script type (depends on input language if nothing selected)
    USHORT nInputScript = nScript;
    OutlinerView* pOutView = pView->GetTextEditOutlinerView();
    if (pOutView && !pOutView->GetSelection().HasRange())
    {
        LanguageType nInputLang = pViewData->GetActiveWin()->GetInputLanguage();
        if (nInputLang != LANGUAGE_DONTKNOW && nInputLang != LANGUAGE_SYSTEM)
            nInputScript = SvtLanguageOptions::GetScriptTypeOfLanguage( nInputLang );
    }

    // #i55929# according to spec, nInputScript is used for font and font height only
	if ( rDestSet.GetItemState( EE_CHAR_FONTINFO ) != SFX_ITEM_UNKNOWN )
		ScViewUtil::PutItemScript( rDestSet, aAttrSet, EE_CHAR_FONTINFO, nInputScript );
	if ( rDestSet.GetItemState( EE_CHAR_FONTHEIGHT ) != SFX_ITEM_UNKNOWN )
		ScViewUtil::PutItemScript( rDestSet, aAttrSet, EE_CHAR_FONTHEIGHT, nInputScript );
	if ( rDestSet.GetItemState( EE_CHAR_WEIGHT ) != SFX_ITEM_UNKNOWN )
		ScViewUtil::PutItemScript( rDestSet, aAttrSet, EE_CHAR_WEIGHT, nScript );
	if ( rDestSet.GetItemState( EE_CHAR_ITALIC ) != SFX_ITEM_UNKNOWN )
		ScViewUtil::PutItemScript( rDestSet, aAttrSet, EE_CHAR_ITALIC, nScript );

	//	Ausrichtung

	SvxAdjust eAdj = ((const SvxAdjustItem&)aAttrSet.Get(EE_PARA_JUST)).GetAdjust();
	switch( eAdj )
	{
		case SVX_ADJUST_LEFT:
			rDestSet.Put( SfxBoolItem( SID_ALIGNLEFT, TRUE ) );
			break;
		case SVX_ADJUST_CENTER:
			rDestSet.Put( SfxBoolItem( SID_ALIGNCENTERHOR, TRUE ) );
			break;
		case SVX_ADJUST_RIGHT:
			rDestSet.Put( SfxBoolItem( SID_ALIGNRIGHT, TRUE ) );
			break;
		case SVX_ADJUST_BLOCK:
			rDestSet.Put( SfxBoolItem( SID_ALIGNBLOCK, TRUE ) );
			break;
        default:
        {
            // added to avoid warnings
        }
	}
    // pseudo slots for Format menu
    rDestSet.Put( SfxBoolItem( SID_ALIGN_ANY_LEFT,      eAdj == SVX_ADJUST_LEFT ) );
    rDestSet.Put( SfxBoolItem( SID_ALIGN_ANY_HCENTER,   eAdj == SVX_ADJUST_CENTER ) );
    rDestSet.Put( SfxBoolItem( SID_ALIGN_ANY_RIGHT,     eAdj == SVX_ADJUST_RIGHT ) );
    rDestSet.Put( SfxBoolItem( SID_ALIGN_ANY_JUSTIFIED, eAdj == SVX_ADJUST_BLOCK ) );

	//	Zeilenabstand

	USHORT nLineSpace = (USHORT)
				((const SvxLineSpacingItem&)aAttrSet.
						Get( EE_PARA_SBL )).GetPropLineSpace();
	switch( nLineSpace )
	{
		case 100:
			rDestSet.Put( SfxBoolItem( SID_ATTR_PARA_LINESPACE_10, TRUE ) );
			break;
		case 150:
			rDestSet.Put( SfxBoolItem( SID_ATTR_PARA_LINESPACE_15, TRUE ) );
			break;
		case 200:
			rDestSet.Put( SfxBoolItem( SID_ATTR_PARA_LINESPACE_20, TRUE ) );
			break;
	}

	//	hoch-/tiefgestellt

	SvxEscapement eEsc = (SvxEscapement) ( (const SvxEscapementItem&)
					aAttrSet.Get( EE_CHAR_ESCAPEMENT ) ).GetEnumValue();
	if( eEsc == SVX_ESCAPEMENT_SUPERSCRIPT )
		rDestSet.Put( SfxBoolItem( SID_SET_SUPER_SCRIPT, TRUE ) );
	else if( eEsc == SVX_ESCAPEMENT_SUBSCRIPT )
		rDestSet.Put( SfxBoolItem( SID_SET_SUB_SCRIPT, TRUE ) );

	//	Unterstreichung

	SfxItemState eState = aAttrSet.GetItemState( EE_CHAR_UNDERLINE, TRUE );
	if ( eState == SFX_ITEM_DONTCARE )
	{
		rDestSet.InvalidateItem( SID_ULINE_VAL_NONE );
		rDestSet.InvalidateItem( SID_ULINE_VAL_SINGLE );
		rDestSet.InvalidateItem( SID_ULINE_VAL_DOUBLE );
		rDestSet.InvalidateItem( SID_ULINE_VAL_DOTTED );
	}
	else
	{
		FontUnderline eUnderline = ((const SvxUnderlineItem&)
					aAttrSet.Get(EE_CHAR_UNDERLINE)).GetLineStyle();
		USHORT nId = SID_ULINE_VAL_NONE;
		switch (eUnderline)
		{
			case UNDERLINE_SINGLE:	nId = SID_ULINE_VAL_SINGLE;	break;
			case UNDERLINE_DOUBLE:	nId = SID_ULINE_VAL_DOUBLE;	break;
			case UNDERLINE_DOTTED:	nId = SID_ULINE_VAL_DOTTED;	break;
			default:
				break;
		}
		rDestSet.Put( SfxBoolItem( nId, TRUE ) );
	}

	//	horizontal / vertical

	BOOL bLeftToRight = TRUE;

	SdrOutliner* pOutl = pView->GetTextEditOutliner();
	if( pOutl )
	{
		if( pOutl->IsVertical() )
			bLeftToRight = FALSE;
	}
	else
		bLeftToRight = ( (const SvxWritingModeItem&) aAttrSet.Get( SDRATTR_TEXTDIRECTION ) ).GetValue() == com::sun::star::text::WritingMode_LR_TB;

	if ( bDisableVerticalText )
	{
		rDestSet.DisableItem( SID_TEXTDIRECTION_LEFT_TO_RIGHT );
		rDestSet.DisableItem( SID_TEXTDIRECTION_TOP_TO_BOTTOM );
	}
	else
	{
		rDestSet.Put( SfxBoolItem( SID_TEXTDIRECTION_LEFT_TO_RIGHT, bLeftToRight ) );
		rDestSet.Put( SfxBoolItem( SID_TEXTDIRECTION_TOP_TO_BOTTOM, !bLeftToRight ) );
	}

	//	left-to-right or right-to-left

	if ( !bLeftToRight || bDisableCTLFont )
	{
		//	disabled if vertical
		rDestSet.DisableItem( SID_ATTR_PARA_LEFT_TO_RIGHT );
		rDestSet.DisableItem( SID_ATTR_PARA_RIGHT_TO_LEFT );
	}
	else if ( aAttrSet.GetItemState( EE_PARA_WRITINGDIR ) == SFX_ITEM_DONTCARE )
	{
		rDestSet.InvalidateItem( SID_ATTR_PARA_LEFT_TO_RIGHT );
		rDestSet.InvalidateItem( SID_ATTR_PARA_RIGHT_TO_LEFT );
	}
	else
	{
	    SvxFrameDirection eAttrDir = (SvxFrameDirection)((const SvxFrameDirectionItem&)
	                                    aAttrSet.Get( EE_PARA_WRITINGDIR )).GetValue();
	    if ( eAttrDir == FRMDIR_ENVIRONMENT )
	    {
	    	//	get "environment" direction from page style
	    	if ( pViewData->GetDocument()->GetEditTextDirection( pViewData->GetTabNo() ) == EE_HTEXTDIR_R2L )
		    	eAttrDir = FRMDIR_HORI_RIGHT_TOP;
		    else
		    	eAttrDir = FRMDIR_HORI_LEFT_TOP;
	    }
		rDestSet.Put( SfxBoolItem( SID_ATTR_PARA_LEFT_TO_RIGHT, ( eAttrDir == FRMDIR_HORI_LEFT_TOP ) ) );
		rDestSet.Put( SfxBoolItem( SID_ATTR_PARA_RIGHT_TO_LEFT, ( eAttrDir == FRMDIR_HORI_RIGHT_TOP ) ) );
	}
}

void ScDrawTextObjectBar::ExecuteTrans( SfxRequest& rReq )
{
	sal_Int32 nType = ScViewUtil::GetTransliterationType( rReq.GetSlot() );
	if ( nType )
	{
		ScDrawView* pView = pViewData->GetScDrawView();
		OutlinerView* pOutView = pView->GetTextEditOutlinerView();
		if ( pOutView )
		{
			//	change selected text in object
			pOutView->TransliterateText( nType );
		}
		else
		{
			//!	apply to whole objects?
		}
	}
}

