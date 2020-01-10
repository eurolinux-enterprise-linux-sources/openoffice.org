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

#include "scitems.hxx"
#include <svx/eeitem.hxx>


#include <vcl/timer.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/app.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/childwin.hxx>

#include "attrib.hxx"
#include "pagedata.hxx"
#include "tabview.hxx"
#include "tabvwsh.hxx"
#include "printfun.hxx"
#include "stlpool.hxx"
#include "docsh.hxx"
#include "gridwin.hxx"
#include "olinewin.hxx"
#include "uiitems.hxx"
#include "sc.hrc"
#include "viewutil.hxx"
#include "colrowba.hxx"
#include "waitoff.hxx"
#include "globstr.hrc"
#include "scmod.hxx"

#define SC_BLOCKMODE_NONE		0
#define SC_BLOCKMODE_NORMAL		1
#define SC_BLOCKMODE_OWN		2



//
//          Markier - Funktionen
//

void ScTabView::PaintMarks(SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow )
{
	if (!ValidCol(nStartCol)) nStartCol = MAXCOL;
	if (!ValidRow(nStartRow)) nStartRow = MAXROW;
	if (!ValidCol(nEndCol)) nEndCol = MAXCOL;
	if (!ValidRow(nEndRow)) nEndRow = MAXROW;

	BOOL bLeft = (nStartCol==0 && nEndCol==MAXCOL);
	BOOL bTop = (nStartRow==0 && nEndRow==MAXROW);

	if (bLeft)
		PaintLeftArea( nStartRow, nEndRow );
	if (bTop)
		PaintTopArea( nStartCol, nEndCol );

	aViewData.GetDocument()->ExtendMerge( nStartCol, nStartRow, nEndCol, nEndRow,
											aViewData.GetTabNo() );
	PaintArea( nStartCol, nStartRow, nEndCol, nEndRow, SC_UPDATE_MARKS );
}

BOOL ScTabView::IsMarking( SCCOL nCol, SCROW nRow, SCTAB nTab ) const
{
	return bIsBlockMode
		&& nBlockStartX == nCol
		&& nBlockStartY == nRow
		&& nBlockStartZ == nTab;
}

void ScTabView::InitOwnBlockMode()
{
	if (!bIsBlockMode)
	{
		//	Wenn keine (alte) Markierung mehr da ist, Anker in SelectionEngine loeschen:

		ScMarkData& rMark = aViewData.GetMarkData();
		if (!rMark.IsMarked() && !rMark.IsMultiMarked())
			GetSelEngine()->CursorPosChanging( FALSE, FALSE );

//		bIsBlockMode = TRUE;
		bIsBlockMode = SC_BLOCKMODE_OWN;			//! Variable umbenennen!
		nBlockStartX = 0;
		nBlockStartY = 0;
		nBlockStartZ = 0;
		nBlockEndX = 0;
		nBlockEndY = 0;
		nBlockEndZ = 0;

		SelectionChanged();		// Status wird mit gesetzer Markierung abgefragt
	}
}

void ScTabView::InitBlockMode( SCCOL nCurX, SCROW nCurY, SCTAB nCurZ,
								BOOL bTestNeg, BOOL bCols, BOOL bRows )
{
	if (!bIsBlockMode)
	{
		if (!ValidCol(nCurX)) nCurX = MAXCOL;
		if (!ValidRow(nCurY)) nCurY = MAXROW;

		ScMarkData& rMark = aViewData.GetMarkData();
		SCTAB nTab = aViewData.GetTabNo();

		//	Teil von Markierung aufheben?
		if (bTestNeg)
		{
			if ( bCols )
				bBlockNeg = rMark.IsColumnMarked( nCurX );
			else if ( bRows )
				bBlockNeg = rMark.IsRowMarked( nCurY );
			else
				bBlockNeg = rMark.IsCellMarked( nCurX, nCurY );
		}
		else
			bBlockNeg = FALSE;
		rMark.SetMarkNegative(bBlockNeg);

//		bIsBlockMode = TRUE;
		bIsBlockMode = SC_BLOCKMODE_NORMAL;			//! Variable umbenennen!
		bBlockCols = bCols;
		bBlockRows = bRows;
		nBlockStartX = nBlockStartXOrig = nCurX;
		nBlockStartY = nBlockStartYOrig = nCurY;
		nBlockStartZ = nCurZ;
		nBlockEndX = nOldCurX = nBlockStartX;
		nBlockEndY = nOldCurY = nBlockStartY;
		nBlockEndZ = nBlockStartZ;

		if (bBlockCols)
		{
			nBlockStartY = nBlockStartYOrig = 0;
			nBlockEndY = MAXROW;
		}

		if (bBlockRows)
		{
			nBlockStartX = nBlockStartXOrig = 0;
			nBlockEndX = MAXCOL;
		}

		rMark.SetMarkArea( ScRange( nBlockStartX,nBlockStartY, nTab, nBlockEndX,nBlockEndY, nTab ) );

#ifdef OLD_SELECTION_PAINT
		InvertBlockMark( nBlockStartX,nBlockStartY,nBlockEndX,nBlockEndY );
#endif
		UpdateSelectionOverlay();

		bNewStartIfMarking = FALSE;		// use only once
	}
}

