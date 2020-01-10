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

#include "datwin.hxx"

#ifndef GCC
#endif

#ifndef _APP_HXX //autogen
#include <vcl/svapp.hxx>
#endif

#ifndef _HELP_HXX
#include <vcl/help.hxx>
#endif
#ifndef _IMAGE_HXX
#include <vcl/image.hxx>
#endif

#include <tools/debug.hxx>

DECLARE_LIST( BrowserColumns, BrowserColumn* )

//===================================================================
void ButtonFrame::Draw( OutputDevice& rDev )
{
	Color aOldFillColor = rDev.GetFillColor();
	Color aOldLineColor = rDev.GetLineColor();

	const StyleSettings &rSettings = rDev.GetSettings().GetStyleSettings();
	Color aColLight( rSettings.GetLightColor() );
	Color aColShadow( rSettings.GetShadowColor() );
	Color aColFace( rSettings.GetFaceColor() );

	rDev.SetLineColor( aColFace );
	rDev.SetFillColor( aColFace );
	rDev.DrawRect( aRect );

    if( rDev.GetOutDevType() == OUTDEV_WINDOW )
    {
        Window *pWin = (Window*) &rDev;
        if( bPressed )
            pWin->DrawSelectionBackground( aRect, 0, TRUE, FALSE, FALSE );
    }
    else
    {
	    rDev.SetLineColor( bPressed ? aColShadow : aColLight );
	    rDev.DrawLine( aRect.TopLeft(), Point( aRect.Right(), aRect.Top() ) );
	    rDev.DrawLine( aRect.TopLeft(), Point( aRect.Left(), aRect.Bottom() - 1 ) );
	    rDev.SetLineColor( bPressed ? aColLight : aColShadow );
	    rDev.DrawLine( aRect.BottomRight(), Point( aRect.Right(), aRect.Top() ) );
	    rDev.DrawLine( aRect.BottomRight(), Point( aRect.Left(), aRect.Bottom() ) );
    }

	if ( aText.Len() )
	{
		String aVal = rDev.GetEllipsisString(aText,aInnerRect.GetWidth() - 2*MIN_COLUMNWIDTH);

		Font aFont( rDev.GetFont() );
		BOOL bOldTransp = aFont.IsTransparent();
		if ( !bOldTransp )
		{
			aFont.SetTransparent( TRUE );
			rDev.SetFont( aFont );
		}

		Color aOldColor = rDev.GetTextColor();
		if (m_bDrawDisabled)
			rDev.SetTextColor(rSettings.GetDisableColor());

		rDev.DrawText( Point(
			( aInnerRect.Left() + aInnerRect.Right() ) / 2 - ( rDev.GetTextWidth(aVal) / 2 ),
			aInnerRect.Top() ), aVal );

		// restore settings
		if ( !bOldTransp )
		{
			aFont.SetTransparent(FALSE);
			rDev.SetFont( aFont );
		}
		if (m_bDrawDisabled)
			rDev.SetTextColor(aOldColor);
	}

	if ( bCurs )
	{
		rDev.SetLineColor( Color( COL_BLACK ) );
		rDev.SetFillColor();
		rDev.DrawRect( Rectangle(
			Point( aRect.Left(), aRect.Top() ), Point( aRect.Right(), aRect.Bottom() ) ) );
	}

	rDev.SetLineColor( aOldLineColor );
	rDev.SetFillColor( aOldFillColor );
}

//-------------------------------------------------------------------

BrowserColumn::BrowserColumn( USHORT nItemId, const class Image &rImage,
							  const String& rTitle, ULONG nWidthPixel, const Fraction& rCurrentZoom,
							  HeaderBarItemBits nFlags )
:	_nId( nItemId ),
	_nWidth( nWidthPixel ),
	_aImage( rImage ),
	_aTitle( rTitle ),
	_bFrozen( FALSE ),
	_nFlags( nFlags )
{
	double n = (double)_nWidth;
	n *= (double)rCurrentZoom.GetDenominator();
	n /= (double)rCurrentZoom.GetNumerator();
	_nOriginalWidth = n>0 ? (long)(n+0.5) : -(long)(-n+0.5);
}

