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


#include <svtools/colorcfg.hxx>
#include <svx/colritem.hxx>
#include <svx/editview.hxx>
#include <svx/fhgtitem.hxx>
#include <svx/scripttypeitem.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/printer.hxx>

#include <svx/svdview.hxx>
#include "tabvwsh.hxx"

#include "gridwin.hxx"
#include "viewdata.hxx"
#include "output.hxx"
#include "document.hxx"
#include "attrib.hxx"
#include "patattr.hxx"			// InvertSimple
#include "dbcolect.hxx"
#include "docoptio.hxx"
#include "notemark.hxx"
#include "dbfunc.hxx"			// oder GetPageBreakData an die ViewData
#include "scmod.hxx"
#include "inputhdl.hxx"
#include "rfindlst.hxx"
#include "hiranges.hxx"
#include "pagedata.hxx"
#include "docpool.hxx"
#include "globstr.hrc"
#include "docsh.hxx"			// oder GetSfxInPlaceObject
#include "cbutton.hxx"
#include "invmerge.hxx"
#include "editutil.hxx"
#include "inputopt.hxx"
#include "fillinfo.hxx"
#include "sc.hrc"
#include <vcl/virdev.hxx>

// #i74769#
#include <svx/sdrpaintwindow.hxx>

//#include "tabvwsh.hxx"			//! Test !!!!

//------------------------------------------------------------------------

void lcl_LimitRect( Rectangle& rRect, const Rectangle& rVisible )
{
	if ( rRect.Top()    < rVisible.Top()-1 )    rRect.Top()    = rVisible.Top()-1;
//	if ( rRect.Left()   < rVisible.Left()-1 )   rRect.Left()   = rVisible.Left()-1;
	if ( rRect.Bottom() > rVisible.Bottom()+1 ) rRect.Bottom() = rVisible.Bottom()+1;
//	if ( rRect.Right()  > rVisible.Right()+1 )  rRect.Right()  = rVisible.Right()+1;

	// #51122# auch wenn das inner-Rectangle nicht sichtbar ist, muss evtl.
	// die Titelzeile gezeichnet werden, darum kein Rueckgabewert mehr.
	// Wenn's weit daneben liegt, wird lcl_DrawOneFrame erst gar nicht gerufen.
}

void lcl_DrawOneFrame( OutputDevice* pDev, const Rectangle& rInnerPixel,
						const String& rTitle, const Color& rColor, BOOL bTextBelow,
						double nPPTX, double nPPTY, const Fraction& rZoomY,
						ScDocument* pDoc, ScViewData* pButtonViewData, BOOL bLayoutRTL )
{
	//	pButtonViewData wird nur benutzt, um die Button-Groesse zu setzen,
	//	darf ansonsten NULL sein!

	Rectangle aInner = rInnerPixel;
	if ( bLayoutRTL )
	{
		aInner.Left() = rInnerPixel.Right();
		aInner.Right() = rInnerPixel.Left();
	}

	Rectangle aVisible( Point(0,0), pDev->GetOutputSizePixel() );
	lcl_LimitRect( aInner, aVisible );

	Rectangle aOuter = aInner;
	long nHor = (long) ( SC_SCENARIO_HSPACE * nPPTX );
	long nVer = (long) ( SC_SCENARIO_VSPACE * nPPTY );
	aOuter.Left()	-= nHor;
	aOuter.Right()	+= nHor;
	aOuter.Top()	-= nVer;
	aOuter.Bottom()	+= nVer;

	//	use ScPatternAttr::GetFont only for font size
	Font aAttrFont;
	((const ScPatternAttr&)pDoc->GetPool()->GetDefaultItem(ATTR_PATTERN)).
									GetFont(aAttrFont,SC_AUTOCOL_BLACK,pDev,&rZoomY);

	//	everything else from application font
	Font aAppFont = pDev->GetSettings().GetStyleSettings().GetAppFont();
	aAppFont.SetSize( aAttrFont.GetSize() );

	aAppFont.SetAlign( ALIGN_TOP );
	pDev->SetFont( aAppFont );

	Size aTextSize( pDev->GetTextWidth( rTitle ), pDev->GetTextHeight() );

	if ( bTextBelow )
		aOuter.Bottom() += aTextSize.Height();
	else
		aOuter.Top()    -= aTextSize.Height();

	pDev->SetLineColor();
	pDev->SetFillColor( rColor );
	//	links, oben, rechts, unten
	pDev->DrawRect( Rectangle( aOuter.Left(),  aOuter.Top(),    aInner.Left(),  aOuter.Bottom() ) );
	pDev->DrawRect( Rectangle( aOuter.Left(),  aOuter.Top(),    aOuter.Right(), aInner.Top()    ) );
	pDev->DrawRect( Rectangle( aInner.Right(), aOuter.Top(),    aOuter.Right(), aOuter.Bottom() ) );
	pDev->DrawRect( Rectangle( aOuter.Left(),  aInner.Bottom(), aOuter.Right(), aOuter.Bottom() ) );

	long nButtonY = bTextBelow ? aInner.Bottom() : aOuter.Top();

	ScDDComboBoxButton aComboButton((Window*)pDev);
	aComboButton.SetOptSizePixel();
	long nBWidth  = ( aComboButton.GetSizePixel().Width() * rZoomY.GetNumerator() )
						/ rZoomY.GetDenominator();
	long nBHeight = nVer + aTextSize.Height() + 1;
	Size aButSize( nBWidth, nBHeight );
    long nButtonPos = bLayoutRTL ? aOuter.Left() : aOuter.Right()-nBWidth+1;
    aComboButton.Draw( Point(nButtonPos, nButtonY), aButSize, FALSE );
	if (pButtonViewData)
		pButtonViewData->SetScenButSize( aButSize );

	long nTextStart = bLayoutRTL ? aInner.Right() - aTextSize.Width() + 1 : aInner.Left();

	BOOL bWasClip = FALSE;
	Region aOldClip;
	BOOL bClip = ( aTextSize.Width() > aOuter.Right() - nBWidth - aInner.Left() );
	if ( bClip )
	{
		if (pDev->IsClipRegion())
		{
			bWasClip = TRUE;
			aOldClip = pDev->GetActiveClipRegion();
		}
		long nClipStartX = bLayoutRTL ? aOuter.Left() + nBWidth : aInner.Left();
		long nClipEndX = bLayoutRTL ? aInner.Right() : aOuter.Right() - nBWidth;
		pDev->SetClipRegion( Rectangle( nClipStartX, nButtonY + nVer/2,
								nClipEndX, nButtonY + nVer/2 + aTextSize.Height() ) );
	}

	pDev->DrawText( Point( nTextStart, nButtonY + nVer/2 ), rTitle );

	if ( bClip )
	{
		if ( bWasClip )
			pDev->SetClipRegion(aOldClip);
		else
			pDev->SetClipRegion();
	}

	pDev->SetFillColor();
	pDev->SetLineColor( COL_BLACK );
	pDev->DrawRect( aInner );
	pDev->DrawRect( aOuter );
}

void lcl_DrawScenarioFrames( OutputDevice* pDev, ScViewData* pViewData, ScSplitPos eWhich,
							SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2 )
{
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	SCTAB nTabCount = pDoc->GetTableCount();
	if ( nTab+1<nTabCount && pDoc->IsScenario(nTab+1) && !pDoc->IsScenario(nTab) )
	{
		if ( nX1 > 0 ) --nX1;
		if ( nY1>=2 ) nY1 -= 2;				// Hack: Titelzeile beruehrt zwei Zellen
		else if ( nY1 > 0 ) --nY1;
		if ( nX2 < MAXCOL ) ++nX2;
		if ( nY2 < MAXROW-1 ) nY2 += 2;		// Hack: Titelzeile beruehrt zwei Zellen
		else if ( nY2 < MAXROW ) ++nY2;
		ScRange aViewRange( nX1,nY1,nTab, nX2,nY2,nTab );

		//!	Ranges an der Table cachen!!!!

		ScMarkData aMarks;
		for (SCTAB i=nTab+1; i<nTabCount && pDoc->IsScenario(i); i++)
			pDoc->MarkScenario( i, nTab, aMarks, FALSE, SC_SCENARIO_SHOWFRAME );
		ScRangeListRef xRanges = new ScRangeList;
		aMarks.FillRangeListWithMarks( xRanges, FALSE );

		BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
		long nLayoutSign = bLayoutRTL ? -1 : 1;

		USHORT nRangeCount = (USHORT)xRanges->Count();
		for (USHORT j=0; j<nRangeCount; j++)
		{
			ScRange aRange = *xRanges->GetObject(j);
			//	Szenario-Rahmen immer dann auf zusammengefasste Zellen erweitern, wenn
			//	dadurch keine neuen nicht-ueberdeckten Zellen mit umrandet werden
			pDoc->ExtendTotalMerge( aRange );

			//!	-> Repaint beim Zusammenfassen erweitern !!!

			if ( aRange.Intersects( aViewRange ) )			//! Platz fuer Text/Button?
			{
				Point aStartPos = pViewData->GetScrPos(
									aRange.aStart.Col(), aRange.aStart.Row(), eWhich, TRUE );
				Point aEndPos = pViewData->GetScrPos(
									aRange.aEnd.Col()+1, aRange.aEnd.Row()+1, eWhich, TRUE );
				//	on the grid:
				aStartPos.X() -= nLayoutSign;
				aStartPos.Y() -= 1;
				aEndPos.X() -= nLayoutSign;
				aEndPos.Y() -= 1;

				BOOL bTextBelow = ( aRange.aStart.Row() == 0 );

				String aCurrent;
				Color aColor( COL_LIGHTGRAY );
				for (SCTAB nAct=nTab+1; nAct<nTabCount && pDoc->IsScenario(nAct); nAct++)
					if ( pDoc->IsActiveScenario(nAct) && pDoc->HasScenarioRange(nAct,aRange) )
					{
						String aDummyComment;
						USHORT nDummyFlags;
						pDoc->GetName( nAct, aCurrent );
						pDoc->GetScenarioData( nAct, aDummyComment, aColor, nDummyFlags );
					}

				if (!aCurrent.Len())
					aCurrent = ScGlobal::GetRscString( STR_EMPTYDATA );

				//!	eigener Text "(keins)" statt "(leer)" ???

				lcl_DrawOneFrame( pDev, Rectangle( aStartPos, aEndPos ),
									aCurrent, aColor, bTextBelow,
									pViewData->GetPPTX(), pViewData->GetPPTY(), pViewData->GetZoomY(),
									pDoc, pViewData, bLayoutRTL );
			}
		}
	}
}

//------------------------------------------------------------------------