void ScTabView::SetNewStartIfMarking()
{
	bNewStartIfMarking = TRUE;
}

void ScTabView::DoneBlockMode( BOOL bContinue )            // Default FALSE
{
	//	Wenn zwischen Tabellen- und Header SelectionEngine gewechselt wird,
	//	wird evtl. DeselectAll gerufen, weil die andere Engine keinen Anker hat.
	//	Mit bMoveIsShift wird verhindert, dass dann die Selektion aufgehoben wird.

	if (bIsBlockMode && !bMoveIsShift)
	{
		ScMarkData& rMark = aViewData.GetMarkData();
		BOOL bFlag = rMark.GetMarkingFlag();
		rMark.SetMarking(FALSE);

		if (bBlockNeg && !bContinue)
			rMark.MarkToMulti();

		if (bContinue)
			rMark.MarkToMulti();
		else
		{
			//	Die Tabelle kann an dieser Stelle ungueltig sein, weil DoneBlockMode
			//	aus SetTabNo aufgerufen wird
			//	(z.B. wenn die aktuelle Tabelle von einer anderen View aus geloescht wird)

			SCTAB nTab = aViewData.GetTabNo();
			ScDocument* pDoc = aViewData.GetDocument();
			if ( pDoc->HasTable(nTab) )
				PaintBlock( TRUE );								// TRUE -> Block loeschen
			else
				rMark.ResetMark();
		}
//		bIsBlockMode = FALSE;
		bIsBlockMode = SC_BLOCKMODE_NONE;			//! Variable umbenennen!

		rMark.SetMarking(bFlag);
		rMark.SetMarkNegative(FALSE);
	}
}

