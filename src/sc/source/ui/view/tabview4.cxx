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

// INCLUDE ---------------------------------------------------------------

#include <vcl/help.hxx>
#include <vcl/svapp.hxx>

#include "tabview.hxx"
#include "document.hxx"
#include "docsh.hxx"
#include "scmod.hxx"
#include "gridwin.hxx"
#include "globstr.hrc"
#include "cell.hxx"
#include "dociter.hxx"

extern USHORT nScFillModeMouseModifier;				// global.cxx

// STATIC DATA -----------------------------------------------------------

//==================================================================

//
// ---	Referenz-Eingabe / Fill-Cursor
//

void ScTabView::HideTip()
{
	if ( nTipVisible )
	{
		Help::HideTip( nTipVisible );
		nTipVisible = 0;
	}
}

void ScTabView::ShowRefTip()
{
	BOOL bDone = FALSE;
	if ( aViewData.GetRefType() == SC_REFTYPE_REF && Help::IsQuickHelpEnabled() )
	{
		SCCOL nStartX = aViewData.GetRefStartX();
		SCROW nStartY = aViewData.GetRefStartY();
		SCCOL nEndX   = aViewData.GetRefEndX();
		SCROW nEndY   = aViewData.GetRefEndY();
		if ( nEndX != nStartX || nEndY != nStartY )		// nicht fuer einzelne Zelle
		{
			BOOL bLeft = ( nEndX < nStartX );
			BOOL bTop  = ( nEndY < nStartY );
			PutInOrder( nStartX, nEndX );
			PutInOrder( nStartY, nEndY );
			SCCOL nCols = nEndX+1-nStartX;
			SCROW nRows = nEndY+1-nStartY;

			String aHelp = ScGlobal::GetRscString( STR_QUICKHELP_REF );
			aHelp.SearchAndReplace( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("%1")),
									String::CreateFromInt32(nRows) );
			aHelp.SearchAndReplace( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("%2")),
									String::CreateFromInt32(nCols) );

			ScSplitPos eWhich = aViewData.GetActivePart();
			Window* pWin = pGridWin[eWhich];
			if ( pWin )
			{
				Point aStart = aViewData.GetScrPos( nStartX, nStartY, eWhich );
				Point aEnd = aViewData.GetScrPos( nEndX+1, nEndY+1, eWhich );

				Point aPos( bLeft ? aStart.X() : ( aEnd.X() + 3 ),
							bTop ? aStart.Y() : ( aEnd.Y() + 3 ) );
				USHORT nFlags = ( bLeft ? QUICKHELP_RIGHT : QUICKHELP_LEFT ) |
								( bTop ? QUICKHELP_BOTTOM : QUICKHELP_TOP );

				// nicht ueber die editierte Formel
				if ( !bTop && aViewData.HasEditView( eWhich ) &&
						nEndY+1 == aViewData.GetEditViewRow() )
				{
					//	dann an der oberen Kante der editierten Zelle ausrichten
					aPos.Y() -= 2;		// die 3 von oben
					nFlags = ( nFlags & ~QUICKHELP_TOP ) | QUICKHELP_BOTTOM;
				}

				Rectangle aRect( pWin->OutputToScreenPixel( aPos ), Size(1,1) );

				//!	Test, ob geaendert ??

				HideTip();
				nTipVisible = Help::ShowTip( pWin, aRect, aHelp, nFlags );
				bDone = TRUE;
			}
		}
	}

	if (!bDone)
		HideTip();
}

