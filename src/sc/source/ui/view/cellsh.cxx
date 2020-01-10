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

//------------------------------------------------------------------

#include "scitems.hxx"

#include <svtools/slstitm.hxx>
#include <svtools/stritem.hxx>
#include <svtools/whiter.hxx>
#include <svtools/moduleoptions.hxx>
#include <svtools/cliplistener.hxx>
#include <svtools/insdlg.hxx>
#include <sot/formats.hxx>
#include <svx/hlnkitem.hxx>
#include <sfx2/app.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <svx/clipfmtitem.hxx>
#include <svx/langitem.hxx>

#include "cellsh.hxx"
#include "sc.hrc"
#include "docsh.hxx"
#include "attrib.hxx"
#include "scresid.hxx"
#include "tabvwsh.hxx"
#include "impex.hxx"
#include "cell.hxx"
#include "scmod.hxx"
#include "globstr.hrc"
#include "transobj.hxx"
#include "drwtrans.hxx"
#include "scabstdlg.hxx"
#include "dociter.hxx"
#include "postit.hxx"

//------------------------------------------------------------------

#define ScCellShell
#define	CellMovement
#include "scslots.hxx"

#define SearchSettings
#include <svx/svxslots.hxx>

TYPEINIT1( ScCellShell, ScFormatShell );

SFX_IMPL_INTERFACE(ScCellShell, ScFormatShell , ScResId(SCSTR_CELLSHELL) )
{
	SFX_OBJECTBAR_REGISTRATION( SFX_OBJECTBAR_OBJECT | SFX_VISIBILITY_STANDARD |
								SFX_VISIBILITY_SERVER,
								ScResId(RID_OBJECTBAR_FORMAT));
	SFX_POPUPMENU_REGISTRATION(ScResId(RID_POPUP_CELLS));
}


ScCellShell::ScCellShell(ScViewData* pData) :
	ScFormatShell(pData),
    pImpl( new CellShell_Impl() ),
	bPastePossible(FALSE)
{
	SetHelpId(HID_SCSHELL_CELLSH);
	SetName(String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("Cell")));
}

ScCellShell::~ScCellShell()
{
    if ( pImpl->m_pClipEvtLstnr )
	{
        pImpl->m_pClipEvtLstnr->AddRemoveListener( GetViewData()->GetActiveWin(), FALSE );

		//  #103849# The listener may just now be waiting for the SolarMutex and call the link
		//  afterwards, in spite of RemoveListener. So the link has to be reset, too.
        pImpl->m_pClipEvtLstnr->ClearCallbackLink();

        pImpl->m_pClipEvtLstnr->release();
	}

    delete pImpl->m_pLinkedDlg;
    delete pImpl->m_pRequest;
    delete pImpl;
}

//------------------------------------------------------------------