void lcl_DrawHighlight( ScOutputData& rOutputData, ScViewData* pViewData,
						ScHighlightRanges& rHighlightRanges )
{
	SCTAB nTab = pViewData->GetTabNo();
	ULONG nCount = rHighlightRanges.Count();
	for (ULONG i=0; i<nCount; i++)
	{
		ScHighlightEntry* pEntry = rHighlightRanges.GetObject( i );
		if (pEntry)
		{
			ScRange aRange = pEntry->aRef;
			if ( nTab >= aRange.aStart.Tab() && nTab <= aRange.aEnd.Tab() )
			{
				rOutputData.DrawRefMark(
									aRange.aStart.Col(), aRange.aStart.Row(),
									aRange.aEnd.Col(), aRange.aEnd.Row(),
									pEntry->aColor, FALSE );
			}
		}
	}
}

//------------------------------------------------------------------------

void ScGridWindow::DoInvertRect( const Rectangle& rPixel )
{
//	Invert( PixelToLogic(rPixel) );

	if ( rPixel == aInvertRect )
		aInvertRect = Rectangle();		// aufheben
	else
	{
		DBG_ASSERT( aInvertRect.IsEmpty(), "DoInvertRect nicht paarig" );

		aInvertRect = rPixel;			// neues Rechteck merken
	}

    UpdateHeaderOverlay();      // uses aInvertRect
}

//------------------------------------------------------------------------

void __EXPORT ScGridWindow::PrePaint()
{
	// forward PrePaint to DrawingLayer
	ScTabViewShell* pTabViewShell = pViewData->GetViewShell();

	if(pTabViewShell)
	{
		SdrView* pDrawView = pTabViewShell->GetSdrView();

		if(pDrawView)
		{
            pDrawView->PrePaint();
        }
    }
}

//------------------------------------------------------------------------

void __EXPORT ScGridWindow::Paint( const Rectangle& rRect )
{
    //TODO/LATER: how to get environment? Do we need that?!
    /*
	ScDocShell* pDocSh = pViewData->GetDocShell();
	SvInPlaceEnvironment* pEnv = pDocSh->GetIPEnv();
	if (pEnv && pEnv->GetRectsChangedLockCount())
	{
		Invalidate(rRect);
		return;
    }*/

	ScDocument* pDoc = pViewData->GetDocument();
	if ( pDoc->IsInInterpreter() )
	{
		//	via Reschedule, interpretierende Zellen nicht nochmal anstossen
		//	hier kein Invalidate, sonst kommt z.B. eine Error-Box nie an die Reihe
		//	(Bug 36381). Durch bNeedsRepaint wird spaeter alles nochmal gemalt.

		if ( bNeedsRepaint )
		{
			//!	Rechtecke zusammenfassen?
			aRepaintPixel = Rectangle();			// mehrfach -> alles painten
		}
		else
		{
			bNeedsRepaint = TRUE;
			aRepaintPixel = LogicToPixel(rRect);	// nur betroffenen Bereich
		}
		return;
	}

	if (bIsInPaint)
		return;

	bIsInPaint = TRUE;

	Rectangle aPixRect = LogicToPixel( rRect );

	SCCOL nX1 = pViewData->GetPosX(eHWhich);
	SCROW nY1 = pViewData->GetPosY(eVWhich);

	SCTAB nTab = pViewData->GetTabNo();

	double nPPTX = pViewData->GetPPTX();
	double nPPTY = pViewData->GetPPTY();

	Rectangle aMirroredPixel = aPixRect;
	if ( pDoc->IsLayoutRTL( nTab ) )
	{
		//	mirror and swap
		long nWidth = GetSizePixel().Width();
		aMirroredPixel.Left()  = nWidth - 1 - aPixRect.Right();
		aMirroredPixel.Right() = nWidth - 1 - aPixRect.Left();
	}

	long nScrX = ScViewData::ToPixel( pDoc->GetColWidth( nX1, nTab ), nPPTX );
	while ( nScrX <= aMirroredPixel.Left() && nX1 < MAXCOL )
	{
		++nX1;
		nScrX += ScViewData::ToPixel( pDoc->GetColWidth( nX1, nTab ), nPPTX );
	}
	SCCOL nX2 = nX1;
	while ( nScrX <= aMirroredPixel.Right() && nX2 < MAXCOL )
	{
		++nX2;
		nScrX += ScViewData::ToPixel( pDoc->GetColWidth( nX2, nTab ), nPPTX );
	}

	long nScrY = ScViewData::ToPixel( pDoc->GetRowHeight( nY1, nTab ), nPPTY );
	while ( nScrY <= aPixRect.Top() && nY1 < MAXROW )
	{
		++nY1;
		nScrY += ScViewData::ToPixel( pDoc->GetRowHeight( nY1, nTab ), nPPTY );
	}
	SCROW nY2 = nY1;
	while ( nScrY <= aPixRect.Bottom() && nY2 < MAXROW )
	{
		++nY2;
		nScrY += ScViewData::ToPixel( pDoc->GetRowHeight( nY2, nTab ), nPPTY );
	}

	Draw( nX1,nY1,nX2,nY2, SC_UPDATE_MARKS );			// nicht weiterzeichnen

	bIsInPaint = FALSE;
}

//
// 	Draw  ----------------------------------------------------------------
//