void ScTabView::StopRefMode()
{
	if (aViewData.IsRefMode())
	{
		aViewData.SetRefMode( FALSE, SC_REFTYPE_NONE );

		HideTip();
        UpdateShrinkOverlay();

		if ( aViewData.GetTabNo() >= aViewData.GetRefStartZ() &&
				aViewData.GetTabNo() <= aViewData.GetRefEndZ() )
		{
			ScDocument* pDoc = aViewData.GetDocument();
			SCCOL nStartX = aViewData.GetRefStartX();
			SCROW nStartY = aViewData.GetRefStartY();
			SCCOL nEndX = aViewData.GetRefEndX();
			SCROW nEndY = aViewData.GetRefEndY();
			if ( nStartX == nEndX && nStartY == nEndY )
				pDoc->ExtendMerge( nStartX, nStartY, nEndX, nEndY, aViewData.GetTabNo() );

			PaintArea( nStartX,nStartY,nEndX,nEndY, SC_UPDATE_MARKS );
		}

		pSelEngine->Reset();
		pSelEngine->SetAddMode( FALSE );		//! sollte das nicht bei Reset passieren?

		ScSplitPos eOld = pSelEngine->GetWhich();
		ScSplitPos eNew = aViewData.GetActivePart();
		if ( eNew != eOld )
		{
			pSelEngine->SetWindow( pGridWin[ eNew ] );
			pSelEngine->SetWhich( eNew );
			pSelEngine->SetVisibleArea( Rectangle(Point(),
										pGridWin[eNew]->GetOutputSizePixel()) );
			pGridWin[eOld]->MoveMouseStatus(*pGridWin[eNew]);
		}
	}

	//	AlignToCursor(SC_FOLLOW_NONE): Only switch active part.
	//	This must also be done if no RefMode was active (for RangeFinder dragging),
	//	but if RefMode was set, AlignToCursor must be after SelectionEngine reset,
	//	so the SelectionEngine SetWindow call from AlignToCursor doesn't capture
	//	the mouse again when called from Tracking/MouseButtonUp (#94562#).
	AlignToCursor( aViewData.GetCurX(), aViewData.GetCurY(), SC_FOLLOW_NONE );
}

void ScTabView::DoneRefMode( BOOL bContinue )
{
	ScDocument* pDoc = aViewData.GetDocument();
	if ( aViewData.GetRefType() == SC_REFTYPE_REF && bContinue )
		SC_MOD()->AddRefEntry();

	BOOL bWasRef = aViewData.IsRefMode();
	aViewData.SetRefMode( FALSE, SC_REFTYPE_NONE );

	HideTip();
    UpdateShrinkOverlay();

	//	Paint:
	if ( bWasRef && aViewData.GetTabNo() >= aViewData.GetRefStartZ() &&
					aViewData.GetTabNo() <= aViewData.GetRefEndZ() )
	{
		SCCOL nStartX = aViewData.GetRefStartX();
		SCROW nStartY = aViewData.GetRefStartY();
		SCCOL nEndX = aViewData.GetRefEndX();
		SCROW nEndY = aViewData.GetRefEndY();
		if ( nStartX == nEndX && nStartY == nEndY )
			pDoc->ExtendMerge( nStartX, nStartY, nEndX, nEndY, aViewData.GetTabNo() );

		PaintArea( nStartX,nStartY,nEndX,nEndY, SC_UPDATE_MARKS );
	}
}