BrowserColumn::~BrowserColumn()
{
}

//-------------------------------------------------------------------

void BrowserColumn::SetWidth(ULONG nNewWidthPixel, const Fraction& rCurrentZoom)
{
	_nWidth = nNewWidthPixel;
	double n = (double)_nWidth;
	n *= (double)rCurrentZoom.GetDenominator();
	n /= (double)rCurrentZoom.GetNumerator();
	_nOriginalWidth = n>0 ? (long)(n+0.5) : -(long)(-n+0.5);
}

//-------------------------------------------------------------------

void BrowserColumn::Draw( BrowseBox& rBox, OutputDevice& rDev, const Point& rPos, BOOL bCurs  )
{
	if ( _nId == 0 )
	{
		// paint handle column
		ButtonFrame( rPos, Size( Width()-1, rBox.GetDataRowHeight()-1 ),
					 String(), FALSE, bCurs,
					 0 != (BROWSER_COLUMN_TITLEABBREVATION&_nFlags) ).Draw( rDev );
		Color aOldLineColor = rDev.GetLineColor();
		rDev.SetLineColor( Color( COL_BLACK ) );
		rDev.DrawLine(
			Point( rPos.X(), rPos.Y()+rBox.GetDataRowHeight()-1 ),
			Point( rPos.X() + Width() - 1, rPos.Y()+rBox.GetDataRowHeight()-1 ) );
		rDev.DrawLine(
			Point( rPos.X() + Width() - 1, rPos.Y() ),
			Point( rPos.X() + Width() - 1, rPos.Y()+rBox.GetDataRowHeight()-1 ) );
		rDev.SetLineColor( aOldLineColor );

		rBox.DoPaintField( rDev,
			Rectangle(
				Point( rPos.X() + 2, rPos.Y() + 2 ),
				Size( Width()-1, rBox.GetDataRowHeight()-1 ) ),
			GetId(),
            BrowseBox::BrowserColumnAccess() );
	}
	else
	{
		// paint data column
		long nWidth = Width() == LONG_MAX ? rBox.GetDataWindow().GetSizePixel().Width() : Width();

		rBox.DoPaintField( rDev,
			Rectangle(
				Point( rPos.X() + MIN_COLUMNWIDTH, rPos.Y() ),
				Size( nWidth-2*MIN_COLUMNWIDTH, rBox.GetDataRowHeight()-1 ) ),
			GetId(),
            BrowseBox::BrowserColumnAccess() );
	}
}

//-------------------------------------------------------------------

void BrowserColumn::ZoomChanged(const Fraction& rNewZoom)
{
	double n = (double)_nOriginalWidth;
	n *= (double)rNewZoom.GetNumerator();
	n /= (double)rNewZoom.GetDenominator();

	_nWidth = n>0 ? (long)(n+0.5) : -(long)(-n+0.5);
}

//-------------------------------------------------------------------

BrowserDataWin::BrowserDataWin( BrowseBox* pParent )
    :Control( pParent, WinBits(WB_CLIPCHILDREN) )
	,DragSourceHelper( this )
	,DropTargetHelper( this )
	,pHeaderBar( 0 )
	,pEventWin( pParent )
	,pCornerWin( 0 )
	,pDtorNotify( 0 )
	,bInPaint( FALSE )
	,bInCommand( FALSE )
	,bNoScrollBack( FALSE )
    ,bNoHScroll( FALSE )
    ,bNoVScroll( FALSE )
	,bUpdateMode( TRUE )
	,bResizeOnPaint( FALSE )
	,bUpdateOnUnlock( FALSE )
	,bInUpdateScrollbars( FALSE )
	,bHadRecursion( FALSE )
	,bOwnDataChangedHdl( FALSE )
	,bCallingDropCallback( FALSE )
	,nUpdateLock( 0 )
	,nCursorHidden( 0 )
    ,m_nDragRowDividerLimit( 0 )
    ,m_nDragRowDividerOffset( 0 )
{
	aMouseTimer.SetTimeoutHdl( LINK( this, BrowserDataWin, RepeatedMouseMove ) );
	aMouseTimer.SetTimeout( 100 );
}

