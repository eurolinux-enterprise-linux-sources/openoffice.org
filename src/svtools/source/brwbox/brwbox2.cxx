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
#include "precompiled_svtools.hxx"
#include <tools/debug.hxx>
#include <svtools/brwbox.hxx>
#include "datwin.hxx"
#include <svtools/colorcfg.hxx>
#include <vcl/salgtype.hxx>

#ifndef GCC
#endif
#include <tools/multisel.hxx>
#include <algorithm>

using namespace ::com::sun::star::datatransfer;

#define getDataWindow() ((BrowserDataWin*)pDataWin)


//===================================================================

DBG_NAMEEX(BrowseBox)

//===================================================================

extern const char* BrowseBoxCheckInvariants( const void * pVoid );

DECLARE_LIST( BrowserColumns, BrowserColumn* )

//===================================================================

void BrowseBox::StartDrag( sal_Int8 /* _nAction */, const Point& /* _rPosPixel */ )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	// not interested in this event
}

//===================================================================

sal_Int8 BrowseBox::AcceptDrop( const AcceptDropEvent& _rEvt )
{
	BrowserDataWin* pDataWindow = static_cast<BrowserDataWin*>(pDataWin);
	AcceptDropEvent aTransformed( _rEvt );
	aTransformed.maPosPixel = pDataWindow->ScreenToOutputPixel( OutputToScreenPixel( _rEvt.maPosPixel ) );
	return pDataWindow->AcceptDrop( aTransformed );
}

//===================================================================

sal_Int8 BrowseBox::ExecuteDrop( const ExecuteDropEvent& _rEvt )
{
	BrowserDataWin* pDataWindow = static_cast<BrowserDataWin*>(pDataWin);
	ExecuteDropEvent aTransformed( _rEvt );
	aTransformed.maPosPixel = pDataWindow->ScreenToOutputPixel( OutputToScreenPixel( _rEvt.maPosPixel ) );
	return pDataWindow->ExecuteDrop( aTransformed );
}

//===================================================================

sal_Int8 BrowseBox::AcceptDrop( const BrowserAcceptDropEvent& )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	// not interested in this event
	return DND_ACTION_NONE;
}

//===================================================================

sal_Int8 BrowseBox::ExecuteDrop( const BrowserExecuteDropEvent& )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	// not interested in this event
	return DND_ACTION_NONE;
}

//===================================================================

void* BrowseBox::implGetDataFlavors() const
{
	if (static_cast<BrowserDataWin*>(pDataWin)->bCallingDropCallback)
		return &static_cast<BrowserDataWin*>(pDataWin)->GetDataFlavorExVector();
	return &GetDataFlavorExVector();
}

//===================================================================

sal_Bool BrowseBox::IsDropFormatSupported( SotFormatStringId _nFormat )
{
	if ( static_cast< BrowserDataWin* >( pDataWin )->bCallingDropCallback )
		return static_cast< BrowserDataWin* >( pDataWin )->IsDropFormatSupported( _nFormat );

	return DropTargetHelper::IsDropFormatSupported( _nFormat );
}

//===================================================================

sal_Bool BrowseBox::IsDropFormatSupported( SotFormatStringId _nFormat ) const
{
	return const_cast< BrowseBox* >( this )->IsDropFormatSupported( _nFormat );
}

//===================================================================

sal_Bool BrowseBox::IsDropFormatSupported( const DataFlavor& _rFlavor )
{
	if ( static_cast< BrowserDataWin* >( pDataWin )->bCallingDropCallback )
		return static_cast< BrowserDataWin* >( pDataWin )->IsDropFormatSupported( _rFlavor );

	return DropTargetHelper::IsDropFormatSupported( _rFlavor );
}

//===================================================================

sal_Bool BrowseBox::IsDropFormatSupported( const DataFlavor& _rFlavor ) const
{
	return const_cast< BrowseBox* >( this )->IsDropFormatSupported( _rFlavor );
}

//===================================================================

void BrowseBox::Command( const CommandEvent& rEvt )
{
	if ( !getDataWindow()->bInCommand )
		Control::Command( rEvt );
}

//===================================================================

bool BrowseBox::IsInCommandEvent() const
{
	return getDataWindow()->bInCommand;
}

//===================================================================

void BrowseBox::StateChanged( StateChangedType nStateChange )
{
    Control::StateChanged( nStateChange );

    if ( STATE_CHANGE_MIRRORING == nStateChange )
    {
        getDataWindow()->EnableRTL( IsRTLEnabled() );

        HeaderBar* pHeaderBar = getDataWindow()->pHeaderBar;
        if ( pHeaderBar )
            pHeaderBar->EnableRTL( IsRTLEnabled() );
        aHScroll.EnableRTL( IsRTLEnabled() );
        if( pVScroll )
            pVScroll->EnableRTL( IsRTLEnabled() );
        Resize();
    }
	else if ( STATE_CHANGE_INITSHOW == nStateChange )
	{
		bBootstrapped = TRUE; // muss zuerst gesetzt werden!

		Resize();
		if ( bMultiSelection )
			uRow.pSel->SetTotalRange( Range( 0, nRowCount - 1 ) );
		if ( nRowCount == 0 )
			nCurRow = BROWSER_ENDOFSELECTION;
		else if ( nCurRow == BROWSER_ENDOFSELECTION )
			nCurRow = 0;


		if ( HasFocus() )
		{
			bSelectionIsVisible = TRUE;
			bHasFocus = TRUE;
		}
		UpdateScrollbars();
		AutoSizeLastColumn();
		CursorMoved();
	}
	else if (STATE_CHANGE_ZOOM == nStateChange)
	{
		pDataWin->SetZoom(GetZoom());
		HeaderBar* pHeaderBar = getDataWindow()->pHeaderBar;
		if (pHeaderBar)
			pHeaderBar->SetZoom(GetZoom());

		// let the cols calc their new widths and adjust the header bar
		for ( USHORT nPos = 0; nPos < pCols->Count(); ++nPos )
		{
			pCols->GetObject(nPos)->ZoomChanged(GetZoom());
			if ( pHeaderBar )
				pHeaderBar->SetItemSize( pCols->GetObject(nPos)->GetId(), pCols->GetObject(nPos)->Width() );
		}

		// all our controls have to be repositioned
		Resize();
	}
	else if (STATE_CHANGE_ENABLE == nStateChange)
	{
		// do we have a handle column?
		sal_Bool bHandleCol	= pCols->Count() && (0 == pCols->GetObject(0)->GetId());
		// do we have a header bar
		sal_Bool bHeaderBar = (NULL != static_cast<BrowserDataWin&>(GetDataWindow()).pHeaderBar);

		if	(	nTitleLines
			&&	(	!bHeaderBar
				||	bHandleCol
				)
			)
			// we draw the text in our header bar in a color dependent on the enabled state. So if this state changed
			// -> redraw
			Invalidate(Rectangle(Point(0, 0), Size(GetOutputSizePixel().Width(), GetTitleHeight() - 1)));
	}
}

//===================================================================

void BrowseBox::Select()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
}

//-------------------------------------------------------------------

void BrowseBox::DoubleClick( const BrowserMouseEvent & )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
}

//-------------------------------------------------------------------

long BrowseBox::QueryMinimumRowHeight()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
    return CalcZoom( 5 );
}

//-------------------------------------------------------------------

void BrowseBox::ImplStartTracking()
{
	DBG_CHKTHIS( BrowseBox, BrowseBoxCheckInvariants );
}

//-------------------------------------------------------------------

void BrowseBox::ImplTracking()
{
	DBG_CHKTHIS( BrowseBox, BrowseBoxCheckInvariants );
}

//-------------------------------------------------------------------

void BrowseBox::ImplEndTracking()
{
	DBG_CHKTHIS( BrowseBox, BrowseBoxCheckInvariants );
}

//-------------------------------------------------------------------

void BrowseBox::RowHeightChanged()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
}

//-------------------------------------------------------------------

long BrowseBox::QueryColumnResize( USHORT, long nWidth )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	return nWidth;
}

//-------------------------------------------------------------------

void BrowseBox::ColumnResized( USHORT )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
}

//-------------------------------------------------------------------

void BrowseBox::ColumnMoved( USHORT )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
}

//-------------------------------------------------------------------

void BrowseBox::StartScroll()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	//((Control*)pDataWin)->HideFocus();
	DoHideCursor( "StartScroll" );
}

//-------------------------------------------------------------------

void BrowseBox::EndScroll()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	UpdateScrollbars();
	AutoSizeLastColumn();
	DoShowCursor( "EndScroll" );
}

//-------------------------------------------------------------------

#ifdef _MSC_VER
#pragma optimize( "", off )
#endif

void BrowseBox::ToggleSelection( BOOL bForce )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	// selection highlight-toggling allowed?
	if ( bHideSelect )
		return;
	if ( !bForce &&
		 ( bNotToggleSel || !IsUpdateMode() || !bSelectionIsVisible ) )
		return;