void ScTabView::UpdateRef( SCCOL nCurX, SCROW nCurY, SCTAB nCurZ )
{
	ScDocument* pDoc = aViewData.GetDocument();

	if (!aViewData.IsRefMode())
	{
		//	Das kommt vor, wenn bei einem Referenz-Dialog als erstes mit Control in die
		//	die Tabelle geklickt wird. Dann die neue Referenz an den alten Inhalt anhaengen:

		ScModule* pScMod = SC_MOD();
		if (pScMod->IsFormulaMode())
			pScMod->AddRefEntry();

		InitRefMode( nCurX, nCurY, nCurZ, SC_REFTYPE_REF );
	}

	if ( nCurX != aViewData.GetRefEndX() || nCurY != aViewData.GetRefEndY() ||
		 nCurZ != aViewData.GetRefEndZ() )
	{
		ScMarkData& rMark = aViewData.GetMarkData();
		SCTAB nTab = aViewData.GetTabNo();

		SCCOL nStartX = aViewData.GetRefStartX();
		SCROW nStartY = aViewData.GetRefStartY();
		SCCOL nEndX = aViewData.GetRefEndX();
		SCROW nEndY = aViewData.GetRefEndY();
		if ( nStartX == nEndX && nStartY == nEndY )
			pDoc->ExtendMerge( nStartX, nStartY, nEndX, nEndY, nTab );
		ScUpdateRect aRect( nStartX, nStartY, nEndX, nEndY );

		aViewData.SetRefEnd( nCurX, nCurY, nCurZ );

		nStartX = aViewData.GetRefStartX();
		nStartY = aViewData.GetRefStartY();
		nEndX = aViewData.GetRefEndX();
		nEndY = aViewData.GetRefEndY();
		if ( nStartX == nEndX && nStartY == nEndY )
			pDoc->ExtendMerge( nStartX, nStartY, nEndX, nEndY, nTab );
		aRect.SetNew( nStartX, nStartY, nEndX, nEndY );

		ScRefType eType = aViewData.GetRefType();
		if ( eType == SC_REFTYPE_REF )
		{
			ScRange aRef(
					aViewData.GetRefStartX(), aViewData.GetRefStartY(), aViewData.GetRefStartZ(),
					aViewData.GetRefEndX(), aViewData.GetRefEndY(), aViewData.GetRefEndZ() );
			SC_MOD()->SetReference( aRef, pDoc, &rMark );
			ShowRefTip();
		}
		else if ( eType == SC_REFTYPE_EMBED_LT || eType == SC_REFTYPE_EMBED_RB )
		{
			PutInOrder(nStartX,nEndX);
			PutInOrder(nStartY,nEndY);
			pDoc->SetEmbedded( ScRange(nStartX,nStartY,nTab, nEndX,nEndY,nTab) );
			ScDocShell* pDocSh = aViewData.GetDocShell();
			pDocSh->UpdateOle( &aViewData, TRUE );
			pDocSh->SetDocumentModified();
		}

		SCCOL nPaintStartX;
		SCROW nPaintStartY;
		SCCOL nPaintEndX;
		SCROW nPaintEndY;
		if (aRect.GetDiff( nPaintStartX, nPaintStartY, nPaintEndX, nPaintEndY ))
			PaintArea( nPaintStartX, nPaintStartY, nPaintEndX, nPaintEndY, SC_UPDATE_MARKS );
	}

	//	Tip-Hilfe fuer Auto-Fill

	if ( aViewData.GetRefType() == SC_REFTYPE_FILL && Help::IsQuickHelpEnabled() )
	{
		String aHelpStr;
		ScRange aMarkRange;
		aViewData.GetSimpleArea( aMarkRange );
		SCCOL nEndX = aViewData.GetRefEndX();
		SCROW nEndY = aViewData.GetRefEndY();
		ScRange aDelRange;
		if ( aViewData.GetFillMode() == SC_FILL_MATRIX && !(nScFillModeMouseModifier & KEY_MOD1) )
		{
			aHelpStr = ScGlobal::GetRscString( STR_TIP_RESIZEMATRIX );
			SCCOL nCols = nEndX + 1 - aViewData.GetRefStartX();	// Reihenfolge ist richtig
			SCROW nRows = nEndY + 1 - aViewData.GetRefStartY();
			aHelpStr.SearchAndReplace( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("%1")),
									   String::CreateFromInt32(nRows) );
			aHelpStr.SearchAndReplace( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("%2")),
									   String::CreateFromInt32(nCols) );
		}
		else if ( aViewData.GetDelMark( aDelRange ) )
		{
			aHelpStr = ScGlobal::GetRscString( STR_UNDO_DELETECELLS );
		}
		else if ( nEndX != aMarkRange.aEnd.Col() || nEndY != aMarkRange.aEnd.Row() )
		{
			aHelpStr = ScGlobal::GetRscString( STR_UNDO_INSERTCELLS );
		}

		//	je nach Richtung die obere oder untere Ecke:
		SCCOL nAddX = ( nEndX >= aMarkRange.aEnd.Col() ) ? 1 : 0;
		SCROW nAddY = ( nEndY >= aMarkRange.aEnd.Row() ) ? 1 : 0;
		Point aPos = aViewData.GetScrPos( nEndX+nAddX, nEndY+nAddY, aViewData.GetActivePart() );
		aPos.X() += 8;
		aPos.Y() += 4;
		Window* pWin = GetActiveWin();
		if ( pWin )
			aPos = pWin->OutputToScreenPixel( aPos );
		Rectangle aRect( aPos, aPos );
		USHORT nAlign = QUICKHELP_LEFT|QUICKHELP_TOP;
		Help::ShowQuickHelp(pWin, aRect, aHelpStr, nAlign);
	}
}