//-------------------------------------------------------------------
BrowserDataWin::~BrowserDataWin()
{
	if( pDtorNotify )
		*pDtorNotify = TRUE;
}

//-------------------------------------------------------------------
void BrowserDataWin::LeaveUpdateLock()
{
	if ( !--nUpdateLock )
	{
		DoOutstandingInvalidations();
		if (bUpdateOnUnlock )
		{
			Control::Update();
			bUpdateOnUnlock = FALSE;
		}
	}
}

//-------------------------------------------------------------------
void InitSettings_Impl( Window *pWin,
					 BOOL bFont, BOOL bForeground, BOOL bBackground )
{
	const StyleSettings& rStyleSettings =
			pWin->GetSettings().GetStyleSettings();

	if ( bFont )
	{
		Font aFont = rStyleSettings.GetFieldFont();
		if ( pWin->IsControlFont() )
			aFont.Merge( pWin->GetControlFont() );
		pWin->SetZoomedPointFont( aFont );
	}

	if ( bFont || bForeground )
	{
		Color aTextColor = rStyleSettings.GetWindowTextColor();
		if ( pWin->IsControlForeground() )
			aTextColor = pWin->GetControlForeground();
		pWin->SetTextColor( aTextColor );
	}

	if ( bBackground )
	{
		if( pWin->IsControlBackground() )
			pWin->SetBackground( pWin->GetControlBackground() );
		else
			pWin->SetBackground( rStyleSettings.GetWindowColor() );
	}
}

//-------------------------------------------------------------------
void BrowserDataWin::Update()
{
	if ( !nUpdateLock )
		Control::Update();
	else
		bUpdateOnUnlock = TRUE;
}

//-------------------------------------------------------------------
void BrowserDataWin::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
		 (rDCEvt.GetFlags() & SETTINGS_STYLE) )
	{
		if( !bOwnDataChangedHdl )
		{
			InitSettings_Impl( this, TRUE, TRUE, TRUE );
			Invalidate();
			InitSettings_Impl( GetParent(), TRUE, TRUE, TRUE );
			GetParent()->Invalidate();
			GetParent()->Resize();
		}
	}
	else
		Control::DataChanged( rDCEvt );
}

//-------------------------------------------------------------------
void BrowserDataWin::Paint( const Rectangle& rRect )
{
	if ( !nUpdateLock && GetUpdateMode() )
	{
		if ( bInPaint )
		{
			aInvalidRegion.Insert( new Rectangle( rRect ) );
			return;
		}
		bInPaint = TRUE;
		( (BrowseBox*) GetParent() )->PaintData( *this, rRect );
		bInPaint = FALSE;
		DoOutstandingInvalidations();
	}
	else
		aInvalidRegion.Insert( new Rectangle( rRect ) );
}

//-------------------------------------------------------------------