void ScTabView::MarkCursor( SCCOL nCurX, SCROW nCurY, SCTAB nCurZ,
                            BOOL bCols, BOOL bRows, BOOL bCellSelection )
{
	if (!ValidCol(nCurX)) nCurX = MAXCOL;
	if (!ValidRow(nCurY)) nCurY = MAXROW;

	if (!bIsBlockMode)
	{
		DBG_ERROR( "MarkCursor nicht im BlockMode" );
		InitBlockMode( nCurX, nCurY, nCurZ, FALSE, bCols, bRows );
	}

	if (bCols)
		nCurY = MAXROW;
	if (bRows)
		nCurX = MAXCOL;

	ScMarkData& rMark = aViewData.GetMarkData();
	DBG_ASSERT(rMark.IsMarked() || rMark.IsMultiMarked(), "MarkCursor, !IsMarked()");
	ScRange aMarkRange;
	rMark.GetMarkArea(aMarkRange);
	if (( aMarkRange.aStart.Col() != nBlockStartX && aMarkRange.aEnd.Col() != nBlockStartX ) ||
		( aMarkRange.aStart.Row() != nBlockStartY && aMarkRange.aEnd.Row() != nBlockStartY ) ||
		( bIsBlockMode == SC_BLOCKMODE_OWN ))
	{
		//	Markierung ist veraendert worden
		//	(z.B. MarkToSimple, wenn per negativ alles bis auf ein Rechteck geloescht wurde)
		//	oder nach InitOwnBlockMode wird mit Shift-Klick weitermarkiert...

		BOOL bOldShift = bMoveIsShift;
		bMoveIsShift = FALSE;				//	wirklich umsetzen
		DoneBlockMode(FALSE);				//!	direkt Variablen setzen? (-> kein Geflacker)
		bMoveIsShift = bOldShift;

		InitBlockMode( aMarkRange.aStart.Col(), aMarkRange.aStart.Row(),
						nBlockStartZ, rMark.IsMarkNegative(), bCols, bRows );
	}

	SCCOL nOldBlockEndX = nBlockEndX;
	SCROW nOldBlockEndY = nBlockEndY;

	if ( nCurX != nOldCurX || nCurY != nOldCurY )
	{
        // Current cursor has moved

		SCTAB		nTab = nCurZ;

#ifdef OLD_SELECTION_PAINT
		SCCOL		nDrawStartCol;
		SCROW		nDrawStartRow;
		SCCOL		nDrawEndCol;
		SCROW		nDrawEndRow;
#endif

        // Set old selection area
		ScUpdateRect aRect( nBlockStartX, nBlockStartY, nOldBlockEndX, nOldBlockEndY );

        if ( bCellSelection )
        {
            // Expand selection area accordingly when the current selection ends
            // with a merged cell.
            SCsCOL nCurXOffset = 0;
            SCsCOL nBlockStartXOffset = 0;
            SCsROW nCurYOffset = 0;
            SCsROW nBlockStartYOffset = 0;
            BOOL bBlockStartMerged = FALSE;
            const ScMergeAttr* pMergeAttr = NULL;
            ScDocument* pDocument = aViewData.GetDocument();

            // The following block checks whether or not the "BlockStart" (anchor)
            // cell is merged.  If it's merged, it'll then move the position of the
            // anchor cell to the corner that's diagonally opposite of the
            // direction of a current selection area.  For instance, if a current
            // selection is moving in the upperleft direction, the anchor cell will
            // move to the lower-right corner of the merged anchor cell, and so on.

            pMergeAttr = static_cast<const ScMergeAttr*>(
                pDocument->GetAttr( nBlockStartXOrig, nBlockStartYOrig, nTab, ATTR_MERGE ) );
            if ( pMergeAttr->IsMerged() )
            {
                SCsCOL nColSpan = pMergeAttr->GetColMerge();
                SCsROW nRowSpan = pMergeAttr->GetRowMerge();

                if ( !( nCurX >= nBlockStartXOrig + nColSpan - 1 && nCurY >= nBlockStartYOrig + nRowSpan - 1 ) )
                {
                    nBlockStartX = nCurX >= nBlockStartXOrig ? nBlockStartXOrig : nBlockStartXOrig + nColSpan - 1;
                    nBlockStartY = nCurY >= nBlockStartYOrig ? nBlockStartYOrig : nBlockStartYOrig + nRowSpan - 1;
                    nCurXOffset  = nCurX >= nBlockStartXOrig && nCurX < nBlockStartXOrig + nColSpan - 1 ?
                        nBlockStartXOrig - nCurX + nColSpan - 1 : 0;
                    nCurYOffset  = nCurY >= nBlockStartYOrig && nCurY < nBlockStartYOrig + nRowSpan - 1 ?
                        nBlockStartYOrig - nCurY + nRowSpan - 1 : 0;
                    bBlockStartMerged = TRUE;
                }
            }

            // The following block checks whether or not the current cell is
            // merged.  If it is, it'll then set the appropriate X & Y offset
            // values (nCurXOffset & nCurYOffset) such that the selection area will
            // grow by those specified offset amounts.  Note that the values of
            // nCurXOffset/nCurYOffset may also be specified in the previous code
            // block, in which case whichever value is greater will take on.

            pMergeAttr = static_cast<const ScMergeAttr*>(
                pDocument->GetAttr( nCurX, nCurY, nTab, ATTR_MERGE ) );
            if ( pMergeAttr->IsMerged() )
            {
                SCsCOL nColSpan = pMergeAttr->GetColMerge();
                SCsROW nRowSpan = pMergeAttr->GetRowMerge();

                if ( !( nBlockStartX >= nCurX + nColSpan - 1 && nBlockStartY >= nCurY + nRowSpan - 1 ) )
                {
                    if ( nBlockStartX <= nCurX + nColSpan - 1 )
                    {
                        SCsCOL nCurXOffsetTemp = nCurX < nCurX + nColSpan - 1 ? nColSpan - 1 : 0;
                        nCurXOffset = nCurXOffset > nCurXOffsetTemp ? nCurXOffset : nCurXOffsetTemp;
                    }
                    if ( nBlockStartY <= nCurY + nRowSpan - 1 )
                    {
                        SCsROW nCurYOffsetTemp = nCurY < nCurY + nRowSpan - 1 ? nRowSpan - 1 : 0;
                        nCurYOffset = nCurYOffset > nCurYOffsetTemp ? nCurYOffset : nCurYOffsetTemp;
                    }
                    if ( !( nBlockStartX <= nCurX && nBlockStartY <= nCurY ) &&
                         !( nBlockStartX > nCurX + nColSpan - 1 && nBlockStartY > nCurY + nRowSpan - 1 ) )
                    {
                        nBlockStartXOffset = nBlockStartX > nCurX && nBlockStartX <= nCurX + nColSpan - 1 ? nCurX - nBlockStartX : 0;
                        nBlockStartYOffset = nBlockStartY > nCurY && nBlockStartY <= nCurY + nRowSpan - 1 ? nCurY - nBlockStartY : 0;
                    }
                }
            }
            else
            {
                // The current cell is not merged.  Move the anchor cell to its
                // original position.
                if ( !bBlockStartMerged )
                {
                    nBlockStartX = nBlockStartXOrig;
                    nBlockStartY = nBlockStartYOrig;
                }
            }

            nBlockStartX = nBlockStartX + nBlockStartXOffset >= 0 ? nBlockStartX + nBlockStartXOffset : 0;
            nBlockStartY = nBlockStartY + nBlockStartYOffset >= 0 ? nBlockStartY + nBlockStartYOffset : 0;
            nBlockEndX = nCurX + nCurXOffset > MAXCOL ? MAXCOL : nCurX + nCurXOffset;
            nBlockEndY = nCurY + nCurYOffset > MAXROW ? MAXROW : nCurY + nCurYOffset;
        }
        else
        {
            nBlockEndX = nCurX;
            nBlockEndY = nCurY;
        }
        // end of "if ( bCellSelection )"

        // Set new selection area
		aRect.SetNew( nBlockStartX, nBlockStartY, nBlockEndX, nBlockEndY );
		rMark.SetMarkArea( ScRange( nBlockStartX, nBlockStartY, nTab, nBlockEndX, nBlockEndY, nTab ) );

#ifdef OLD_SELECTION_PAINT
		BOOL bCont;
		BOOL bDraw = aRect.GetXorDiff( nDrawStartCol, nDrawStartRow,
										nDrawEndCol, nDrawEndRow, bCont );
		if ( bDraw )
		{
//?			PutInOrder( nDrawStartCol, nDrawEndCol );
//?			PutInOrder( nDrawStartRow, nDrawEndRow );

			HideAllCursors();
			InvertBlockMark( nDrawStartCol, nDrawStartRow, nDrawEndCol, nDrawEndRow );
			if (bCont)
			{
				aRect.GetContDiff( nDrawStartCol, nDrawStartRow, nDrawEndCol, nDrawEndRow );
				InvertBlockMark( nDrawStartCol, nDrawStartRow, nDrawEndCol, nDrawEndRow );
			}
			ShowAllCursors();
		}
#endif
        UpdateSelectionOverlay();

        nOldCurX = nCurX;
        nOldCurY = nCurY;

		aViewData.GetViewShell()->UpdateInputHandler();
//		InvalidateAttribs();
	}

	if ( !bCols && !bRows )
		aHdrFunc.SetAnchorFlag( FALSE );
}