void ScTabView::InitRefMode( SCCOL nCurX, SCROW nCurY, SCTAB nCurZ, ScRefType eType, BOOL bPaint )
{
	ScDocument* pDoc = aViewData.GetDocument();
	ScMarkData& rMark = aViewData.GetMarkData();
	if (!aViewData.IsRefMode())
	{
		aViewData.SetRefMode( TRUE, eType );
		aViewData.SetRefStart( nCurX, nCurY, nCurZ );
		aViewData.SetRefEnd( nCurX, nCurY, nCurZ );

		if (nCurZ == aViewData.GetTabNo() && bPaint)
		{
			SCCOL nStartX = nCurX;
			SCROW nStartY = nCurY;
			SCCOL nEndX = nCurX;
			SCROW nEndY = nCurY;
			pDoc->ExtendMerge( nStartX, nStartY, nEndX, nEndY, aViewData.GetTabNo() );

			//!	nur Markierung ueber Inhalte zeichnen!
			PaintArea( nStartX,nStartY,nEndX,nEndY, SC_UPDATE_MARKS );

			//	SetReference ohne Merge-Anpassung
			ScRange aRef( nCurX,nCurY,nCurZ, nCurX,nCurY,nCurZ );
			SC_MOD()->SetReference( aRef, pDoc, &rMark );
		}
	}
}

//UNUSED2008-05  void ScTabView::EndSelection()
//UNUSED2008-05  {
//UNUSED2008-05      ScModule* pScMod = SC_MOD();
//UNUSED2008-05      BOOL bRefMode = pScMod->IsFormulaMode();
//UNUSED2008-05      if ( bRefMode )
//UNUSED2008-05          pScMod->EndReference();
//UNUSED2008-05  }

// static
void ScTabView::SetScrollBar( ScrollBar& rScroll, long nRangeMax, long nVisible, long nPos, BOOL bLayoutRTL )
{
    if ( nVisible == 0 )
        nVisible = 1;       // #i59893# don't use visible size 0

	//	RTL layout uses a negative range to simulate a mirrored scroll bar.
	//	SetScrollBar/GetScrollBarPos hide this so outside of these functions normal cell
	//	addresses can be used.

	if ( bLayoutRTL )
	{
		rScroll.SetRange( Range( -nRangeMax, 0 ) );
		rScroll.SetVisibleSize( nVisible );
		rScroll.SetThumbPos( -nPos - nVisible );
	}
	else
	{
		rScroll.SetRange( Range( 0, nRangeMax ) );
		rScroll.SetVisibleSize( nVisible );
		rScroll.SetThumbPos( nPos );
	}
}

// static
long ScTabView::GetScrollBarPos( ScrollBar& rScroll, BOOL bLayoutRTL )
{
	if ( bLayoutRTL )
		return -rScroll.GetThumbPos() - rScroll.GetVisibleSize();
	else
		return rScroll.GetThumbPos();
}

//	UpdateScrollBars - sichtbaren Bereich und Scrollweite der Scrollbars einstellen

long lcl_UpdateBar( ScrollBar& rScroll, SCCOLROW nSize )		// Size = (komplette) Zellen
{
	long nOldPos;
	long nNewPos;

	nOldPos = rScroll.GetThumbPos();
	rScroll.SetPageSize( static_cast<long>(nSize) );
	nNewPos = rScroll.GetThumbPos();
#ifndef UNX
	rScroll.SetPageSize( 1 );				// immer moeglich !
#endif

	return nNewPos - nOldPos;
}