void ScGridWindow::Draw( SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2, ScUpdateMode eMode )
{
	ScModule* pScMod = SC_MOD();
	BOOL bTextWysiwyg = pScMod->GetInputOptions().GetTextWysiwyg();
	BOOL bGridFirst = TRUE;		//! entscheiden!!!

	if (pViewData->IsMinimized())
		return;

	PutInOrder( nX1, nX2 );
	PutInOrder( nY1, nY2 );

	DBG_ASSERT( ValidCol(nX2) && ValidRow(nY2), "GridWin Draw Bereich zu gross" );

	SCCOL nPosX = pViewData->GetPosX( eHWhich );
	SCROW nPosY = pViewData->GetPosY( eVWhich );
	if (nX2 < nPosX || nY2 < nPosY)
		return;											// unsichtbar
	if (nX1 < nPosX) nX1 = nPosX;
	if (nY1 < nPosY) nY1 = nPosY;

	SCCOL nXRight = nPosX + pViewData->VisibleCellsX(eHWhich);
	if (nXRight > MAXCOL) nXRight = MAXCOL;
	SCROW nYBottom = nPosY + pViewData->VisibleCellsY(eVWhich);
	if (nYBottom > MAXROW) nYBottom = MAXROW;

	if (nX1 > nXRight || nY1 > nYBottom)
		return;											// unsichtbar
	if (nX2 > nXRight) nX2 = nXRight;
	if (nY2 > nYBottom) nY2 = nYBottom;

	if ( eMode != SC_UPDATE_MARKS )
		if (nX2 < nXRight)
			nX2 = nXRight;								// zum Weiterzeichnen

		//	ab hier kein return mehr

	++nPaintCount;					// merken, dass gemalt wird (wichtig beim Invertieren)

	ScDocShell* pDocSh = pViewData->GetDocShell();
	ScDocument* pDoc = pDocSh->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();

	pDoc->ExtendHidden( nX1, nY1, nX2, nY2, nTab );

	Point aScrPos = pViewData->GetScrPos( nX1, nY1, eWhich );
	long nMirrorWidth = GetSizePixel().Width();
	BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
	long nLayoutSign = bLayoutRTL ? -1 : 1;
	if ( bLayoutRTL )
	{
		long nEndPixel = pViewData->GetScrPos( nX2+1, nPosY, eWhich ).X();
		nMirrorWidth = aScrPos.X() - nEndPixel;
		aScrPos.X() = nEndPixel + 1;
	}

	long nScrX = aScrPos.X();
	long nScrY = aScrPos.Y();

	SCCOL nCurX = pViewData->GetCurX();
	SCROW nCurY = pViewData->GetCurY();
	SCCOL nCurEndX = nCurX;
	SCROW nCurEndY = nCurY;
	pDoc->ExtendMerge( nCurX, nCurY, nCurEndX, nCurEndY, nTab );
	BOOL bCurVis = nCursorHideCount==0 &&
					( nCurEndX+1 >= nX1 && nCurX <= nX2+1 && nCurEndY+1 >= nY1 && nCurY <= nY2+1 );

	//	AutoFill-Anfasser
	if ( !bCurVis && nCursorHideCount==0 && bAutoMarkVisible && aAutoMarkPos.Tab() == nTab &&
			( aAutoMarkPos.Col() != nCurX || aAutoMarkPos.Row() != nCurY ) )
	{
		SCCOL nHdlX = aAutoMarkPos.Col();
		SCROW nHdlY = aAutoMarkPos.Row();
		pDoc->ExtendMerge( nHdlX, nHdlY, nHdlX, nHdlY, nTab );
		bCurVis = ( nHdlX+1 >= nX1 && nHdlX <= nX2 && nHdlY+1 >= nY1 && nHdlY <= nY2 );
		//	links und oben ist nicht betroffen

		//!	AutoFill-Anfasser alleine (ohne Cursor) zeichnen ???
	}

	double nPPTX = pViewData->GetPPTX();
	double nPPTY = pViewData->GetPPTY();

	const ScViewOptions& rOpts = pViewData->GetOptions();
	BOOL bFormulaMode = rOpts.GetOption( VOPT_FORMULAS );
	BOOL bMarkClipped = rOpts.GetOption( VOPT_CLIPMARKS );

		// Datenblock

    ScTableInfo aTabInfo;
    pDoc->FillInfo( aTabInfo, nX1, nY1, nX2, nY2, nTab,
										nPPTX, nPPTY, FALSE, bFormulaMode,
										&pViewData->GetMarkData() );

	//--------------------------------------------------------------------

	Fraction aZoomX = pViewData->GetZoomX();
	Fraction aZoomY = pViewData->GetZoomY();
    ScOutputData aOutputData( this, OUTTYPE_WINDOW, aTabInfo, pDoc, nTab,
								nScrX, nScrY, nX1, nY1, nX2, nY2, nPPTX, nPPTY,
								&aZoomX, &aZoomY );

	aOutputData.SetMirrorWidth( nMirrorWidth );			// needed for RTL

    std::auto_ptr< VirtualDevice > xFmtVirtDev;
    BOOL bLogicText = bTextWysiwyg;                     // call DrawStrings in logic MapMode?

	if ( bTextWysiwyg )
	{
		//	use printer for text formatting

		OutputDevice* pFmtDev = pDoc->GetPrinter();
		pFmtDev->SetMapMode( pViewData->GetLogicMode(eWhich) );
		aOutputData.SetFmtDevice( pFmtDev );
	}
    else if ( aZoomX != aZoomY && pViewData->IsOle() )
    {
        //  #i45033# For OLE inplace editing with different zoom factors,
        //  use a virtual device with 1/100th mm as text formatting reference

        xFmtVirtDev.reset( new VirtualDevice );
        xFmtVirtDev->SetMapMode( MAP_100TH_MM );
        aOutputData.SetFmtDevice( xFmtVirtDev.get() );

        bLogicText = TRUE;                      // use logic MapMode
    }

    const svtools::ColorConfig& rColorCfg = pScMod->GetColorConfig();
    Color aGridColor( rColorCfg.GetColorValue( svtools::CALCGRID, FALSE ).nColor );
	if ( aGridColor.GetColor() == COL_TRANSPARENT )
	{
		//	use view options' grid color only if color config has "automatic" color
		aGridColor = rOpts.GetGridColor();
	}

	aOutputData.SetSyntaxMode		( pViewData->IsSyntaxMode() );
	aOutputData.SetGridColor		( aGridColor );
	aOutputData.SetShowNullValues	( rOpts.GetOption( VOPT_NULLVALS ) );
	aOutputData.SetShowFormulas		( bFormulaMode );
    aOutputData.SetShowSpellErrors  ( pDoc->GetDocOptions().IsAutoSpell() );
	aOutputData.SetMarkClipped		( bMarkClipped );

	aOutputData.SetUseStyleColor( TRUE );		// always set in table view

	aOutputData.SetEditObject( GetEditObject() );
	aOutputData.SetViewShell( pViewData->GetViewShell() );

	BOOL bGrid = rOpts.GetOption( VOPT_GRID );
	BOOL bPage = rOpts.GetOption( VOPT_PAGEBREAKS );

	if ( eMode == SC_UPDATE_CHANGED )
	{
		aOutputData.FindChanged();
		aOutputData.SetSingleGrid(TRUE);
	}

	BOOL bPageMode = pViewData->IsPagebreakMode();
	if (bPageMode)										// nach FindChanged
	{
		// SetPagebreakMode initialisiert auch bPrinted Flags
		aOutputData.SetPagebreakMode( pViewData->GetView()->GetPageBreakData() );
	}

	EditView*	pEditView = NULL;
	BOOL		bEditMode = pViewData->HasEditView(eWhich);
	if ( bEditMode && pViewData->GetRefTabNo() == nTab )
	{
		SCCOL nEditCol;
		SCROW nEditRow;
		pViewData->GetEditView( eWhich, pEditView, nEditCol, nEditRow );
		SCCOL nEditEndCol = pViewData->GetEditEndCol();
		SCROW nEditEndRow = pViewData->GetEditEndRow();

		if ( nEditEndCol >= nX1 && nEditCol <= nX2 && nEditEndRow >= nY1 && nEditRow <= nY2 )
			aOutputData.SetEditCell( nEditCol, nEditRow );
		else
			bEditMode = FALSE;

		//	nur Edit-Area zu zeichnen?
		//!	dann muss trotzdem noch der Rand / das Gitter gemalt werden!

//		if ( nEditCol <= nX1 && nEditEndCol >= nX2 && nEditRow <= nY1 && nEditEndRow >= nY2 )
//			bOnlyEdit = TRUE;
	}

	// define drawing layer map mode and paint rectangle
	const MapMode aDrawMode = GetDrawMapMode();
	Rectangle aDrawingRectLogic;

	{
		// get drawing pixel rect
		Rectangle aDrawingRectPixel(Point(nScrX, nScrY), Size(aOutputData.GetScrW(), aOutputData.GetScrH()));

		// correct for border (left/right)
		if(MAXCOL == nX2)
		{
			if(bLayoutRTL)
			{
				aDrawingRectPixel.Left() = 0L;
			}
			else
			{
				aDrawingRectPixel.Right() = GetOutputSizePixel().getWidth();
			}
		}

		// correct for border (bottom)
		if(MAXROW == nY2)
		{
			aDrawingRectPixel.Bottom() = GetOutputSizePixel().getHeight();
		}

		// get logic positions
		aDrawingRectLogic = PixelToLogic(aDrawingRectPixel, aDrawMode);
	}

// not necessary with overlay
//	if (bCurVis)
//		HideCursor();

    OutputDevice* pContentDev = this;       // device for document content, used by overlay manager
	SdrPaintWindow* pTargetPaintWindow = 0;	// #i74769# work with SdrPaintWindow directly

	{
		// init redraw
		ScTabViewShell* pTabViewShell = pViewData->GetViewShell();

		if(pTabViewShell)
		{
			MapMode aCurrentMapMode(pContentDev->GetMapMode());
			pContentDev->SetMapMode(aDrawMode);
			SdrView* pDrawView = pTabViewShell->GetSdrView();

			if(pDrawView)
			{
				// #i74769# Use new BeginDrawLayers() interface
				Region aDrawingRegion(aDrawingRectLogic);
				pTargetPaintWindow = pDrawView->BeginDrawLayers(this, aDrawingRegion);
				OSL_ENSURE(pTargetPaintWindow, "BeginDrawLayers: Got no SdrPaintWindow (!)");

				// #i74769# get target device from SdrPaintWindow, this may be the prerender
				// device now, too.
				pContentDev = &(pTargetPaintWindow->GetTargetOutputDevice());
                aOutputData.SetContentDevice( pContentDev );
			}

			pContentDev->SetMapMode(aCurrentMapMode);
		}
	}

	//	Rand (Wiese) (Pixel)
	if ( nX2==MAXCOL || nY2==MAXROW )
	{
		// save MapMode and set to pixel
		MapMode aCurrentMapMode(pContentDev->GetMapMode());
		pContentDev->SetMapMode(MAP_PIXEL);

		Rectangle aPixRect = Rectangle( Point(), GetOutputSizePixel() );
        pContentDev->SetFillColor( rColorCfg.GetColorValue(svtools::APPBACKGROUND).nColor );
		pContentDev->SetLineColor();
		if ( nX2==MAXCOL )
		{
			Rectangle aDrawRect( aPixRect );
			if ( bLayoutRTL )
				aDrawRect.Right() = nScrX - 1;
			else
				aDrawRect.Left() = nScrX + aOutputData.GetScrW();
			if (aDrawRect.Right() >= aDrawRect.Left())
				pContentDev->DrawRect( aDrawRect );
		}
		if ( nY2==MAXROW )
		{
			Rectangle aDrawRect( aPixRect );
			aDrawRect.Top() = nScrY + aOutputData.GetScrH();
			if ( nX2==MAXCOL )
			{
				// no double painting of the corner
				if ( bLayoutRTL )
					aDrawRect.Left() = nScrX;
				else
					aDrawRect.Right() = nScrX + aOutputData.GetScrW() - 1;
			}
			if (aDrawRect.Bottom() >= aDrawRect.Top())
				pContentDev->DrawRect( aDrawRect );
		}

		// restore MapMode
		pContentDev->SetMapMode(aCurrentMapMode);
	}

	if ( pDoc->HasBackgroundDraw( nTab, aDrawingRectLogic ) )
	{
		pContentDev->SetMapMode(MAP_PIXEL);
		aOutputData.DrawClear();

			// Drawing Hintergrund

		pContentDev->SetMapMode(aDrawMode);
		DrawRedraw( aOutputData, eMode, SC_LAYER_BACK );
	}
	else
		aOutputData.SetSolidBackground(TRUE);

	pContentDev->SetMapMode(MAP_PIXEL);
	aOutputData.DrawBackground();
	if ( bGridFirst && ( bGrid || bPage ) )
		aOutputData.DrawGrid( bGrid, bPage );
	if ( bPageMode )
	{
		// #87655# DrawPagePreview draws complete lines/page numbers, must always be clipped
		if ( aOutputData.SetChangedClip() )
		{
            DrawPagePreview(nX1,nY1,nX2,nY2, pContentDev);
			pContentDev->SetClipRegion();
		}
	}
	aOutputData.DrawShadow();
	aOutputData.DrawFrame();
	if ( !bLogicText )
		aOutputData.DrawStrings(FALSE);		// in pixel MapMode

		// Autofilter- und Pivot-Buttons

    DrawButtons( nX1, nY1, nX2, nY2, aTabInfo, pContentDev );          // Pixel

		// Notiz-Anzeiger

	if ( rOpts.GetOption( VOPT_NOTES ) )
		aOutputData.DrawNoteMarks();

		// Edit-Zellen

	pContentDev->SetMapMode(pViewData->GetLogicMode(eWhich));
	if ( bLogicText )
		aOutputData.DrawStrings(TRUE);		// in logic MapMode if bTextWysiwyg is set
	aOutputData.DrawEdit(TRUE);

	pContentDev->SetMapMode(MAP_PIXEL);
	if ( !bGridFirst && ( bGrid || bPage ) )
	{
		aOutputData.DrawGrid( bGrid, bPage );
	}
	aOutputData.DrawClipMarks();

	//	Szenario / ChangeTracking muss auf jeden Fall nach DrawGrid sein, auch bei !bGridFirst

	//!	Test, ob ChangeTrack-Anzeige aktiv ist
	//!	Szenario-Rahmen per View-Optionen abschaltbar?

	SCTAB nTabCount = pDoc->GetTableCount();
	ScHighlightRanges* pHigh = pViewData->GetView()->GetHighlightRanges();
	BOOL bHasScenario = ( nTab+1<nTabCount && pDoc->IsScenario(nTab+1) && !pDoc->IsScenario(nTab) );
	BOOL bHasChange = ( pDoc->GetChangeTrack() != NULL );

	if ( bHasChange || bHasScenario || pHigh != NULL )
	{

		//! SetChangedClip() mit DrawMarks() zusammenfassen?? (anderer MapMode!)

		BOOL bAny = TRUE;
		if (eMode == SC_UPDATE_CHANGED)
			bAny = aOutputData.SetChangedClip();
		if (bAny)
		{
			if ( bHasChange )
				aOutputData.DrawChangeTrack();

			if ( bHasScenario )
				lcl_DrawScenarioFrames( pContentDev, pViewData, eWhich, nX1,nY1,nX2,nY2 );

			if ( pHigh )
				lcl_DrawHighlight( aOutputData, pViewData, *pHigh );

			if (eMode == SC_UPDATE_CHANGED)
				pContentDev->SetClipRegion();
		}
	}

		// Drawing Vordergrund

	pContentDev->SetMapMode(aDrawMode);

	DrawRedraw( aOutputData, eMode, SC_LAYER_FRONT );
	DrawRedraw( aOutputData, eMode, SC_LAYER_INTERN );
    DrawSdrGrid( aDrawingRectLogic, pContentDev );

	if (!bIsInScroll)								// Drawing Markierungen
	{
		if(eMode == SC_UPDATE_CHANGED && aOutputData.SetChangedClip())
		{
			pContentDev->SetClipRegion();
		}

		//BOOL bDraw = TRUE;
		//if (eMode == SC_UPDATE_CHANGED)
		//	bDraw = NeedDrawMarks() && aOutputData.SetChangedClip();
		//if (bDraw)
		//{
		//	DrawMarks();
		//	if (eMode == SC_UPDATE_CHANGED)
		//		pContentDev->SetClipRegion();
		//}
	}

	pContentDev->SetMapMode(MAP_PIXEL);

#ifdef OLD_SELECTION_PAINT
	if (pViewData->IsActive())
		aOutputData.DrawMark( this );
#endif

	if ( pViewData->IsRefMode() && nTab >= pViewData->GetRefStartZ() && nTab <= pViewData->GetRefEndZ() )
	{
        // The AutoFill shrink area has an own overlay now
#if 0
		//	Schraffur beim Loeschen per AutoFill
		if ( pViewData->GetRefType() == SC_REFTYPE_FILL )
		{
			ScRange aRange;
			if ( pViewData->GetDelMark( aRange ) )
			{
				if ( aRange.aStart.Col() < nX1 ) aRange.aStart.SetCol(nX1);
				if ( aRange.aEnd.Col() > nX2 )	 aRange.aEnd.SetCol(nX2);
				if ( aRange.aStart.Row() < nY1 ) aRange.aStart.SetRow(nY1);
				if ( aRange.aEnd.Row() > nY2 )	 aRange.aEnd.SetRow(nY2);
				if ( aRange.aStart.Col() <= aRange.aEnd.Col() &&
					 aRange.aStart.Row() <= aRange.aEnd.Row() )
				{
					Point aStart = pViewData->GetScrPos( aRange.aStart.Col(),
														 aRange.aStart.Row(), eWhich );
					Point aEnd = pViewData->GetScrPos( aRange.aEnd.Col()+1,
													   aRange.aEnd.Row()+1, eWhich );
					aEnd.X() -= 1;
					aEnd.Y() -= 1;

					//	Markierung aufheben - roter Rahmen bleibt stehen
					Rectangle aRect( aStart,aEnd );
					Invert( aRect, INVERT_HIGHLIGHT );

					//!	Delete-Bereich extra kennzeichnen?!?!?
				}
			}
		}
#endif

        Color aRefColor( rColorCfg.GetColorValue(svtools::CALCREFERENCE).nColor );
		aOutputData.DrawRefMark( pViewData->GetRefStartX(), pViewData->GetRefStartY(),
								 pViewData->GetRefEndX(), pViewData->GetRefEndY(),
								 aRefColor, FALSE );
	}

		//	Range-Finder

	ScInputHandler* pHdl = pScMod->GetInputHdl( pViewData->GetViewShell() );
	if (pHdl)
	{
		ScRangeFindList* pRangeFinder = pHdl->GetRangeFindList();
		if ( pRangeFinder && !pRangeFinder->IsHidden() &&
				pRangeFinder->GetDocName() == pDocSh->GetTitle() )
		{
			USHORT nCount = (USHORT)pRangeFinder->Count();
			for (USHORT i=0; i<nCount; i++)
			{
				ScRangeFindData* pData = pRangeFinder->GetObject(i);
				if (pData)
				{
					ScRange aRef = pData->aRef;
					aRef.Justify();
					if ( aRef.aStart.Tab() >= nTab && aRef.aEnd.Tab() <= nTab )
						aOutputData.DrawRefMark( aRef.aStart.Col(), aRef.aStart.Row(),
												aRef.aEnd.Col(), aRef.aEnd.Row(),
												Color( ScRangeFindList::GetColorName( i ) ),
												TRUE );
				}
			}
		}
	}

	{
		// end redraw
		ScTabViewShell* pTabViewShell = pViewData->GetViewShell();

		if(pTabViewShell)
		{
			MapMode aCurrentMapMode(pContentDev->GetMapMode());
			pContentDev->SetMapMode(aDrawMode);
			SdrView* pDrawView = pTabViewShell->GetSdrView();

			if(pDrawView)
			{
				// #i74769# work with SdrPaintWindow directly
				pDrawView->EndDrawLayers(*pTargetPaintWindow, true);
			}

			pContentDev->SetMapMode(aCurrentMapMode);
		}
	}

	//	InPlace Edit-View
	// moved after EndDrawLayers() to get it outside the overlay buffer and
	// on top of everything
	if ( bEditMode && (pViewData->GetRefTabNo() == pViewData->GetTabNo()) )
	{
        //! use pContentDev for EditView?
        SetMapMode(MAP_PIXEL);
		SCCOL nCol1 = pViewData->GetEditStartCol();
		SCROW nRow1 = pViewData->GetEditStartRow();
		SCCOL nCol2 = pViewData->GetEditEndCol();
		SCROW nRow2 = pViewData->GetEditEndRow();
		SetLineColor();
		SetFillColor( pEditView->GetBackgroundColor() );
		Point aStart = pViewData->GetScrPos( nCol1, nRow1, eWhich );
		Point aEnd = pViewData->GetScrPos( nCol2+1, nRow2+1, eWhich );
		aEnd.X() -= 2 * nLayoutSign;		// don't overwrite grid
		aEnd.Y() -= 2;
		DrawRect( Rectangle( aStart,aEnd ) );

		SetMapMode(pViewData->GetLogicMode());
		pEditView->Paint( PixelToLogic( Rectangle( Point( nScrX, nScrY ),
							Size( aOutputData.GetScrW(), aOutputData.GetScrH() ) ) ) );
		SetMapMode(MAP_PIXEL);
	}

	if (pViewData->HasEditView(eWhich))
	{
		// flush OverlayManager before changing the MapMode
		flushOverlayManager();

		// set MapMode for text edit
		SetMapMode(pViewData->GetLogicMode());
	}
	else
		SetMapMode(aDrawMode);

	if ( pNoteMarker )
		pNoteMarker->Draw();		// ueber den Cursor, im Drawing-MapMode

	//DrawStartTimer();				// fuer bunte Handles ohne System-Clipping

	//
	//	Wenn waehrend des Paint etwas invertiert wurde (Selektion geaendert aus Basic-Macro),
	//	ist das jetzt durcheinandergekommen und es muss neu gemalt werden
	//

	DBG_ASSERT(nPaintCount, "nPaintCount falsch");
	--nPaintCount;
	if (!nPaintCount)
		CheckNeedsRepaint();
}