void ScTabView::UpdateSelectionOverlay()
{
    for (USHORT i=0; i<4; i++)
        if ( pGridWin[i] && pGridWin[i]->IsVisible() )
            pGridWin[i]->UpdateSelectionOverlay();
}

void ScTabView::UpdateShrinkOverlay()
{
    for (USHORT i=0; i<4; i++)
        if ( pGridWin[i] && pGridWin[i]->IsVisible() )
            pGridWin[i]->UpdateShrinkOverlay();
}

void ScTabView::UpdateAllOverlays()
{
    for (USHORT i=0; i<4; i++)
        if ( pGridWin[i] && pGridWin[i]->IsVisible() )
            pGridWin[i]->UpdateAllOverlays();
}

//!
//!	PaintBlock in zwei Methoden aufteilen: RepaintBlock und RemoveBlock o.ae.
//!

void ScTabView::PaintBlock( BOOL bReset )
{
	ScDocument* pDoc = aViewData.GetDocument();
	ScMarkData& rMark = aViewData.GetMarkData();
	SCTAB nTab = aViewData.GetTabNo();
	BOOL bMark = rMark.IsMarked();
	BOOL bMulti = rMark.IsMultiMarked();
	if (bMark || bMulti)
	{
		ScRange aMarkRange;
		HideAllCursors();
		if (bMulti)
		{
			BOOL bFlag = rMark.GetMarkingFlag();
			rMark.SetMarking(FALSE);
			rMark.MarkToMulti();
			rMark.GetMultiMarkArea(aMarkRange);
			rMark.MarkToSimple();
			rMark.SetMarking(bFlag);

			bMark = rMark.IsMarked();
			bMulti = rMark.IsMultiMarked();
		}
		else
			rMark.GetMarkArea(aMarkRange);

		nBlockStartX = aMarkRange.aStart.Col();
		nBlockStartY = aMarkRange.aStart.Row();
		nBlockStartZ = aMarkRange.aStart.Tab();
		nBlockEndX = aMarkRange.aEnd.Col();
		nBlockEndY = aMarkRange.aEnd.Row();
		nBlockEndZ = aMarkRange.aEnd.Tab();

		BOOL bDidReset = FALSE;

		if ( nTab>=nBlockStartZ && nTab<=nBlockEndZ )
		{
			if ( bReset )
			{
				// Invertieren beim Loeschen nur auf aktiver View
				if ( aViewData.IsActive() )
				{
					USHORT i;
					if ( bMulti )
					{
#ifdef OLD_SELECTION_PAINT
						for (i=0; i<4; i++)
							if (pGridWin[i] && pGridWin[i]->IsVisible())
								pGridWin[i]->InvertSimple( nBlockStartX, nBlockStartY,
															nBlockEndX, nBlockEndY,
															TRUE, TRUE );
#endif
						rMark.ResetMark();
                        UpdateSelectionOverlay();
						bDidReset = TRUE;
					}
					else
					{
#ifdef OLD_SELECTION_PAINT
						// (mis)use InvertBlockMark to remove all of the selection
						// -> set bBlockNeg (like when removing parts of a selection)
						//	  and convert everything to Multi

						rMark.MarkToMulti();
						BOOL bOld = bBlockNeg;
						bBlockNeg = TRUE;
						// #73130# (negative) MarkArea must be set in case of repaint
						rMark.SetMarkArea( ScRange( nBlockStartX,nBlockStartY, nTab,
													nBlockEndX,nBlockEndY, nTab ) );

						InvertBlockMark( nBlockStartX, nBlockStartY, nBlockEndX, nBlockEndY );

						bBlockNeg = bOld;
#endif
						rMark.ResetMark();
                        UpdateSelectionOverlay();
						bDidReset = TRUE;
					}

					//	repaint if controls are touched (#69680# in both cases)
					// #i74768# Forms are rendered by DrawingLayer's EndDrawLayers()
					static bool bSuppressControlExtraStuff(true);

					if(!bSuppressControlExtraStuff)
					{
						Rectangle aMMRect = pDoc->GetMMRect(nBlockStartX,nBlockStartY,nBlockEndX,nBlockEndY, nTab);
						if (pDoc->HasControl( nTab, aMMRect ))
						{
							for (i=0; i<4; i++)
							{
								if (pGridWin[i] && pGridWin[i]->IsVisible())
								{
									//	MapMode muss logischer (1/100mm) sein !!!
									pDoc->InvalidateControls( pGridWin[i], nTab, aMMRect );
									pGridWin[i]->Update();
								}
							}
						}
					}
				}
			}
			else
				PaintMarks( nBlockStartX, nBlockStartY, nBlockEndX, nBlockEndY );
		}

		if ( bReset && !bDidReset )
			rMark.ResetMark();

		ShowAllCursors();
	}
}