long lcl_GetScrollRange( SCCOLROW nDocEnd, SCCOLROW nPos, SCCOLROW nVis, SCCOLROW nMax, SCCOLROW nStart )
{
	// get the end (positive) of a scroll bar range that always starts at 0

	++nVis;
	++nMax;		// for partially visible cells
	SCCOLROW nEnd = Max(nDocEnd, (SCCOLROW)(nPos+nVis)) + nVis;
	if (nEnd > nMax)
		nEnd = nMax;

	return ( nEnd - nStart );		// for range starting at 0
}

void ScTabView::UpdateScrollBars()
{
	long		nDiff;
	BOOL		bTop =   ( aViewData.GetVSplitMode() != SC_SPLIT_NONE );
	BOOL		bRight = ( aViewData.GetHSplitMode() != SC_SPLIT_NONE );
	ScDocument*	pDoc = aViewData.GetDocument();
	SCTAB		nTab = aViewData.GetTabNo();
    BOOL        bMirror = pDoc->IsLayoutRTL( nTab ) != Application::GetSettings().GetLayoutRTL();
	SCCOL		nUsedX;
	SCROW		nUsedY;
	pDoc->GetTableArea( nTab, nUsedX, nUsedY );		//! cachen !!!!!!!!!!!!!!!

	SCCOL nVisXL = 0;
	SCCOL nVisXR = 0;
	SCROW nVisYB = 0;
	SCROW nVisYT = 0;

	SCCOL nStartX = 0;
	SCROW nStartY = 0;
	if (aViewData.GetHSplitMode()==SC_SPLIT_FIX)
		nStartX = aViewData.GetFixPosX();
	if (aViewData.GetVSplitMode()==SC_SPLIT_FIX)
		nStartY = aViewData.GetFixPosY();

	nVisXL = aViewData.VisibleCellsX( SC_SPLIT_LEFT );
	long nMaxXL = lcl_GetScrollRange( nUsedX, aViewData.GetPosX(SC_SPLIT_LEFT), nVisXL, MAXCOL, 0 );
    SetScrollBar( aHScrollLeft, nMaxXL, nVisXL, aViewData.GetPosX( SC_SPLIT_LEFT ), bMirror );

	nVisYB = aViewData.VisibleCellsY( SC_SPLIT_BOTTOM );
	long nMaxYB = lcl_GetScrollRange( nUsedY, aViewData.GetPosY(SC_SPLIT_BOTTOM), nVisYB, MAXROW, nStartY );
	SetScrollBar( aVScrollBottom, nMaxYB, nVisYB, aViewData.GetPosY( SC_SPLIT_BOTTOM ) - nStartY, FALSE );

	if (bRight)
	{
		nVisXR = aViewData.VisibleCellsX( SC_SPLIT_RIGHT );
		long nMaxXR = lcl_GetScrollRange( nUsedX, aViewData.GetPosX(SC_SPLIT_RIGHT), nVisXR, MAXCOL, nStartX );
        SetScrollBar( aHScrollRight, nMaxXR, nVisXR, aViewData.GetPosX( SC_SPLIT_RIGHT ) - nStartX, bMirror );
	}

	if (bTop)
	{
		nVisYT = aViewData.VisibleCellsY( SC_SPLIT_TOP );
		long nMaxYT = lcl_GetScrollRange( nUsedY, aViewData.GetPosY(SC_SPLIT_TOP), nVisYT, MAXROW, 0 );
		SetScrollBar( aVScrollTop, nMaxYT, nVisYT, aViewData.GetPosY( SC_SPLIT_TOP ), FALSE );
	}

	//		Bereich testen

	nDiff = lcl_UpdateBar( aHScrollLeft, nVisXL );
	if (nDiff) ScrollX( nDiff, SC_SPLIT_LEFT );
	if (bRight)
	{
		nDiff = lcl_UpdateBar( aHScrollRight, nVisXR );
		if (nDiff) ScrollX( nDiff, SC_SPLIT_RIGHT );
	}

	nDiff = lcl_UpdateBar( aVScrollBottom, nVisYB );
	if (nDiff) ScrollY( nDiff, SC_SPLIT_BOTTOM );
	if (bTop)
	{
		nDiff = lcl_UpdateBar( aVScrollTop, nVisYT );
		if (nDiff) ScrollY( nDiff, SC_SPLIT_TOP );
	}

	//		set visible area for online spelling

	if ( aViewData.IsActive() )
	{
		ScSplitPos eActive = aViewData.GetActivePart();
		ScHSplitPos eHWhich = WhichH( eActive );
		ScVSplitPos eVWhich = WhichV( eActive );
		SCCOL nPosX = aViewData.GetPosX(eHWhich);
		SCROW nPosY = aViewData.GetPosY(eVWhich);
		SCCOL nEndX = nPosX + ( ( eHWhich == SC_SPLIT_LEFT ) ? nVisXL : nVisXR );
		SCROW nEndY = nPosY + ( ( eVWhich == SC_SPLIT_TOP ) ? nVisYT : nVisYB );
		if ( nEndX > MAXCOL ) nEndX = MAXCOL;
		if ( nEndY > MAXROW ) nEndY = MAXROW;
		ScRange aVisible( nPosX, nPosY, nTab, nEndX, nEndY, nTab );
		if ( pDoc->SetVisibleSpellRange( aVisible ) )
			SC_MOD()->AnythingChanged();				// if visible area has changed
	}
}