void ScGridWindow::CheckNeedsRepaint()
{
	//	called at the end of painting, and from timer after background text width calculation

	if (bNeedsRepaint)
	{
		bNeedsRepaint = FALSE;
		if (aRepaintPixel.IsEmpty())
			Invalidate();
		else
			Invalidate(PixelToLogic(aRepaintPixel));
		aRepaintPixel = Rectangle();

        // selection function in status bar might also be invalid
        SfxBindings& rBindings = pViewData->GetBindings();
        rBindings.Invalidate( SID_STATUS_SUM );
        rBindings.Invalidate( SID_ATTR_SIZE );
        rBindings.Invalidate( SID_TABLE_CELL );
	}
}

void ScGridWindow::DrawPagePreview( SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2, OutputDevice* pContentDev )
{
	ScPageBreakData* pPageData = pViewData->GetView()->GetPageBreakData();
	if (pPageData)
	{
		ScDocument* pDoc = pViewData->GetDocument();
		SCTAB nTab = pViewData->GetTabNo();
		Size aWinSize = GetOutputSizePixel();
        const svtools::ColorConfig& rColorCfg = SC_MOD()->GetColorConfig();
        Color aManual( rColorCfg.GetColorValue(svtools::CALCPAGEBREAKMANUAL).nColor );
        Color aAutomatic( rColorCfg.GetColorValue(svtools::CALCPAGEBREAK).nColor );

		String aPageText = ScGlobal::GetRscString( STR_PAGE );
		if ( nPageScript == 0 )
		{
			//	get script type of translated "Page" string only once
			nPageScript = pDoc->GetStringScriptType( aPageText );
			if (nPageScript == 0)
				nPageScript = ScGlobal::GetDefaultScriptType();
		}
		aPageText += ' ';

		Font aFont;
		ScEditEngineDefaulter* pEditEng = NULL;
		const ScPatternAttr& rDefPattern = ((const ScPatternAttr&)pDoc->GetPool()->GetDefaultItem(ATTR_PATTERN));
		if ( nPageScript == SCRIPTTYPE_LATIN )
		{
			//	use single font and call DrawText directly
			rDefPattern.GetFont( aFont, SC_AUTOCOL_BLACK );
			aFont.SetColor( Color( COL_LIGHTGRAY ) );
			//	font size is set as needed
		}
		else
		{
			//	use EditEngine to draw mixed-script string
			pEditEng = new ScEditEngineDefaulter( EditEngine::CreatePool(), TRUE );
			pEditEng->SetRefMapMode( pContentDev->GetMapMode() );
			SfxItemSet* pEditDefaults = new SfxItemSet( pEditEng->GetEmptyItemSet() );
			rDefPattern.FillEditItemSet( pEditDefaults );
			pEditDefaults->Put( SvxColorItem( Color( COL_LIGHTGRAY ), EE_CHAR_COLOR ) );
			pEditEng->SetDefaults( pEditDefaults );
		}

        USHORT nCount = sal::static_int_cast<USHORT>( pPageData->GetCount() );
		for (USHORT nPos=0; nPos<nCount; nPos++)
		{
			ScPrintRangeData& rData = pPageData->GetData(nPos);
			ScRange aRange = rData.GetPrintRange();
			if ( aRange.aStart.Col() <= nX2+1  && aRange.aEnd.Col()+1 >= nX1 &&
				 aRange.aStart.Row() <= nY2+1 && aRange.aEnd.Row()+1 >= nY1 )
			{
				//	3 Pixel Rahmen um den Druckbereich
				//	(mittlerer Pixel auf den Gitterlinien)

				pContentDev->SetLineColor();
				if (rData.IsAutomatic())
					pContentDev->SetFillColor( aAutomatic );
				else
					pContentDev->SetFillColor( aManual );

				Point aStart = pViewData->GetScrPos(
									aRange.aStart.Col(), aRange.aStart.Row(), eWhich, TRUE );
				Point aEnd = pViewData->GetScrPos(
									aRange.aEnd.Col() + 1, aRange.aEnd.Row() + 1, eWhich, TRUE );
				aStart.X() -= 2;
				aStart.Y() -= 2;

				//	Ueberlaeufe verhindern:
				if ( aStart.X() < -10 )	aStart.X() = -10;
				if ( aStart.Y() < -10 )	aStart.Y() = -10;
				if ( aEnd.X() > aWinSize.Width() + 10 )
					aEnd.X() = aWinSize.Width() + 10;
				if ( aEnd.Y() > aWinSize.Height() + 10 )
					aEnd.Y() = aWinSize.Height() + 10;

				pContentDev->DrawRect( Rectangle( aStart, Point(aEnd.X(),aStart.Y()+2) ) );
				pContentDev->DrawRect( Rectangle( aStart, Point(aStart.X()+2,aEnd.Y()) ) );
				pContentDev->DrawRect( Rectangle( Point(aStart.X(),aEnd.Y()-2), aEnd ) );
				pContentDev->DrawRect( Rectangle( Point(aEnd.X()-2,aStart.Y()), aEnd ) );

				//	Seitenumbrueche
				//!	anders darstellen (gestrichelt ????)

				size_t nColBreaks = rData.GetPagesX();
				const SCCOL* pColEnd = rData.GetPageEndX();
				size_t nColPos;
				for (nColPos=0; nColPos+1<nColBreaks; nColPos++)
				{
					SCCOL nBreak = pColEnd[nColPos]+1;
					if ( nBreak >= nX1 && nBreak <= nX2+1 )
					{
						//! hidden suchen
						if ( pDoc->GetColFlags( nBreak, nTab ) & CR_MANUALBREAK )
							pContentDev->SetFillColor( aManual );
						else
							pContentDev->SetFillColor( aAutomatic );
						Point aBreak = pViewData->GetScrPos(
										nBreak, aRange.aStart.Row(), eWhich, TRUE );
						pContentDev->DrawRect( Rectangle( aBreak.X()-1, aStart.Y(), aBreak.X(), aEnd.Y() ) );
					}
				}

				size_t nRowBreaks = rData.GetPagesY();
				const SCROW* pRowEnd = rData.GetPageEndY();
				size_t nRowPos;
				for (nRowPos=0; nRowPos+1<nRowBreaks; nRowPos++)
				{
					SCROW nBreak = pRowEnd[nRowPos]+1;
					if ( nBreak >= nY1 && nBreak <= nY2+1 )
					{
						//! hidden suchen
						if ( pDoc->GetRowFlags( nBreak, nTab ) & CR_MANUALBREAK )
							pContentDev->SetFillColor( aManual );
						else
							pContentDev->SetFillColor( aAutomatic );
						Point aBreak = pViewData->GetScrPos(
										aRange.aStart.Col(), nBreak, eWhich, TRUE );
						pContentDev->DrawRect( Rectangle( aStart.X(), aBreak.Y()-1, aEnd.X(), aBreak.Y() ) );
					}
				}

				//	Seitenzahlen

				SCROW nPrStartY = aRange.aStart.Row();
				for (nRowPos=0; nRowPos<nRowBreaks; nRowPos++)
				{
					SCROW nPrEndY = pRowEnd[nRowPos];
					if ( nPrEndY >= nY1 && nPrStartY <= nY2 )
					{
						SCCOL nPrStartX = aRange.aStart.Col();
						for (nColPos=0; nColPos<nColBreaks; nColPos++)
						{
							SCCOL nPrEndX = pColEnd[nColPos];
							if ( nPrEndX >= nX1 && nPrStartX <= nX2 )
							{
								Point aPageStart = pViewData->GetScrPos(
														nPrStartX, nPrStartY, eWhich, TRUE );
								Point aPageEnd = pViewData->GetScrPos(
														nPrEndX+1,nPrEndY+1, eWhich, TRUE );

								long nPageNo = rData.GetFirstPage();
								if ( rData.IsTopDown() )
									nPageNo += ((long)nColPos)*nRowBreaks+nRowPos;
								else
									nPageNo += ((long)nRowPos)*nColBreaks+nColPos;
								String aPageStr = aPageText;
								aPageStr += String::CreateFromInt32(nPageNo);

								if ( pEditEng )
								{
									//	find right font size with EditEngine
									long nHeight = 100;
									pEditEng->SetDefaultItem( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT ) );
									pEditEng->SetDefaultItem( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT_CJK ) );
									pEditEng->SetDefaultItem( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT_CTL ) );
									pEditEng->SetText( aPageStr );
									Size aSize100( pEditEng->CalcTextWidth(), pEditEng->GetTextHeight() );

									//	40% of width or 60% of height
									long nSizeX = 40 * ( aPageEnd.X() - aPageStart.X() ) / aSize100.Width();
									long nSizeY = 60 * ( aPageEnd.Y() - aPageStart.Y() ) / aSize100.Height();
									nHeight = Min(nSizeX,nSizeY);
									pEditEng->SetDefaultItem( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT ) );
									pEditEng->SetDefaultItem( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT_CJK ) );
									pEditEng->SetDefaultItem( SvxFontHeightItem( nHeight, 100, EE_CHAR_FONTHEIGHT_CTL ) );

									//	centered output with EditEngine
									Size aTextSize( pEditEng->CalcTextWidth(), pEditEng->GetTextHeight() );
									Point aPos( (aPageStart.X()+aPageEnd.X()-aTextSize.Width())/2,
												(aPageStart.Y()+aPageEnd.Y()-aTextSize.Height())/2 );
									pEditEng->Draw( pContentDev, aPos );
								}
								else
								{
									//	find right font size for DrawText
									aFont.SetSize( Size( 0,100 ) );
									pContentDev->SetFont( aFont );
									Size aSize100( pContentDev->GetTextWidth( aPageStr ), pContentDev->GetTextHeight() );

									//	40% of width or 60% of height
									long nSizeX = 40 * ( aPageEnd.X() - aPageStart.X() ) / aSize100.Width();
									long nSizeY = 60 * ( aPageEnd.Y() - aPageStart.Y() ) / aSize100.Height();
									aFont.SetSize( Size( 0,Min(nSizeX,nSizeY) ) );
									pContentDev->SetFont( aFont );

									//	centered output with DrawText
									Size aTextSize( pContentDev->GetTextWidth( aPageStr ), pContentDev->GetTextHeight() );
									Point aPos( (aPageStart.X()+aPageEnd.X()-aTextSize.Width())/2,
												(aPageStart.Y()+aPageEnd.Y()-aTextSize.Height())/2 );
									pContentDev->DrawText( aPos, aPageStr );
								}
							}
							nPrStartX = nPrEndX + 1;
						}
					}
					nPrStartY = nPrEndY + 1;
				}
			}
		}

		delete pEditEng;
	}
}