BrowseEvent BrowserDataWin::CreateBrowseEvent( const Point& rPosPixel )
{
	BrowseBox *pBox = GetParent();

	// seek to row under mouse
	long nRelRow = rPosPixel.Y() < 0
			? -1
			: rPosPixel.Y() / pBox->GetDataRowHeight();
	long nRow = nRelRow < 0 ? -1 : nRelRow + pBox->nTopRow;

	// find column under mouse
	long nMouseX = rPosPixel.X();
	long nColX = 0;
	USHORT nCol;
	for ( nCol = 0;
		  nCol < pBox->pCols->Count() && nColX < GetSizePixel().Width();
		  ++nCol )
		if ( pBox->pCols->GetObject(nCol)->IsFrozen() || nCol >= pBox->nFirstCol )
		{
			nColX += pBox->pCols->GetObject(nCol)->Width();
			if ( nMouseX < nColX )
				break;
		}
	USHORT nColId = BROWSER_INVALIDID;
	if ( nCol < pBox->pCols->Count() )
		nColId = pBox->pCols->GetObject(nCol)->GetId();

	// compute the field rectangle and field relative MouseEvent
	Rectangle aFieldRect;
	if ( nCol < pBox->pCols->Count() )
	{
		nColX -= pBox->pCols->GetObject(nCol)->Width();
		aFieldRect = Rectangle(
			Point( nColX, nRelRow * pBox->GetDataRowHeight() ),
			Size( pBox->pCols->GetObject(nCol)->Width(),
				  pBox->GetDataRowHeight() ) );
	}

	// assemble and return the BrowseEvent
	return BrowseEvent( this, nRow, nCol, nColId, aFieldRect );
}

//-------------------------------------------------------------------
sal_Int8 BrowserDataWin::AcceptDrop( const AcceptDropEvent& _rEvt )
{
	bCallingDropCallback = sal_True;
	sal_Int8 nReturn = DND_ACTION_NONE;
	nReturn = GetParent()->AcceptDrop( BrowserAcceptDropEvent( this, _rEvt ) );
	bCallingDropCallback = sal_False;
	return nReturn;
}

//-------------------------------------------------------------------
sal_Int8 BrowserDataWin::ExecuteDrop( const ExecuteDropEvent& _rEvt )
{
	bCallingDropCallback = sal_True;
	sal_Int8 nReturn = DND_ACTION_NONE;
	nReturn = GetParent()->ExecuteDrop( BrowserExecuteDropEvent( this, _rEvt ) );
	bCallingDropCallback = sal_False;
	return nReturn;
}

//-------------------------------------------------------------------
void BrowserDataWin::StartDrag( sal_Int8 _nAction, const Point& _rPosPixel )
{
    if ( !GetParent()->bRowDividerDrag )
    {
	    Point aEventPos( _rPosPixel );
	    aEventPos.Y() += GetParent()->GetTitleHeight();
	    GetParent()->StartDrag( _nAction, aEventPos );
    }
}

//-------------------------------------------------------------------
void BrowserDataWin::Command( const CommandEvent& rEvt )
{
	// Scrollmaus-Event?
	BrowseBox *pBox = GetParent();
	if ( ( (rEvt.GetCommand() == COMMAND_WHEEL) ||
		   (rEvt.GetCommand() == COMMAND_STARTAUTOSCROLL) ||
		   (rEvt.GetCommand() == COMMAND_AUTOSCROLL) ) &&
		 ( HandleScrollCommand( rEvt, &pBox->aHScroll, pBox->pVScroll ) ) )
	  return;

	Point aEventPos( rEvt.GetMousePosPixel() );
	long nRow = pBox->GetRowAtYPosPixel( aEventPos.Y(), FALSE);
	MouseEvent aMouseEvt( aEventPos, 1, MOUSE_SELECT, MOUSE_LEFT );
	if ( COMMAND_CONTEXTMENU == rEvt.GetCommand() && rEvt.IsMouseEvent() &&
		 nRow < pBox->GetRowCount() && !pBox->IsRowSelected(nRow) )
	{
		BOOL bDeleted = FALSE;
		pDtorNotify = &bDeleted;
		bInCommand = TRUE;
		MouseButtonDown( aMouseEvt );
		if( bDeleted )
			return;
		MouseButtonUp( aMouseEvt );
		if( bDeleted )
			return;
		pDtorNotify = 0;
		bInCommand = FALSE;
	}

	aEventPos.Y() += GetParent()->GetTitleHeight();
	CommandEvent aEvt( aEventPos, rEvt.GetCommand(),
						rEvt.IsMouseEvent(), rEvt.GetData() );
	bInCommand = TRUE;
	BOOL bDeleted = FALSE;
	pDtorNotify = &bDeleted;
	GetParent()->Command( aEvt );
	if( bDeleted )
		return;
	pDtorNotify = 0;
	bInCommand = FALSE;

	if ( COMMAND_STARTDRAG == rEvt.GetCommand() )
		MouseButtonUp( aMouseEvt );

	Control::Command( rEvt );
}