void ScCellShell::GetBlockState( SfxItemSet& rSet )
{
	ScTabViewShell*	pTabViewShell  	= GetViewData()->GetViewShell();
	ScRange aMarkRange;
    ScMarkType eMarkType = GetViewData()->GetSimpleArea( aMarkRange );
	BOOL bSimpleArea = (eMarkType == SC_MARK_SIMPLE);
	BOOL bOnlyNotBecauseOfMatrix;
	BOOL bEditable = pTabViewShell->SelectionEditable( &bOnlyNotBecauseOfMatrix );
	ScDocument* pDoc = GetViewData()->GetDocument();
    ScDocShell* pDocShell = GetViewData()->GetDocShell();
    ScMarkData& rMark = GetViewData()->GetMarkData();
	SCCOL nCol1, nCol2;
	SCROW nRow1, nRow2;
	nCol1 = aMarkRange.aStart.Col();
	nRow1 = aMarkRange.aStart.Row();
	nCol2 = aMarkRange.aEnd.Col();
	nRow2 = aMarkRange.aEnd.Row();

	SfxWhichIter aIter(rSet);
	USHORT nWhich = aIter.FirstWhich();
	while ( nWhich )
	{
		BOOL bDisable = FALSE;
		BOOL bNeedEdit = TRUE;		// muss Selektion editierbar sein?
		switch ( nWhich )
		{
			case FID_FILL_TO_BOTTOM:	// Fuellen oben/unten
			case FID_FILL_TO_TOP:		// mind. 2 Zeilen markiert?
				bDisable = (!bSimpleArea) || (nRow1 == nRow2);
				if ( !bDisable && bEditable )
				{	// Matrix nicht zerreissen
					if ( nWhich == FID_FILL_TO_BOTTOM )
						bDisable = pDoc->HasSelectedBlockMatrixFragment(
							nCol1, nRow1, nCol2, nRow1, rMark );	// erste Zeile
					else
						bDisable = pDoc->HasSelectedBlockMatrixFragment(
							nCol1, nRow2, nCol2, nRow2, rMark );	// letzte Zeile
				}
				break;

			case FID_FILL_TO_RIGHT:		// Fuellen links/rechts
			case FID_FILL_TO_LEFT:		// mind. 2 Spalten markiert?
				bDisable = (!bSimpleArea) || (nCol1 == nCol2);
				if ( !bDisable && bEditable )
				{	// Matrix nicht zerreissen
					if ( nWhich == FID_FILL_TO_RIGHT )
						bDisable = pDoc->HasSelectedBlockMatrixFragment(
							nCol1, nRow1, nCol1, nRow2, rMark );	// erste Spalte
					else
						bDisable = pDoc->HasSelectedBlockMatrixFragment(
							nCol2, nRow1, nCol2, nRow2, rMark );	// letzte Spalte
				}
				break;

			case FID_FILL_SERIES:		// Block fuellen
			case SID_OPENDLG_TABOP:		// Mehrfachoperationen, mind. 2 Zellen markiert?
				if (pDoc->GetChangeTrack()!=NULL &&nWhich ==SID_OPENDLG_TABOP)
					bDisable = TRUE;
				else
					bDisable = (!bSimpleArea) || (nCol1 == nCol2 && nRow1 == nRow2);

				if ( !bDisable && bEditable && nWhich == FID_FILL_SERIES )
				{	// Matrix nicht zerreissen
					bDisable = pDoc->HasSelectedBlockMatrixFragment(
							nCol1, nRow1, nCol2, nRow1, rMark )	// erste Zeile
						||	pDoc->HasSelectedBlockMatrixFragment(
							nCol1, nRow2, nCol2, nRow2, rMark )	// letzte Zeile
						||	pDoc->HasSelectedBlockMatrixFragment(
							nCol1, nRow1, nCol1, nRow2, rMark )	// erste Spalte
						||	pDoc->HasSelectedBlockMatrixFragment(
							nCol2, nRow1, nCol2, nRow2, rMark );	// letzte Spalte
				}
				break;

			case SID_CUT:				// Ausschneiden,
			case FID_INS_CELL:			// Zellen einfuegen, nur einf. Selektion
				bDisable = (!bSimpleArea);
				break;

            case FID_INS_ROW:           // insert rows
            case FID_INS_CELLSDOWN:
                bDisable = (!bSimpleArea) || GetViewData()->SimpleColMarked();
                break;

            case FID_INS_COLUMN:        // insert columns
            case FID_INS_CELLSRIGHT:
                bDisable = (!bSimpleArea) || GetViewData()->SimpleRowMarked();
                break;

			case SID_COPY:						// Kopieren
				// nur wegen Matrix nicht editierbar? Matrix nicht zerreissen
				//! schlaegt nicht zu, wenn geschuetzt UND Matrix, aber damit
				//! muss man leben.. wird in Copy-Routine abgefangen, sonst
				//! muesste hier nochmal Aufwand getrieben werden
				if ( !(!bEditable && bOnlyNotBecauseOfMatrix) )
					bNeedEdit = FALSE;			// erlaubt, wenn geschuetzt/ReadOnly
				break;

			case SID_AUTOFORMAT:		// Autoformat, mind. 3x3 selektiert
				bDisable =    (!bSimpleArea)
						   || ((nCol2 - nCol1) < 2) || ((nRow2 - nRow1) < 2);
				break;

			case SID_OPENDLG_CONDFRMT :
                {
                    if ( !bEditable && bOnlyNotBecauseOfMatrix )
                    {
                        bNeedEdit = FALSE;
                    }
                    if ( pDocShell && pDocShell->IsDocShared() )
                    {
                        bDisable = TRUE;
                    }
                }
                break;

			case FID_CONDITIONAL_FORMAT :
			case SID_CELL_FORMAT_RESET :
			case FID_CELL_FORMAT :
			case SID_ENABLE_HYPHENATION :
				// nur wegen Matrix nicht editierbar? Attribute trotzdem ok
				if ( !bEditable && bOnlyNotBecauseOfMatrix )
					bNeedEdit = FALSE;
				break;

            case FID_VALIDATION:
                {
                    if ( pDocShell && pDocShell->IsDocShared() )
                    {
                        bDisable = TRUE;
                    }
                }
                break;

			case SID_TRANSLITERATE_HALFWIDTH:
			case SID_TRANSLITERATE_FULLWIDTH:
			case SID_TRANSLITERATE_HIRAGANA:
			case SID_TRANSLITERATE_KATAGANA:
                ScViewUtil::HideDisabledSlot( rSet, GetViewData()->GetBindings(), nWhich );
            break;
		}
		if (!bDisable && bNeedEdit && !bEditable)
			bDisable = TRUE;

		if (bDisable)
			rSet.DisableItem(nWhich);
		else if (nWhich == SID_ENABLE_HYPHENATION)
		{
			// toggle slots need a bool item
			rSet.Put( SfxBoolItem( nWhich, FALSE ) );
		}
		nWhich = aIter.NextWhich();
	}
}