void ScGridWindow::DrawButtons( SCCOL nX1, SCROW /*nY1*/, SCCOL nX2, SCROW /*nY2*/, ScTableInfo& rTabInfo, OutputDevice* pContentDev )
{
    aComboButton.SetOutputDevice( pContentDev );

	SCCOL nCol;
	SCROW nRow;
	SCSIZE nArrY;
	SCSIZE nQuery;
	SCTAB			nTab = pViewData->GetTabNo();
	ScDocument*		pDoc = pViewData->GetDocument();
	ScDBData*		pDBData = NULL;
	ScQueryParam*	pQueryParam = NULL;

    RowInfo*        pRowInfo = rTabInfo.mpRowInfo;
    USHORT          nArrCount = rTabInfo.mnArrCount;

	BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );

	Point aOldPos  = aComboButton.GetPosPixel();	// Zustand fuer MouseDown/Up
	Size  aOldSize = aComboButton.GetSizePixel();   // merken

	for (nArrY=1; nArrY+1<nArrCount; nArrY++)
	{
		if ( pRowInfo[nArrY].bAutoFilter && pRowInfo[nArrY].bChanged )
		{
			RowInfo* pThisRowInfo = &pRowInfo[nArrY];

			nRow = pThisRowInfo->nRowNo;


			for (nCol=nX1; nCol<=nX2; nCol++)
			{
				CellInfo* pInfo = &pThisRowInfo->pCellInfo[nCol+1];
				if ( pInfo->bAutoFilter && !pInfo->bHOverlapped && !pInfo->bVOverlapped )
				{
					if (!pQueryParam)
						pQueryParam = new ScQueryParam;

					BOOL bNewData = TRUE;
					if (pDBData)
					{
						SCCOL nStartCol;
						SCROW nStartRow;
						SCCOL nEndCol;
						SCROW nEndRow;
						SCTAB nAreaTab;
						pDBData->GetArea( nAreaTab, nStartCol, nStartRow, nEndCol, nEndRow );
						if ( nCol >= nStartCol && nCol <= nEndCol &&
							 nRow >= nStartRow && nRow <= nEndRow )
							bNewData = FALSE;
					}
					if (bNewData)
					{
						pDBData = pDoc->GetDBAtCursor( nCol, nRow, nTab );
						if (pDBData)
							pDBData->GetQueryParam( *pQueryParam );
						else
						{
							// can also be part of DataPilot table
							// DBG_ERROR("Auto-Filter-Button ohne DBData");
						}
					}

					//	pQueryParam kann nur MAXQUERY Eintraege enthalten

					BOOL bSimpleQuery = TRUE;
					BOOL bColumnFound = FALSE;
					if (!pQueryParam->bInplace)
						bSimpleQuery = FALSE;
					for (nQuery=0; nQuery<MAXQUERY && bSimpleQuery; nQuery++)
						if (pQueryParam->GetEntry(nQuery).bDoQuery)
						{
							//	hier nicht auf EQUAL beschraenken
							//	(auch bei ">1" soll der Spaltenkopf blau werden)

							if (pQueryParam->GetEntry(nQuery).nField == nCol)
								bColumnFound = TRUE;
							if (nQuery > 0)
								if (pQueryParam->GetEntry(nQuery).eConnect != SC_AND)
									bSimpleQuery = FALSE;
						}

                    bool bArrowState = bSimpleQuery && bColumnFound;
					long	nSizeX;
					long	nSizeY;

					pViewData->GetMergeSizePixel( nCol, nRow, nSizeX, nSizeY );
					aComboButton.SetOptSizePixel();
					DrawComboButton( pViewData->GetScrPos( nCol, nRow, eWhich ),
                                     nSizeX, nSizeY, bArrowState );

					aComboButton.SetPosPixel( aOldPos );	// alten Zustand
					aComboButton.SetSizePixel( aOldSize );	// fuer MouseUp/Down
				}
			}
		}

		if ( pRowInfo[nArrY].bPushButton && pRowInfo[nArrY].bChanged )
		{
			RowInfo* pThisRowInfo = &pRowInfo[nArrY];
			nRow = pThisRowInfo->nRowNo;
			for (nCol=nX1; nCol<=nX2; nCol++)
			{
				CellInfo* pInfo = &pThisRowInfo->pCellInfo[nCol+1];
				if ( pInfo->bPushButton && !pInfo->bHOverlapped && !pInfo->bVOverlapped )
				{
					Point aScrPos = pViewData->GetScrPos( nCol, nRow, eWhich );
					long nSizeX;
					long nSizeY;
					pViewData->GetMergeSizePixel( nCol, nRow, nSizeX, nSizeY );
					long nPosX = aScrPos.X();
					long nPosY = aScrPos.Y();
					if ( bLayoutRTL )
					{
						// overwrite the right, not left (visually) grid as long as the
						// left/right colors of the button borders aren't mirrored.
						nPosX -= nSizeX - 2;
					}

                    pContentDev->SetLineColor( GetSettings().GetStyleSettings().GetLightColor() );
					pContentDev->DrawLine( Point(nPosX,nPosY), Point(nPosX,nPosY+nSizeY-1) );
					pContentDev->DrawLine( Point(nPosX,nPosY), Point(nPosX+nSizeX-1,nPosY) );
                    pContentDev->SetLineColor( GetSettings().GetStyleSettings().GetDarkShadowColor() );
					pContentDev->DrawLine( Point(nPosX,nPosY+nSizeY-1), Point(nPosX+nSizeX-1,nPosY+nSizeY-1) );
					pContentDev->DrawLine( Point(nPosX+nSizeX-1,nPosY), Point(nPosX+nSizeX-1,nPosY+nSizeY-1) );
					pContentDev->SetLineColor( COL_BLACK );
				}
			}
		}

        if ( bListValButton && pRowInfo[nArrY].nRowNo == aListValPos.Row() && pRowInfo[nArrY].bChanged )
        {
            Rectangle aRect = GetListValButtonRect( aListValPos );
            aComboButton.SetPosPixel( aRect.TopLeft() );
            aComboButton.SetSizePixel( aRect.GetSize() );
            pContentDev->SetClipRegion( aRect );
            aComboButton.Draw( FALSE, FALSE );
            pContentDev->SetClipRegion();           // always called from Draw() without clip region
            aComboButton.SetPosPixel( aOldPos );    // restore old state
            aComboButton.SetSizePixel( aOldSize );  // for MouseUp/Down (AutoFilter)
        }
	}

	delete pQueryParam;
    aComboButton.SetOutputDevice( this );
}