//MI, 28.01.98
//	if ( !getDataWindow()->bHighlightToggle &&
//		 !getDataWindow()->bHighlightAuto )
//		return;

	// only highlight painted areas!
	bNotToggleSel = TRUE;
	if ( FALSE && !getDataWindow()->bInPaint )
		pDataWin->Update();

	// accumulate areas of rows to highlight
	RectangleList aHighlightList;
	long nLastRowInRect = 0; // fuer den CFront

	// Handle-Column nicht highlighten
	BrowserColumn *pFirstCol = pCols->GetObject(0);
	long nOfsX = (!pFirstCol || pFirstCol->GetId()) ? 0 : pFirstCol->Width();

	// accumulate old row selection
	long nBottomRow = nTopRow +
		pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight();
	if ( nBottomRow > GetRowCount() && GetRowCount() )
		nBottomRow = GetRowCount();
	for ( long nRow = bMultiSelection ? uRow.pSel->FirstSelected() : uRow.nSel;
		  nRow != BROWSER_ENDOFSELECTION && nRow <= nBottomRow;
		  nRow = bMultiSelection ? uRow.pSel->NextSelected() : BROWSER_ENDOFSELECTION )
	{
		if ( nRow < nTopRow )
			continue;

		Rectangle aAddRect(
			Point( nOfsX, (nRow-nTopRow)*GetDataRowHeight() ),
			Size( pDataWin->GetSizePixel().Width(), GetDataRowHeight() ) );
		if ( aHighlightList.Count() && nLastRowInRect == ( nRow - 1 ) )
			aHighlightList.First()->Union( aAddRect );
		else
			aHighlightList.Insert( new Rectangle( aAddRect ), (ULONG) 0 );
		nLastRowInRect = nRow;
	}

	// unhighlight the old selection (if any)
	while ( aHighlightList.Count() )
	{
		Rectangle *pRect = aHighlightList.Remove( aHighlightList.Count() - 1 );
		pDataWin->Invalidate( *pRect );
		delete pRect;
	}

	// unhighlight old column selection (if any)
	for ( long nColId = pColSel ? pColSel->FirstSelected() : BROWSER_ENDOFSELECTION;
		  nColId != BROWSER_ENDOFSELECTION;
		  nColId = pColSel->NextSelected() )
	{
		Rectangle aRect( GetFieldRectPixel(nCurRow,
										   pCols->GetObject(nColId)->GetId(),
										   FALSE ) );
		aRect.Left() -= MIN_COLUMNWIDTH;
		aRect.Right() += MIN_COLUMNWIDTH;
		aRect.Top() = 0;
		aRect.Bottom() = pDataWin->GetOutputSizePixel().Height();
		pDataWin->Invalidate( aRect );
	}

	bNotToggleSel = FALSE;
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

//-------------------------------------------------------------------

void BrowseBox::DrawCursor()
{
	BOOL bReallyHide = FALSE;
	if ( SMART_CURSOR_HIDE == bHideCursor )
	{
		if ( !GetSelectRowCount() && !GetSelectColumnCount() )
			bReallyHide = TRUE;
	}
	else if ( HARD_CURSOR_HIDE == bHideCursor )
	{
		bReallyHide = TRUE;
	}

	bReallyHide |= !bSelectionIsVisible || !IsUpdateMode() || bScrolling || nCurRow < 0;

	if (PaintCursorIfHiddenOnce())
		bReallyHide |= ( GetCursorHideCount() > 1 );
	else
		bReallyHide |= ( GetCursorHideCount() > 0 );

	// keine Cursor auf Handle-Column
	if ( nCurColId == 0 )
		nCurColId = GetColumnId(1);

	// Cursor-Rechteck berechnen
	Rectangle aCursor;
	if ( bColumnCursor )
	{
		aCursor = GetFieldRectPixel( nCurRow, nCurColId, FALSE );
		//! --aCursor.Bottom();
		aCursor.Left() -= MIN_COLUMNWIDTH;
		aCursor.Right() += 1;
		aCursor.Bottom() += 1;
	}
	else
		aCursor = Rectangle(
			Point( ( pCols->Count() && pCols->GetObject(0)->GetId() == 0 ) ?
						pCols->GetObject(0)->Width() : 0,
						(nCurRow - nTopRow) * GetDataRowHeight() + 1 ),
			Size( pDataWin->GetOutputSizePixel().Width() + 1,
				  GetDataRowHeight() - 2 ) );
	if ( bHLines )
	{
		if ( !bMultiSelection )
			--aCursor.Top();
		--aCursor.Bottom();
	}

	//!mi_mac pDataWin->Update();

	if (m_aCursorColor == COL_TRANSPARENT)
	{
		// auf diesem Plattformen funktioniert der StarView-Focus richtig
		if ( bReallyHide )
			((Control*)pDataWin)->HideFocus();
		else
			((Control*)pDataWin)->ShowFocus( aCursor );
	}
	else
	{
		Color rCol = bReallyHide ? pDataWin->GetFillColor() : m_aCursorColor;
		Color aOldFillColor = pDataWin->GetFillColor();
		Color aOldLineColor = pDataWin->GetLineColor();
		pDataWin->SetFillColor();
		pDataWin->SetLineColor( rCol );
		pDataWin->DrawRect( aCursor );
		pDataWin->SetLineColor( aOldLineColor );
		pDataWin->SetFillColor( aOldFillColor );
	}
}

//-------------------------------------------------------------------

ULONG BrowseBox::GetColumnWidth( USHORT nId ) const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	USHORT nItemPos = GetColumnPos( nId );
	if ( nItemPos >= pCols->Count() )
		return 0;
	return pCols->GetObject(nItemPos)->Width();
}

//-------------------------------------------------------------------

USHORT BrowseBox::GetColumnId( USHORT nPos ) const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	if ( nPos >= pCols->Count() )
		return 0;
	return pCols->GetObject(nPos)->GetId();
}

//-------------------------------------------------------------------

USHORT BrowseBox::GetColumnPos( USHORT nId ) const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	for ( USHORT nPos = 0; nPos < pCols->Count(); ++nPos )
		if ( pCols->GetObject(nPos)->GetId() == nId )
			return nPos;
	return BROWSER_INVALIDID;
}

//-------------------------------------------------------------------

BOOL BrowseBox::IsFrozen( USHORT nColumnId ) const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	for ( USHORT nPos = 0; nPos < pCols->Count(); ++nPos )
		if ( pCols->GetObject(nPos)->GetId() == nColumnId )
			return pCols->GetObject(nPos)->IsFrozen();
	return FALSE;
}

//-------------------------------------------------------------------

void BrowseBox::ExpandRowSelection( const BrowserMouseEvent& rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	DoHideCursor( "ExpandRowSelection" );

	// expand the last selection
	if ( bMultiSelection )
	{
		Range aJustifiedRange( aSelRange );
		aJustifiedRange.Justify();

        BOOL bSelectThis = ( bSelect != aJustifiedRange.IsInside( rEvt.GetRow() ) );

		if ( aJustifiedRange.IsInside( rEvt.GetRow() ) )
		{
			// down and up
			while ( rEvt.GetRow() < aSelRange.Max() )
			{   // ZTC/Mac bug - dont put these statemants together!
				SelectRow( aSelRange.Max(), bSelectThis, TRUE );
				--aSelRange.Max();
			}
			while ( rEvt.GetRow() > aSelRange.Max() )
			{   // ZTC/Mac bug - dont put these statemants together!
				SelectRow( aSelRange.Max(), bSelectThis, TRUE );
				++aSelRange.Max();
			}
		}
		else
		{
			// up and down
			BOOL bOldSelecting = bSelecting;
			bSelecting = TRUE;
			while ( rEvt.GetRow() < aSelRange.Max() )
			{   // ZTC/Mac bug - dont put these statemants together!
				--aSelRange.Max();
				if ( !IsRowSelected( aSelRange.Max() ) )
				{
					SelectRow( aSelRange.Max(), bSelectThis, TRUE );
					bSelect = TRUE;
				}
			}
			while ( rEvt.GetRow() > aSelRange.Max() )
			{   // ZTC/Mac bug - dont put these statemants together!
				++aSelRange.Max();
				if ( !IsRowSelected( aSelRange.Max() ) )
				{
					SelectRow( aSelRange.Max(), bSelectThis, TRUE );
					bSelect = TRUE;
				}
			}
			bSelecting = bOldSelecting;
			if ( bSelect )
				Select();
		}
	}
	else
		if ( !bMultiSelection || !IsRowSelected( rEvt.GetRow() ) )
			SelectRow( rEvt.GetRow(), TRUE );

	GoToRow( rEvt.GetRow(), FALSE );
	DoShowCursor( "ExpandRowSelection" );
}

//-------------------------------------------------------------------