//	Funktionen, die je nach Cursorposition disabled sind
//	Default:
//		SID_INSERT_POSTIT, SID_CHARMAP, SID_OPENDLG_FUNCTION

void ScCellShell::GetCellState( SfxItemSet& rSet )
{
    ScDocShell* pDocShell = GetViewData()->GetDocShell();
	ScDocument* pDoc = GetViewData()->GetDocShell()->GetDocument();
	ScAddress aCursor( GetViewData()->GetCurX(), GetViewData()->GetCurY(),
						GetViewData()->GetTabNo() );

	SfxWhichIter aIter(rSet);
	USHORT nWhich = aIter.FirstWhich();
	while ( nWhich )
	{
		BOOL bDisable = FALSE;
		BOOL bNeedEdit = TRUE;		// muss Cursorposition editierbar sein?
		switch ( nWhich )
		{
			case SID_THESAURUS:
				{
					CellType eType = pDoc->GetCellType( aCursor );
					bDisable = ( eType != CELLTYPE_STRING && eType != CELLTYPE_EDIT);
					if (!bDisable)
					{
						//	test for available languages
						USHORT nLang = ScViewUtil::GetEffLanguage( pDoc, aCursor );
						bDisable = !ScModule::HasThesaurusLanguage( nLang );
					}
				}
				break;
			case SID_OPENDLG_FUNCTION:
				{
					ScMarkData aMarkData=GetViewData()->GetMarkData();
					aMarkData.MarkToSimple();
					ScRange aRange;
					aMarkData.GetMarkArea(aRange);
					if(aMarkData.IsMarked())
					{
						if (!pDoc->IsBlockEditable( aCursor.Tab(), aRange.aStart.Col(),aRange.aStart.Row(),
											aRange.aEnd.Col(),aRange.aEnd.Row() ))
						{
							bDisable = TRUE;
						}
						bNeedEdit=FALSE;
					}

				}
				break;
            case SID_INSERT_POSTIT:
                {
                    if ( pDocShell && pDocShell->IsDocShared() )
                    {
                        bDisable = TRUE;
                    }
                }
                break;
		}
		if (!bDisable && bNeedEdit)
			if (!pDoc->IsBlockEditable( aCursor.Tab(), aCursor.Col(),aCursor.Row(),
										aCursor.Col(),aCursor.Row() ))
				bDisable = TRUE;
		if (bDisable)
			rSet.DisableItem(nWhich);
		nWhich = aIter.NextWhich();
	}
}

sal_Bool lcl_TestFormat( SvxClipboardFmtItem& rFormats, const TransferableDataHelper& rDataHelper,
						SotFormatStringId nFormatId )
{
	if ( rDataHelper.HasFormat( nFormatId ) )
	{
		//	#90675# translated format name strings are no longer inserted here,
		//	handled by "paste special" dialog / toolbox controller instead.
		//	Only the object type name has to be set here:
		String aStrVal;
		if ( nFormatId == SOT_FORMATSTR_ID_EMBED_SOURCE )
		{
			TransferableObjectDescriptor aDesc;
			if ( ((TransferableDataHelper&)rDataHelper).GetTransferableObjectDescriptor(
										SOT_FORMATSTR_ID_OBJECTDESCRIPTOR, aDesc ) )
				aStrVal = aDesc.maTypeName;
		}
		else if ( nFormatId == SOT_FORMATSTR_ID_EMBED_SOURCE_OLE
		  || nFormatId == SOT_FORMATSTR_ID_EMBEDDED_OBJ_OLE )
		{
			String aSource;
            SvPasteObjectHelper::GetEmbeddedName( rDataHelper, aStrVal, aSource, nFormatId );
		}

		if ( aStrVal.Len() )
			rFormats.AddClipbrdFormat( nFormatId, aStrVal );
		else
			rFormats.AddClipbrdFormat( nFormatId );

		return sal_True;
	}

	return sal_False;
}