void ScTabView::SelectAll( BOOL bContinue )
{
	ScMarkData& rMark = aViewData.GetMarkData();
	SCTAB nTab = aViewData.GetTabNo();

	if (rMark.IsMarked())
	{
		ScRange aMarkRange;
		rMark.GetMarkArea( aMarkRange );
		if ( aMarkRange == ScRange( 0,0,nTab, MAXCOL,MAXROW,nTab ) )
			return;
	}

	DoneBlockMode( bContinue );
	InitBlockMode( 0,0,nTab );
	MarkCursor( MAXCOL,MAXROW,nTab );

	SelectionChanged();
}

void ScTabView::SelectAllTables()
{
	ScDocument* pDoc = aViewData.GetDocument();
	ScMarkData& rMark = aViewData.GetMarkData();
//    SCTAB nTab = aViewData.GetTabNo();
	SCTAB nCount = pDoc->GetTableCount();

	if (nCount>1)
	{
		for (SCTAB i=0; i<nCount; i++)
			rMark.SelectTable( i, TRUE );

		//		Markierungen werden per Default nicht pro Tabelle gehalten
//		pDoc->ExtendMarksFromTable( nTab );

		aViewData.GetDocShell()->PostPaintExtras();
        SfxBindings& rBind = aViewData.GetBindings();
        rBind.Invalidate( FID_FILL_TAB );
        rBind.Invalidate( FID_TAB_DESELECTALL );
	}
}

void ScTabView::DeselectAllTables()
{
    ScDocument* pDoc = aViewData.GetDocument();
    ScMarkData& rMark = aViewData.GetMarkData();
    SCTAB nTab = aViewData.GetTabNo();
    SCTAB nCount = pDoc->GetTableCount();

    for (SCTAB i=0; i<nCount; i++)
        rMark.SelectTable( i, ( i == nTab ) );

    aViewData.GetDocShell()->PostPaintExtras();
    SfxBindings& rBind = aViewData.GetBindings();
    rBind.Invalidate( FID_FILL_TAB );
    rBind.Invalidate( FID_TAB_DESELECTALL );
}