//-------------------------------------------------------------------

BOOL BrowserDataWin::ImplRowDividerHitTest( const BrowserMouseEvent& _rEvent )
{
    if ( ! (  GetParent()->IsInteractiveRowHeightEnabled()
           && ( _rEvent.GetRow() >= 0 )
           && ( _rEvent.GetRow() < GetParent()->GetRowCount() )
           && ( _rEvent.GetColumnId() == 0 )
           )
       )
       return FALSE;

    long nDividerDistance = GetParent()->GetDataRowHeight() - ( _rEvent.GetPosPixel().Y() % GetParent()->GetDataRowHeight() );
    return ( nDividerDistance <= 4 );
}

//-------------------------------------------------------------------

void BrowserDataWin::MouseButtonDown( const MouseEvent& rEvt )
{
	aLastMousePos = OutputToScreenPixel( rEvt.GetPosPixel() );

    BrowserMouseEvent aBrowserEvent( this, rEvt );
    if ( ( aBrowserEvent.GetClicks() == 1 ) && ImplRowDividerHitTest( aBrowserEvent ) )
    {
        StartRowDividerDrag( aBrowserEvent.GetPosPixel() );
        return;
    }

    GetParent()->MouseButtonDown( BrowserMouseEvent( this, rEvt ) );
}

//-------------------------------------------------------------------

void BrowserDataWin::MouseMove( const MouseEvent& rEvt )
{
	// Pseudo MouseMoves verhindern
	Point aNewPos = OutputToScreenPixel( rEvt.GetPosPixel() );
	if ( ( aNewPos == aLastMousePos ) )
		return;
	aLastMousePos = aNewPos;

	// transform to a BrowseEvent
    BrowserMouseEvent aBrowserEvent( this, rEvt );
	GetParent()->MouseMove( aBrowserEvent );

    // pointer shape
	PointerStyle ePointerStyle = POINTER_ARROW;
    if ( ImplRowDividerHitTest( aBrowserEvent ) )
        ePointerStyle = POINTER_VSIZEBAR;
	SetPointer( Pointer( ePointerStyle ) );

    // dragging out of the visible area?
	if ( rEvt.IsLeft() &&
		 ( rEvt.GetPosPixel().Y() > GetSizePixel().Height() ||
		   rEvt.GetPosPixel().Y() < 0 ) )
	{
		// repeat the event
		aRepeatEvt = rEvt;
		aMouseTimer.Start();
	}
	else
		// killing old repeat-event
		if ( aMouseTimer.IsActive() )
			aMouseTimer.Stop();
}

//-------------------------------------------------------------------

IMPL_LINK_INLINE_START( BrowserDataWin, RepeatedMouseMove, void *, EMPTYARG )
{
	GetParent()->MouseMove( BrowserMouseEvent( this, aRepeatEvt ) );
	return 0;
}
IMPL_LINK_INLINE_END( BrowserDataWin, RepeatedMouseMove, void *, EMPTYARG )

//-------------------------------------------------------------------

void BrowserDataWin::MouseButtonUp( const MouseEvent& rEvt )
{
	// Pseudo MouseMoves verhindern
	Point aNewPos = OutputToScreenPixel( rEvt.GetPosPixel() );
	aLastMousePos = aNewPos;

	// Move an die aktuelle Position simulieren
	MouseMove( rEvt );

	// eigentliches Up-Handling
	ReleaseMouse();
	if ( aMouseTimer.IsActive() )
		aMouseTimer.Stop();
	GetParent()->MouseButtonUp( BrowserMouseEvent( this, rEvt ) );
}