void BrowseBox::Resize()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	if ( !bBootstrapped && IsReallyVisible() )
		BrowseBox::StateChanged( STATE_CHANGE_INITSHOW );
	if ( !pCols->Count() )
	{
		getDataWindow()->bResizeOnPaint = TRUE;
		return;
	}
	getDataWindow()->bResizeOnPaint = FALSE;

	// calc the size of the scrollbars
	// (we can't ask the scrollbars for their widths cause if we're zoomed they still have to be
	// resized - which is done in UpdateScrollbars)
	ULONG nSBSize = GetSettings().GetStyleSettings().GetScrollBarSize();
	if (IsZoom())
		nSBSize = (ULONG)(nSBSize * (double)GetZoom());

	long nSize = pDataWin->GetPosPixel().Y();
	if( !getDataWindow()->bNoHScroll )
		nSize += aHScroll.GetSizePixel().Height();

    if ( GetOutputSizePixel().Height() < nSize )
        return;

	DoHideCursor( "Resize" );
	USHORT nOldVisibleRows =
		(USHORT)(pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight() + 1);

	// did we need a horiz. scroll bar oder gibt es eine Control Area?
	if ( !getDataWindow()->bNoHScroll &&
		 ( ( pCols->Count() - FrozenColCount() ) > 1 ) )
		aHScroll.Show();
	else
		aHScroll.Hide();

	// calculate the size of the data window
	long nDataHeight = GetOutputSizePixel().Height() - GetTitleHeight();
	if ( aHScroll.IsVisible() || ( nControlAreaWidth != USHRT_MAX ) )
		nDataHeight -= nSBSize;

	long nDataWidth = GetOutputSizePixel().Width();
	if ( pVScroll->IsVisible() )
		nDataWidth -= nSBSize;

	// adjust position and size of data window
	pDataWin->SetPosSizePixel(
		Point( 0, GetTitleHeight() ),
		Size( nDataWidth, nDataHeight ) );

	USHORT nVisibleRows =
		(USHORT)(pDataWin->GetOutputSizePixel().Height() / GetDataRowHeight() + 1);

	// TopRow ist unveraendert, aber die Anzahl sichtbarer Zeilen hat sich
	// geaendert
	if ( nVisibleRows != nOldVisibleRows )
		VisibleRowsChanged(nTopRow, nVisibleRows);

	UpdateScrollbars();

	// Control-Area
	Rectangle aInvalidArea( GetControlArea() );
	aInvalidArea.Right() = GetOutputSizePixel().Width();
	aInvalidArea.Left() = 0;
	Invalidate( aInvalidArea );

	// external header-bar
	HeaderBar* pHeaderBar = getDataWindow()->pHeaderBar;
	if ( pHeaderBar )
	{
		// Handle-Column beruecksichtigen
		BrowserColumn *pFirstCol = pCols->GetObject(0);
		long nOfsX = pFirstCol->GetId() ? 0 : pFirstCol->Width();
		pHeaderBar->SetPosSizePixel( Point( nOfsX, 0 ), Size( GetOutputSizePixel().Width() - nOfsX, GetTitleHeight() ) );
	}

	AutoSizeLastColumn(); // adjust last column width
	DoShowCursor( "Resize" );
}

//-------------------------------------------------------------------

void BrowseBox::Paint( const Rectangle& rRect )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	// initializations
	if ( !bBootstrapped && IsReallyVisible() )
		BrowseBox::StateChanged( STATE_CHANGE_INITSHOW );
	if ( !pCols->Count() )
		return;

	BrowserColumn *pFirstCol = pCols->GetObject(0);
	BOOL bHandleCol	= pFirstCol && pFirstCol->GetId() == 0;
	BOOL bHeaderBar = getDataWindow()->pHeaderBar != NULL;

	// draw delimitational lines
	if ( !getDataWindow()->bNoHScroll )
		DrawLine( Point( 0, aHScroll.GetPosPixel().Y() ),
				  Point( GetOutputSizePixel().Width(),
						 aHScroll.GetPosPixel().Y() ) );

	if ( nTitleLines )
	{
		if ( !bHeaderBar )
			DrawLine( Point( 0, GetTitleHeight() - 1 ),
					  Point( GetOutputSizePixel().Width(),
							 GetTitleHeight() - 1 ) );
		else if ( bHandleCol )
			DrawLine( Point( 0, GetTitleHeight() - 1 ),
					  Point( pFirstCol->Width(), GetTitleHeight() - 1 ) );
	}

	// Title Bar
	// Wenn es eine Handle Column gibt und die Headerbar verfuegbar ist, dann nur
	// die HandleColumn
	// Handle-Column beruecksichtigen
	if ( nTitleLines && (!bHeaderBar || bHandleCol) )
	{
		// iterate through columns to redraw
		long nX = 0;
		USHORT nCol;
		for ( nCol = 0;
			  nCol < pCols->Count() && nX < rRect.Right();
			  ++nCol )
		{
			// skip invisible colums between frozen and scrollable area
			if ( nCol < nFirstCol && !pCols->GetObject(nCol)->IsFrozen() )
				nCol = nFirstCol;

			// nur die HandleCol ?
			if (bHeaderBar && bHandleCol && nCol > 0)
				break;

			BrowserColumn *pCol = pCols->GetObject(nCol);

			// draw the column and increment position
			if ( pCol->Width() > 4 )
			{
				ButtonFrame aButtonFrame( Point( nX, 0 ),
					Size( pCol->Width()-1, GetTitleHeight()-1 ),
					pCol->Title(), FALSE, FALSE,
					0 != (BROWSER_COLUMN_TITLEABBREVATION&pCol->Flags()),
					!IsEnabled());
				aButtonFrame.Draw( *this );
				DrawLine( Point( nX + pCol->Width() - 1, 0 ),
				   Point( nX + pCol->Width() - 1, GetTitleHeight()-1 ) );
			}
			else
			{
				Color aOldFillColor = GetFillColor();
				SetFillColor( Color( COL_BLACK ) );
				DrawRect( Rectangle( Point( nX, 0 ), Size( pCol->Width(), GetTitleHeight() - 1 ) ) );
				SetFillColor( aOldFillColor );
			}

			// skip column
			nX += pCol->Width();
		}

		// retouching
		if ( !bHeaderBar && nCol == pCols->Count() )
		{
			const StyleSettings &rSettings = GetSettings().GetStyleSettings();
			Color aColFace( rSettings.GetFaceColor() );
			Color aOldFillColor = GetFillColor();
			Color aOldLineColor = GetLineColor();
			SetFillColor( aColFace );
			SetLineColor( aColFace );
			DrawRect( Rectangle(
				Point( nX, 0 ),
				Point( rRect.Right(), GetTitleHeight() - 2 ) ) );
			SetFillColor( aOldFillColor); // aOldLineColor );  oj 09.02.00 seems to be a copy&paste bug
			SetLineColor( aOldLineColor); // aOldFillColor );
		}
	}
}

//-------------------------------------------------------------------

void BrowseBox::PaintRow( OutputDevice&, const Rectangle& )
{
}

//-------------------------------------------------------------------

void BrowseBox::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, ULONG nFlags )
{
	BOOL bDrawSelection = (nFlags & WINDOW_DRAW_NOSELECTION) == 0;

	// we need pixel coordinates
	Size aRealSize = pDev->LogicToPixel(rSize);
	Point aRealPos = pDev->LogicToPixel(rPos);

	if ((rSize.Width() < 3) || (rSize.Height() < 3))
		// we want to have two pixels frame ...
		return;

	Font aFont = GetDataWindow().GetDrawPixelFont( pDev );
		// the 'normal' painting uses always the data window as device to output to, so we have to calc the new font
		// relative to the data wins current settings

	pDev->Push();
	pDev->SetMapMode();
	pDev->SetFont( aFont );

	// draw a frame
	const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
	pDev->SetLineColor(rStyleSettings.GetDarkShadowColor());
	pDev->DrawLine(Point(aRealPos.X(), aRealPos.Y()),
				   Point(aRealPos.X(), aRealPos.Y() + aRealSize.Height() - 1));
	pDev->DrawLine(Point(aRealPos.X(), aRealPos.Y()),
				   Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y()));
	pDev->SetLineColor(rStyleSettings.GetShadowColor());
	pDev->DrawLine(Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y() + 1),
				   Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y() + aRealSize.Height() - 1));
	pDev->DrawLine(Point(aRealPos.X() + aRealSize.Width() - 1, aRealPos.Y() + aRealSize.Height() - 1),
				   Point(aRealPos.X() + 1, aRealPos.Y() + aRealSize.Height() - 1));

	HeaderBar* pBar = getDataWindow()->pHeaderBar;

	// we're drawing onto a foreign device, so we have to fake the DataRowHeight for the subsequent ImplPaintData
	// (as it is based on the settings of our data window, not the foreign device)
	if (!nDataRowHeight)
		ImpGetDataRowHeight();
	long nHeightLogic = PixelToLogic(Size(0, nDataRowHeight), MAP_10TH_MM).Height();
	long nForeignHeightPixel = pDev->LogicToPixel(Size(0, nHeightLogic), MAP_10TH_MM).Height();

	long nOriginalHeight = nDataRowHeight;
	nDataRowHeight = nForeignHeightPixel;

	// this counts for the column widths, too
	USHORT nPos;
	for ( nPos = 0; nPos < pCols->Count(); ++nPos )
	{
		BrowserColumn* pCurrent = pCols->GetObject(nPos);

		long nWidthLogic = PixelToLogic(Size(pCurrent->Width(), 0), MAP_10TH_MM).Width();
		long nForeignWidthPixel = pDev->LogicToPixel(Size(nWidthLogic, 0), MAP_10TH_MM).Width();

		pCurrent->SetWidth(nForeignWidthPixel, GetZoom());
		if ( pBar )
			pBar->SetItemSize( pCurrent->GetId(), pCurrent->Width() );
	}

	// a smaller area for the content
	++aRealPos.X();
	++aRealPos.Y();
	aRealSize.Width() -= 2;
	aRealSize.Height() -= 2;

	// let the header bar draw itself
	if ( pBar )
	{
		// the title height with respect to the font set for the given device
		long nTitleHeight = PixelToLogic(Size(0, GetTitleHeight()), MAP_10TH_MM).Height();
		nTitleHeight = pDev->LogicToPixel(Size(0, nTitleHeight), MAP_10TH_MM).Height();

		BrowserColumn* pFirstCol = pCols->Count() ? pCols->GetObject(0) : NULL;

		Point aHeaderPos(pFirstCol && (pFirstCol->GetId() == 0) ? pFirstCol->Width() : 0, 0);
		Size aHeaderSize(aRealSize.Width() - aHeaderPos.X(), nTitleHeight);

		aHeaderPos += aRealPos;
			// do this before converting to logics !

		// the header's draw expects logic coordinates, again
		aHeaderPos = pDev->PixelToLogic(aHeaderPos);
		aHeaderSize = pDev->PixelToLogic(aHeaderSize);

		pBar->Draw(pDev, aHeaderPos, aHeaderSize, nFlags);

		// draw the "upper left cell" (the intersection between the header bar and the handle column)
		if (( pFirstCol->GetId() == 0 ) && ( pFirstCol->Width() > 4 ))
		{
			ButtonFrame aButtonFrame( aRealPos,
				Size( pFirstCol->Width()-1, nTitleHeight-1 ),
				pFirstCol->Title(), FALSE, FALSE, FALSE, !IsEnabled());
			aButtonFrame.Draw( *pDev );

            pDev->Push( PUSH_LINECOLOR );
			pDev->SetLineColor( Color( COL_BLACK ) );

			pDev->DrawLine( Point( aRealPos.X(), aRealPos.Y() + nTitleHeight-1 ),
			   Point( aRealPos.X() + pFirstCol->Width() - 1, aRealPos.Y() + nTitleHeight-1 ) );
			pDev->DrawLine( Point( aRealPos.X() + pFirstCol->Width() - 1, aRealPos.Y() ),
			   Point( aRealPos.X() + pFirstCol->Width() - 1, aRealPos.Y() + nTitleHeight-1 ) );

            pDev->Pop();
		}

		aRealPos.Y() += aHeaderSize.Height();
		aRealSize.Height() -= aHeaderSize.Height();
	}

	// draw our own content (with clipping)
	Region aRegion(Rectangle(aRealPos, aRealSize));
	pDev->SetClipRegion( pDev->PixelToLogic( aRegion ) );

    // do we have to paint the background
    BOOL bBackground = !(nFlags & WINDOW_DRAW_NOBACKGROUND) && GetDataWindow().IsControlBackground();
    if ( bBackground )
    {
        Rectangle aRect( aRealPos, aRealSize );
        pDev->SetFillColor( GetDataWindow().GetControlBackground() );
        pDev->DrawRect( aRect );
    }

	ImplPaintData( *pDev, Rectangle( aRealPos, aRealSize ), TRUE, bDrawSelection );

	// restore the column widths/data row height
	nDataRowHeight = nOriginalHeight;
	for ( nPos = 0; nPos < pCols->Count(); ++nPos )
	{
		BrowserColumn* pCurrent = pCols->GetObject(nPos);

		long nForeignWidthLogic = pDev->PixelToLogic(Size(pCurrent->Width(), 0), MAP_10TH_MM).Width();
		long nWidthPixel = LogicToPixel(Size(nForeignWidthLogic, 0), MAP_10TH_MM).Width();

		pCurrent->SetWidth(nWidthPixel, GetZoom());
		if ( pBar )
			pBar->SetItemSize( pCurrent->GetId(), pCurrent->Width() );
	}

	pDev->Pop();
}