BOOL lcl_FitsInWindow( double fScaleX, double fScaleY, USHORT nZoom,
						long nWindowX, long nWindowY, ScDocument* pDoc, SCTAB nTab,
						SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
						SCCOL nFixPosX, SCROW nFixPosY )
{
	double fZoomFactor = (double)Fraction(nZoom,100);
	fScaleX *= fZoomFactor;
	fScaleY *= fZoomFactor;

	long nBlockX = 0;
	SCCOL nCol;
	for (nCol=0; nCol<nFixPosX; nCol++)
	{
		//	for frozen panes, add both parts
		USHORT nColTwips = pDoc->GetColWidth( nCol, nTab );
		if (nColTwips)
		{
			nBlockX += (long)(nColTwips * fScaleX);
			if (nBlockX > nWindowX)
				return FALSE;
		}
	}
	for (nCol=nStartCol; nCol<=nEndCol; nCol++)
	{
		USHORT nColTwips = pDoc->GetColWidth( nCol, nTab );
		if (nColTwips)
		{
			nBlockX += (long)(nColTwips * fScaleX);
			if (nBlockX > nWindowX)
				return FALSE;
		}
	}

	long nBlockY = 0;
    ScCoupledCompressedArrayIterator< SCROW, BYTE, USHORT> aIter(
            pDoc->GetRowFlagsArray( nTab), 0, nFixPosY-1, CR_HIDDEN, 0,
            pDoc->GetRowHeightArray( nTab));
    for ( ; aIter; ++aIter)
	{
		//	for frozen panes, add both parts
		USHORT nRowTwips = *aIter;
		if (nRowTwips)
		{
			nBlockY += (long)(nRowTwips * fScaleY);
			if (nBlockY > nWindowY)
				return FALSE;
		}
	}
    aIter.NewLimits( nStartRow, nEndRow);
    for ( ; aIter; ++aIter)
	{
		USHORT nRowTwips = *aIter;
		if (nRowTwips)
		{
			nBlockY += (long)(nRowTwips * fScaleY);
			if (nBlockY > nWindowY)
				return FALSE;
		}
	}

	return TRUE;
}