void ScCellShell::GetPossibleClipboardFormats( SvxClipboardFmtItem& rFormats )
{
	Window* pWin = GetViewData()->GetActiveWin();
	BOOL bDraw = ( ScDrawTransferObj::GetOwnClipboard( pWin ) != NULL );

	TransferableDataHelper aDataHelper( TransferableDataHelper::CreateFromSystemClipboard( pWin ) );

	lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_DRAWING );
	lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_SVXB );
	lcl_TestFormat( rFormats, aDataHelper, SOT_FORMAT_GDIMETAFILE );
	lcl_TestFormat( rFormats, aDataHelper, SOT_FORMAT_BITMAP );
	lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_EMBED_SOURCE );

	if ( !bDraw )
	{
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_LINK );
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMAT_STRING );
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_DIF );
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMAT_RTF );
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_HTML );
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_HTML_SIMPLE );
        lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_BIFF_8 );
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_BIFF_5 );
	}

	if ( !lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_EMBED_SOURCE_OLE ) )
		lcl_TestFormat( rFormats, aDataHelper, SOT_FORMATSTR_ID_EMBEDDED_OBJ_OLE );
}

//	Einfuegen, Inhalte einfuegen

BOOL lcl_IsCellPastePossible( const TransferableDataHelper& rData )
{
	BOOL bPossible = FALSE;
	if ( ScTransferObj::GetOwnClipboard( NULL ) || ScDrawTransferObj::GetOwnClipboard( NULL ) )
		bPossible = TRUE;
	else
	{
		if ( rData.HasFormat( SOT_FORMAT_BITMAP ) ||
			 rData.HasFormat( SOT_FORMAT_GDIMETAFILE ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_SVXB ) ||
			 rData.HasFormat( FORMAT_PRIVATE ) ||
			 rData.HasFormat( SOT_FORMAT_RTF ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_EMBED_SOURCE ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_LINK_SOURCE ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_EMBED_SOURCE_OLE ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_LINK_SOURCE_OLE ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_EMBEDDED_OBJ_OLE ) ||
			 rData.HasFormat( SOT_FORMAT_STRING ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_SYLK ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_LINK ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_HTML ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_HTML_SIMPLE ) ||
			 rData.HasFormat( SOT_FORMATSTR_ID_DIF ) )
		{
			bPossible = TRUE;
		}
	}
	return bPossible;
}

IMPL_LINK( ScCellShell, ClipboardChanged, TransferableDataHelper*, pDataHelper )
{
	if ( pDataHelper )
	{
		bPastePossible = lcl_IsCellPastePossible( *pDataHelper );

		SfxBindings& rBindings = GetViewData()->GetBindings();
		rBindings.Invalidate( SID_PASTE );
        rBindings.Invalidate( SID_PASTE_SPECIAL );
		rBindings.Invalidate( SID_CLIPBOARD_FORMAT_ITEMS );
	}
	return 0;
}