//-------------------------------------------------------------------

void BrowseBox::ImplPaintData(OutputDevice& _rOut, const Rectangle& _rRect, BOOL _bForeignDevice, BOOL _bDrawSelections)
{
	Point aOverallAreaPos = _bForeignDevice ? _rRect.TopLeft() : Point(0,0);
	Size aOverallAreaSize = _bForeignDevice ? _rRect.GetSize() : GetDataWindow().GetOutputSizePixel();
	Point aOverallAreaBRPos = _bForeignDevice ? _rRect.BottomRight() : Point( aOverallAreaSize.Width(), aOverallAreaSize.Height() );

	long nDataRowHeigt = GetDataRowHeight();

	// compute relative rows to redraw
	ULONG nRelTopRow = _bForeignDevice ? 0 : ((ULONG)_rRect.Top() / nDataRowHeigt);
	ULONG nRelBottomRow = (ULONG)(_bForeignDevice ? aOverallAreaSize.Height() : _rRect.Bottom()) / nDataRowHeigt;

	// cache frequently used values
	Point aPos( aOverallAreaPos.X(), nRelTopRow * nDataRowHeigt + aOverallAreaPos.Y() );
	_rOut.SetLineColor( Color( COL_WHITE ) );
	const AllSettings& rAllSets = _rOut.GetSettings();
	const StyleSettings &rSettings = rAllSets.GetStyleSettings();
	const Color &rHighlightTextColor = rSettings.GetHighlightTextColor();
	const Color &rHighlightFillColor = rSettings.GetHighlightColor();
	Color aOldTextColor = _rOut.GetTextColor();
	Color aOldFillColor = _rOut.GetFillColor();
	Color aOldLineColor = _rOut.GetLineColor();
	long nHLineX = 0 == pCols->GetObject(0)->GetId()
					? pCols->GetObject(0)->Width()
					: 0;
	nHLineX += aOverallAreaPos.X();

    Color aDelimiterLineColor( ::svtools::ColorConfig().GetColorValue( ::svtools::CALCGRID ).nColor );

    // redraw the invalid fields
	BOOL bRetouching = FALSE;
	for ( ULONG nRelRow = nRelTopRow;
		  nRelRow <= nRelBottomRow && (ULONG)nTopRow+nRelRow < (ULONG)nRowCount;
		  ++nRelRow, aPos.Y() += nDataRowHeigt )
	{
		// get row
		// Zur Sicherheit auf zul"assigen Bereich abfragen:
		DBG_ASSERT( (USHORT)(nTopRow+nRelRow) < nRowCount, "BrowseBox::ImplPaintData: invalid seek" );
		if ( (nTopRow+long(nRelRow)) < 0 || (USHORT)(nTopRow+nRelRow) >= nRowCount )
			continue;

		// prepare row
		ULONG nRow = nTopRow+nRelRow;
		if ( !SeekRow( nRow) ) {
			DBG_ERROR("BrowseBox::ImplPaintData: SeekRow gescheitert");
        }
		_rOut.SetClipRegion();
		aPos.X() = aOverallAreaPos.X();


		// #73325# don't paint the row outside the painting rectangle (DG)
		// prepare auto-highlight
		Rectangle aRowRect( Point( _rRect.TopLeft().X(), aPos.Y() ),
				Size( _rRect.GetSize().Width(), nDataRowHeigt ) );
		PaintRow( _rOut, aRowRect );

		BOOL bRowAutoHighlight	=	_bDrawSelections
								&&	!bHideSelect
								&&	((BrowserDataWin&)GetDataWindow()).bHighlightAuto
								&&	IsRowSelected( nRow );
		if ( bRowAutoHighlight )
		{
			_rOut.SetTextColor( rHighlightTextColor );
			_rOut.SetFillColor( rHighlightFillColor );
			_rOut.SetLineColor();
			_rOut.DrawRect( aRowRect );
		}

		// iterate through columns to redraw
		USHORT nCol;
		for ( nCol = 0; nCol < pCols->Count(); ++nCol )
		{
			// get column
			BrowserColumn *pCol = pCols->GetObject(nCol);

			// at end of invalid area
			if ( aPos.X() >= _rRect.Right() )
				break;

			// skip invisible colums between frozen and scrollable area
			if ( nCol < nFirstCol && !pCol->IsFrozen() )
			{
				nCol = nFirstCol;
				pCol = pCols->GetObject(nCol);
				if (!pCol)
				{	// FS - 21.05.99 - 66325
					// ist zwar eigentlich woanders (an der richtigen Stelle) gefixt, aber sicher ist sicher ...
					DBG_ERROR("BrowseBox::PaintData : nFirstCol is probably invalid !");
					break;
				}
			}

			// prepare Column-AutoHighlight
			BOOL bColAutoHighlight	=	_bDrawSelections
									&&	bColumnCursor
									&&	IsColumnSelected( pCol->GetId() );
			if ( bColAutoHighlight )
			{
				_rOut.SetClipRegion();
				_rOut.SetTextColor( rHighlightTextColor );
				_rOut.SetFillColor( rHighlightFillColor );
				_rOut.SetLineColor();
				Rectangle aFieldRect( aPos,
						Size( pCol->Width(), nDataRowHeigt ) );
				_rOut.DrawRect( aFieldRect );
			}

			if (!m_bFocusOnlyCursor && (pCol->GetId() == GetCurColumnId()) && (nRow == (ULONG)GetCurRow()))
				DrawCursor();

			// draw a single field
			// #63864#, Sonst wird auch etwas gezeichnet, bsp Handle Column
			if (pCol->Width())
			{
				// clip the column's output to the field area
				if (_bForeignDevice)
				{	// (not neccessary if painting onto the data window)
					Size aFieldSize(pCol->Width(), nDataRowHeigt);

					if (aPos.X() + aFieldSize.Width() > aOverallAreaBRPos.X())
						aFieldSize.Width() = aOverallAreaBRPos.X() - aPos.X();

					if (aPos.Y() + aFieldSize.Height() > aOverallAreaBRPos.Y() + 1)
					{
						// for non-handle cols we don't clip vertically : we just don't draw the cell if the line isn't completely visible
						if (pCol->GetId() != 0)
							continue;
						aFieldSize.Height() = aOverallAreaBRPos.Y() + 1 - aPos.Y();
					}

					Region aClipToField(Rectangle(aPos, aFieldSize));
					_rOut.SetClipRegion(aClipToField);
				}
				pCol->Draw( *this, _rOut, aPos, FALSE );
				if (_bForeignDevice)
					_rOut.SetClipRegion();
			}

			// reset Column-auto-highlight
			if ( bColAutoHighlight )
			{
				_rOut.SetTextColor( aOldTextColor );
				_rOut.SetFillColor( aOldFillColor );
				_rOut.SetLineColor( aOldLineColor );
			}

			// skip column
			aPos.X() += pCol->Width();
		}

		if ( nCol == pCols->Count() )
			bRetouching = TRUE;

		// reset auto-highlight
		if ( bRowAutoHighlight )
		{
			_rOut.SetTextColor( aOldTextColor );
			_rOut.SetFillColor( aOldFillColor );
			_rOut.SetLineColor( aOldLineColor );
		}

		if ( bHLines )
		{
			// draw horizontal delimitation lines
			_rOut.SetClipRegion();
            _rOut.Push( PUSH_LINECOLOR );
			_rOut.SetLineColor( aDelimiterLineColor );
			long nY = aPos.Y() + nDataRowHeigt - 1;
			if (nY <= aOverallAreaBRPos.Y())
				_rOut.DrawLine(	Point( nHLineX, nY ),
								Point( bVLines
										? std::min(long(long(aPos.X()) - 1), aOverallAreaBRPos.X())
										: aOverallAreaBRPos.X(),
									  nY ) );
            _rOut.Pop();
		}
	}

	if (aPos.Y() > aOverallAreaBRPos.Y() + 1)
		aPos.Y() = aOverallAreaBRPos.Y() + 1;
		// needed for some of the following drawing

	// retouching
	_rOut.SetClipRegion();
	aOldLineColor = _rOut.GetLineColor();
	aOldFillColor = _rOut.GetFillColor();
	_rOut.SetFillColor( rSettings.GetFaceColor() );
	if ( pCols->Count() && ( pCols->GetObject(0)->GetId() == 0 ) && ( aPos.Y() <= _rRect.Bottom() ) )
	{
		// fill rectangle gray below handle column
		// DG: fill it only until the end of the drawing rect and not to the end, as this may overpaint handle columns
		_rOut.SetLineColor( Color( COL_BLACK ) );
		_rOut.DrawRect( Rectangle(
			Point( aOverallAreaPos.X() - 1, aPos.Y() - 1 ),
			Point( aOverallAreaPos.X() + pCols->GetObject(0)->Width() - 1,
				   _rRect.Bottom() + 1) ) );
	}
	_rOut.SetFillColor( aOldFillColor );

	// draw vertical delimitational line between frozen and scrollable cols
	_rOut.SetLineColor( COL_BLACK );
	long nFrozenWidth = GetFrozenWidth()-1;
	_rOut.DrawLine( Point( aOverallAreaPos.X() + nFrozenWidth, aPos.Y() ),
				   Point( aOverallAreaPos.X() + nFrozenWidth, bHLines
							? aPos.Y() - 1
							: aOverallAreaBRPos.Y() ) );

	// draw vertical delimitational lines?
	if ( bVLines )
	{
		_rOut.SetLineColor( aDelimiterLineColor );
        Point aVertPos( aOverallAreaPos.X() - 1, aOverallAreaPos.Y() );
		long nDeltaY = aOverallAreaBRPos.Y();
		for ( USHORT nCol = 0; nCol < pCols->Count(); ++nCol )
		{
			// get column
			BrowserColumn *pCol = pCols->GetObject(nCol);

			// skip invisible colums between frozen and scrollable area
			if ( nCol < nFirstCol && !pCol->IsFrozen() )
			{
				nCol = nFirstCol;
				pCol = pCols->GetObject(nCol);
			}

			// skip column
			aVertPos.X() += pCol->Width();

			// at end of invalid area
			// invalid area is first reached when X > Right
			// and not >=
			if ( aVertPos.X() > _rRect.Right() )
				break;

			// draw a single line
			if ( pCol->GetId() != 0 )
				_rOut.DrawLine( aVertPos, Point( aVertPos.X(),
							   bHLines
								? aPos.Y() - 1
								: aPos.Y() + nDeltaY ) );
		}
	}

	_rOut.SetLineColor( aOldLineColor );
}