USHORT ScTabView::CalcZoom( SvxZoomType eType, USHORT nOldZoom )
{
	USHORT nZoom = 0; // Ergebnis

	switch ( eType )
	{
		case SVX_ZOOM_PERCENT: // rZoom ist kein besonderer prozentualer Wert
			nZoom = nOldZoom;
			break;

		case SVX_ZOOM_OPTIMAL:	// nZoom entspricht der optimalen Gr"o\se
			{
				ScMarkData& rMark = aViewData.GetMarkData();
				ScDocument* pDoc = aViewData.GetDocument();

				if (!rMark.IsMarked() && !rMark.IsMultiMarked())
					nZoom = 100;				// nothing selected
				else
				{
					SCTAB	nTab = aViewData.GetTabNo();
					ScRange aMarkRange;
					if ( aViewData.GetSimpleArea( aMarkRange ) != SC_MARK_SIMPLE )
						rMark.GetMultiMarkArea( aMarkRange );

					SCCOL	nStartCol = aMarkRange.aStart.Col();
					SCROW	nStartRow = aMarkRange.aStart.Row();
					SCTAB	nStartTab = aMarkRange.aStart.Tab();
					SCCOL	nEndCol = aMarkRange.aEnd.Col();
					SCROW	nEndRow = aMarkRange.aEnd.Row();
					SCTAB	nEndTab = aMarkRange.aEnd.Tab();

					if ( nTab < nStartTab && nTab > nEndTab )
						nTab = nStartTab;

					ScSplitPos eUsedPart = aViewData.GetActivePart();

					SCCOL nFixPosX = 0;
					SCROW nFixPosY = 0;
					if ( aViewData.GetHSplitMode() == SC_SPLIT_FIX )
					{
						//	use right part
						eUsedPart = (WhichV(eUsedPart)==SC_SPLIT_TOP) ? SC_SPLIT_TOPRIGHT : SC_SPLIT_BOTTOMRIGHT;
						nFixPosX = aViewData.GetFixPosX();
						if ( nStartCol < nFixPosX )
							nStartCol = nFixPosX;
					}
					if ( aViewData.GetVSplitMode() == SC_SPLIT_FIX )
					{
						//	use bottom part
						eUsedPart = (WhichH(eUsedPart)==SC_SPLIT_LEFT) ? SC_SPLIT_BOTTOMLEFT : SC_SPLIT_BOTTOMRIGHT;
						nFixPosY = aViewData.GetFixPosY();
						if ( nStartRow < nFixPosY )
							nStartRow = nFixPosY;
					}

					if (pGridWin[eUsedPart])
					{
						//	Because scale is rounded to pixels, the only reliable way to find
						//	the right scale is to check if a zoom fits

						Size aWinSize = pGridWin[eUsedPart]->GetOutputSizePixel();

						//	for frozen panes, use sum of both parts for calculation

						if ( nFixPosX != 0 )
							aWinSize.Width() += GetGridWidth( SC_SPLIT_LEFT );
						if ( nFixPosY != 0 )
							aWinSize.Height() += GetGridHeight( SC_SPLIT_TOP );

						ScDocShell* pDocSh = aViewData.GetDocShell();
						double nPPTX = ScGlobal::nScreenPPTX / pDocSh->GetOutputFactor();
						double nPPTY = ScGlobal::nScreenPPTY;

						USHORT nMin = MINZOOM;
						USHORT nMax = MAXZOOM;
						while ( nMax > nMin )
						{
							USHORT nTest = (nMin+nMax+1)/2;
							if ( lcl_FitsInWindow(
										nPPTX, nPPTY, nTest, aWinSize.Width(), aWinSize.Height(),
										pDoc, nTab, nStartCol, nStartRow, nEndCol, nEndRow,
										nFixPosX, nFixPosY ) )
								nMin = nTest;
							else
								nMax = nTest-1;
						}
						DBG_ASSERT( nMin == nMax, "Schachtelung ist falsch" );
						nZoom = nMin;

						if ( nZoom != nOldZoom )
						{
							// scroll to block only in active split part
							// (the part for which the size was calculated)

							if ( nStartCol <= nEndCol )
								aViewData.SetPosX( WhichH(eUsedPart), nStartCol );
							if ( nStartRow <= nEndRow )
								aViewData.SetPosY( WhichV(eUsedPart), nStartRow );
						}
					}
				}
			}
			break;

			case SVX_ZOOM_WHOLEPAGE:	// nZoom entspricht der ganzen Seite oder
			case SVX_ZOOM_PAGEWIDTH:	// nZoom entspricht der Seitenbreite
				{
					SCTAB				nCurTab		= aViewData.GetTabNo();
					ScDocument*			pDoc		= aViewData.GetDocument();
					ScStyleSheetPool*	pStylePool  = pDoc->GetStyleSheetPool();
					SfxStyleSheetBase*	pStyleSheet =
											pStylePool->Find( pDoc->GetPageStyle( nCurTab ),
															  SFX_STYLE_FAMILY_PAGE );

					DBG_ASSERT( pStyleSheet, "PageStyle not found :-/" );

					if ( pStyleSheet )
					{
						ScPrintFunc aPrintFunc( aViewData.GetDocShell(),
												aViewData.GetViewShell()->GetPrinter(TRUE),
												nCurTab );

						Size aPageSize = aPrintFunc.GetDataSize();

						//	use the size of the largest GridWin for normal split,
						//	or both combined for frozen panes, with the (document) size
						//	of the frozen part added to the page size
						//	(with frozen panes, the size of the individual parts
						//	depends on the scale that is to be calculated)

						if ( !pGridWin[SC_SPLIT_BOTTOMLEFT] ) return 0;
						Size aWinSize = pGridWin[SC_SPLIT_BOTTOMLEFT]->GetOutputSizePixel();
						ScSplitMode eHMode = aViewData.GetHSplitMode();
						if ( eHMode != SC_SPLIT_NONE && pGridWin[SC_SPLIT_BOTTOMRIGHT] )
						{
							long nOtherWidth = pGridWin[SC_SPLIT_BOTTOMRIGHT]->
														GetOutputSizePixel().Width();
							if ( eHMode == SC_SPLIT_FIX )
							{
								aWinSize.Width() += nOtherWidth;
								for ( SCCOL nCol = aViewData.GetPosX(SC_SPLIT_LEFT);
										nCol < aViewData.GetFixPosX(); nCol++ )
									aPageSize.Width() += pDoc->GetColWidth( nCol, nCurTab );
							}
							else if ( nOtherWidth > aWinSize.Width() )
								aWinSize.Width() = nOtherWidth;
						}
						ScSplitMode eVMode = aViewData.GetVSplitMode();
						if ( eVMode != SC_SPLIT_NONE && pGridWin[SC_SPLIT_TOPLEFT] )
						{
							long nOtherHeight = pGridWin[SC_SPLIT_TOPLEFT]->
														GetOutputSizePixel().Height();
							if ( eVMode == SC_SPLIT_FIX )
							{
								aWinSize.Height() += nOtherHeight;
                                aPageSize.Height() += pDoc->GetRowHeight(
                                        aViewData.GetPosY(SC_SPLIT_TOP),
                                        aViewData.GetFixPosY()-1, nCurTab);
							}
							else if ( nOtherHeight > aWinSize.Height() )
								aWinSize.Height() = nOtherHeight;
						}

						double nPPTX = ScGlobal::nScreenPPTX / aViewData.GetDocShell()->GetOutputFactor();
						double nPPTY = ScGlobal::nScreenPPTY;

						long nZoomX = (long) ( aWinSize.Width() * 100 /
											   ( aPageSize.Width() * nPPTX ) );
						long nZoomY = (long) ( aWinSize.Height() * 100 /
											   ( aPageSize.Height() * nPPTY ) );
						long nNew = nZoomX;

						if (eType == SVX_ZOOM_WHOLEPAGE && nZoomY < nNew)
							nNew = nZoomY;

						nZoom = (USHORT) nNew;
					}
				}
				break;

		default:
			DBG_ERROR("Unknown Zoom-Revision");
			nZoom = 0;
	}

	return nZoom;
}