void __EXPORT ScCellShell::GetClipState( SfxItemSet& rSet )
{
// SID_PASTE
// SID_PASTE_SPECIAL
// SID_CLIPBOARD_FORMAT_ITEMS

    if ( !pImpl->m_pClipEvtLstnr )
	{
		// create listener
        pImpl->m_pClipEvtLstnr = new TransferableClipboardListener( LINK( this, ScCellShell, ClipboardChanged ) );
        pImpl->m_pClipEvtLstnr->acquire();
		Window* pWin = GetViewData()->GetActiveWin();
        pImpl->m_pClipEvtLstnr->AddRemoveListener( pWin, TRUE );

		// get initial state
		TransferableDataHelper aDataHelper( TransferableDataHelper::CreateFromSystemClipboard( pWin ) );
		bPastePossible = lcl_IsCellPastePossible( aDataHelper );
	}

	BOOL bDisable = !bPastePossible;

	//	Zellschutz / Multiselektion

	if (!bDisable)
	{
		SCCOL nCol = GetViewData()->GetCurX();
		SCROW nRow = GetViewData()->GetCurY();
		SCTAB nTab = GetViewData()->GetTabNo();
		ScDocument* pDoc = GetViewData()->GetDocShell()->GetDocument();
		if (!pDoc->IsBlockEditable( nTab, nCol,nRow, nCol,nRow ))
			bDisable = TRUE;
        ScRange aDummy;
        ScMarkType eMarkType = GetViewData()->GetSimpleArea( aDummy);
        if (eMarkType != SC_MARK_SIMPLE && eMarkType != SC_MARK_SIMPLE_FILTERED)
			bDisable = TRUE;
	}

	if (bDisable)
	{
		rSet.DisableItem( SID_PASTE );
        rSet.DisableItem( SID_PASTE_SPECIAL );
		rSet.DisableItem( SID_CLIPBOARD_FORMAT_ITEMS );
	}
	else if ( rSet.GetItemState( SID_CLIPBOARD_FORMAT_ITEMS ) != SFX_ITEM_UNKNOWN )
	{
		SvxClipboardFmtItem aFormats( SID_CLIPBOARD_FORMAT_ITEMS );
		GetPossibleClipboardFormats( aFormats );
		rSet.Put( aFormats );
	}
}

//	only SID_HYPERLINK_GETLINK:

void ScCellShell::GetHLinkState( SfxItemSet& rSet )
{
	//	always return an item (or inserting will be disabled)
	//	if the cell at the cursor contains only a link, return that link

	SvxHyperlinkItem aHLinkItem;
	if ( !GetViewData()->GetView()->HasBookmarkAtCursor( &aHLinkItem ) )
	{
		//!	put selected text into item?
	}

	rSet.Put(aHLinkItem);
}