//-------------------------------------------------------------------

void BrowseBox::PaintData( Window& rWin, const Rectangle& rRect )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	if ( !bBootstrapped && IsReallyVisible() )
		BrowseBox::StateChanged( STATE_CHANGE_INITSHOW );

	// initializations
	if ( !pCols || !pCols->Count() || !rWin.IsUpdateMode() )
		return;
	if ( getDataWindow()->bResizeOnPaint )
		Resize();
	// MI: wer war das denn? Window::Update();

	ImplPaintData(rWin, rRect, FALSE, TRUE);
}

//-------------------------------------------------------------------

void BrowseBox::UpdateScrollbars()
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	if ( !bBootstrapped || !IsUpdateMode() )
		return;

	// Rekursionsschutz
	BrowserDataWin *pBDW = (BrowserDataWin*) pDataWin;
	if ( pBDW->bInUpdateScrollbars )
	{
		pBDW->bHadRecursion = TRUE;
		return;
	}
	pBDW->bInUpdateScrollbars = TRUE;

	// the size of the corner window (and the width of the VSB/height of the HSB)
	ULONG nCornerSize = GetSettings().GetStyleSettings().GetScrollBarSize();
	if (IsZoom())
		nCornerSize = (ULONG)(nCornerSize * (double)GetZoom());

	// needs VScroll?
	long nMaxRows = (pDataWin->GetSizePixel().Height()) / GetDataRowHeight();
	BOOL bNeedsVScroll =    getDataWindow()->bAutoVScroll
						?   nTopRow || ( nRowCount > nMaxRows )
						:   !getDataWindow()->bNoVScroll;
	Size aDataWinSize = pDataWin->GetSizePixel();
	if ( !bNeedsVScroll )
	{
		if ( pVScroll->IsVisible() )
		{
			pVScroll->Hide();
			Size aNewSize( aDataWinSize );
			aNewSize.Width() = GetOutputSizePixel().Width();
			aDataWinSize = aNewSize;
		}
	}
	else if ( !pVScroll->IsVisible() )
	{
		Size aNewSize( aDataWinSize );
		aNewSize.Width() = GetOutputSizePixel().Width() - nCornerSize;
		aDataWinSize = aNewSize;
	}

	// needs HScroll?
	ULONG nLastCol = GetColumnAtXPosPixel( aDataWinSize.Width() - 1 );

	USHORT nFrozenCols = FrozenColCount();
	BOOL bNeedsHScroll =    getDataWindow()->bAutoHScroll
		                ?   ( nFirstCol > nFrozenCols ) || ( nLastCol <= pCols->Count() )
		                :   !getDataWindow()->bNoHScroll;
	if ( !bNeedsHScroll )
	{
		if ( aHScroll.IsVisible() )
		{
			aHScroll.Hide();
		}
        aDataWinSize.Height() = GetOutputSizePixel().Height() - GetTitleHeight();
        if ( nControlAreaWidth != USHRT_MAX )
            aDataWinSize.Height() -= nCornerSize;
	}
	else if ( !aHScroll.IsVisible() )
	{
		Size aNewSize( aDataWinSize );
		aNewSize.Height() = GetOutputSizePixel().Height() - GetTitleHeight() - nCornerSize;
		aDataWinSize = aNewSize;
	}

	// adjust position and Width of horizontal scrollbar
	ULONG nHScrX = nControlAreaWidth == USHRT_MAX
		? 0
		: nControlAreaWidth;

	aHScroll.SetPosSizePixel(
		Point( nHScrX, GetOutputSizePixel().Height() - nCornerSize ),
		Size( aDataWinSize.Width() - nHScrX, nCornerSize ) );

	// Scrollable Columns insgesamt
	short nScrollCols = short(pCols->Count()) - (short)nFrozenCols;
	/*short nVisibleHSize= std::max(nLastCol == BROWSER_INVALIDID
								? pCols->Count() - nFirstCol -1
								: nLastCol - nFirstCol - 1, 0);

	aHScroll.SetVisibleSize( nVisibleHSize );
	aHScroll.SetRange( Range( 0, Max( std::min(nScrollCols, nVisibleHSize), (short)0 ) ) );
	if ( bNeedsHScroll && !aHScroll.IsVisible() )
		aHScroll.Show();*/

	// Sichtbare Columns
	short nVisibleHSize = nLastCol == BROWSER_INVALIDID
		? (short)( pCols->Count() - nFirstCol )
		: (short)( nLastCol - nFirstCol );

	short nRange = Max( nScrollCols, (short)0 );
	aHScroll.SetVisibleSize( nVisibleHSize );
	aHScroll.SetRange( Range( 0, nRange ));
	if ( bNeedsHScroll && !aHScroll.IsVisible() )
		aHScroll.Show();

	// adjust position and height of vertical scrollbar
	pVScroll->SetPageSize( nMaxRows );

	if ( nTopRow > nRowCount )
	{
		nTopRow = nRowCount - 1;
		DBG_ERROR("BrowseBox: nTopRow > nRowCount");
	}

	if ( pVScroll->GetThumbPos() != nTopRow )
		pVScroll->SetThumbPos( nTopRow );
	long nVisibleSize = Min( Min( nRowCount, nMaxRows ), long(nRowCount-nTopRow) );
	pVScroll->SetVisibleSize( nVisibleSize ? nVisibleSize : 1 );
	pVScroll->SetRange( Range( 0, nRowCount ) );
	pVScroll->SetPosSizePixel(
		Point( aDataWinSize.Width(), GetTitleHeight() ),
		Size( nCornerSize, aDataWinSize.Height()) );
	if ( nRowCount <
		 long( aDataWinSize.Height() / GetDataRowHeight() ) )
		ScrollRows( -nTopRow );
	if ( bNeedsVScroll && !pVScroll->IsVisible() )
		pVScroll->Show();

	pDataWin->SetPosSizePixel(
		Point( 0, GetTitleHeight() ),
		aDataWinSize );

	// needs corner-window?
	// (do that AFTER positioning BOTH scrollbars)
    ULONG nActualCorderWidth = 0;
    if (aHScroll.IsVisible() && pVScroll && pVScroll->IsVisible() )
    {
        // if we have both scrollbars, the corner window fills the point of intersection of these two
        nActualCorderWidth = nCornerSize;
    }
    else if ( !aHScroll.IsVisible() && ( nControlAreaWidth != USHRT_MAX ) )
    {
        // if we have no horizontal scrollbar, but a control area, we need the corner window to
        // fill the space between the control are and the right border
        nActualCorderWidth = GetOutputSizePixel().Width() - nControlAreaWidth;
    }
	if ( nActualCorderWidth )
	{
		if ( !getDataWindow()->pCornerWin )
			getDataWindow()->pCornerWin = new ScrollBarBox( this, 0 );
		getDataWindow()->pCornerWin->SetPosSizePixel(
			Point( GetOutputSizePixel().Width() - nActualCorderWidth, aHScroll.GetPosPixel().Y() ),
			Size( nActualCorderWidth, nCornerSize ) );
		getDataWindow()->pCornerWin->Show();
	}
	else
		DELETEZ( getDataWindow()->pCornerWin );

	// ggf. Headerbar mitscrollen
	if ( getDataWindow()->pHeaderBar )
	{
		long nWidth = 0;
		for ( USHORT nCol = 0;
			  nCol < pCols->Count() && nCol < nFirstCol;
			  ++nCol )
		{
			// HandleColumn nicht
			if ( pCols->GetObject(nCol)->GetId() )
				nWidth += pCols->GetObject(nCol)->Width();
		}

		getDataWindow()->pHeaderBar->SetOffset( nWidth );
	}

	pBDW->bInUpdateScrollbars = FALSE;
	if ( pBDW->bHadRecursion )
	{
		pBDW->bHadRecursion = FALSE;
		UpdateScrollbars();
	}
}