Rectangle ScGridWindow::GetListValButtonRect( const ScAddress& rButtonPos )
{
    ScDocument* pDoc = pViewData->GetDocument();
    SCTAB nTab = pViewData->GetTabNo();
    BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
    long nLayoutSign = bLayoutRTL ? -1 : 1;

    ScDDComboBoxButton aButton( this );             // for optimal size
    Size aBtnSize = aButton.GetSizePixel();

    SCCOL nCol = rButtonPos.Col();
    SCROW nRow = rButtonPos.Row();

    long nCellSizeX;    // width of this cell, including merged
    long nDummy;
    pViewData->GetMergeSizePixel( nCol, nRow, nCellSizeX, nDummy );

    // for height, only the cell's row is used, excluding merged cells
    long nCellSizeY = ScViewData::ToPixel( pDoc->GetRowHeight( nRow, nTab ), pViewData->GetPPTY() );
    long nAvailable = nCellSizeX;

    //  left edge of next cell if there is a non-hidden next column
    SCCOL nNextCol = nCol + 1;
    const ScMergeAttr* pMerge = static_cast<const ScMergeAttr*>(pDoc->GetAttr( nCol,nRow,nTab, ATTR_MERGE ));
    if ( pMerge->GetColMerge() > 1 )
        nNextCol = nCol + pMerge->GetColMerge();    // next cell after the merged area
    while ( nNextCol <= MAXCOL && (pDoc->GetColFlags( nNextCol, nTab ) & CR_HIDDEN) )
        ++nNextCol;
    BOOL bNextCell = ( nNextCol <= MAXCOL );
    if ( bNextCell )
        nAvailable = ScViewData::ToPixel( pDoc->GetColWidth( nNextCol, nTab ), pViewData->GetPPTX() );

    if ( nAvailable < aBtnSize.Width() )
        aBtnSize.Width() = nAvailable;
    if ( nCellSizeY < aBtnSize.Height() )
        aBtnSize.Height() = nCellSizeY;

    Point aPos = pViewData->GetScrPos( nCol, nRow, eWhich, TRUE );
    aPos.X() += nCellSizeX * nLayoutSign;               // start of next cell
    if (!bNextCell)
        aPos.X() -= aBtnSize.Width() * nLayoutSign;     // right edge of cell if next cell not available
    aPos.Y() += nCellSizeY - aBtnSize.Height();
    // X remains at the left edge

    if ( bLayoutRTL )
        aPos.X() -= aBtnSize.Width()-1;     // align right edge of button with cell border

    return Rectangle( aPos, aBtnSize );
}

BOOL ScGridWindow::IsAutoFilterActive( SCCOL nCol, SCROW nRow, SCTAB nTab )
{
	ScDocument*		pDoc	= pViewData->GetDocument();
	ScDBData*		pDBData = pDoc->GetDBAtCursor( nCol, nRow, nTab );
	ScQueryParam	aQueryParam;

	if ( pDBData )
		pDBData->GetQueryParam( aQueryParam );
	else
	{
		DBG_ERROR("Auto-Filter-Button ohne DBData");
	}

	BOOL	bSimpleQuery = TRUE;
	BOOL	bColumnFound = FALSE;
	SCSIZE	nQuery;

	if ( !aQueryParam.bInplace )
		bSimpleQuery = FALSE;

	//	aQueryParam kann nur MAXQUERY Eintraege enthalten

	for ( nQuery=0; nQuery<MAXQUERY && bSimpleQuery; nQuery++ )
		if ( aQueryParam.GetEntry(nQuery).bDoQuery )
		{
			if (aQueryParam.GetEntry(nQuery).nField == nCol)
				bColumnFound = TRUE;

			if (nQuery > 0)
				if (aQueryParam.GetEntry(nQuery).eConnect != SC_AND)
					bSimpleQuery = FALSE;
		}

	return ( bSimpleQuery && bColumnFound );
}

void ScGridWindow::DrawComboButton( const Point&	rCellPos,
									long			nCellSizeX,
									long			nCellSizeY,
                                    BOOL            bArrowState,
									BOOL			bBtnIn )
{
	Point	aScrPos	 = rCellPos;
	Size	aBtnSize = aComboButton.GetSizePixel();

	if ( nCellSizeX < aBtnSize.Width() || nCellSizeY < aBtnSize.Height() )
	{
		if ( nCellSizeX < aBtnSize.Width() )
			aBtnSize.Width() = nCellSizeX;

		if ( nCellSizeY < aBtnSize.Height() )
			aBtnSize.Height() = nCellSizeY;

		aComboButton.SetSizePixel( aBtnSize );
	}

	BOOL bLayoutRTL = pViewData->GetDocument()->IsLayoutRTL( pViewData->GetTabNo() );

	if ( bLayoutRTL )
		aScrPos.X() -= nCellSizeX - 1;
	else
		aScrPos.X() += nCellSizeX - aBtnSize.Width();
	aScrPos.Y() += nCellSizeY - aBtnSize.Height();

	aComboButton.SetPosPixel( aScrPos );

	HideCursor();
    aComboButton.Draw( bArrowState, bBtnIn );
	ShowCursor();
}

void ScGridWindow::InvertSimple( SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2,
									BOOL bTestMerge, BOOL bRepeat )
{
	//!	if INVERT_HIGHLIGHT swaps foreground and background (like on Mac),
	//!	use INVERT_HIGHLIGHT only for cells that have no background color set
	//!	(here and in ScOutputData::DrawMark)

	PutInOrder( nX1, nX2 );
	PutInOrder( nY1, nY2 );

	ScMarkData& rMark = pViewData->GetMarkData();
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();

	BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
	long nLayoutSign = bLayoutRTL ? -1 : 1;

	SCCOL nTestX2 = nX2;
	SCROW nTestY2 = nY2;
	if (bTestMerge)
		pDoc->ExtendMerge( nX1,nY1, nTestX2,nTestY2, nTab );

	SCCOL nPosX = pViewData->GetPosX( eHWhich );
	SCROW nPosY = pViewData->GetPosY( eVWhich );
	if (nTestX2 < nPosX || nTestY2 < nPosY)
		return;											// unsichtbar
	SCCOL nRealX1 = nX1;
	if (nX1 < nPosX)
		nX1 = nPosX;
	if (nY1 < nPosY)
		nY1 = nPosY;

	SCCOL nXRight = nPosX + pViewData->VisibleCellsX(eHWhich);
	if (nXRight > MAXCOL) nXRight = MAXCOL;
	SCROW nYBottom = nPosY + pViewData->VisibleCellsY(eVWhich);
	if (nYBottom > MAXROW) nYBottom = MAXROW;

	if (nX1 > nXRight || nY1 > nYBottom)
		return;											// unsichtbar
	if (nX2 > nXRight) nX2 = nXRight;
	if (nY2 > nYBottom) nY2 = nYBottom;

	MapMode aOld = GetMapMode(); SetMapMode(MAP_PIXEL);		// erst nach den return's !!!

	double nPPTX = pViewData->GetPPTX();
	double nPPTY = pViewData->GetPPTY();

	ScInvertMerger aInvert( this );

	Point aScrPos = pViewData->GetScrPos( nX1, nY1, eWhich );
	long nScrY = aScrPos.Y();
	BOOL bWasHidden = FALSE;
	for (SCROW nY=nY1; nY<=nY2; nY++)
	{
		BOOL bFirstRow = ( nY == nPosY );						// first visible row?
		BOOL bDoHidden = FALSE;									// versteckte nachholen ?
		USHORT nHeightTwips = pDoc->GetRowHeight( nY,nTab );
		BOOL bDoRow = ( nHeightTwips != 0 );
		if (bDoRow)
		{
			if (bTestMerge)
				if (bWasHidden)					// auf versteckte zusammengefasste testen
				{
//					--nY;						// nY geaendert -> vorherige zeichnen
					bDoHidden = TRUE;
					bDoRow = TRUE;
				}

			bWasHidden = FALSE;
		}
		else
		{
			bWasHidden = TRUE;
			if (bTestMerge)
				if (nY==nY2)
					bDoRow = TRUE;				// letzte Zeile aus Block
		}

		if ( bDoRow )
		{
			SCCOL nLoopEndX = nX2;
			if (nX2 < nX1)						// Rest von zusammengefasst
			{
				SCCOL nStartX = nX1;
				while ( ((const ScMergeFlagAttr*)pDoc->
							GetAttr(nStartX,nY,nTab,ATTR_MERGE_FLAG))->IsHorOverlapped() )
					--nStartX;
				if (nStartX <= nX2)
					nLoopEndX = nX1;
			}

			long nEndY = nScrY + ScViewData::ToPixel( nHeightTwips, nPPTY ) - 1;
			long nScrX = aScrPos.X();
			for (SCCOL nX=nX1; nX<=nLoopEndX; nX++)
			{
				long nWidth = ScViewData::ToPixel( pDoc->GetColWidth( nX,nTab ), nPPTX );
				if ( nWidth > 0 )
				{
					long nEndX = nScrX + ( nWidth - 1 ) * nLayoutSign;
					if (bTestMerge)
					{
						SCROW nThisY = nY;
						const ScPatternAttr* pPattern = pDoc->GetPattern( nX, nY, nTab );
						const ScMergeFlagAttr* pMergeFlag = (const ScMergeFlagAttr*) &pPattern->
																		GetItem(ATTR_MERGE_FLAG);
						if ( pMergeFlag->IsVerOverlapped() && ( bDoHidden || bFirstRow ) )
						{
							while ( pMergeFlag->IsVerOverlapped() && nThisY > 0 &&
										( (pDoc->GetRowFlags( nThisY-1, nTab ) & CR_HIDDEN) || bFirstRow ) )
							{
								--nThisY;
								pPattern = pDoc->GetPattern( nX, nThisY, nTab );
								pMergeFlag = (const ScMergeFlagAttr*) &pPattern->GetItem(ATTR_MERGE_FLAG);
							}
						}

						// nur Rest von zusammengefasster zu sehen ?
						SCCOL nThisX = nX;
						if ( pMergeFlag->IsHorOverlapped() && nX == nPosX && nX > nRealX1 )
						{
							while ( pMergeFlag->IsHorOverlapped() )
							{
								--nThisX;
								pPattern = pDoc->GetPattern( nThisX, nThisY, nTab );
								pMergeFlag = (const ScMergeFlagAttr*) &pPattern->GetItem(ATTR_MERGE_FLAG);
							}
						}

						if ( rMark.IsCellMarked( nThisX, nThisY, TRUE ) == bRepeat )
						{
							if ( !pMergeFlag->IsOverlapped() )
							{
								ScMergeAttr* pMerge = (ScMergeAttr*)&pPattern->GetItem(ATTR_MERGE);
								if (pMerge->GetColMerge() > 0 || pMerge->GetRowMerge() > 0)
								{
									Point aEndPos = pViewData->GetScrPos(
											nThisX + pMerge->GetColMerge(),
											nThisY + pMerge->GetRowMerge(), eWhich );
									if ( aEndPos.X() * nLayoutSign > nScrX * nLayoutSign && aEndPos.Y() > nScrY )
									{
										aInvert.AddRect( Rectangle( nScrX,nScrY,
													aEndPos.X()-nLayoutSign,aEndPos.Y()-1 ) );
									}
								}
								else if ( nEndX * nLayoutSign >= nScrX * nLayoutSign && nEndY >= nScrY )
								{
									aInvert.AddRect( Rectangle( nScrX,nScrY,nEndX,nEndY ) );
								}
							}
						}
					}
					else		// !bTestMerge
					{
						if ( rMark.IsCellMarked( nX, nY, TRUE ) == bRepeat &&
												nEndX * nLayoutSign >= nScrX * nLayoutSign && nEndY >= nScrY )
						{
							aInvert.AddRect( Rectangle( nScrX,nScrY,nEndX,nEndY ) );
						}
					}

					nScrX = nEndX + nLayoutSign;
				}
			}
			nScrY = nEndY + 1;
		}
	}

	aInvert.Flush();		// before restoring MapMode

	SetMapMode(aOld);

	CheckInverted();
}