void ScCellShell::GetState(SfxItemSet &rSet)
{
	// removed: SID_BORDER_OBJECT (old Basic)

	ScTabViewShell*	pTabViewShell  	= GetViewData()->GetViewShell();
//     BOOL bOle = pTabViewShell->GetViewFrame()->GetFrame()->IsInPlace();
// 	BOOL bTabProt = GetViewData()->GetDocument()->IsTabProtected(GetViewData()->GetTabNo());
	ScDocShell* pDocSh = GetViewData()->GetDocShell();
    ScViewData* pData       = GetViewData();
	ScDocument* pDoc		= pData->GetDocument();
	ScMarkData& rMark		= pData->GetMarkData();
	SCCOL		nPosX		= pData->GetCurX();
	SCROW		nPosY		= pData->GetCurY();
	SCTAB		nTab		= pData->GetTabNo();

	SCTAB nTabCount = pDoc->GetTableCount();
	SCTAB nTabSelCount = rMark.GetSelectCount();



	SfxWhichIter aIter(rSet);
	USHORT nWhich = aIter.FirstWhich();
	while ( nWhich )
	{
		switch ( nWhich )
		{
			case SID_DETECTIVE_REFRESH:
				if (!pDoc->HasDetectiveOperations())
					rSet.DisableItem( nWhich );
				break;

			case SID_RANGE_ADDRESS:
				{
					ScRange aRange;
					if ( pData->GetSimpleArea( aRange ) == SC_MARK_SIMPLE )
					{
						String aStr;
						USHORT nFlags = SCA_VALID | SCA_TAB_3D;
						aRange.Format(aStr,nFlags,pDoc);
						rSet.Put( SfxStringItem( nWhich, aStr ) );
					}
				}
				break;

			case SID_RANGE_NOTETEXT:
				{
					//	#43343# always take cursor position, do not use top-left cell of selection
					ScAddress aPos( nPosX, nPosY, nTab );
					String aNoteText;
					if ( const ScPostIt* pNote = pDoc->GetNote( aPos ) )
                        aNoteText = pNote->GetText();
					rSet.Put( SfxStringItem( nWhich, aNoteText ) );
				}
				break;

			case SID_RANGE_ROW:
				rSet.Put( SfxInt32Item( nWhich, nPosY+1 ) );
				break;

			case SID_RANGE_COL:
				rSet.Put( SfxInt16Item( nWhich, nPosX+1 ) );
				break;

			case SID_RANGE_TABLE:
				rSet.Put( SfxInt16Item( nWhich, nTab+1 ) );
				break;

			case SID_RANGE_VALUE:
				{
					double nValue;
					pDoc->GetValue( nPosX, nPosY, nTab, nValue );
					rSet.Put( ScDoubleItem( nWhich, nValue ) );
				}
				break;

			case SID_RANGE_FORMULA:
				{
					String aString;
					pDoc->GetFormula( nPosX, nPosY, nTab, aString );
					if( aString.Len() == 0 )
					{
						pDoc->GetInputString( nPosX, nPosY, nTab, aString );
					}
					rSet.Put( SfxStringItem( nWhich, aString ) );
				}
				break;

			case SID_RANGE_TEXTVALUE:
				{
					String aString;
					pDoc->GetString( nPosX, nPosY, nTab, aString );
					rSet.Put( SfxStringItem( nWhich, aString ) );
				}
				break;

			case SID_STATUS_SELMODE:
				{
					/* 0: STD	Click hebt Sel auf
					 * 1: ER	Click erweitert Selektion
					 * 2: ERG	Click definiert weitere Selektion
					 */
					USHORT nMode = pTabViewShell->GetLockedModifiers();

					switch ( nMode )
					{
						case KEY_SHIFT: nMode = 1;	break;
						case KEY_MOD1:	nMode = 2;	break; // Control-Taste
						case 0:
						default:
							nMode = 0;
					}

					rSet.Put( SfxUInt16Item( nWhich, nMode ) );
				}
				break;

			case SID_STATUS_DOCPOS:
				{
					String	aStr( ScGlobal::GetRscString( STR_TABLE ) );

					aStr += ' ';
					aStr += String::CreateFromInt32( nTab + 1 );
					aStr.AppendAscii(RTL_CONSTASCII_STRINGPARAM( " / " ));
					aStr += String::CreateFromInt32( nTabCount );
					rSet.Put( SfxStringItem( nWhich, aStr ) );
				}
				break;

			//	Summe etc. mit Datum/Zeit/Fehler/Pos&Groesse zusammengefasst

            // #i34458# The SfxStringItem belongs only into SID_TABLE_CELL. It no longer has to be
            // duplicated in SID_ATTR_POSITION or SID_ATTR_SIZE for SvxPosSizeStatusBarControl.
			case SID_TABLE_CELL:
				{
					//	Testen, ob Fehler unter Cursor
					//	(nicht pDoc->GetErrCode, um keine zirkulaeren Referenzen auszuloesen)

					// In interpreter may happen via rescheduled Basic
					if ( pDoc->IsInInterpreter() )
						rSet.Put( SfxStringItem( nWhich,
							String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("...")) ) );
					else
					{
                        String aFuncStr(pTabViewShell->GetStatusBarStr());
                        rSet.Put( SfxStringItem( nWhich, aFuncStr ) );
					}
				}
				break;

			case SID_DATA_SELECT:
                // HasSelectionData includes column content and validity,
                // page fields have to be checked separately.
                if ( !pDoc->HasSelectionData( nPosX, nPosY, nTab ) &&
                     !pTabViewShell->HasPageFieldDataAtCursor() )
					rSet.DisableItem( nWhich );
				break;

			case SID_STATUS_SUM:
				{
					String aFuncStr(pTabViewShell->GetStatusBarStr());
					rSet.Put( SfxStringItem( nWhich, aFuncStr ) );
				}
				break;

			case FID_MERGE_ON:
				if ( pDoc->GetChangeTrack() || !pTabViewShell->TestMergeCells() )
					rSet.DisableItem( nWhich );
				break;

			case FID_MERGE_OFF:
				if ( pDoc->GetChangeTrack() || !pTabViewShell->TestRemoveMerge() )
					rSet.DisableItem( nWhich );
				break;

            case FID_MERGE_TOGGLE:
                if ( pDoc->GetChangeTrack() )
                    rSet.DisableItem( nWhich );
                else
                {
                    bool bCanMerge = pTabViewShell->TestMergeCells();
                    bool bCanSplit = pTabViewShell->TestRemoveMerge();
                    if( !bCanMerge && !bCanSplit )
                        rSet.DisableItem( nWhich );
                    else
                        rSet.Put( SfxBoolItem( nWhich, bCanSplit ) );
                }
                break;

			case FID_INS_ROWBRK:
				if ( nPosY==0 || (pDoc->GetRowFlags(nPosY,nTab) & CR_MANUALBREAK) )
					rSet.DisableItem( nWhich );
				break;

			case FID_INS_COLBRK:
				if ( nPosX==0 || (pDoc->GetColFlags(nPosX,nTab) & CR_MANUALBREAK) )
					rSet.DisableItem( nWhich );
				break;

			case FID_DEL_ROWBRK:
				if ( nPosY==0 || (pDoc->GetRowFlags(nPosY,nTab) & CR_MANUALBREAK)==0 )
					rSet.DisableItem( nWhich );
				break;

			case FID_DEL_COLBRK:
				if ( nPosX==0 || (pDoc->GetColFlags(nPosX,nTab) & CR_MANUALBREAK)==0 )
					rSet.DisableItem( nWhich );
				break;

			case FID_FILL_TAB:
				if ( nTabSelCount < 2 )
					rSet.DisableItem( nWhich );
				break;

			case SID_SELECT_SCENARIO:
				{
                    // ScDocument* pDoc = GetViewData()->GetDocument();
                    // SCTAB       nTab = GetViewData()->GetTabNo();
					List		aList;

					Color	aDummyCol;

					if ( !pDoc->IsScenario(nTab) )
					{
						String aStr;
						USHORT nFlags;
						SCTAB nScTab = nTab + 1;
						String aProtect;
						bool bSheetProtected = pDoc->IsTabProtected(nTab);

						while ( pDoc->IsScenario(nScTab) )
						{
							pDoc->GetName( nScTab, aStr );
							aList.Insert( new String( aStr ), LIST_APPEND );
							pDoc->GetScenarioData( nScTab, aStr, aDummyCol, nFlags );
							aList.Insert( new String( aStr ), LIST_APPEND );
							// Protection is TRUE if both Sheet and Scenario are protected
							aProtect = (bSheetProtected && (nFlags & SC_SCENARIO_PROTECT)) ? '1' : '0';
							aList.Insert( new String( aProtect), LIST_APPEND );
							++nScTab;
						}
					}
					else
					{
						String	aComment;
						USHORT	nDummyFlags;
						pDoc->GetScenarioData( nTab, aComment, aDummyCol, nDummyFlags );
						DBG_ASSERT( aList.Count() == 0, "List not empty!" );
						aList.Insert( new String( aComment ) );
					}

					rSet.Put( SfxStringListItem( nWhich, &aList ) );

					ULONG nCount = aList.Count();
					for ( ULONG i=0; i<nCount; i++ )
						delete (String*) aList.GetObject(i);
				}
				break;

			case FID_ROW_HIDE:
			case FID_ROW_SHOW:
			case FID_COL_HIDE:
			case FID_COL_SHOW:
			case FID_COL_OPT_WIDTH:
			case FID_ROW_OPT_HEIGHT:
			case FID_DELETE_CELL:
				if ( pDoc->IsTabProtected(nTab) || pDocSh->IsReadOnly())
					rSet.DisableItem( nWhich );
				break;

/*	Zellschutz bei selektierten Zellen wird bei anderen Funktionen auch nicht abgefragt...
			case SID_DELETE:
				{
					if ( pDoc->IsTabProtected(nTab) )
					{
						const SfxItemSet&		rAttrSet  = GetSelectionPattern()->GetItemSet();
						const ScProtectionAttr& rProtAttr = (const ScProtectionAttr&)rAttrSet.Get( ATTR_PROTECTION, TRUE );
						if ( rProtAttr.GetProtection() )
							rSet.DisableItem( nWhich );
					}
				}
				break;
*/
			case SID_OUTLINE_MAKE:
				{
    				if ( GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
                							GetViewData()->GetCurY(), GetViewData()->GetTabNo() ) )
                    {
                        //! test for data pilot operation
                    }
					else if (pDoc->GetChangeTrack()!=NULL || GetViewData()->IsMultiMarked())
					{
						rSet.DisableItem( nWhich );
					}
				}
				break;
			case SID_OUTLINE_SHOW:
				if ( GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
            							GetViewData()->GetCurY(), GetViewData()->GetTabNo() ) )
                {
                    //! test for data pilot operation
                }
				else if (!pTabViewShell->OutlinePossible(FALSE))
					rSet.DisableItem( nWhich );
				break;

			case SID_OUTLINE_HIDE:
				if ( GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
            							GetViewData()->GetCurY(), GetViewData()->GetTabNo() ) )
                {
                    //! test for data pilot operation
                }
				else if (!pTabViewShell->OutlinePossible(TRUE))
					rSet.DisableItem( nWhich );
				break;

			case SID_OUTLINE_REMOVE:
				{
    				if ( GetViewData()->GetDocument()->GetDPAtCursor( GetViewData()->GetCurX(),
                							GetViewData()->GetCurY(), GetViewData()->GetTabNo() ) )
                    {
                        //! test for data pilot operation
                    }
                    else
                    {
    					BOOL bCol, bRow;
    					pTabViewShell->TestRemoveOutline( bCol, bRow );
    					if ( !bCol && !bRow )
    						rSet.DisableItem( nWhich );
                    }
				}
				break;

			case FID_COL_WIDTH:
				{
					//GetViewData()->GetCurX();
					SfxUInt16Item aWidthItem( FID_COL_WIDTH, pDoc->GetColWidth( nPosX , nTab) );
					rSet.Put( aWidthItem );
					if ( pDocSh->IsReadOnly())
						rSet.DisableItem( nWhich );

					//XXX Disablen wenn nicht eindeutig
				}
				break;

			case FID_ROW_HEIGHT:
				{
					//GetViewData()->GetCurY();
					SfxUInt16Item aHeightItem( FID_ROW_HEIGHT, pDoc->GetRowHeight( nPosY , nTab) );
					rSet.Put( aHeightItem );
					//XXX Disablen wenn nicht eindeutig
					if ( pDocSh->IsReadOnly())
						rSet.DisableItem( nWhich );
				}
				break;

			case SID_DETECTIVE_FILLMODE:
				rSet.Put(SfxBoolItem( nWhich, pTabViewShell->IsAuditShell() ));
				break;

			case FID_INPUTLINE_STATUS:
				DBG_ERROR( "Old update method. Use ScTabViewShell::UpdateInputHandler()." );
				break;

			case SID_SCENARIOS:										// Szenarios:
				if (!(rMark.IsMarked() || rMark.IsMultiMarked()))	// nur, wenn etwas selektiert
					rSet.DisableItem( nWhich );
				break;

			case FID_NOTE_VISIBLE:
				{
                    const ScPostIt* pNote = pDoc->GetNote( ScAddress( nPosX, nPosY, nTab ) );
					if ( pNote && pDoc->IsBlockEditable( nTab, nPosX,nPosY, nPosX,nPosY ) )
						rSet.Put( SfxBoolItem( nWhich, pNote->IsCaptionShown() ) );
					else
						rSet.DisableItem( nWhich );
				}
				break;

            case SID_DELETE_NOTE:
                {
                    BOOL bEnable = FALSE;
                    if ( rMark.IsMarked() || rMark.IsMultiMarked() )
                    {
                        if ( pDoc->IsSelectionEditable( rMark ) )
                        {
                            // look for at least one note in selection
                            ScRangeList aRanges;
                            rMark.FillRangeListWithMarks( &aRanges, FALSE );
                            ULONG nCount = aRanges.Count();
                            for (ULONG nPos=0; nPos<nCount && !bEnable; nPos++)
                            {
                                ScCellIterator aCellIter( pDoc, *aRanges.GetObject(nPos) );
                                for( ScBaseCell* pCell = aCellIter.GetFirst(); pCell && !bEnable; pCell = aCellIter.GetNext() )
                                    if ( pCell->HasNote() )
                                        bEnable = TRUE;             // note found
                            }
                        }
                    }
                    else
                    {
                        bEnable = pDoc->IsBlockEditable( nTab, nPosX,nPosY, nPosX,nPosY ) &&
                                  pDoc->GetNote( ScAddress( nPosX, nPosY, nTab ) );
                    }
                    if ( !bEnable )
                        rSet.DisableItem( nWhich );
                }
                break;

            case SID_CHINESE_CONVERSION:
            case SID_HANGUL_HANJA_CONVERSION:
                ScViewUtil::HideDisabledSlot( rSet, pData->GetBindings(), nWhich );
            break;

            case FID_USE_NAME:
                {
                    if ( pDocSh && pDocSh->IsDocShared() )
                        rSet.DisableItem( nWhich );
                    else
                    {
                        ScRange aRange;
                        if ( pData->GetSimpleArea( aRange ) != SC_MARK_SIMPLE )
                            rSet.DisableItem( nWhich );
                    }
                }
                break;

            case FID_DEFINE_NAME:
            case FID_INSERT_NAME:
            case SID_DEFINE_COLROWNAMERANGES:
                {
                    if ( pDocSh && pDocSh->IsDocShared() )
                    {
                        rSet.DisableItem( nWhich );
                    }
                }
                break;

		} // switch ( nWitch )
		nWhich = aIter.NextWhich();
	} // while ( nWitch )
}

//------------------------------------------------------------------