//-------------------------------------------------------------------

void BrowseBox::SetUpdateMode( BOOL bUpdate )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	BOOL bWasUpdate = IsUpdateMode();
	if ( bWasUpdate == bUpdate )
		return;

	Control::SetUpdateMode( bUpdate );
	// OV
	// Wenn an der BrowseBox WB_CLIPCHILDREN gesetzt ist (wg. Flackerminimierung),
	// wird das Datenfenster nicht von SetUpdateMode invalidiert.
	if( bUpdate )
		getDataWindow()->Invalidate();
	getDataWindow()->SetUpdateMode( bUpdate );


	if ( bUpdate )
	{
		if ( bBootstrapped )
		{
			UpdateScrollbars();
			AutoSizeLastColumn();
		}
		DoShowCursor( "SetUpdateMode" );
	}
	else
		DoHideCursor( "SetUpdateMode" );
}

//-------------------------------------------------------------------

BOOL BrowseBox::GetUpdateMode() const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	return getDataWindow()->IsUpdateMode();
}

//-------------------------------------------------------------------

long BrowseBox::GetFrozenWidth() const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	long nWidth = 0;
	for ( USHORT nCol = 0;
		  nCol < pCols->Count() && pCols->GetObject(nCol)->IsFrozen();
		  ++nCol )
		nWidth += pCols->GetObject(nCol)->Width();
	return nWidth;
}

//-------------------------------------------------------------------

void BrowseBox::ColumnInserted( USHORT nPos )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	if ( pColSel )
		pColSel->Insert( nPos );
	UpdateScrollbars();
}

//-------------------------------------------------------------------

USHORT BrowseBox::FrozenColCount() const
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
	USHORT nCol;
	for ( nCol = 0;
		  nCol < pCols->Count() && pCols->GetObject(nCol)->IsFrozen();
		  ++nCol )
		/* empty loop */;
	return nCol;
}

//-------------------------------------------------------------------

IMPL_LINK(BrowseBox,ScrollHdl,ScrollBar*,pBar)
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	if ( pBar->GetDelta() == 0 )
		return 0;

	if ( pBar->GetDelta() < 0 && getDataWindow()->bNoScrollBack )
	{
		UpdateScrollbars();
		return 0;
	}

	if ( pBar == &aHScroll )
		ScrollColumns( aHScroll.GetDelta() );
	if ( pBar == pVScroll )
		ScrollRows( pVScroll->GetDelta() );

	return 0;
}

//-------------------------------------------------------------------

IMPL_LINK( BrowseBox,EndScrollHdl,ScrollBar*, EMPTYARG )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	// kein Focus grabben!
	/// GrabFocus();

	if ( /*pBar->GetDelta() <= 0 &&*/ getDataWindow()->bNoScrollBack )
	{
		// UpdateScrollbars();
		EndScroll();
		return 0;
	}

	return 0;
}

//-------------------------------------------------------------------

IMPL_LINK( BrowseBox, StartDragHdl, HeaderBar*, pBar )
{
	pBar->SetDragSize( pDataWin->GetOutputSizePixel().Height() );
	return 0;
}

//-------------------------------------------------------------------
// MI: es wurde immer nur die 1. Spalte resized
#ifdef _MSC_VER
#pragma optimize("",off)
#endif

void BrowseBox::MouseButtonDown( const MouseEvent& rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	GrabFocus();

	// onl< mouse events in the title-line are supported
	const Point &rEvtPos = rEvt.GetPosPixel();
	if ( rEvtPos.Y() >= GetTitleHeight() )
		return;

	long nX = 0;
	long nWidth = GetOutputSizePixel().Width();
	for ( USHORT nCol = 0; nCol < pCols->Count() && nX < nWidth; ++nCol )
	{
		// is this column visible?
		BrowserColumn *pCol = pCols->GetObject(nCol);
		if ( pCol->IsFrozen() || nCol >= nFirstCol )
		{
			// compute right end of column
			long nR = nX + pCol->Width() - 1;

			// at the end of a column (and not handle column)?
			if ( pCol->GetId() && Abs( nR - rEvtPos.X() ) < 2 )
			{
				// start resizing the column
				bResizing = TRUE;
				nResizeCol = nCol;
				nDragX = nResizeX = rEvtPos.X();
				SetPointer( Pointer( POINTER_HSPLIT ) );
				CaptureMouse();
				pDataWin->DrawLine( Point( nDragX, 0 ),
					Point( nDragX, pDataWin->GetSizePixel().Height() ) );
				nMinResizeX = nX + MIN_COLUMNWIDTH;
				return;
			}
			else if ( nX < rEvtPos.X() && nR > rEvtPos.X() )
			{
				MouseButtonDown( BrowserMouseEvent(
					this, rEvt, -1, nCol, pCol->GetId(), Rectangle() ) );
				return;
			}
			nX = nR + 1;
		}
	}

	// event occured out of data area
	if ( rEvt.IsRight() )
		pDataWin->Command(
			CommandEvent( Point( 1, LONG_MAX ), COMMAND_CONTEXTMENU, TRUE ) );
	else
		SetNoSelection();
}

#ifdef _MSC_VER
#pragma optimize("",on)
#endif

//-------------------------------------------------------------------

void BrowseBox::MouseMove( const MouseEvent& rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
    DBG_TRACE( "BrowseBox::MouseMove( MouseEvent )" );

	Pointer aNewPointer;

	USHORT nX = 0;
	for ( USHORT nCol = 0;
		  nCol < USHORT(pCols->Count()) &&
			( nX + pCols->GetObject(nCol)->Width() ) < USHORT(GetOutputSizePixel().Width());
		  ++nCol )
		// is this column visible?
		if ( pCols->GetObject(nCol)->IsFrozen() || nCol >= nFirstCol )
		{
			// compute right end of column
			BrowserColumn *pCol = pCols->GetObject(nCol);
			USHORT nR = (USHORT)(nX + pCol->Width() - 1);

			// show resize-pointer?
			if ( bResizing || ( pCol->GetId() &&
				 Abs( ((long) nR ) - rEvt.GetPosPixel().X() ) < MIN_COLUMNWIDTH ) )
			{
				aNewPointer = Pointer( POINTER_HSPLIT );
				if ( bResizing )
				{
					// alte Hilfslinie loeschen
					pDataWin->HideTracking() ;

					// erlaubte breite abholen und neues Delta
					nDragX = Max( rEvt.GetPosPixel().X(), nMinResizeX );
					long nDeltaX = nDragX - nResizeX;
					USHORT nId = GetColumnId(nResizeCol);
					ULONG nOldWidth = GetColumnWidth(nId);
					nDragX = QueryColumnResize( GetColumnId(nResizeCol),
									nOldWidth + nDeltaX )
							 + nResizeX - nOldWidth;

					// neue Hilfslinie zeichnen
					pDataWin->ShowTracking( Rectangle( Point( nDragX, 0 ),
							Size( 1, pDataWin->GetSizePixel().Height() ) ),
							SHOWTRACK_SPLIT|SHOWTRACK_WINDOW );
				}

			}

			nX = nR + 1;
		}

	SetPointer( aNewPointer );
}

//-------------------------------------------------------------------

void BrowseBox::MouseButtonUp( const MouseEvent & rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	if ( bResizing )
	{
		// Hilfslinie loeschen
		pDataWin->HideTracking();

		// width changed?
		nDragX = Max( rEvt.GetPosPixel().X(), nMinResizeX );
		if ( (nDragX - nResizeX) != (long)pCols->GetObject(nResizeCol)->Width() )
		{
			// resize column
			long nMaxX = pDataWin->GetSizePixel().Width();
			nDragX = Min( nDragX, nMaxX );
			long nDeltaX = nDragX - nResizeX;
			USHORT nId = GetColumnId(nResizeCol);
			SetColumnWidth( GetColumnId(nResizeCol), GetColumnWidth(nId) + nDeltaX );
			ColumnResized( nId );
		}

		// end action
		SetPointer( Pointer() );
		ReleaseMouse();
		bResizing = FALSE;
	}
	else
		MouseButtonUp( BrowserMouseEvent( (BrowserDataWin*)pDataWin,
				MouseEvent( Point( rEvt.GetPosPixel().X(),
						rEvt.GetPosPixel().Y() - pDataWin->GetPosPixel().Y() ),
					rEvt.GetClicks(), rEvt.GetMode(), rEvt.GetButtons(),
					rEvt.GetModifier() ) ) );
}