void ScGridWindow::GetSelectionRects( ::std::vector< Rectangle >& rPixelRects )
{
    // transformed from ScGridWindow::InvertSimple

//  ScMarkData& rMark = pViewData->GetMarkData();
    ScMarkData aMultiMark( pViewData->GetMarkData() );
    aMultiMark.SetMarking( FALSE );
    aMultiMark.MarkToMulti();

	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();

	BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
	long nLayoutSign = bLayoutRTL ? -1 : 1;

    if ( !aMultiMark.IsMultiMarked() )
        return;

    ScRange aMultiRange;
    aMultiMark.GetMultiMarkArea( aMultiRange );
    SCCOL nX1 = aMultiRange.aStart.Col();
    SCROW nY1 = aMultiRange.aStart.Row();
    SCCOL nX2 = aMultiRange.aEnd.Col();
    SCROW nY2 = aMultiRange.aEnd.Row();

	PutInOrder( nX1, nX2 );
	PutInOrder( nY1, nY2 );

    BOOL bTestMerge = TRUE;
    BOOL bRepeat = TRUE;

	SCCOL nTestX2 = nX2;
	SCROW nTestY2 = nY2;
	if (bTestMerge)
		pDoc->ExtendMerge( nX1,nY1, nTestX2,nTestY2, nTab );

	SCCOL nPosX = pViewData->GetPosX( eHWhich );
	SCROW nPosY = pViewData->GetPosY( eVWhich );
	if (nTestX2 < nPosX || nTestY2 < nPosY)
		return;											// unsichtbar
	SCCOL nRealX1 = nX1;
	if (nX1 < nPosX)
		nX1 = nPosX;
	if (nY1 < nPosY)
		nY1 = nPosY;

	SCCOL nXRight = nPosX + pViewData->VisibleCellsX(eHWhich);
	if (nXRight > MAXCOL) nXRight = MAXCOL;
	SCROW nYBottom = nPosY + pViewData->VisibleCellsY(eVWhich);
	if (nYBottom > MAXROW) nYBottom = MAXROW;

	if (nX1 > nXRight || nY1 > nYBottom)
		return;											// unsichtbar
	if (nX2 > nXRight) nX2 = nXRight;
	if (nY2 > nYBottom) nY2 = nYBottom;

//	MapMode aOld = GetMapMode(); SetMapMode(MAP_PIXEL);		// erst nach den return's !!!

	double nPPTX = pViewData->GetPPTX();
	double nPPTY = pViewData->GetPPTY();

    ScInvertMerger aInvert( &rPixelRects );

	Point aScrPos = pViewData->GetScrPos( nX1, nY1, eWhich );
	long nScrY = aScrPos.Y();
	BOOL bWasHidden = FALSE;
	for (SCROW nY=nY1; nY<=nY2; nY++)
	{
		BOOL bFirstRow = ( nY == nPosY );						// first visible row?
		BOOL bDoHidden = FALSE;									// versteckte nachholen ?
		USHORT nHeightTwips = pDoc->GetRowHeight( nY,nTab );
		BOOL bDoRow = ( nHeightTwips != 0 );
		if (bDoRow)
		{
			if (bTestMerge)
				if (bWasHidden)					// auf versteckte zusammengefasste testen
				{
					bDoHidden = TRUE;
					bDoRow = TRUE;
				}

			bWasHidden = FALSE;
		}
		else
		{
			bWasHidden = TRUE;
			if (bTestMerge)
				if (nY==nY2)
					bDoRow = TRUE;				// letzte Zeile aus Block
		}

		if ( bDoRow )
		{
			SCCOL nLoopEndX = nX2;
			if (nX2 < nX1)						// Rest von zusammengefasst
			{
				SCCOL nStartX = nX1;
				while ( ((const ScMergeFlagAttr*)pDoc->
							GetAttr(nStartX,nY,nTab,ATTR_MERGE_FLAG))->IsHorOverlapped() )
					--nStartX;
				if (nStartX <= nX2)
					nLoopEndX = nX1;
			}

			long nEndY = nScrY + ScViewData::ToPixel( nHeightTwips, nPPTY ) - 1;
			long nScrX = aScrPos.X();
			for (SCCOL nX=nX1; nX<=nLoopEndX; nX++)
			{
				long nWidth = ScViewData::ToPixel( pDoc->GetColWidth( nX,nTab ), nPPTX );
				if ( nWidth > 0 )
				{
					long nEndX = nScrX + ( nWidth - 1 ) * nLayoutSign;
					if (bTestMerge)
					{
						SCROW nThisY = nY;
						const ScPatternAttr* pPattern = pDoc->GetPattern( nX, nY, nTab );
						const ScMergeFlagAttr* pMergeFlag = (const ScMergeFlagAttr*) &pPattern->
																		GetItem(ATTR_MERGE_FLAG);
						if ( pMergeFlag->IsVerOverlapped() && ( bDoHidden || bFirstRow ) )
						{
							while ( pMergeFlag->IsVerOverlapped() && nThisY > 0 &&
										( (pDoc->GetRowFlags( nThisY-1, nTab ) & CR_HIDDEN) || bFirstRow ) )
							{
								--nThisY;
								pPattern = pDoc->GetPattern( nX, nThisY, nTab );
								pMergeFlag = (const ScMergeFlagAttr*) &pPattern->GetItem(ATTR_MERGE_FLAG);
							}
						}

						// nur Rest von zusammengefasster zu sehen ?
						SCCOL nThisX = nX;
						if ( pMergeFlag->IsHorOverlapped() && nX == nPosX && nX > nRealX1 )
						{
							while ( pMergeFlag->IsHorOverlapped() )
							{
								--nThisX;
								pPattern = pDoc->GetPattern( nThisX, nThisY, nTab );
								pMergeFlag = (const ScMergeFlagAttr*) &pPattern->GetItem(ATTR_MERGE_FLAG);
							}
						}

						if ( aMultiMark.IsCellMarked( nThisX, nThisY, TRUE ) == bRepeat )
						{
							if ( !pMergeFlag->IsOverlapped() )
							{
								ScMergeAttr* pMerge = (ScMergeAttr*)&pPattern->GetItem(ATTR_MERGE);
								if (pMerge->GetColMerge() > 0 || pMerge->GetRowMerge() > 0)
								{
									Point aEndPos = pViewData->GetScrPos(
											nThisX + pMerge->GetColMerge(),
											nThisY + pMerge->GetRowMerge(), eWhich );
									if ( aEndPos.X() * nLayoutSign > nScrX * nLayoutSign && aEndPos.Y() > nScrY )
									{
										aInvert.AddRect( Rectangle( nScrX,nScrY,
													aEndPos.X()-nLayoutSign,aEndPos.Y()-1 ) );
									}
								}
								else if ( nEndX * nLayoutSign >= nScrX * nLayoutSign && nEndY >= nScrY )
								{
									aInvert.AddRect( Rectangle( nScrX,nScrY,nEndX,nEndY ) );
								}
							}
						}
					}
					else		// !bTestMerge
					{
						if ( aMultiMark.IsCellMarked( nX, nY, TRUE ) == bRepeat &&
												nEndX * nLayoutSign >= nScrX * nLayoutSign && nEndY >= nScrY )
						{
							aInvert.AddRect( Rectangle( nScrX,nScrY,nEndX,nEndY ) );
						}
					}

					nScrX = nEndX + nLayoutSign;
				}
			}
			nScrY = nEndY + 1;
		}
	}

//	aInvert.Flush();		// before restoring MapMode
}

// -------------------------------------------------------------------------