//-------------------------------------------------------------------

void BrowserDataWin::StartRowDividerDrag( const Point& _rStartPos )
{
    long nDataRowHeight = GetParent()->GetDataRowHeight();
    // the exact separation pos of the two rows
    long nDragRowDividerCurrentPos = _rStartPos.Y();
    if ( ( nDragRowDividerCurrentPos % nDataRowHeight ) > nDataRowHeight / 2 )
        nDragRowDividerCurrentPos += nDataRowHeight;
    nDragRowDividerCurrentPos /= nDataRowHeight;
    nDragRowDividerCurrentPos *= nDataRowHeight;

    m_nDragRowDividerOffset = nDragRowDividerCurrentPos - _rStartPos.Y();

    m_nDragRowDividerLimit = nDragRowDividerCurrentPos - nDataRowHeight;

    GetParent()->bRowDividerDrag = TRUE;
    GetParent()->ImplStartTracking();

	Rectangle aDragSplitRect( 0, m_nDragRowDividerLimit, GetOutputSizePixel().Width(), nDragRowDividerCurrentPos );
	ShowTracking( aDragSplitRect, SHOWTRACK_SMALL );

    StartTracking();
}

//-------------------------------------------------------------------

void BrowserDataWin::Tracking( const TrackingEvent& rTEvt )
{
    if ( !GetParent()->bRowDividerDrag )
        return;

	Point aMousePos = rTEvt.GetMouseEvent().GetPosPixel();
    // stop resizing at our bottom line
    if ( aMousePos.Y() > GetOutputSizePixel().Height() )
        aMousePos.Y() = GetOutputSizePixel().Height();

	if ( rTEvt.IsTrackingEnded() )
    {
    	HideTracking();
        GetParent()->bRowDividerDrag = FALSE;
        GetParent()->ImplEndTracking();

        if ( !rTEvt.IsTrackingCanceled() )
        {
            long nNewRowHeight = aMousePos.Y() + m_nDragRowDividerOffset - m_nDragRowDividerLimit;

            // care for minimum row height
            if ( nNewRowHeight < GetParent()->QueryMinimumRowHeight() )
                nNewRowHeight = GetParent()->QueryMinimumRowHeight();

            GetParent()->SetDataRowHeight( nNewRowHeight );
            GetParent()->RowHeightChanged();
        }
    }
	else
    {
        GetParent()->ImplTracking();

        long nDragRowDividerCurrentPos = aMousePos.Y() + m_nDragRowDividerOffset;

        // care for minimum row height
        if ( nDragRowDividerCurrentPos < m_nDragRowDividerLimit + GetParent()->QueryMinimumRowHeight() )
            nDragRowDividerCurrentPos = m_nDragRowDividerLimit + GetParent()->QueryMinimumRowHeight();

        Rectangle aDragSplitRect( 0, m_nDragRowDividerLimit, GetOutputSizePixel().Width(), nDragRowDividerCurrentPos );
        ShowTracking( aDragSplitRect, SHOWTRACK_SMALL );
    }
}

//-------------------------------------------------------------------

void BrowserDataWin::KeyInput( const KeyEvent& rEvt )
{
	// pass to parent window
	if ( !GetParent()->ProcessKey( rEvt ) )
		Control::KeyInput( rEvt );
}

//-------------------------------------------------------------------

void BrowserDataWin::RequestHelp( const HelpEvent& rHEvt )
{
	pEventWin = this;
	GetParent()->RequestHelp( rHEvt );
	pEventWin = GetParent();
}

//===================================================================

BrowseEvent::BrowseEvent( Window* pWindow,
						  long nAbsRow, USHORT nColumn, USHORT nColumnId,
						  const Rectangle& rRect ):
	pWin(pWindow),
	nRow(nAbsRow),
	aRect(rRect),
	nCol(nColumn),
	nColId(nColumnId)
{
}