//-------------------------------------------------------------------

BOOL bExtendedMode = FALSE;
BOOL bFieldMode = FALSE;

void BrowseBox::MouseButtonDown( const BrowserMouseEvent& rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	GrabFocus();

	// adjust selection while and after double-click
	if ( rEvt.GetClicks() == 2 )
	{
		SetNoSelection();
		if ( rEvt.GetRow() >= 0 )
		{
			GoToRow( rEvt.GetRow() );
			SelectRow( rEvt.GetRow(), TRUE, FALSE );
		}
		else
		{
			if ( bColumnCursor && rEvt.GetColumn() != 0 )
			{
				if ( rEvt.GetColumn() < pCols->Count() )
					SelectColumnPos( rEvt.GetColumn(), TRUE, FALSE);
			}
		}
		DoubleClick( rEvt );
	}
	// selections
	else if ( ( rEvt.GetMode() & ( MOUSE_SELECT | MOUSE_SIMPLECLICK ) ) &&
		 ( bColumnCursor || rEvt.GetRow() >= 0 ) )
	{
		if ( rEvt.GetClicks() == 1 )
		{
			// initialise flags
			bHit            = FALSE;
			a1stPoint       =
			a2ndPoint       = PixelToLogic( rEvt.GetPosPixel() );

			// selection out of range?
			if ( rEvt.GetRow() >= nRowCount ||
				 rEvt.GetColumnId() == BROWSER_INVALIDID )
			{
				SetNoSelection();
				return;
			}

			// while selecting, no cursor
			bSelecting = TRUE;
			DoHideCursor( "MouseButtonDown" );

			// DataRow?
			if ( rEvt.GetRow() >= 0 )
			{
				// Zeilenselektion?
				if ( rEvt.GetColumnId() == 0 || !bColumnCursor )
				{
					if ( bMultiSelection )
					{
						// remove column-selection, if exists
						if ( pColSel && pColSel->GetSelectCount() )
						{
							ToggleSelection();
							if ( bMultiSelection )
								uRow.pSel->SelectAll(FALSE);
							else
								uRow.nSel = BROWSER_ENDOFSELECTION;
							if ( pColSel )
								pColSel->SelectAll(FALSE);
							bSelect = TRUE;
						}

						// expanding mode?
						if ( rEvt.GetMode() & MOUSE_RANGESELECT )
						{
							// select the further touched rows too
							bSelect = TRUE;
							ExpandRowSelection( rEvt );
							return;
						}

						// click in the selected area?
						else if ( IsRowSelected( rEvt.GetRow() ) )
						{
							// auf Drag&Drop warten
							bHit = TRUE;
							bExtendedMode = MOUSE_MULTISELECT ==
									( rEvt.GetMode() & MOUSE_MULTISELECT );
							return;
						}

						// extension mode?
						else if ( rEvt.GetMode() & MOUSE_MULTISELECT )
						{
							// determine the new selection range
							// and selection/deselection
							aSelRange = Range( rEvt.GetRow(), rEvt.GetRow() );
							SelectRow( rEvt.GetRow(),
									!uRow.pSel->IsSelected( rEvt.GetRow() ) );
							bSelect = TRUE;
							return;
						}
					}

					// select directly
					SetNoSelection();
					GoToRow( rEvt.GetRow() );
					SelectRow( rEvt.GetRow(), TRUE );
					aSelRange = Range( rEvt.GetRow(), rEvt.GetRow() );
					bSelect = TRUE;
				}
				else // Column/Field-Selection
				{
					// click in selected column
					if ( IsColumnSelected( rEvt.GetColumn() ) ||
						 IsRowSelected( rEvt.GetRow() ) )
					{
						bHit = TRUE;
						bFieldMode = TRUE;
						return;
					}

					SetNoSelection();
					GoToRowColumnId( rEvt.GetRow(), rEvt.GetColumnId() );
					bSelect = TRUE;
				}
			}
			else
			{
				if ( bMultiSelection && rEvt.GetColumnId() == 0 )
				{
					// toggle all-selection
					if ( uRow.pSel->GetSelectCount() > ( GetRowCount() / 2 ) )
						SetNoSelection();
					else
						SelectAll();
				}
				else
					SelectColumnId( rEvt.GetColumnId(), TRUE, FALSE );
			}

			// ggf. Cursor wieder an
			bSelecting = FALSE;
			DoShowCursor( "MouseButtonDown" );
			if ( bSelect )
				Select();
		}
	}
}

//-------------------------------------------------------------------

void BrowseBox::MouseMove( const BrowserMouseEvent& )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);
}

//-------------------------------------------------------------------

void BrowseBox::MouseButtonUp( const BrowserMouseEvent &rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	// D&D was possible, but did not occur
	if ( bHit )
	{
		aSelRange = Range( rEvt.GetRow(), rEvt.GetRow() );
		if ( bExtendedMode )
			SelectRow( rEvt.GetRow(), FALSE );
		else
		{
			SetNoSelection();
			if ( bFieldMode )
				GoToRowColumnId( rEvt.GetRow(), rEvt.GetColumnId() );
			else
			{
				GoToRow( rEvt.GetRow() );
				SelectRow( rEvt.GetRow(), TRUE );
			}
		}
		bSelect = TRUE;
		bExtendedMode = FALSE;
		bFieldMode = FALSE;
		bHit = FALSE;
	}

	// activate cursor
	if ( bSelecting )
	{
		bSelecting = FALSE;
		DoShowCursor( "MouseButtonUp" );
		if ( bSelect )
			Select();
	}
}

//-------------------------------------------------------------------

void BrowseBox::KeyInput( const KeyEvent& rEvt )
{
	if ( !ProcessKey( rEvt ) )
		Control::KeyInput( rEvt );
}

//-------------------------------------------------------------------

BOOL BrowseBox::ProcessKey( const KeyEvent& rEvt )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	USHORT nCode = rEvt.GetKeyCode().GetCode();
	BOOL   bShift = rEvt.GetKeyCode().IsShift();
	BOOL   bCtrl = rEvt.GetKeyCode().IsMod1();
	BOOL   bAlt = rEvt.GetKeyCode().IsMod2();

	USHORT nId = BROWSER_NONE;

	if ( !bAlt && !bCtrl && !bShift )
	{
		switch ( nCode )
		{
			case KEY_DOWN:          nId = BROWSER_CURSORDOWN; break;
			case KEY_UP:            nId = BROWSER_CURSORUP; break;
			case KEY_HOME:          nId = BROWSER_CURSORHOME; break;
			case KEY_END:           nId = BROWSER_CURSOREND; break;
			case KEY_TAB:
				if ( !bColumnCursor )
					break;
			case KEY_RIGHT:         nId = BROWSER_CURSORRIGHT; break;
			case KEY_LEFT:          nId = BROWSER_CURSORLEFT; break;
			case KEY_SPACE:         nId = BROWSER_SELECT; break;
		}
		if ( BROWSER_NONE != nId )
			SetNoSelection();

		switch ( nCode )
		{
			case KEY_PAGEDOWN:      nId = BROWSER_CURSORPAGEDOWN; break;
			case KEY_PAGEUP:        nId = BROWSER_CURSORPAGEUP; break;
		}
	}

	if ( !bAlt && !bCtrl && bShift )
		switch ( nCode )
		{
			case KEY_DOWN:          nId = BROWSER_SELECTDOWN; break;
			case KEY_UP:            nId = BROWSER_SELECTUP; break;
			case KEY_TAB:
				if ( !bColumnCursor )
					break;
									nId = BROWSER_CURSORLEFT; break;
			case KEY_HOME:          nId = BROWSER_SELECTHOME; break;
			case KEY_END:           nId = BROWSER_SELECTEND; break;
		}


	if ( !bAlt && bCtrl && !bShift )
		switch ( nCode )
		{
			case KEY_DOWN:          nId = BROWSER_CURSORDOWN; break;
			case KEY_UP:            nId = BROWSER_CURSORUP; break;
			case KEY_PAGEDOWN:      nId = BROWSER_CURSORENDOFFILE; break;
			case KEY_PAGEUP:        nId = BROWSER_CURSORTOPOFFILE; break;
			case KEY_HOME:          nId = BROWSER_CURSORTOPOFSCREEN; break;
			case KEY_END:           nId = BROWSER_CURSORENDOFSCREEN; break;
			case KEY_SPACE:         nId = BROWSER_ENHANCESELECTION; break;
			case KEY_LEFT:          nId = BROWSER_MOVECOLUMNLEFT; break;
			case KEY_RIGHT:         nId = BROWSER_MOVECOLUMNRIGHT; break;
		}

	if ( nId != BROWSER_NONE )
		Dispatch( nId );
	return nId != BROWSER_NONE;
}

//-------------------------------------------------------------------