//UNUSED2008-05  void ScGridWindow::DrawDragRect( SCCOL nX1, SCROW nY1, SCCOL nX2, SCROW nY2 )
//UNUSED2008-05  {
//UNUSED2008-05      if ( nX2 < pViewData->GetPosX(eHWhich) || nY2 < pViewData->GetPosY(eVWhich) )
//UNUSED2008-05          return;
//UNUSED2008-05  
//UNUSED2008-05      Update();           // wegen XOR
//UNUSED2008-05  
//UNUSED2008-05      MapMode aOld = GetMapMode(); SetMapMode(MAP_PIXEL);
//UNUSED2008-05  
//UNUSED2008-05      SCTAB nTab = pViewData->GetTabNo();
//UNUSED2008-05  
//UNUSED2008-05      SCCOL nPosX = pViewData->GetPosX(WhichH(eWhich));
//UNUSED2008-05      SCROW nPosY = pViewData->GetPosY(WhichV(eWhich));
//UNUSED2008-05      if (nX1 < nPosX) nX1 = nPosX;
//UNUSED2008-05      if (nX2 < nPosX) nX2 = nPosX;
//UNUSED2008-05      if (nY1 < nPosY) nY1 = nPosY;
//UNUSED2008-05      if (nY2 < nPosY) nY2 = nPosY;
//UNUSED2008-05  
//UNUSED2008-05      Point aScrPos( pViewData->GetScrPos( nX1, nY1, eWhich ) );
//UNUSED2008-05  
//UNUSED2008-05      long nSizeXPix=0;
//UNUSED2008-05      long nSizeYPix=0;
//UNUSED2008-05      ScDocument* pDoc = pViewData->GetDocument();
//UNUSED2008-05      double nPPTX = pViewData->GetPPTX();
//UNUSED2008-05      double nPPTY = pViewData->GetPPTY();
//UNUSED2008-05      SCCOLROW i;
//UNUSED2008-05  
//UNUSED2008-05      BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
//UNUSED2008-05      long nLayoutSign = bLayoutRTL ? -1 : 1;
//UNUSED2008-05  
//UNUSED2008-05      if (ValidCol(nX2) && nX2>=nX1)
//UNUSED2008-05          for (i=nX1; i<=nX2; i++)
//UNUSED2008-05              nSizeXPix += ScViewData::ToPixel( pDoc->GetColWidth( static_cast<SCCOL>(i), nTab ), nPPTX );
//UNUSED2008-05      else
//UNUSED2008-05      {
//UNUSED2008-05          aScrPos.X() -= nLayoutSign;
//UNUSED2008-05          nSizeXPix   += 2;
//UNUSED2008-05      }
//UNUSED2008-05  
//UNUSED2008-05      if (ValidRow(nY2) && nY2>=nY1)
//UNUSED2008-05          for (i=nY1; i<=nY2; i++)
//UNUSED2008-05              nSizeYPix += ScViewData::ToPixel( pDoc->GetRowHeight( i, nTab ), nPPTY );
//UNUSED2008-05      else
//UNUSED2008-05      {
//UNUSED2008-05          aScrPos.Y() -= 1;
//UNUSED2008-05          nSizeYPix   += 2;
//UNUSED2008-05      }
//UNUSED2008-05  
//UNUSED2008-05      aScrPos.X() -= 2 * nLayoutSign;
//UNUSED2008-05      aScrPos.Y() -= 2;
//UNUSED2008-05  //	Rectangle aRect( aScrPos, Size( nSizeXPix + 3, nSizeYPix + 3 ) );
//UNUSED2008-05      Rectangle aRect( aScrPos.X(), aScrPos.Y(),
//UNUSED2008-05                       aScrPos.X() + ( nSizeXPix + 2 ) * nLayoutSign, aScrPos.Y() + nSizeYPix + 2 );
//UNUSED2008-05      if ( bLayoutRTL )
//UNUSED2008-05      {
//UNUSED2008-05          aRect.Left() = aRect.Right();   // end position is left
//UNUSED2008-05          aRect.Right() = aScrPos.X();
//UNUSED2008-05      }
//UNUSED2008-05  
//UNUSED2008-05      Invert(Rectangle( aRect.Left(), aRect.Top(), aRect.Left()+2, aRect.Bottom() ));
//UNUSED2008-05      Invert(Rectangle( aRect.Right()-2, aRect.Top(), aRect.Right(), aRect.Bottom() ));
//UNUSED2008-05      Invert(Rectangle( aRect.Left()+3, aRect.Top(), aRect.Right()-3, aRect.Top()+2 ));
//UNUSED2008-05      Invert(Rectangle( aRect.Left()+3, aRect.Bottom()-2, aRect.Right()-3, aRect.Bottom() ));
//UNUSED2008-05  
//UNUSED2008-05      SetMapMode(aOld);
//UNUSED2008-05  }

// -------------------------------------------------------------------------

void ScGridWindow::DrawCursor()
{
// #114409#
//	SCTAB nTab = pViewData->GetTabNo();
//	SCCOL nX = pViewData->GetCurX();
//	SCROW nY = pViewData->GetCurY();
//
//	//	in verdeckten Zellen nicht zeichnen
//
//	ScDocument* pDoc = pViewData->GetDocument();
//	const ScPatternAttr* pPattern = pDoc->GetPattern(nX,nY,nTab);
//	const ScMergeFlagAttr& rMerge = (const ScMergeFlagAttr&) pPattern->GetItem(ATTR_MERGE_FLAG);
//	if (rMerge.IsOverlapped())
//		return;
//
//	//	links/oben ausserhalb des Bildschirms ?
//
//	BOOL bVis = ( nX>=pViewData->GetPosX(eHWhich) && nY>=pViewData->GetPosY(eVWhich) );
//	if (!bVis)
//	{
//		SCCOL nEndX = nX;
//		SCROW nEndY = nY;
//		ScDocument* pDoc = pViewData->GetDocument();
//		const ScMergeAttr& rMerge = (const ScMergeAttr&) pPattern->GetItem(ATTR_MERGE);
//		if (rMerge.GetColMerge() > 1)
//			nEndX += rMerge.GetColMerge()-1;
//		if (rMerge.GetRowMerge() > 1)
//			nEndY += rMerge.GetRowMerge()-1;
//		bVis = ( nEndX>=pViewData->GetPosX(eHWhich) && nEndY>=pViewData->GetPosY(eVWhich) );
//	}
//
//	if ( bVis )
//	{
//		//	hier kein Update, da aus Paint gerufen und laut Zaehler Cursor schon da
//		//	wenn Update noetig, dann bei Hide/Showcursor vor dem Hoch-/Runterzaehlen
//
//		MapMode aOld = GetMapMode(); SetMapMode(MAP_PIXEL);
//
//		Point aScrPos = pViewData->GetScrPos( nX, nY, eWhich, TRUE );
//		BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
//
//		//	completely right of/below the screen?
//		//	(test with logical start position in aScrPos)
//		BOOL bMaybeVisible;
//		if ( bLayoutRTL )
//			bMaybeVisible = ( aScrPos.X() >= -2 && aScrPos.Y() >= -2 );
//		else
//		{
//			Size aOutSize = GetOutputSizePixel();
//			bMaybeVisible = ( aScrPos.X() <= aOutSize.Width() + 2 && aScrPos.Y() <= aOutSize.Height() + 2 );
//		}
//		if ( bMaybeVisible )
//		{
//			long nSizeXPix;
//			long nSizeYPix;
//			pViewData->GetMergeSizePixel( nX, nY, nSizeXPix, nSizeYPix );
//
//			if ( bLayoutRTL )
//				aScrPos.X() -= nSizeXPix - 2;		// move instead of mirroring
//
//			BOOL bFix = ( pViewData->GetHSplitMode() == SC_SPLIT_FIX ||
//							pViewData->GetVSplitMode() == SC_SPLIT_FIX );
//			if ( pViewData->GetActivePart()==eWhich || bFix )
//			{
//				//	old UNX version with two Invert calls causes flicker.
//				//	if optimization is needed, a new flag should be added
//				//	to InvertTracking
//
//				aScrPos.X() -= 2;
//				aScrPos.Y() -= 2;
//				Rectangle aRect( aScrPos, Size( nSizeXPix + 3, nSizeYPix + 3 ) );
//
//				Invert(Rectangle( aRect.Left(), aRect.Top(), aRect.Left()+2, aRect.Bottom() ));
//				Invert(Rectangle( aRect.Right()-2, aRect.Top(), aRect.Right(), aRect.Bottom() ));
//				Invert(Rectangle( aRect.Left()+3, aRect.Top(), aRect.Right()-3, aRect.Top()+2 ));
//				Invert(Rectangle( aRect.Left()+3, aRect.Bottom()-2, aRect.Right()-3, aRect.Bottom() ));
//			}
//			else
//			{
//				Rectangle aRect( aScrPos, Size( nSizeXPix - 1, nSizeYPix - 1 ) );
//				Invert( aRect );
//			}
//		}
//
//		SetMapMode(aOld);
//	}
}

	//	AutoFill-Anfasser:

void ScGridWindow::DrawAutoFillMark()
{
// #114409#
//	if ( bAutoMarkVisible && aAutoMarkPos.Tab() == pViewData->GetTabNo() )
//	{
//		SCCOL nX = aAutoMarkPos.Col();
//		SCROW nY = aAutoMarkPos.Row();
//		SCTAB nTab = pViewData->GetTabNo();
//		ScDocument* pDoc = pViewData->GetDocument();
//		BOOL bLayoutRTL = pDoc->IsLayoutRTL( nTab );
//
//		Point aFillPos = pViewData->GetScrPos( nX, nY, eWhich, TRUE );
//		long nSizeXPix;
//		long nSizeYPix;
//		pViewData->GetMergeSizePixel( nX, nY, nSizeXPix, nSizeYPix );
//		if ( bLayoutRTL )
//			aFillPos.X() -= nSizeXPix + 3;
//		else
//			aFillPos.X() += nSizeXPix - 2;
//
//		aFillPos.Y() += nSizeYPix;
//		aFillPos.Y() -= 2;
//		Rectangle aFillRect( aFillPos, Size(6,6) );
//		//	Anfasser von Zeichenobjekten sind 7*7
//
//		MapMode aOld = GetMapMode(); SetMapMode(MAP_PIXEL);
//		Invert( aFillRect );
//		SetMapMode(aOld);
//	}
}

// -------------------------------------------------------------------------

void ScGridWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
	Window::DataChanged(rDCEvt);

	if ( (rDCEvt.GetType() == DATACHANGED_PRINTER) ||
		 (rDCEvt.GetType() == DATACHANGED_DISPLAY) ||
		 (rDCEvt.GetType() == DATACHANGED_FONTS) ||
		 (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
		 ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
		  (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
	{
		if ( rDCEvt.GetType() == DATACHANGED_FONTS && eWhich == pViewData->GetActivePart() )
			pViewData->GetDocShell()->UpdateFontList();

		if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
			 (rDCEvt.GetFlags() & SETTINGS_STYLE) )
		{
			if ( eWhich == pViewData->GetActivePart() )		// only once for the view
			{
				ScTabView* pView = pViewData->GetView();

				//	update scale in case the UI ScreenZoom has changed
				ScGlobal::UpdatePPT(this);
				pView->RecalcPPT();

				//	RepeatResize in case scroll bar sizes have changed
				pView->RepeatResize();
                pView->UpdateAllOverlays();

				//	invalidate cell attribs in input handler, in case the
				//	EditEngine BackgroundColor has to be changed
				if ( pViewData->IsActive() )
				{
					ScInputHandler* pHdl = SC_MOD()->GetInputHdl();
					if (pHdl)
						pHdl->ForgetLastPattern();
				}
			}
		}

		Invalidate();
	}
}