//	wird z.B. gerufen, wenn sich das View-Fenster verschiebt:

void ScTabView::StopMarking()
{
	ScSplitPos eActive = aViewData.GetActivePart();
	if (pGridWin[eActive])
		pGridWin[eActive]->StopMarking();

	ScHSplitPos eH = WhichH(eActive);
	if (pColBar[eH])
		pColBar[eH]->StopMarking();

	ScVSplitPos eV = WhichV(eActive);
	if (pRowBar[eV])
		pRowBar[eV]->StopMarking();
}

void ScTabView::HideNoteMarker()
{
	for (USHORT i=0; i<4; i++)
		if (pGridWin[i] && pGridWin[i]->IsVisible())
			pGridWin[i]->HideNoteMarker();
}

void ScTabView::MakeDrawLayer()
{
	if (!pDrawView)
	{
		aViewData.GetDocShell()->MakeDrawLayer();

		//	pDrawView wird per Notify gesetzt
		DBG_ASSERT(pDrawView,"ScTabView::MakeDrawLayer funktioniert nicht");

		// #114409#
		for(sal_uInt16 a(0); a < 4; a++)
		{
			if(pGridWin[a])
			{
				pGridWin[a]->DrawLayerCreated();
			}
		}
	}
}

void ScTabView::ErrorMessage( USHORT nGlobStrId )
{
    if ( SC_MOD()->IsInExecuteDrop() )
    {
        // #i28468# don't show error message when called from Drag&Drop, silently abort instead
        return;
    }

	StopMarking();		// falls per Focus aus MouseButtonDown aufgerufen

	Window* pParent = aViewData.GetDialogParent();
	ScWaitCursorOff aWaitOff( pParent );
	BOOL bFocus = pParent && pParent->HasFocus();

	if(nGlobStrId==STR_PROTECTIONERR)
	{
		if(aViewData.GetDocShell()->IsReadOnly())
		{
			nGlobStrId=STR_READONLYERR;
		}
	}

	InfoBox aBox( pParent, ScGlobal::GetRscString( nGlobStrId ) );
	aBox.Execute();
	if (bFocus)
		pParent->GrabFocus();
}

Window* ScTabView::GetParentOrChild( USHORT nChildId )
{
	SfxViewFrame* pViewFrm = aViewData.GetViewShell()->GetViewFrame();

	if ( pViewFrm->HasChildWindow(nChildId) )
	{
		SfxChildWindow* pChild = pViewFrm->GetChildWindow(nChildId);
		if (pChild)
		{
			Window* pWin = pChild->GetWindow();
			if (pWin && pWin->IsVisible())
				return pWin;
		}
	}

	return aViewData.GetDialogParent();
}

void ScTabView::UpdatePageBreakData( BOOL bForcePaint )
{
	ScPageBreakData* pNewData = NULL;

	if (aViewData.IsPagebreakMode())
	{
		ScDocShell* pDocSh = aViewData.GetDocShell();
		ScDocument* pDoc = pDocSh->GetDocument();
		SCTAB nTab = aViewData.GetTabNo();

		USHORT nCount = pDoc->GetPrintRangeCount(nTab);
		if (!nCount)
			nCount = 1;
		pNewData = new ScPageBreakData(nCount);

		ScPrintFunc aPrintFunc( pDocSh, pDocSh->GetPrinter(), nTab, 0,0,NULL, NULL, pNewData );
		//	ScPrintFunc fuellt im ctor die PageBreakData
		if ( nCount > 1 )
		{
			aPrintFunc.ResetBreaks(nTab);
			pNewData->AddPages();
		}

		//	Druckbereiche veraendert?
		if ( bForcePaint || ( pPageBreakData && !pPageBreakData->IsEqual( *pNewData ) ) )
			PaintGrid();
	}

	delete pPageBreakData;
	pPageBreakData = pNewData;
}