void BrowseBox::Dispatch( USHORT nId )
{
	DBG_CHKTHIS(BrowseBox,BrowseBoxCheckInvariants);

	long nRowsOnPage = pDataWin->GetSizePixel().Height() / GetDataRowHeight();
	BOOL bDone = FALSE;

	switch ( nId )
	{
		case BROWSER_SELECTCOLUMN:
			if ( ColCount() )
				SelectColumnId( GetCurColumnId() );
			break;

		case BROWSER_CURSORDOWN:
			if ( ( GetCurRow() + 1 ) < nRowCount )
				bDone = GoToRow( GetCurRow() + 1, FALSE );
			break;
		case BROWSER_CURSORUP:
			if ( GetCurRow() > 0 )
				bDone = GoToRow( GetCurRow() - 1, FALSE );
			break;
		case BROWSER_SELECTHOME:
			if ( GetRowCount() )
			{
				DoHideCursor( "BROWSER_SELECTHOME" );
				for ( long nRow = GetCurRow(); nRow >= 0; --nRow )
					SelectRow( nRow );
				GoToRow( 0, TRUE );
				DoShowCursor( "BROWSER_SELECTHOME" );
			}
			break;
		case BROWSER_SELECTEND:
			if ( GetRowCount() )
			{
				DoHideCursor( "BROWSER_SELECTEND" );
				long nRows = GetRowCount();
				for ( long nRow = GetCurRow(); nRow < nRows; ++nRow )
					SelectRow( nRow );
				GoToRow( GetRowCount() - 1, TRUE );
				DoShowCursor( "BROWSER_SELECTEND" );
			}
			break;
		case BROWSER_SELECTDOWN:
		{
			if ( GetRowCount() && ( GetCurRow() + 1 ) < nRowCount )
			{
				// deselect the current row, if it isn't the first
				// and there is no other selected row above
				long nRow = GetCurRow();
				BOOL bLocalSelect = ( !IsRowSelected( nRow ) ||
								 GetSelectRowCount() == 1 || IsRowSelected( nRow - 1 ) );
				SelectRow( nRow, bLocalSelect, TRUE );
				bDone = GoToRow( GetCurRow() + 1 , FALSE );
				if ( bDone )
					SelectRow( GetCurRow(), TRUE, TRUE );
			}
			else
				bDone = ScrollRows( 1 ) != 0;
			break;
		}
		case BROWSER_SELECTUP:
			if ( GetRowCount() )
			{
				// deselect the current row, if it isn't the first
				// and there is no other selected row under
				long nRow = GetCurRow();
				BOOL bLocalSelect = ( !IsRowSelected( nRow ) ||
								 GetSelectRowCount() == 1 || IsRowSelected( nRow + 1 ) );
				SelectRow( nCurRow, bLocalSelect, TRUE );
                bDone = GoToRow( nRow - 1 , FALSE );
				if ( bDone )
					SelectRow( GetCurRow(), TRUE, TRUE );
			}
			break;
		case BROWSER_CURSORPAGEDOWN:
			bDone = (BOOL)ScrollRows( nRowsOnPage );
			break;
		case BROWSER_CURSORPAGEUP:
			bDone = (BOOL)ScrollRows( -nRowsOnPage );
			break;
		case BROWSER_CURSOREND:
			if ( bColumnCursor )
			{
				USHORT nNewId = GetColumnId(ColCount() -1);
				bDone = (nNewId != 0) && GoToColumnId( nNewId );
				break;
			}
		case BROWSER_CURSORENDOFFILE:
			bDone = GoToRow( nRowCount - 1, FALSE );
			break;
		case BROWSER_CURSORRIGHT:
			if ( bColumnCursor )
			{
				USHORT nNewPos = GetColumnPos( GetCurColumnId() ) + 1;
				USHORT nNewId = GetColumnId( nNewPos );
				if (nNewId != 0)	// Am Zeilenende ?
					bDone = GoToColumnId( nNewId );
				else
				{
					USHORT nColId = ( GetColumnId(0) == 0 ) ? GetColumnId(1) : GetColumnId(0);
					if ( GetRowCount() )
						bDone = ( nCurRow < GetRowCount() - 1 ) && GoToRowColumnId( nCurRow + 1, nColId );
					else if ( ColCount() )
						GoToColumnId( nColId );
				}
			}
			else
				bDone = ScrollColumns( 1 ) != 0;
			break;
		case BROWSER_CURSORHOME:
			if ( bColumnCursor )
			{
				USHORT nNewId = GetColumnId(1);
				bDone = (nNewId != 0) && GoToColumnId( nNewId );
				break;
			}
		case BROWSER_CURSORTOPOFFILE:
			bDone = GoToRow( 0, FALSE );
			break;
		case BROWSER_CURSORLEFT:
			if ( bColumnCursor )
			{
				USHORT nNewPos = GetColumnPos( GetCurColumnId() ) - 1;
				USHORT nNewId = GetColumnId( nNewPos );
				if (nNewId != 0)
					bDone = GoToColumnId( nNewId );
				else
				{
					if ( GetRowCount() )
						bDone = (nCurRow > 0) && GoToRowColumnId(nCurRow - 1, GetColumnId(ColCount() -1));
					else if ( ColCount() )
						GoToColumnId( GetColumnId(ColCount() -1) );
				}
			}
			else
				bDone = ScrollColumns( -1 ) != 0;
			break;
		case BROWSER_ENHANCESELECTION:
			if ( GetRowCount() )
				SelectRow( GetCurRow(), !IsRowSelected( GetCurRow() ), TRUE );
			bDone = TRUE;
			break;
		case BROWSER_SELECT:
			if ( GetRowCount() )
				SelectRow( GetCurRow(), !IsRowSelected( GetCurRow() ), FALSE );
			bDone = TRUE;
			break;
		case BROWSER_MOVECOLUMNLEFT:
		case BROWSER_MOVECOLUMNRIGHT:
			{ // check if column moving is allowed
				BrowserHeader* pHeaderBar = getDataWindow()->pHeaderBar;
				if ( pHeaderBar && pHeaderBar->IsDragable() )
				{
					USHORT nColId = GetCurColumnId();
					BOOL bColumnSelected = IsColumnSelected(nColId);
					USHORT nNewPos = GetColumnPos(nColId);
					BOOL bMoveAllowed = FALSE;
					if ( BROWSER_MOVECOLUMNLEFT == nId && nNewPos > 1 )
						--nNewPos,bMoveAllowed = TRUE;
					else if ( BROWSER_MOVECOLUMNRIGHT == nId && nNewPos < (ColCount()-1) )
						++nNewPos,bMoveAllowed = TRUE;

					if ( bMoveAllowed )
					{
						SetColumnPos( nColId, nNewPos );
						ColumnMoved( nColId );
						MakeFieldVisible(GetCurRow(),nColId,TRUE);
						if ( bColumnSelected )
							SelectColumnId(nColId);
					}
				}
			}
			break;
	}

	//! return bDone;
}

//-------------------------------------------------------------------

void BrowseBox::SetCursorColor(const Color& _rCol)
{
	if (_rCol == m_aCursorColor)
		return;

	// ensure the cursor is hidden
	DoHideCursor("SetCursorColor");
	if (!m_bFocusOnlyCursor)
		DoHideCursor("SetCursorColor - force");

	m_aCursorColor = _rCol;

	if (!m_bFocusOnlyCursor)
		DoShowCursor("SetCursorColor - force");
	DoShowCursor("SetCursorColor");
}
// -----------------------------------------------------------------------------
Rectangle BrowseBox::calcHeaderRect(sal_Bool _bIsColumnBar,BOOL _bOnScreen)
{
	Window* pParent = NULL;
	if ( !_bOnScreen )
		pParent = GetAccessibleParentWindow();

	Point aTopLeft;
	long nWidth;
	long nHeight;
	if ( _bIsColumnBar )
	{
		nWidth = GetDataWindow().GetOutputSizePixel().Width();
		nHeight = GetDataRowHeight();
	}
	else
	{
		aTopLeft.Y() = GetDataRowHeight();
		nWidth = GetColumnWidth(0);
		nHeight = GetWindowExtentsRelative( pParent ).GetHeight() - aTopLeft.Y() - GetControlArea().GetSize().B();
	}
	aTopLeft += GetWindowExtentsRelative( pParent ).TopLeft();
	return Rectangle(aTopLeft,Size(nWidth,nHeight));
}
// -----------------------------------------------------------------------------
Rectangle BrowseBox::calcTableRect(BOOL _bOnScreen)
{
	Window* pParent = NULL;
	if ( !_bOnScreen )
		pParent = GetAccessibleParentWindow();

	Rectangle aRect( GetWindowExtentsRelative( pParent ) );
	Rectangle aRowBar = calcHeaderRect(FALSE,pParent == NULL);

	long nX = aRowBar.Right() - aRect.Left();
	long nY = aRowBar.Top() - aRect.Top();
	Size aSize(aRect.GetSize());

	return Rectangle(aRowBar.TopRight(), Size(aSize.A() - nX, aSize.B() - nY - aHScroll.GetSizePixel().Height()) );
}
// -----------------------------------------------------------------------------
Rectangle BrowseBox::GetFieldRectPixelAbs( sal_Int32 _nRowId,sal_uInt16 _nColId, BOOL /*_bIsHeader*/, BOOL _bOnScreen )
{
	Window* pParent = NULL;
	if ( !_bOnScreen )
		pParent = GetAccessibleParentWindow();

	Rectangle aRect = GetFieldRectPixel(_nRowId,_nColId,_bOnScreen);

	Point aTopLeft = aRect.TopLeft();
	aTopLeft += GetWindowExtentsRelative( pParent ).TopLeft();

	return Rectangle(aTopLeft,aRect.GetSize());
}

// ------------------------------------------------------------------------- EOF