//===================================================================
BrowserMouseEvent::BrowserMouseEvent( BrowserDataWin *pWindow,
						  const MouseEvent& rEvt ):
	MouseEvent(rEvt),
	BrowseEvent( pWindow->CreateBrowseEvent( rEvt.GetPosPixel() ) )
{
}

//-------------------------------------------------------------------

BrowserMouseEvent::BrowserMouseEvent( Window *pWindow, const MouseEvent& rEvt,
						  long nAbsRow, USHORT nColumn, USHORT nColumnId,
						  const Rectangle& rRect ):
	MouseEvent(rEvt),
	BrowseEvent( pWindow, nAbsRow, nColumn, nColumnId, rRect )
{
}

//===================================================================

BrowserAcceptDropEvent::BrowserAcceptDropEvent( BrowserDataWin *pWindow, const AcceptDropEvent& rEvt )
	:AcceptDropEvent(rEvt)
	,BrowseEvent( pWindow->CreateBrowseEvent( rEvt.maPosPixel ) )
{
}

//===================================================================

BrowserExecuteDropEvent::BrowserExecuteDropEvent( BrowserDataWin *pWindow, const ExecuteDropEvent& rEvt )
	:ExecuteDropEvent(rEvt)
	,BrowseEvent( pWindow->CreateBrowseEvent( rEvt.maPosPixel ) )
{
}

//===================================================================

//-------------------------------------------------------------------

void BrowserDataWin::SetUpdateMode( BOOL bMode )
{
	DBG_ASSERT( !bUpdateMode || aInvalidRegion.Count() == 0,
				"invalid region not empty" );
	if ( bMode == bUpdateMode )
		return;

	bUpdateMode = bMode;
	if ( bMode )
		DoOutstandingInvalidations();
}

//-------------------------------------------------------------------
void BrowserDataWin::DoOutstandingInvalidations()
{
	for ( Rectangle* pRect = aInvalidRegion.First();
		  pRect;
		  pRect = aInvalidRegion.Next() )
	{
		Control::Invalidate( *pRect );
		delete pRect;
	}
	aInvalidRegion.Clear();
}

//-------------------------------------------------------------------

void BrowserDataWin::Invalidate( USHORT nFlags )
{
	if ( !GetUpdateMode() )
	{
		for ( Rectangle* pRect = aInvalidRegion.First();
			  pRect;
			  pRect = aInvalidRegion.Next() )
			delete pRect;
		aInvalidRegion.Clear();
		aInvalidRegion.Insert( new Rectangle( Point( 0, 0 ), GetOutputSizePixel() ) );
	}
	else
		Window::Invalidate( nFlags );
}

//-------------------------------------------------------------------

void BrowserDataWin::Invalidate( const Rectangle& rRect, USHORT nFlags )
{
	if ( !GetUpdateMode() )
		aInvalidRegion.Insert( new Rectangle( rRect ) );
	else
		Window::Invalidate( rRect, nFlags );
}

//===================================================================

void BrowserScrollBar::Tracking( const TrackingEvent& rTEvt )
{
	ULONG nPos = GetThumbPos();
	if ( nPos != _nLastPos )
	{
		if ( _nTip )
			Help::HideTip( _nTip );

		String aTip( String::CreateFromInt32(nPos) );
		aTip += '/';
		if ( _pDataWin->GetRealRowCount().Len() )
			aTip += _pDataWin->GetRealRowCount();
		else
			aTip += String::CreateFromInt32(GetRangeMax());
		Rectangle aRect( GetPointerPosPixel(), Size( GetTextHeight(), GetTextWidth( aTip ) ) );
		_nTip = Help::ShowTip( this, aRect, aTip );
		_nLastPos = nPos;
	}

	ScrollBar::Tracking( rTEvt );
}

//-------------------------------------------------------------------

void BrowserScrollBar::EndScroll()
{
	if ( _nTip )
		Help::HideTip( _nTip );
	_nTip = 0;
	ScrollBar::EndScroll();
}