#ifndef HDR_SLIDERSIZE
#define HDR_SLIDERSIZE		2
#endif

void ScTabView::InvertHorizontal( ScVSplitPos eWhich, long nDragPos )
{
	for (USHORT i=0; i<4; i++)
		if (WhichV((ScSplitPos)i)==eWhich)
		{
			ScGridWindow* pWin = pGridWin[i];
			if (pWin)
			{
				Rectangle aRect( 0,nDragPos, pWin->GetOutputSizePixel().Width()-1,nDragPos+HDR_SLIDERSIZE-1 );
				pWin->Update();
				pWin->DoInvertRect( aRect );	// Pixel
			}
		}
}

void ScTabView::InvertVertical( ScHSplitPos eWhich, long nDragPos )
{
	for (USHORT i=0; i<4; i++)
		if (WhichH((ScSplitPos)i)==eWhich)
		{
			ScGridWindow* pWin = pGridWin[i];
			if (pWin)
			{
				Rectangle aRect( nDragPos,0, nDragPos+HDR_SLIDERSIZE-1,pWin->GetOutputSizePixel().Height()-1 );
				pWin->Update();
				pWin->DoInvertRect( aRect );	// Pixel
			}
		}
}

//==================================================================

void ScTabView::InterpretVisible()
{
	//	make sure all visible cells are interpreted,
	//	so the next paint will not execute a macro function

	ScDocument* pDoc = aViewData.GetDocument();
	if ( !pDoc->GetAutoCalc() )
		return;

	SCTAB nTab = aViewData.GetTabNo();
	for (USHORT i=0; i<4; i++)
	{
		//	rely on gridwin pointers to find used panes
		//	no IsVisible test in case the whole view is not yet shown

		if (pGridWin[i])
		{
			ScHSplitPos eHWhich = WhichH( ScSplitPos(i) );
			ScVSplitPos eVWhich = WhichV( ScSplitPos(i) );

			SCCOL	nX1 = aViewData.GetPosX( eHWhich );
			SCROW	nY1 = aViewData.GetPosY( eVWhich );
			SCCOL	nX2 = nX1 + aViewData.VisibleCellsX( eHWhich );
			SCROW	nY2 = nY1 + aViewData.VisibleCellsY( eVWhich );

			if (nX2 > MAXCOL) nX2 = MAXCOL;
			if (nY2 > MAXROW) nY2 = MAXROW;

			ScCellIterator aIter( pDoc, nX1, nY1, nTab, nX2, nY2, nTab );
			ScBaseCell* pCell = aIter.GetFirst();
			while ( pCell )
			{
				if ( pCell->GetCellType() == CELLTYPE_FORMULA && ((ScFormulaCell*)pCell)->GetDirty() )
					((ScFormulaCell*)pCell)->Interpret();

				pCell = aIter.GetNext();
			}
		}
	}

    // #i65047# repaint during the above loop may have set the bNeedsRepaint flag
    CheckNeedsRepaint();
}





