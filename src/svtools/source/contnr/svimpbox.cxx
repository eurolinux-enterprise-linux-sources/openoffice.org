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
#include <vcl/svapp.hxx>
#include <vcl/salnativewidgets.hxx>

#ifndef _HELP_HXX
#include <vcl/help.hxx>
#endif
#include <tabbar.hxx>

#ifndef _STACK_
#include <stack>
#endif

#define _SVTREEBX_CXX
#include <svtools/svtreebx.hxx>
#include <svtools/svlbox.hxx>
#include <svimpbox.hxx>
#include <rtl/instance.hxx>
#include <svtools/svtdata.hxx>

#ifndef _SVTOOLS_HRC
#include <svtools/svtools.hrc>
#endif

// #102891# --------------------
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif

#define NODE_BMP_TABDIST_NOTVALID 	-2000000
#define FIRST_ENTRY_TAB				1

// #i27063# (pl), #i32300# (pb) never access VCL after DeInitVCL - also no destructors
Image*	SvImpLBox::s_pDefCollapsed		= NULL;
Image*	SvImpLBox::s_pDefExpanded		= NULL;
Image*	SvImpLBox::s_pDefCollapsedHC	= NULL;
Image*	SvImpLBox::s_pDefExpandedHC		= NULL;
sal_Int32 SvImpLBox::s_nImageRefCount	= 0;

SvImpLBox::SvImpLBox( SvTreeListBox* pLBView, SvLBoxTreeList* pLBTree, WinBits nWinStyle) :

	pTabBar( NULL ),
	aVerSBar( pLBView, WB_DRAG | WB_VSCROLL ),
	aHorSBar( pLBView, WB_DRAG | WB_HSCROLL ),
	aScrBarBox( pLBView ),
	aOutputSize( 0, 0 ),
	aSelEng( pLBView, (FunctionSet*)0 ),
	aFctSet( this, &aSelEng, pLBView ),
    nExtendedWinBits( 0 ),
    bAreChildrenTransient( sal_True ),
	pIntlWrapper( NULL ) // #102891# -----------------------
{
	osl_incrementInterlockedCount(&s_nImageRefCount);
	pView = pLBView;
	pTree = pLBTree;
	aSelEng.SetFunctionSet( (FunctionSet*)&aFctSet );
	aSelEng.ExpandSelectionOnMouseMove( FALSE );
	SetWindowBits( nWinStyle );
	SetSelectionMode( SINGLE_SELECTION );
	SetDragDropMode( 0 );

	aVerSBar.SetScrollHdl( LINK( this, SvImpLBox, ScrollUpDownHdl ) );
	aHorSBar.SetScrollHdl( LINK( this, SvImpLBox, ScrollLeftRightHdl ) );
	aHorSBar.SetEndScrollHdl( LINK( this, SvImpLBox, EndScrollHdl ) );
	aVerSBar.SetEndScrollHdl( LINK( this, SvImpLBox, EndScrollHdl ) );
	aVerSBar.SetRange( Range(0,0) );
	aVerSBar.Hide();
	aHorSBar.SetRange( Range(0,0) );
	aHorSBar.SetPageSize( 24 ); // Pixel
	aHorSBar.SetLineSize( 8 ); // Pixel

	nHorSBarHeight = (short)aHorSBar.GetSizePixel().Height();
	nVerSBarWidth = (short)aVerSBar.GetSizePixel().Width();

	pStartEntry	= 0;
	pCursor 			= 0;
	pAnchor				= 0;
	nVisibleCount  		= 0; 	// Anzahl Daten-Zeilen im Control
	nNodeBmpTabDistance = NODE_BMP_TABDIST_NOTVALID;
	nYoffsNodeBmp 		= 0;
	nNodeBmpWidth		= 0;

	bAsyncBeginDrag		= FALSE;
	aAsyncBeginDragTimer.SetTimeout( 0 );
	aAsyncBeginDragTimer.SetTimeoutHdl( LINK(this,SvImpLBox,BeginDragHdl));
	// Button-Animation in Listbox
	pActiveButton = 0;
	pActiveEntry = 0;
	pActiveTab = 0;

	nFlags = 0;
	nCurTabPos = FIRST_ENTRY_TAB;

	aEditTimer.SetTimeout( 800 );
	aEditTimer.SetTimeoutHdl( LINK(this,SvImpLBox,EditTimerCall) );

	nMostRight = -1;
	pMostRightEntry = 0;
	nCurUserEvent = 0xffffffff;

	bUpdateMode = TRUE;
	bInVScrollHdl = FALSE;
	nFlags |= F_FILLING;

	bSubLstOpRet = bSubLstOpLR = bContextMenuHandling = bIsCellFocusEnabled = FALSE;
}

SvImpLBox::~SvImpLBox()
{
	aEditTimer.Stop();
	StopUserEvent();

	// #102891# ---------------------
	if( pIntlWrapper )
		delete pIntlWrapper;
	if ( osl_decrementInterlockedCount(&s_nImageRefCount) == 0 )
	{
		DELETEZ(s_pDefCollapsed);
		DELETEZ(s_pDefExpanded);
		DELETEZ(s_pDefCollapsedHC);
		DELETEZ(s_pDefExpandedHC);
	}
}

// #102891# --------------------
void SvImpLBox::UpdateIntlWrapper()
{
	const ::com::sun::star::lang::Locale & aNewLocale = Application::GetSettings().GetLocale();
	if( !pIntlWrapper )
		pIntlWrapper = new IntlWrapper( ::comphelper::getProcessServiceFactory(), aNewLocale );
	else
	{
		const ::com::sun::star::lang::Locale &aLocale = pIntlWrapper->getLocale();
		if( aLocale.Language != aNewLocale.Language || // different Locale from the older one
			aLocale.Country != aNewLocale.Country ||
			aLocale.Variant != aNewLocale.Variant )
		{
			delete pIntlWrapper;
			pIntlWrapper = new IntlWrapper( ::comphelper::getProcessServiceFactory(), aNewLocale );
		}
	}
}

// #97680# ----------------------
short SvImpLBox::UpdateContextBmpWidthVector( SvLBoxEntry* pEntry, short nWidth )
{
	DBG_ASSERT( pView->pModel, "View and Model aren't valid!" );

	USHORT nDepth = pView->pModel->GetDepth( pEntry );
	// initialize vector if necessary
	std::vector< short >::size_type nSize = aContextBmpWidthVector.size();
	while ( nDepth > nSize )
	{
		aContextBmpWidthVector.resize( nSize + 1 );
		aContextBmpWidthVector.at( nSize ) = nWidth;
		++nSize;
	}
	if( aContextBmpWidthVector.size() == nDepth )
	{
		aContextBmpWidthVector.resize( nDepth + 1 );
		aContextBmpWidthVector.at( nDepth ) = 0;
	}
	short nContextBmpWidth = aContextBmpWidthVector[ nDepth ];
	if( nContextBmpWidth < nWidth )
	{
		aContextBmpWidthVector.at( nDepth ) = nWidth;
		return nWidth;
	}
	else
		return nContextBmpWidth;
}

void SvImpLBox::UpdateContextBmpWidthVectorFromMovedEntry( SvLBoxEntry* pEntry )
{
	DBG_ASSERT( pEntry, "Moved Entry is invalid!" );

	SvLBoxContextBmp* pBmpItem = static_cast< SvLBoxContextBmp* >( pEntry->GetFirstItem( SV_ITEM_ID_LBOXCONTEXTBMP ) );
	short nExpWidth = (short)pBmpItem->GetBitmap1().GetSizePixel().Width();
	short nColWidth = (short)pBmpItem->GetBitmap2().GetSizePixel().Width();
	short nMax = Max(nExpWidth, nColWidth);
	UpdateContextBmpWidthVector( pEntry, nMax );

	if( pEntry->HasChilds() ) // recursive call, whether expanded or not
	{
		SvLBoxEntry* pChild = pView->FirstChild( pEntry );
		DBG_ASSERT( pChild, "The first child is invalid!" );
		do
		{
			UpdateContextBmpWidthVectorFromMovedEntry( pChild );
			pChild = pView->Next( pChild );
		} while ( pChild );
	}
}

void SvImpLBox::UpdateContextBmpWidthMax( SvLBoxEntry* pEntry )
{
	USHORT nDepth = pView->pModel->GetDepth( pEntry );
	if( aContextBmpWidthVector.size() < 1 )
		return;
	short nWidth = aContextBmpWidthVector[ nDepth ];
	if( nWidth != pView->nContextBmpWidthMax ) {
		pView->nContextBmpWidthMax = nWidth;
		nFlags |= F_IGNORE_CHANGED_TABS;
		pView->SetTabs();
		nFlags &= ~F_IGNORE_CHANGED_TABS;
	}
}

void SvImpLBox::CalcCellFocusRect( SvLBoxEntry* pEntry, Rectangle& rRect )
{
	if ( pEntry && bIsCellFocusEnabled )
	{
		if ( nCurTabPos > FIRST_ENTRY_TAB )
		{
			SvLBoxItem* pItem = pCursor->GetItem( nCurTabPos );
			rRect.Left() = pView->GetTab( pCursor, pItem )->GetPos();
		}
        if ( pCursor->ItemCount() > ( nCurTabPos + 1 ) )
        {
            SvLBoxItem* pNextItem = pCursor->GetItem( nCurTabPos + 1 );
            long nRight = pView->GetTab( pCursor, pNextItem )->GetPos() - 1;
            if ( nRight < rRect.Right() )
                rRect.Right() = nRight;
        }
	}
}

void SvImpLBox::SetWindowBits( WinBits nWinStyle )
{
	nWinBits = nWinStyle;
	if((nWinStyle & WB_SIMPLEMODE) && aSelEng.GetSelectionMode()==MULTIPLE_SELECTION)
		aSelEng.AddAlways( TRUE );
}

void SvImpLBox::SetExtendedWindowBits( ExtendedWinBits _nBits )
{
	nExtendedWinBits = _nBits;
}

// Das Model darf hier nicht mehr angefasst werden
void SvImpLBox::Clear()
{
	StopUserEvent();
	pStartEntry	= 0;
	pAnchor = 0;

	pActiveButton = 0;
	pActiveEntry = 0;
	pActiveTab = 0;

	nMostRight = -1;
	pMostRightEntry = 0;

	// Der Cursor darf hier nicht mehr angefasst werden!
	if( pCursor )
	{
		if( pView->HasFocus() )
			pView->HideFocus();
		pCursor = 0;
	}
	aVerSBar.Hide();
	aVerSBar.SetThumbPos( 0 );
	Range aRange( 0, 0 );
	aVerSBar.SetRange( aRange );
	aOutputSize = pView->Control::GetOutputSizePixel();
	nFlags &= ~(F_VER_SBARSIZE_WITH_HBAR | F_HOR_SBARSIZE_WITH_VBAR );
	if( pTabBar )
	{
		aOutputSize.Height() -= nHorSBarHeight;
		nFlags |= F_VER_SBARSIZE_WITH_HBAR;
	}
	if( !pTabBar )
		aHorSBar.Hide();
	aHorSBar.SetThumbPos( 0 );
	MapMode aMapMode( pView->GetMapMode());
	aMapMode.SetOrigin( Point(0,0) );
	pView->Control::SetMapMode( aMapMode );
	aHorSBar.SetRange( aRange );
	aHorSBar.SetSizePixel(Size(aOutputSize.Width(),nHorSBarHeight));
	pView->SetClipRegion();
	if( GetUpdateMode() )
		pView->Invalidate( GetVisibleArea() );
	nFlags |= F_FILLING;
	if( !aHorSBar.IsVisible() && !aVerSBar.IsVisible() )
		aScrBarBox.Hide();

	// #97680# ---------
	aContextBmpWidthVector.clear();
}

// *********************************************************************
// Painten, Navigieren, Scrollen
// *********************************************************************

IMPL_LINK_INLINE_START( SvImpLBox, EndScrollHdl, ScrollBar *, EMPTYARG )
{
	if( nFlags & F_ENDSCROLL_SET_VIS_SIZE )
	{
		aVerSBar.SetVisibleSize( nNextVerVisSize );
		nFlags &= ~F_ENDSCROLL_SET_VIS_SIZE;
	}
	EndScroll();
	return 0;
}
IMPL_LINK_INLINE_END( SvImpLBox, EndScrollHdl, ScrollBar *, pScrollBar )


// Handler vertikale ScrollBar

IMPL_LINK( SvImpLBox, ScrollUpDownHdl, ScrollBar *, pScrollBar )
{
	DBG_ASSERT(!bInVScrollHdl,"Scroll-Handler ueberholt sich!");
	long nDelta = pScrollBar->GetDelta();
	if( !nDelta )
		return 0;

	nFlags &= (~F_FILLING);

	bInVScrollHdl = TRUE;

	if( pView->IsEditingActive() )
	{
		pView->EndEditing( TRUE ); // Cancel
		pView->Update();
	}
	BeginScroll();

	if( nDelta > 0 )
	{
		if( nDelta == 1 )
			CursorDown();
		else
			PageDown( (USHORT) nDelta );
	}
	else
	{
		nDelta *= (-1);
		if( nDelta == 1 )
			CursorUp();
		else
			PageUp( (USHORT) nDelta );
	}
	bInVScrollHdl = FALSE;
	return 0;
}


void SvImpLBox::CursorDown()
{
	SvLBoxEntry* pNextFirstToDraw = (SvLBoxEntry*)(pView->NextVisible( pStartEntry));
	if( pNextFirstToDraw )
	{
		nFlags &= (~F_FILLING);
		pView->NotifyScrolling( -1 );
		ShowCursor( FALSE );
		pView->Update();
		pStartEntry = pNextFirstToDraw;
		Rectangle aArea( GetVisibleArea() );
		pView->Scroll( 0, -(pView->GetEntryHeight()), aArea, SCROLL_NOCHILDREN );
		pView->Update();
		ShowCursor( TRUE );
		pView->NotifyScrolled();
	}
}

void SvImpLBox::CursorUp()
{
	SvLBoxEntry* pPrevFirstToDraw = (SvLBoxEntry*)(pView->PrevVisible( pStartEntry));
	if( pPrevFirstToDraw )
	{
		nFlags &= (~F_FILLING);
		long nEntryHeight = pView->GetEntryHeight();
		pView->NotifyScrolling( 1 );
		ShowCursor( FALSE );
		pView->Update();
		pStartEntry = pPrevFirstToDraw;
		Rectangle aArea( GetVisibleArea() );
		aArea.Bottom() -= nEntryHeight;
		pView->Scroll( 0, nEntryHeight, aArea, SCROLL_NOCHILDREN );
		pView->Update();
		ShowCursor( TRUE );
		pView->NotifyScrolled();
	}
}

void SvImpLBox::PageDown( USHORT nDelta )
{
	USHORT nRealDelta = nDelta;

	if( !nDelta )
		return;

	SvLBoxEntry* pNext;
	pNext = (SvLBoxEntry*)(pView->NextVisible( pStartEntry, nRealDelta ));
	if( (ULONG)pNext == (ULONG)pStartEntry )
		return;

	ShowCursor( FALSE );

	nFlags &= (~F_FILLING);
	pView->Update();
	pStartEntry = pNext;

	if( nRealDelta >= nVisibleCount )
	{
		pView->Invalidate( GetVisibleArea() );
		pView->Update();
	}
	else
	{
		long nScroll = nRealDelta * (-1);
		pView->NotifyScrolling( nScroll );
		Rectangle aArea( GetVisibleArea() );
		nScroll = pView->GetEntryHeight()*nRealDelta;
		nScroll = -nScroll;
		pView->Update();
		pView->Scroll( 0, nScroll, aArea, SCROLL_NOCHILDREN );
		pView->Update();
		pView->NotifyScrolled();
	}

	ShowCursor( TRUE );
}

void SvImpLBox::PageUp( USHORT nDelta )
{
	USHORT nRealDelta = nDelta;
	if( !nDelta )
		return;

	SvLBoxEntry* pPrev = (SvLBoxEntry*)(pView->PrevVisible( pStartEntry, nRealDelta ));
	if( (ULONG)pPrev == (ULONG)pStartEntry )
		return;

	nFlags &= (~F_FILLING);
	ShowCursor( FALSE );

	pView->Update();
	pStartEntry = pPrev;
	if( nRealDelta >= nVisibleCount )
	{
		pView->Invalidate( GetVisibleArea() );
		pView->Update();
	}
	else
	{
		long nEntryHeight = pView->GetEntryHeight();
		pView->NotifyScrolling( (long)nRealDelta );
		Rectangle aArea( GetVisibleArea() );
		pView->Update();
		pView->Scroll( 0, nEntryHeight*nRealDelta, aArea, SCROLL_NOCHILDREN );
		pView->Update();
		pView->NotifyScrolled();
	}

	ShowCursor( TRUE );
}

void SvImpLBox::KeyUp( BOOL bPageUp, BOOL bNotifyScroll )
{
	if( !aVerSBar.IsVisible() )
		return;

	long nDelta;
	if( bPageUp )
		nDelta = aVerSBar.GetPageSize();
	else
		nDelta = 1;

	long nThumbPos = aVerSBar.GetThumbPos();

	if( nThumbPos < nDelta )
		nDelta = nThumbPos;

	if( nDelta <= 0 )
		return;

	nFlags &= (~F_FILLING);
	if( bNotifyScroll )
		BeginScroll();

	aVerSBar.SetThumbPos( nThumbPos - nDelta );
	if( bPageUp )
		PageUp( (short)nDelta );
	else
		CursorUp();

	if( bNotifyScroll )
		EndScroll();
}


void SvImpLBox::KeyDown( BOOL bPageDown, BOOL bNotifyScroll )
{
	if( !aVerSBar.IsVisible() )
		return;

	long nDelta;
	if( bPageDown )
		nDelta = aVerSBar.GetPageSize();
	else
		nDelta = 1;

	long nThumbPos = aVerSBar.GetThumbPos();
	long nVisibleSize = aVerSBar.GetVisibleSize();
	long nRange = aVerSBar.GetRange().Len();

	long nTmp = nThumbPos+nVisibleSize;
	while( (nDelta > 0) && (nTmp+nDelta) >= nRange )
		nDelta--;

	if( nDelta <= 0 )
		return;

	nFlags &= (~F_FILLING);
	if( bNotifyScroll )
		BeginScroll();

	aVerSBar.SetThumbPos( nThumbPos+nDelta );
	if( bPageDown )
		PageDown( (short)nDelta );
	else
		CursorDown();

	if( bNotifyScroll )
		EndScroll();
}



void SvImpLBox::InvalidateEntriesFrom( long nY ) const
{
	if( !(nFlags & F_IN_PAINT ))
	{
		Rectangle aRect( GetVisibleArea() );
		aRect.Top() = nY;
		pView->Invalidate( aRect );
	}
}

void SvImpLBox::InvalidateEntry( long nY ) const
{
	if( !(nFlags & F_IN_PAINT ))
	{
		Rectangle aRect( GetVisibleArea() );
		long nMaxBottom = aRect.Bottom();
		aRect.Top() = nY;
		aRect.Bottom() = nY; aRect.Bottom() += pView->GetEntryHeight();
		if( aRect.Top() > nMaxBottom )
			return;
		if( aRect.Bottom() > nMaxBottom )
			aRect.Bottom() = nMaxBottom;
		pView->Invalidate( aRect );
	}
}

void SvImpLBox::InvalidateEntry( SvLBoxEntry* pEntry )
{
	if( GetUpdateMode() )
	{
		long nPrev = nMostRight;
		SetMostRight( pEntry );
		if( nPrev < nMostRight )
			ShowVerSBar();
	}
	if( !(nFlags & F_IN_PAINT ))
	{
		BOOL bHasFocusRect = FALSE;
		if( pEntry==pCursor && pView->HasFocus() )
		{
			bHasFocusRect = TRUE;
			ShowCursor( FALSE );
		}
		InvalidateEntry( GetEntryLine( pEntry ) );
		if( bHasFocusRect )
			ShowCursor( TRUE );
	}
}


void SvImpLBox::RecalcFocusRect()
{
	if( pView->HasFocus() && pCursor )
	{
		pView->HideFocus();
		long nY = GetEntryLine( pCursor );
		Rectangle aRect = pView->GetFocusRect( pCursor, nY );
		CalcCellFocusRect( pCursor, aRect );
		Region aOldClip( pView->GetClipRegion());
		Region aClipRegion( GetClipRegionRect() );
		pView->SetClipRegion( aClipRegion );
		pView->ShowFocus( aRect );
		pView->SetClipRegion( aOldClip );
	}
}

//
//  Setzt Cursor. Passt bei SingleSelection die Selektion an
//

void SvImpLBox::SetCursor( SvLBoxEntry* pEntry, BOOL bForceNoSelect )
{
	SvViewDataEntry* pViewDataNewCur = 0;
	if( pEntry )
		pViewDataNewCur= pView->GetViewDataEntry(pEntry);
	if( pEntry &&
		pEntry == pCursor &&
		pViewDataNewCur->HasFocus() &&
		pViewDataNewCur->IsSelected())
	{
		return;
	}

	// if this cursor is not selectable, find first visible that is and use it
	while( pEntry && pViewDataNewCur && !pViewDataNewCur->IsSelectable() )
	{
		pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
		pViewDataNewCur = pEntry ? pView->GetViewDataEntry(pEntry) : 0;
	}

	SvLBoxEntry* pOldCursor = pCursor;
	if( pCursor && pEntry != pCursor )
	{
		pView->SetEntryFocus( pCursor, FALSE );
		if( bSimpleTravel )
			pView->Select( pCursor, FALSE );
		pView->HideFocus();
	}
	pCursor = pEntry;
	if( pCursor )
	{
		pViewDataNewCur->SetFocus( TRUE );
		if(!bForceNoSelect && bSimpleTravel && !(nFlags & F_DESEL_ALL) && GetUpdateMode())
		{
			pView->Select( pCursor, TRUE );
		}
		// Mehrfachselektion: Im Cursor-Move selektieren, wenn
		// nicht im Add-Mode (Ctrl-F8)
		else if( GetUpdateMode() &&
				 pView->GetSelectionMode() == MULTIPLE_SELECTION &&
				 !(nFlags & F_DESEL_ALL) && !aSelEng.IsAddMode() &&
				 !bForceNoSelect )
		{
			pView->Select( pCursor, TRUE );
		}
		else
		{
			ShowCursor( TRUE );
		}

		if( pAnchor )
		{
			DBG_ASSERT(aSelEng.GetSelectionMode() != SINGLE_SELECTION,"Mode?");
			SetAnchorSelection( pOldCursor, pCursor );
		}
	}
	nFlags &= (~F_DESEL_ALL);
}

void SvImpLBox::ShowCursor( BOOL bShow )
{
	if( !bShow || !pCursor || !pView->HasFocus() )
	{
		Region aOldClip( pView->GetClipRegion());
		Region aClipRegion( GetClipRegionRect() );
		pView->SetClipRegion( aClipRegion );
		pView->HideFocus();
		pView->SetClipRegion( aOldClip );
	}
	else
	{
		long nY = GetEntryLine( pCursor );
		Rectangle aRect = pView->GetFocusRect( pCursor, nY );
		CalcCellFocusRect( pCursor, aRect );
		Region aOldClip( pView->GetClipRegion());
		Region aClipRegion( GetClipRegionRect() );
		pView->SetClipRegion( aClipRegion );
		pView->ShowFocus( aRect );
		pView->SetClipRegion( aOldClip );
	}
}



void SvImpLBox::UpdateAll( BOOL bInvalidateCompleteView,
	BOOL bUpdateVerScrollBar )
{
	if( bUpdateVerScrollBar )
		FindMostRight(0);
	aVerSBar.SetRange( Range(0, pView->GetVisibleCount()-1 ) );
	SyncVerThumb();
	FillView();
	ShowVerSBar();
	if( bSimpleTravel && pCursor && pView->HasFocus() )
		pView->Select( pCursor, TRUE );
	ShowCursor( TRUE );
	if( bInvalidateCompleteView )
		pView->Invalidate();
	else
		pView->Invalidate( GetVisibleArea() );
}

IMPL_LINK_INLINE_START( SvImpLBox, ScrollLeftRightHdl, ScrollBar *, pScrollBar )
{
	long nDelta = pScrollBar->GetDelta();
	if( nDelta )
	{
		if( pView->IsEditingActive() )
		{
			pView->EndEditing( TRUE ); // Cancel
			pView->Update();
		}
		pView->nFocusWidth = -1;
		KeyLeftRight( nDelta );
	}
	return 0;
}
IMPL_LINK_INLINE_END( SvImpLBox, ScrollLeftRightHdl, ScrollBar *, pScrollBar )

void SvImpLBox::KeyLeftRight( long nDelta )
{
	if( !(nFlags & F_IN_RESIZE) )
		pView->Update();
	BeginScroll();
	nFlags &= (~F_FILLING);
	pView->NotifyScrolling( 0 ); // 0 == horizontales Scrolling
	ShowCursor( FALSE );

	// neuen Origin berechnen
	long nPos = aHorSBar.GetThumbPos();
	Point aOrigin( -nPos, 0 );

	MapMode aMapMode( pView->GetMapMode() );
	aMapMode.SetOrigin( aOrigin );
	pView->SetMapMode( aMapMode );

	if( !(nFlags & F_IN_RESIZE) )
	{
		Rectangle aRect( GetVisibleArea() );
		pView->Scroll( -nDelta, 0, aRect, SCROLL_NOCHILDREN );
	}
	else
		pView->Invalidate();
	RecalcFocusRect();
	ShowCursor( TRUE );
	pView->NotifyScrolled();
}


// gibt letzten Eintrag zurueck, wenn Position unter
// dem letzten Eintrag ist
SvLBoxEntry* SvImpLBox::GetClickedEntry( const Point& rPoint ) const
{
    DBG_ASSERT( pView->GetModel(), "SvImpLBox::GetClickedEntry: how can this ever happen? Please tell me (frank.schoenheit@sun.com) how to reproduce!" );
    if ( !pView->GetModel() )
        // this is quite impossible. Nevertheless, stack traces from the crash reporter
        // suggest it isn't. Okay, make it safe, and wait for somebody to reproduce it
        // reliably :-\ ....
        // #122359# / 2005-05-23 / frank.schoenheit@sun.com
        return NULL;
    if( pView->GetEntryCount() == 0 || !pStartEntry || !pView->GetEntryHeight())
		return 0;

	USHORT nClickedEntry = (USHORT)(rPoint.Y() / pView->GetEntryHeight() );
	USHORT nTemp = nClickedEntry;
	SvLBoxEntry* pEntry = (SvLBoxEntry*)(pView->NextVisible( pStartEntry, nTemp ));
	return pEntry;
}

//
//  prueft, ob der Eintrag "richtig" getroffen wurde
//  (Focusrect+ ContextBitmap bei TreeListBox)
//
BOOL SvImpLBox::EntryReallyHit(SvLBoxEntry* pEntry,const Point& rPosPixel,long nLine)
{
	BOOL bRet;
	// bei "besonderen" Entries (mit CheckButtons usw.) sind wir
	// nicht so pingelig
	if( pEntry->ItemCount() >= 3 )
		return TRUE;

	Rectangle aRect( pView->GetFocusRect( pEntry, nLine ));
	aRect.Right() = GetOutputSize().Width() - pView->GetMapMode().GetOrigin().X();
	if( pView->IsA() == SV_LISTBOX_ID_TREEBOX )
	{
		SvLBoxContextBmp* pBmp = (SvLBoxContextBmp*)(pEntry->GetFirstItem(SV_ITEM_ID_LBOXCONTEXTBMP));
		aRect.Left() -= pBmp->GetSize(pView,pEntry).Width();
		aRect.Left() -= 4; // etwas Speilraum lassen
	}
	Point aPos( rPosPixel );
	aPos -= pView->GetMapMode().GetOrigin();
	if( aRect.IsInside( aPos ) )
		bRet = TRUE;
	else
		bRet = FALSE;
	return bRet;
}


// gibt 0 zurueck, wenn Position unter dem letzten Eintrag ist
SvLBoxEntry* SvImpLBox::GetEntry( const Point& rPoint ) const
{
	if( (pView->GetEntryCount() == 0) || !pStartEntry ||
        (rPoint.Y() > aOutputSize.Height())
        || !pView->GetEntryHeight())
		return 0;

	USHORT nClickedEntry = (USHORT)(rPoint.Y() / pView->GetEntryHeight() );
	USHORT nTemp = nClickedEntry;
	SvLBoxEntry* pEntry = (SvLBoxEntry*)(pView->NextVisible( pStartEntry, nTemp ));
	if( nTemp != nClickedEntry )
		pEntry = 0;
	return pEntry;
}


SvLBoxEntry* SvImpLBox::MakePointVisible(const Point& rPoint,BOOL bNotifyScroll)
{
	if( !pCursor )
		return 0;
	long nY = rPoint.Y();
	SvLBoxEntry* pEntry = 0;
	long nMax = aOutputSize.Height();
	if( nY < 0 || nY >= nMax ) // aOutputSize.Height() )
	{
		if( nY < 0 )
			pEntry = (SvLBoxEntry*)(pView->PrevVisible( pCursor ));
		else
			pEntry = (SvLBoxEntry*)(pView->NextVisible( pCursor ));

		if( pEntry && pEntry != pCursor )
			pView->SetEntryFocus( pCursor, FALSE );

		if( nY < 0 )
			KeyUp( FALSE, bNotifyScroll );
		else
			KeyDown( FALSE, bNotifyScroll );
	}
	else
	{
		pEntry = GetClickedEntry( rPoint );
		if( !pEntry )
		{
			USHORT nSteps = 0xFFFF;
			// LastVisible ist noch nicht implementiert!
			pEntry = (SvLBoxEntry*)(pView->NextVisible( pStartEntry, nSteps ));
		}
		if( pEntry )
		{
			if( pEntry != pCursor &&
				 aSelEng.GetSelectionMode() == SINGLE_SELECTION
			)
				pView->Select( pCursor, FALSE );
		}
	}
	return pEntry;
}

Rectangle SvImpLBox::GetClipRegionRect() const
{
	Point aOrigin( pView->GetMapMode().GetOrigin() );
	aOrigin.X() *= -1; // Umrechnung Dokumentkoord.
	Rectangle aClipRect( aOrigin, aOutputSize );
	aClipRect.Bottom()++;
	return aClipRect;
}


void SvImpLBox::Paint( const Rectangle& rRect )
{
	if( !pView->GetVisibleCount() )
		return;

	nFlags |= F_IN_PAINT;

	if( nFlags & F_FILLING )
	{
		SvLBoxEntry* pFirst = pView->First();
		if( pFirst != pStartEntry )
		{
			ShowCursor( FALSE );
			pStartEntry = pView->First();
			aVerSBar.SetThumbPos( 0 );
			StopUserEvent();
			ShowCursor( TRUE );
			nCurUserEvent = Application::PostUserEvent(LINK(this,SvImpLBox,MyUserEvent),(void*)1);
			return;
		}
	}

	if( !pStartEntry )
	{
		pStartEntry = pView->First();
	}

#ifdef XX_OV
	ULONG nXAbsPos = (USHORT)pTree->GetAbsPos( pStartEntry );
	ULONG nXVisPos = pView->GetVisiblePos( pStartEntry );
	SvLBoxString* pXStr = (SvLBoxString*)pStartEntry->GetFirstItem( SV_ITEM_ID_LBOXSTRING);
#endif



	if( nNodeBmpTabDistance == NODE_BMP_TABDIST_NOTVALID )
		SetNodeBmpTabDistance();

	long nRectHeight = rRect.GetHeight();
	long nEntryHeight = pView->GetEntryHeight();

	// Bereich der zu zeichnenden Entries berechnen
	USHORT nStartLine = (USHORT)( rRect.Top() / nEntryHeight );
	USHORT nCount = (USHORT)( nRectHeight / nEntryHeight );
		nCount += 2; // keine Zeile vergessen

	long nY = nStartLine * nEntryHeight;
	SvLBoxEntry* pEntry = pStartEntry;
	while( nStartLine && pEntry )
	{
		pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
		nStartLine--;
	}

	Region aClipRegion( GetClipRegionRect() );

	// erst die Linien Zeichnen, dann clippen!
	pView->SetClipRegion();
	if( nWinBits & ( WB_HASLINES | WB_HASLINESATROOT ) )
		DrawNet();

	pView->SetClipRegion( aClipRegion );

	for( USHORT n=0; n< nCount && pEntry; n++ )
	{
		/*long nMaxRight=*/
		pView->PaintEntry1( pEntry, nY, 0xffff, TRUE );
		nY += nEntryHeight;
		pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
	}

    if ( !pCursor && ( ( nExtendedWinBits & EWB_NO_AUTO_CURENTRY ) == 0 ) )
	{
        // do not select if multiselection or explicit set
        BOOL bNotSelect = ( aSelEng.GetSelectionMode() == MULTIPLE_SELECTION )
                || ( ( nWinBits & WB_NOINITIALSELECTION ) == WB_NOINITIALSELECTION );
        SetCursor( pStartEntry, bNotSelect );
	}

    nFlags &= (~F_DESEL_ALL);
	pView->SetClipRegion();
	Rectangle aRect;
	if( !(nFlags & F_PAINTED) )
	{
		nFlags |= F_PAINTED;
		RepaintScrollBars();
	}
	nFlags &= (~F_IN_PAINT);
}

void SvImpLBox::MakeVisible( SvLBoxEntry* pEntry, BOOL bMoveToTop )
{
	if( !pEntry )
		return;

	BOOL bInView = IsEntryInView( pEntry );

	if( bInView && (!bMoveToTop || pStartEntry == pEntry) )
		return;  // ist schon sichtbar

	if( pStartEntry || (nWinBits & WB_FORCE_MAKEVISIBLE) )
		nFlags &= (~F_FILLING);
	if( !bInView )
	{
		if( !pView->IsEntryVisible(pEntry) )  // Parent(s) zugeklappt ?
		{
			SvLBoxEntry* pParent = pView->GetParent( pEntry );
			while( pParent )
			{
				if( !pView->IsExpanded( pParent ) )
				{
                    #ifdef DBG_UTIL
                    BOOL bRet =
                    #endif
					    pView->Expand( pParent );
					DBG_ASSERT(bRet,"Not expanded!");
				}
				pParent = pView->GetParent( pParent );
			}
			// Passen Childs der Parents in View oder muessen wir scrollen ?
			if( IsEntryInView( pEntry ) && !bMoveToTop )
				return;  // Scrollen nicht noetig -> tschuess
		}
	}

	pStartEntry = pEntry;
	ShowCursor( FALSE );
	FillView();
	aVerSBar.SetThumbPos( (long)(pView->GetVisiblePos( pStartEntry )) );
	ShowCursor( TRUE );
	pView->Invalidate();
}


void SvImpLBox::RepaintSelectionItems()
{
	if( !pView->GetVisibleCount() )
		return;

	if( !pStartEntry )
		pStartEntry = pView->First();

	if( nNodeBmpTabDistance == NODE_BMP_TABDIST_NOTVALID )
		SetNodeBmpTabDistance();

	ShowCursor( FALSE );

	long nEntryHeight = pView->GetEntryHeight();

	ULONG nCount = nVisibleCount;
	long nY = 0;
	SvLBoxEntry* pEntry = pStartEntry;
	for( ULONG n=0; n< nCount && pEntry; n++ )
	{
		pView->PaintEntry1( pEntry, nY, 0xffff ); //wg. ItemsetBrowser SV_LBOXTAB_SHOW_SELECTION );
		nY += nEntryHeight;
		pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
	}

	ShowCursor( TRUE );
}


void SvImpLBox::DrawNet()
{
	if( pView->GetVisibleCount() < 2 && !pStartEntry->HasChildsOnDemand() &&
		!pStartEntry->HasChilds() )
		return;

    //for platforms who don't have nets, DrawNativeControl does nothing and return true
    //so that SvImpLBox::DrawNet() doesn't draw anything too
 	if(pView->IsNativeControlSupported( CTRL_LISTNET, PART_ENTIRE_CONTROL)) {
        ImplControlValue	aControlValue;
        Point aTemp(0,0);   // temporary needed for g++ 3.3.5
        Region			  aCtrlRegion( Rectangle(aTemp, Size( 0, 0 )) );
        ControlState		nState = CTRL_STATE_ENABLED;
            if( pView->DrawNativeControl( CTRL_LISTNET, PART_ENTIRE_CONTROL,
					aCtrlRegion, nState, aControlValue, rtl::OUString() ) )
            {
                return;
            }

    }

	long nEntryHeight = pView->GetEntryHeight();
	long nEntryHeightDIV2 = nEntryHeight / 2;
	if( nEntryHeightDIV2 && !(nEntryHeight & 0x0001))
		nEntryHeightDIV2--;

	SvLBoxEntry* pChild;
	SvLBoxEntry* pEntry = pStartEntry;

	SvLBoxTab* pFirstDynamicTab = pView->GetFirstDynamicTab();
	while( pTree->GetDepth( pEntry ) > 0 )
		pEntry = pView->GetParent( pEntry );
	USHORT nOffs = (USHORT)(pView->GetVisiblePos( pStartEntry ) -
							pView->GetVisiblePos( pEntry ));
	long nY = 0;
	nY -= ( nOffs * nEntryHeight );

	DBG_ASSERT(pFirstDynamicTab,"No Tree!");

	Color aOldLineColor = pView->GetLineColor();
	const StyleSettings& rStyleSettings = pView->GetSettings().GetStyleSettings();
	Color aCol= rStyleSettings.GetFaceColor();

	if( aCol.IsRGBEqual( pView->GetBackground().GetColor()) )
		aCol = rStyleSettings.GetShadowColor();
	pView->SetLineColor( aCol );
	Point aPos1, aPos2;
	USHORT nDistance;
	ULONG nMax = nVisibleCount + nOffs + 1;

	const Image& rExpandedNodeBitmap = GetExpandedNodeBmp();

	for( ULONG n=0; n< nMax && pEntry; n++ )
	{
		if( pView->IsExpanded(pEntry) )
		{
			aPos1.X() = pView->GetTabPos(pEntry, pFirstDynamicTab);
			// wenn keine ContextBitmap, dann etwas nach rechts
			// unter den ersten Text (Node.Bmp ebenfalls
			if( !pView->nContextBmpWidthMax )
				aPos1.X() += rExpandedNodeBitmap.GetSizePixel().Width() / 2;

			aPos1.Y() = nY;
			aPos1.Y() += nEntryHeightDIV2;

			pChild = pView->FirstChild( pEntry );
			DBG_ASSERT(pChild,"Child?");
			pChild = pTree->LastSibling( pChild );
			nDistance = (USHORT)(pView->GetVisiblePos(pChild) -
								 pView->GetVisiblePos(pEntry));
			aPos2 = aPos1;
			aPos2.Y() += nDistance * nEntryHeight;
			pView->DrawLine( aPos1, aPos2 );
		}
		// Sichtbar im Control ?
		if( n>= nOffs && ((nWinBits & WB_HASLINESATROOT) || !pTree->IsAtRootDepth(pEntry)))
		{
			// kann aPos1 recyclet werden ?
			if( !pView->IsExpanded(pEntry) )
			{
				// njet
				aPos1.X() = pView->GetTabPos(pEntry, pFirstDynamicTab);
				// wenn keine ContextBitmap, dann etwas nach rechts
				// unter den ersten Text (Node.Bmp ebenfalls
				if( !pView->nContextBmpWidthMax )
					aPos1.X() += rExpandedNodeBitmap.GetSizePixel().Width() / 2;
				aPos1.Y() = nY;
				aPos1.Y() += nEntryHeightDIV2;
				aPos2.X() = aPos1.X();
			}
			aPos2.Y() = aPos1.Y();
			aPos2.X() -= pView->GetIndent();
			pView->DrawLine( aPos1, aPos2 );
		}
		nY += nEntryHeight;
		pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
	}
	if( nWinBits & WB_HASLINESATROOT )
	{
		pEntry = pView->First();
		aPos1.X() = pView->GetTabPos( pEntry, pFirstDynamicTab);
		// wenn keine ContextBitmap, dann etwas nach rechts
		// unter den ersten Text (Node.Bmp ebenfalls
		if( !pView->nContextBmpWidthMax )
			aPos1.X() += rExpandedNodeBitmap.GetSizePixel().Width() / 2;
		aPos1.X() -=  pView->GetIndent();
		aPos1.Y() = GetEntryLine( pEntry );
		aPos1.Y() += nEntryHeightDIV2;
		pChild = pTree->LastSibling( pEntry );
		aPos2.X() = aPos1.X();
		aPos2.Y() = GetEntryLine( pChild );
		aPos2.Y() += nEntryHeightDIV2;
		pView->DrawLine( aPos1, aPos2 );
	}
	pView->SetLineColor( aOldLineColor );
}


static long GetOptSize( TabBar* pTabBar )
{
	return pTabBar->CalcWindowSizePixel().Width();
}

void SvImpLBox::PositionScrollBars( Size& rSize, USHORT nMask )
{
	long nOverlap = 0;

	Size aVerSize( nVerSBarWidth, rSize.Height() );
	Size aHorSize( rSize.Width(), nHorSBarHeight );
	long nTabBarWidth = 0;
	if( pTabBar )
	{
		nTabBarWidth = GetOptSize( pTabBar );
		long nMaxWidth = (rSize.Width() * 700) / 1000;
		if( nTabBarWidth > nMaxWidth )
		{
			nTabBarWidth = nMaxWidth;
			pTabBar->SetStyle( pTabBar->GetStyle() | WB_MINSCROLL );
		}
		else
		{
			WinBits nStyle = pTabBar->GetStyle();
			nStyle &= ~(WB_MINSCROLL);
			pTabBar->SetStyle( nStyle );
		}
		aHorSize.Width() -= nTabBarWidth;
		Size aTabSize( pTabBar->GetSizePixel() );
		aTabSize.Width() = nTabBarWidth;
		pTabBar->SetSizePixel( aTabSize );
	}
	if( nMask & 0x0001 )
		aHorSize.Width() -= nVerSBarWidth;
	if( nMask & 0x0002 )
		aVerSize.Height() -= nHorSBarHeight;

	aVerSize.Height() += 2 * nOverlap;
	Point aVerPos( rSize.Width() - aVerSize.Width() + nOverlap, -nOverlap );
	aVerSBar.SetPosSizePixel( aVerPos, aVerSize );

	aHorSize.Width() += 2 * nOverlap;
	Point aHorPos( -nOverlap, rSize.Height() - aHorSize.Height() + nOverlap );
	if( pTabBar )
		pTabBar->SetPosPixel( aHorPos );
	aHorPos.X() += nTabBarWidth;
	aHorSBar.SetPosSizePixel( aHorPos, aHorSize );

	if( nMask & 0x0001 )
		rSize.Width() = aVerPos.X();
	if( nMask & 0x0002 )
		rSize.Height() = aHorPos.Y();
	if( pTabBar )
		pTabBar->Show();

	if( (nMask & (0x0001|0x0002)) == (0x0001|0x0002) )
		aScrBarBox.Show();
	else
		aScrBarBox.Hide();

}

// nResult: Bit0 == VerSBar Bit1 == HorSBar
USHORT SvImpLBox::AdjustScrollBars( Size& rSize )
{
	long nEntryHeight = pView->GetEntryHeight();
	if( !nEntryHeight )
		return 0;

	USHORT nResult = 0;

	Size aOSize( pView->Control::GetOutputSizePixel() );

	BOOL bVerSBar = ( pView->nWindowStyle & WB_VSCROLL ) != 0;
	BOOL bHorBar = FALSE;
	long nMaxRight = aOSize.Width(); //GetOutputSize().Width();
	Point aOrigin( pView->GetMapMode().GetOrigin() );
	aOrigin.X() *= -1;
	nMaxRight += aOrigin.X() - 1;
	long nVis = nMostRight - aOrigin.X();
	if( pTabBar || (
		(pView->nWindowStyle & WB_HSCROLL) &&
		(nVis < nMostRight || nMaxRight < nMostRight) ))
		bHorBar = TRUE;

	// Anzahl aller nicht eingeklappten Eintraege
	ULONG nTotalCount = pView->GetVisibleCount();

	// Anzahl in der View sichtbarer Eintraege
	nVisibleCount = aOSize.Height() / nEntryHeight;

	// muessen wir eine vertikale Scrollbar einblenden?
    if( bVerSBar || nTotalCount > nVisibleCount )
	{
		nResult = 1;
		nFlags |= F_HOR_SBARSIZE_WITH_VBAR;
		nMaxRight -= nVerSBarWidth;
		if( !bHorBar )
		{
			if( (pView->nWindowStyle & WB_HSCROLL) &&
				(nVis < nMostRight || nMaxRight < nMostRight) )
				bHorBar = TRUE;
		}
	}

	// muessen wir eine horizontale Scrollbar einblenden?
	if( bHorBar )
	{
		nResult |= 0x0002;
		// die Anzahl der in der View sichtbaren Eintraege
		// muss neu berechnet werden, da die horizontale
		// ScrollBar eingeblendet wird
		nVisibleCount =  (aOSize.Height() - nHorSBarHeight) / nEntryHeight;
		// eventuell brauchen wir jetzt doch eine vertikale ScrollBar
		if( !(nResult & 0x0001) &&
            ((nTotalCount > nVisibleCount) || bVerSBar) )
		{
			nResult = 3;
			nFlags |= F_VER_SBARSIZE_WITH_HBAR;
		}
	}

	PositionScrollBars( aOSize, nResult );

	// Range, VisibleRange usw. anpassen

	// Output-Size aktualisieren, falls wir scrollen muessen
	Rectangle aRect;
	aRect.SetSize( aOSize );
	aSelEng.SetVisibleArea( aRect );

	// Vertikale ScrollBar
	long nTemp = (long)nVisibleCount;
	nTemp--;
	if( nTemp != aVerSBar.GetVisibleSize() )
	{
		if( !bInVScrollHdl )
		{
			aVerSBar.SetPageSize( nTemp - 1 );
			aVerSBar.SetVisibleSize( nTemp );
		}
		else
		{
			nFlags |= F_ENDSCROLL_SET_VIS_SIZE;
			nNextVerVisSize = nTemp;
		}
	}

	// Horizontale ScrollBar
	nTemp = aHorSBar.GetThumbPos();
	aHorSBar.SetVisibleSize( aOSize.Width() );
	long nNewThumbPos = aHorSBar.GetThumbPos();
	Range aRange( aHorSBar.GetRange() );
	if( aRange.Max() < nMostRight+25 )
	{
		aRange.Max() = nMostRight+25;
		aHorSBar.SetRange( aRange );
	}

	if( nTemp != nNewThumbPos )
	{
		nTemp = nNewThumbPos - nTemp;
		if( pView->IsEditingActive() )
		{
			pView->EndEditing( TRUE ); // Cancel
			pView->Update();
		}
		pView->nFocusWidth = -1;
		KeyLeftRight( nTemp );
	}

	if( nResult & 0x0001 )
		aVerSBar.Show();
	else
		aVerSBar.Hide();

	if( nResult & 0x0002 )
		aHorSBar.Show();
	else
	{
		if( !pTabBar )
			aHorSBar.Hide();
	}
	rSize = aOSize;
	return nResult;
}

void SvImpLBox::InitScrollBarBox()
{
	aScrBarBox.SetSizePixel( Size(nVerSBarWidth, nHorSBarHeight) );
	Size aSize( pView->Control::GetOutputSizePixel() );
	aScrBarBox.SetPosPixel( Point(aSize.Width()-nVerSBarWidth, aSize.Height()-nHorSBarHeight));
}

void SvImpLBox::Resize()
{
	Size aSize( pView->Control::GetOutputSizePixel());
	if( aSize.Width() <= 0 || aSize.Height() <= 0 )
		return;
	nFlags |= F_IN_RESIZE;
	InitScrollBarBox();

	if( pView->GetEntryHeight())
	{
		AdjustScrollBars( aOutputSize );
		FillView();
	}
	// !!!HACK, da in Floating- & Docking-Windows nach Resizes
	// die Scrollbars nicht richtig, bzw. ueberhaupt nicht gezeichnet werden
	if( aHorSBar.IsVisible())
		aHorSBar.Invalidate();
	if( aVerSBar.IsVisible())
		aVerSBar.Invalidate();
	nFlags &= (~(F_IN_RESIZE | F_PAINTED));
}

void SvImpLBox::FillView()
{
	if( !pStartEntry )
	{
		USHORT nVisibleViewCount = (USHORT)(pView->GetVisibleCount());
		USHORT nTempThumb = (USHORT)aVerSBar.GetThumbPos();
		if( nTempThumb >= nVisibleViewCount )
			nTempThumb = nVisibleViewCount - 1;
		pStartEntry = (SvLBoxEntry*)(pView->GetEntryAtVisPos(nTempThumb));
	}
	if( pStartEntry )
	{
		USHORT nLast = (USHORT)(pView->GetVisiblePos( (SvLBoxEntry*)(pView->LastVisible())));
		USHORT nThumb = (USHORT)(pView->GetVisiblePos( pStartEntry ));
		USHORT nCurDispEntries = nLast-nThumb+1;
		if( nCurDispEntries <  nVisibleCount )
		{
			ShowCursor( FALSE );
			// Fenster fuellen, indem der Thumb schrittweise
			// nach oben bewegt wird
			BOOL bFound = FALSE;
			SvLBoxEntry* pTemp = pStartEntry;
			while( nCurDispEntries < nVisibleCount && pTemp )
			{
				pTemp = (SvLBoxEntry*)(pView->PrevVisible(pStartEntry));
				if( pTemp )
				{
					nThumb--;
					pStartEntry = pTemp;
					nCurDispEntries++;
					bFound = TRUE;
				}
			}
			if( bFound )
			{
				aVerSBar.SetThumbPos( nThumb );
				ShowCursor( TRUE );	// Focusrect neu berechnen
				pView->Invalidate();
			}
		}
	}
}




void SvImpLBox::ShowVerSBar()
{
	BOOL bVerBar = ( pView->nWindowStyle & WB_VSCROLL ) != 0;
	ULONG nVis = 0;
	if( !bVerBar )
		nVis = pView->GetVisibleCount();
	if( bVerBar || (nVisibleCount && nVis > (ULONG)(nVisibleCount-1)) )
	{
		if( !aVerSBar.IsVisible() )
		{
			pView->nFocusWidth = -1;
			AdjustScrollBars( aOutputSize );
			if( GetUpdateMode() )
				aVerSBar.Update();
		}
	}
	else
	{
		if( aVerSBar.IsVisible() )
		{
			pView->nFocusWidth = -1;
			AdjustScrollBars( aOutputSize );
		}
	}

	long nMaxRight = GetOutputSize().Width();
	Point aPos( pView->GetMapMode().GetOrigin() );
	aPos.X() *= -1; // Umrechnung Dokumentkoord.
	nMaxRight = nMaxRight + aPos.X() - 1;
	if( nMaxRight < nMostRight  )
	{
		if( !aHorSBar.IsVisible() )
		{
			pView->nFocusWidth = -1;
			AdjustScrollBars( aOutputSize );
			if( GetUpdateMode() )
				aHorSBar.Update();
		}
		else
		{
			Range aRange( aHorSBar.GetRange() );
			if( aRange.Max() < nMostRight+25 )
			{
				aRange.Max() = nMostRight+25;
				aHorSBar.SetRange( aRange );
			}
			else
			{
				pView->nFocusWidth = -1;
				AdjustScrollBars( aOutputSize );
			}
		}
	}
	else
	{
		if( aHorSBar.IsVisible() )
		{
			pView->nFocusWidth = -1;
			AdjustScrollBars( aOutputSize );
		}
	}
}


void SvImpLBox::SyncVerThumb()
{
	if( pStartEntry )
	{
		long nEntryPos = pView->GetVisiblePos( pStartEntry );
		aVerSBar.SetThumbPos( nEntryPos );
	}
	else
		aVerSBar.SetThumbPos( 0 );
}

BOOL SvImpLBox::IsEntryInView( SvLBoxEntry* pEntry ) const
{
	// Parent eingeklappt
	if( !pView->IsEntryVisible(pEntry) )
		return FALSE;
	long nY = GetEntryLine( pEntry );
	if( nY < 0 )
		return FALSE;
	long nMax = nVisibleCount * pView->GetEntryHeight();
	if( nY >= nMax )
		return FALSE;
	return TRUE;
}


long SvImpLBox::GetEntryLine( SvLBoxEntry* pEntry ) const
{
	if(!pStartEntry )
		return -1; // unsichtbare Position

	long nFirstVisPos = pView->GetVisiblePos( pStartEntry );
	long nEntryVisPos = pView->GetVisiblePos( pEntry );
	nFirstVisPos = nEntryVisPos - nFirstVisPos;
	nFirstVisPos *= pView->GetEntryHeight();
	return nFirstVisPos;
}

void SvImpLBox::SetEntryHeight( short /* nHeight */ )
{
	SetNodeBmpYOffset( GetExpandedNodeBmp() );
	SetNodeBmpYOffset( GetCollapsedNodeBmp() );
	if(!pView->HasViewData()) // stehen wir im Clear?
	{
		Size aSize = pView->Control::GetOutputSizePixel();
		AdjustScrollBars( aSize );
	}
	else
	{
		Resize();
		if( GetUpdateMode() )
			pView->Invalidate();
	}
}



// ***********************************************************************
// Callback-Functions
// ***********************************************************************

void SvImpLBox::IndentChanged( short /* nIndentPixel */ ) {}

void SvImpLBox::EntryExpanded( SvLBoxEntry* pEntry )
{
	// SelAllDestrAnch( FALSE, TRUE ); //DeselectAll();
	if( GetUpdateMode() )
	{
		ShowCursor( FALSE );
		long nY = GetEntryLine( pEntry );
		if( IsLineVisible(nY) )
		{
			InvalidateEntriesFrom( nY );
			FindMostRight( pEntry, 0  );
		}
		aVerSBar.SetRange( Range(0, pView->GetVisibleCount()-1 ) );
		// falls vor dem Thumb expandiert wurde, muss
		// die Thumb-Position korrigiert werden.
		SyncVerThumb();
		ShowVerSBar();
		ShowCursor( TRUE );
	}
}

void SvImpLBox::EntryCollapsed( SvLBoxEntry* pEntry )
{
	if( !pView->IsEntryVisible( pEntry ) )
		return;

	ShowCursor( FALSE );

	if( !pMostRightEntry || pTree->IsChild( pEntry,pMostRightEntry ) )
	{
		FindMostRight(0);
	}

	if( pStartEntry )
	{
		long nOldThumbPos   = aVerSBar.GetThumbPos();
		ULONG nVisList      = pView->GetVisibleCount();
		aVerSBar.SetRange( Range(0, nVisList-1) );
		long nNewThumbPos   = aVerSBar.GetThumbPos();
		if( nNewThumbPos != nOldThumbPos  )
		{
			pStartEntry = pView->First();
			USHORT nDistance = (USHORT)nNewThumbPos;
			if( nDistance )
				pStartEntry = (SvLBoxEntry*)(pView->NextVisible( pStartEntry,
														nDistance));
			if( GetUpdateMode() )
				pView->Invalidate();
		}
		else
			SyncVerThumb();
		ShowVerSBar();
	}
	// wurde Cursor eingeklappt ?
	if( pTree->IsChild( pEntry, pCursor ) )
		SetCursor( pEntry );
	if( GetUpdateMode() )
		ShowVerSBar();
	ShowCursor( TRUE );
	if( GetUpdateMode() && pCursor )
		pView->Select( pCursor, TRUE );
}

void SvImpLBox::CollapsingEntry( SvLBoxEntry* pEntry )
{
	if( !pView->IsEntryVisible( pEntry ) || !pStartEntry )
		return;

	SelAllDestrAnch( FALSE, TRUE ); // deselectall

	// ist der eingeklappte Parent sichtbar ?
	long nY = GetEntryLine( pEntry );
	if( IsLineVisible(nY) )
	{
		if( GetUpdateMode() )
			InvalidateEntriesFrom( nY );
	}
	else
	{
		if( pTree->IsChild(pEntry, pStartEntry) )
		{
			pStartEntry = pEntry;
			if( GetUpdateMode() )
				pView->Invalidate();
		}
	}
}


void SvImpLBox::SetNodeBmpYOffset( const Image& rBmp )
{
	Size aSize;
	nYoffsNodeBmp = pView->GetHeightOffset( rBmp, aSize );
	nNodeBmpWidth = aSize.Width();
}

void SvImpLBox::SetNodeBmpTabDistance()
{
	nNodeBmpTabDistance = -pView->GetIndent();
	if( pView->nContextBmpWidthMax )
	{
		// nur, wenn der erste dynamische Tab zentriert ist
		// (setze ich momentan voraus)
		Size aSize = GetExpandedNodeBmp().GetSizePixel();
		nNodeBmpTabDistance -= aSize.Width() / 2;
	}
}

//
// korrigiert bei SingleSelection den Cursor
//
void SvImpLBox::EntrySelected( SvLBoxEntry* pEntry, BOOL bSelect )
{
	if( nFlags & F_IGNORE_SELECT )
		return;

	/*
	if( (nWinBits & WB_HIDESELECTION) && pEntry && !pView->HasFocus() )
	{
		SvViewData* pViewData = pView->GetViewData( pEntry );
		pViewData->SetCursored( bSelect );
	}
	*/

	nFlags &= (~F_DESEL_ALL);
	if( bSelect &&
		aSelEng.GetSelectionMode() == SINGLE_SELECTION &&
		pEntry != pCursor )
	{
		SetCursor( pEntry );
		DBG_ASSERT(pView->GetSelectionCount()==1,"selection count?");
	}

	if( GetUpdateMode() && pView->IsEntryVisible(pEntry) )
	{
		long nY = GetEntryLine( pEntry );
		if( IsLineVisible( nY ) )
		{
			ShowCursor( FALSE );
			pView->PaintEntry1( pEntry, nY, 0xffff ); // wg. ItemsetBrowser SV_LBOXTAB_SHOW_SELECTION );
			ShowCursor( TRUE );
		}
	}
}


void SvImpLBox::RemovingEntry( SvLBoxEntry* pEntry )
{
	DestroyAnchor();

	if( !pView->IsEntryVisible( pEntry ) )
	{
		// wenn Parent eingeklappt, dann tschuess
		nFlags |= F_REMOVED_ENTRY_INVISIBLE;
		return;
	}

	if( pEntry == pMostRightEntry || (
		pEntry->HasChilds() && pView->IsExpanded(pEntry) &&
		pTree->IsChild(pEntry, pMostRightEntry)))
	{
		nFlags |= F_REMOVED_RECALC_MOST_RIGHT;
	}

	SvLBoxEntry* pOldStartEntry = pStartEntry;

	SvLBoxEntry* pParent = (SvLBoxEntry*)(pView->GetModel()->GetParent(pEntry));

	if( pParent && pView->GetModel()->GetChildList(pParent)->Count() == 1 )
	{
		DBG_ASSERT( pView->IsExpanded( pParent ), "Parent not expanded");
		pParent->SetFlags( pParent->GetFlags() | SV_ENTRYFLAG_NO_NODEBMP);
		InvalidateEntry( pParent );
	}

	if( pCursor && pTree->IsChild( pEntry, pCursor) )
		pCursor = pEntry;
	if( pStartEntry && pTree->IsChild(pEntry,pStartEntry) )
		pStartEntry = pEntry;

	SvLBoxEntry* pTemp;
	if( pCursor && pCursor == pEntry )
	{
		if( bSimpleTravel )
			pView->Select( pCursor, FALSE );
		ShowCursor( FALSE );	// Focus-Rect weg
		// NextSibling, weil auch Childs des Cursors geloescht werden
		pTemp = pView->NextSibling( pCursor );
		if( !pTemp )
			pTemp = (SvLBoxEntry*)(pView->PrevVisible( pCursor ));

		SetCursor( pTemp, TRUE );
	}
	if( pStartEntry && pStartEntry == pEntry )
	{
		pTemp = pView->NextSibling( pStartEntry );
		if( !pTemp )
			pTemp = (SvLBoxEntry*)(pView->PrevVisible( pStartEntry ));
		pStartEntry = pTemp;
	}
	if( GetUpdateMode())
	{
		// wenns der letzte ist, muss invalidiert werden, damit die Linien
		// richtig gezeichnet (in diesem Fall geloescht) werden.
		if( pStartEntry && (pStartEntry != pOldStartEntry || pEntry == (SvLBoxEntry*)pView->GetModel()->Last()) )
		{
			aVerSBar.SetThumbPos( pView->GetVisiblePos( pStartEntry ));
			pView->Invalidate( GetVisibleArea() );
		}
		else
			InvalidateEntriesFrom( GetEntryLine( pEntry ) );
	}
}

void SvImpLBox::EntryRemoved()
{
	if( nFlags & F_REMOVED_ENTRY_INVISIBLE )
	{
		nFlags &= (~F_REMOVED_ENTRY_INVISIBLE);
		return;
	}
	if( !pStartEntry )
		pStartEntry = pTree->First();
	if( !pCursor )
		SetCursor( pStartEntry, TRUE );

	if( pCursor && (bSimpleTravel || !pView->GetSelectionCount() ))
		pView->Select( pCursor, TRUE );

	if( GetUpdateMode())
	{
		if( nFlags & F_REMOVED_RECALC_MOST_RIGHT )
			FindMostRight(0);
		aVerSBar.SetRange( Range(0, pView->GetVisibleCount()-1 ) );
		FillView();
		if( pStartEntry )
			// falls ueber dem Thumb geloescht wurde
			aVerSBar.SetThumbPos( pView->GetVisiblePos( pStartEntry) );

		ShowVerSBar();
		if( pCursor && pView->HasFocus() && !pView->IsSelected(pCursor) )
		{
			if( pView->GetSelectionCount() )
			{
				// ist ein benachbarter Eintrag selektiert?
				SvLBoxEntry* pNextCursor = (SvLBoxEntry*)pView->PrevVisible( pCursor );
				if( !pNextCursor || !pView->IsSelected( pNextCursor ))
					pNextCursor = (SvLBoxEntry*)pView->NextVisible( pCursor );
				if( !pNextCursor || !pView->IsSelected( pNextCursor ))
					// kein Nachbar selektiert: Ersten selektierten nehmen
					pNextCursor = pView->FirstSelected();
				SetCursor( pNextCursor );
				MakeVisible( pCursor );
			}
			else
				pView->Select( pCursor, TRUE );
		}
		ShowCursor( TRUE );
	}
	nFlags &= (~F_REMOVED_RECALC_MOST_RIGHT);
}


void SvImpLBox::MovingEntry( SvLBoxEntry* pEntry )
{
	int bDeselAll = nFlags & F_DESEL_ALL;
	SelAllDestrAnch( FALSE, TRUE );  // DeselectAll();
	if( !bDeselAll )
		nFlags &= (~F_DESEL_ALL);

	if( pEntry == pCursor )
		ShowCursor( FALSE );
	if( IsEntryInView( pEntry ) )
		pView->Invalidate();
	if( pEntry == pStartEntry )
	{
		SvLBoxEntry* pNew = 0;
		if( !pEntry->HasChilds() )
		{
			pNew = (SvLBoxEntry*)(pView->NextVisible( pStartEntry ));
			if( !pNew )
				pNew = (SvLBoxEntry*)(pView->PrevVisible( pStartEntry ));
		}
		else
		{
			pNew = pTree->NextSibling( pEntry );
			if( !pNew )
				pNew = pTree->PrevSibling( pEntry );
		}
		pStartEntry = pNew;
	}
}

void SvImpLBox::EntryMoved( SvLBoxEntry* pEntry )
{
	// #97680# --------------
	UpdateContextBmpWidthVectorFromMovedEntry( pEntry );

    if ( !pStartEntry )
        // this might happen if the only entry in the view is moved to its very same position
        // #i97346#
        pStartEntry = pView->First();

	aVerSBar.SetRange( Range(0, pView->GetVisibleCount()-1));
	USHORT nFirstPos = (USHORT)pTree->GetAbsPos( pStartEntry );
	USHORT nNewPos = (USHORT)pTree->GetAbsPos( pEntry );
	FindMostRight(0);
	if( nNewPos < nFirstPos ) //!!!Notloesung
		pStartEntry = pEntry;
	// #97702# ---------------
	SyncVerThumb();
	if( pEntry == pCursor )
	{
		if( pView->IsEntryVisible( pCursor ) )
			ShowCursor( TRUE );
		else
		{
			SvLBoxEntry* pParent = pEntry;
			do {
				pParent = pTree->GetParent( pParent );
			}
			while( !pView->IsEntryVisible( pParent ) );
			SetCursor( pParent );
		}
	}
	if( IsEntryInView( pEntry ) )
		pView->Invalidate();
}



void SvImpLBox::EntryInserted( SvLBoxEntry* pEntry )
{
	if( GetUpdateMode() )
	{
		SvLBoxEntry* pParent = (SvLBoxEntry*)pTree->GetParent(pEntry);
		if( pParent && pTree->GetChildList(pParent)->Count() == 1 )
			// Pluszeichen zeichnen
			pTree->InvalidateEntry( pParent );

		if( !pView->IsEntryVisible( pEntry ) )
			return;
		int bDeselAll = nFlags & F_DESEL_ALL;
		if( bDeselAll )
			SelAllDestrAnch( FALSE, TRUE );
		else
			DestroyAnchor();
		//	nFlags &= (~F_DESEL_ALL);
//		ShowCursor( FALSE ); // falls sich Cursor nach unten verschiebt
		long nY = GetEntryLine( pEntry );
		BOOL bEntryVisible = IsLineVisible( nY );
		if( bEntryVisible )
		{
			ShowCursor( FALSE ); // falls sich Cursor nach unten verschiebt
			nY -= pView->GetEntryHeight(); // wg. Linien
			InvalidateEntriesFrom( nY );
		}
		else if( pStartEntry && nY < GetEntryLine(pStartEntry) )
		{
			// pruefen, ob die View komplett gefuellt ist. Wenn
			// nicht, dann pStartEntry und den Cursor anpassen
			// (automatisches scrollen)
			USHORT nLast = (USHORT)(pView->GetVisiblePos( (SvLBoxEntry*)(pView->LastVisible())));
			USHORT nThumb = (USHORT)(pView->GetVisiblePos( pStartEntry ));
			USHORT nCurDispEntries = nLast-nThumb+1;
			if( nCurDispEntries < nVisibleCount )
			{
				// beim naechsten Paint-Event setzen
				pStartEntry = 0;
				SetCursor( 0 );
				pView->Invalidate();
			}
		}
		else if( !pStartEntry )
			pView->Invalidate();

		// die Linien invalidieren
		/*
		if( (bEntryVisible || bPrevEntryVisible) &&
			(nWinBits & ( WB_HASLINES | WB_HASLINESATROOT )) )
		{
			SvLBoxTab* pTab = pView->GetFirstDynamicTab();
			if( pTab )
			{
				long nDX = pView->GetTabPos( pEntry, pTab );
				Point aTmpPoint;
				Size aSize( nDX, nY );
				Rectangle aRect( aTmpPoint, aSize );
				pView->Invalidate( aRect );
			}
		}
		*/

		SetMostRight( pEntry );
		aVerSBar.SetRange( Range(0, pView->GetVisibleCount()-1));
		SyncVerThumb(); // falls vor Thumb eingefuegt wurde
		ShowVerSBar();
		ShowCursor( TRUE );
		if( pStartEntry != pView->First() && (nFlags & F_FILLING) )
			pView->Update();
	}
}



// ********************************************************************
// Eventhandler
// ********************************************************************


// ****** Steuerung der Controlanimation

BOOL SvImpLBox::ButtonDownCheckCtrl(const MouseEvent& rMEvt, SvLBoxEntry* pEntry,
								   long nY	)
{
	SvLBoxItem* pItem = pView->GetItem(pEntry,rMEvt.GetPosPixel().X(),&pActiveTab);
	if( pItem && (pItem->IsA()==SV_ITEM_ID_LBOXBUTTON))
	{
		pActiveButton = (SvLBoxButton*)pItem;
		pActiveEntry = pEntry;
		if( pCursor == pActiveEntry )
			pView->HideFocus();
		pView->CaptureMouse();
		pActiveButton->SetStateHilighted( TRUE );
		pView->PaintEntry1( pActiveEntry, nY,
					SV_LBOXTAB_PUSHABLE | SV_LBOXTAB_ADJUST_CENTER |
					SV_LBOXTAB_ADJUST_RIGHT );
		return TRUE;
	}
	else
		pActiveButton = 0;
	return FALSE;
}

BOOL SvImpLBox::MouseMoveCheckCtrl( const MouseEvent& rMEvt, SvLBoxEntry* pEntry)
{
	if( pActiveButton )
	{
		long nY;
		long nMouseX = rMEvt.GetPosPixel().X();
		if( pEntry == pActiveEntry &&
			 pView->GetItem(pActiveEntry, nMouseX) == pActiveButton )
		{
			if( !pActiveButton->IsStateHilighted() )
			{
				pActiveButton->SetStateHilighted(TRUE );
				nY = GetEntryLine( pActiveEntry );
				pView->PaintEntry1( pActiveEntry, nY,
					SV_LBOXTAB_PUSHABLE | SV_LBOXTAB_ADJUST_CENTER |
					SV_LBOXTAB_ADJUST_RIGHT );
			}
		}
		else
		{
			if( pActiveButton->IsStateHilighted() )
			{
				pActiveButton->SetStateHilighted(FALSE );
				nY = GetEntryLine( pActiveEntry );
				pView->PaintEntry1( pActiveEntry, nY, SV_LBOXTAB_PUSHABLE );
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL SvImpLBox::ButtonUpCheckCtrl( const MouseEvent& rMEvt )
{
	if( pActiveButton )
	{
		pView->ReleaseMouse();
		SvLBoxEntry* pEntry = GetClickedEntry( rMEvt.GetPosPixel() );
		long nY = GetEntryLine( pActiveEntry );
		pActiveButton->SetStateHilighted( FALSE );
		long nMouseX = rMEvt.GetPosPixel().X();
		if( pEntry == pActiveEntry &&
			 pView->GetItem( pActiveEntry, nMouseX ) == pActiveButton )
			pActiveButton->ClickHdl( pView, pActiveEntry );
		pView->PaintEntry1( pActiveEntry, nY,
					SV_LBOXTAB_PUSHABLE | SV_LBOXTAB_ADJUST_CENTER |
					SV_LBOXTAB_ADJUST_RIGHT );
		if( pCursor == pActiveEntry )
			ShowCursor( TRUE );
		pActiveButton = 0;
		pActiveEntry = 0;
		pActiveTab = 0;
		return TRUE;
	}
	return FALSE;
}

// ******* Steuerung Plus/Minus-Button zum Expandieren/Kollabieren

// FALSE == kein Expand/Collapse-Button getroffen
BOOL SvImpLBox::IsNodeButton( const Point& rPosPixel, SvLBoxEntry* pEntry ) const
{
	if( !pEntry->HasChilds() && !pEntry->HasChildsOnDemand() )
		return FALSE;

	SvLBoxTab* pFirstDynamicTab = pView->GetFirstDynamicTab();
	if( !pFirstDynamicTab )
		return FALSE;

	long nMouseX = rPosPixel.X();
	// in Doc-Koords umrechnen
	Point aOrigin( pView->GetMapMode().GetOrigin() );
	nMouseX -= aOrigin.X();

	long nX = pView->GetTabPos( pEntry, pFirstDynamicTab);
	nX += nNodeBmpTabDistance;
	if( nMouseX < nX )
		return FALSE;
	nX += nNodeBmpWidth;
	if( nMouseX > nX )
		return FALSE;
	return TRUE;
}

// FALSE == hit no node button
BOOL SvImpLBox::ButtonDownCheckExpand( const MouseEvent& rMEvt, SvLBoxEntry* pEntry, long /* nY */ )
{
	BOOL bRet = FALSE;

	if ( pView->IsEditingActive() && pEntry == pView->pEdEntry )
		// inplace editing -> nothing to do
		bRet = TRUE;
	else if ( IsNodeButton( rMEvt.GetPosPixel(), pEntry ) )
	{
		if ( pView->IsExpanded( pEntry ) )
		{
			pView->EndEditing( TRUE );
			pView->Collapse( pEntry );
		}
		else
		{
			// you can expand an entry, which is in editing
			pView->Expand( pEntry );
		}
		bRet = TRUE;
	}

	return bRet;
}

void SvImpLBox::MouseButtonDown( const MouseEvent& rMEvt )
{
	if ( !rMEvt.IsLeft() && !rMEvt.IsRight())
		return;

#ifdef OS2
	// unter OS/2 kommt zwischen MouseButtonDown und
	// MouseButtonUp ein MouseMove
	nFlags |= F_IGNORE_NEXT_MOUSEMOVE;
#endif
	aEditTimer.Stop();
	Point aPos( rMEvt.GetPosPixel());

	if( aPos.X() > aOutputSize.Width() || aPos.Y() > aOutputSize.Height() )
		return;

	SvLBoxEntry* pEntry = GetEntry( aPos );
	if ( pEntry != pCursor )
		// new entry selected -> reset current tab position to first tab
		nCurTabPos = FIRST_ENTRY_TAB;
	nFlags &= (~F_FILLING);
	pView->GrabFocus();
	// #120417# the entry can still be invalid!
    if( !pEntry || !pView->GetViewData( pEntry ))
		return;

	long nY = GetEntryLine( pEntry );
	// Node-Button?
	if( ButtonDownCheckExpand( rMEvt, pEntry, nY ) )
		return;

	if( !EntryReallyHit(pEntry,aPos,nY))
		return;

	SvLBoxItem* pXItem = pView->GetItem( pEntry, aPos.X() );
	if( pXItem )
	{
		SvLBoxTab* pXTab = pView->GetTab( pEntry, pXItem );
		if ( !rMEvt.IsMod1() && !rMEvt.IsMod2() && rMEvt.IsLeft() && pXTab->IsEditable()
			&& pEntry == pView->FirstSelected() && NULL == pView->NextSelected( pEntry ) )
				// #i8234# FirstSelected() and NextSelected() ensures, that inplace editing is only triggered, when only one entry is selected
			nFlags |= F_START_EDITTIMER;
		if ( !pView->IsSelected( pEntry ) )
			nFlags &= ~F_START_EDITTIMER;
	}


	if( (rMEvt.GetClicks() % 2) == 0 )
	{
		nFlags &= (~F_START_EDITTIMER);
		pView->pHdlEntry = pEntry;
		if( pView->DoubleClickHdl() )
		{
			// falls im Handler der Eintrag geloescht wurde
			pEntry = GetClickedEntry( aPos );
			if( !pEntry )
				return;
			if( pEntry != pView->pHdlEntry )
			{
				// neu selektieren & tschuess
				if( !bSimpleTravel && !aSelEng.IsAlwaysAdding())
					SelAllDestrAnch( FALSE, TRUE ); // DeselectAll();
				SetCursor( pEntry );

				DBG_ERROR( "Please report what you did to get this assertion to FS!" );
					// The entry which has been double-clicked changed - and we select it, again.
					// I have situations where this behaviour does not make any sense at all - even more, it
					// leads to hacks to revert it's results.
					// So I'm not sure if this behaviour here is nonsense (which I believe at the moment),
					// or if there are really scenarious where it dones make sense ....
					// 07.12.2001 - 95727 - fs@openoffice.org

				return;
			}
			if( pEntry->HasChilds() || pEntry->HasChildsOnDemand() )
			{
				if( pView->IsExpanded(pEntry) )
					pView->Collapse( pEntry );
				else
					pView->Expand( pEntry );
				if( pEntry == pCursor )  // nur wenn Entryitem angeklickt wurde
										  // (Nodebutton ist kein Entryitem!)
					pView->Select( pCursor, TRUE );
				return;
			}
		}
	}
	else
	{
		// CheckButton? (TreeListBox: Check + Info)
		if( ButtonDownCheckCtrl(rMEvt, pEntry, nY) == TRUE)
			return;
		// Inplace-Editing?
#if 0
		if( rMEvt.IsMod2() && pView->IsInplaceEditingEnabled() )
		{
			SvLBoxItem* pItem = pView->GetItem( pEntry, aPos.X() );
			if( pItem )
				pView->EditingRequest( pEntry, pItem, aPos );
			return;
		}
#endif
	}
    if ( aSelEng.GetSelectionMode() != NO_SELECTION )
	    aSelEng.SelMouseButtonDown( rMEvt );
}

void SvImpLBox::MouseButtonUp( const MouseEvent& rMEvt)
{
#ifdef OS2
	nFlags &= (~F_IGNORE_NEXT_MOUSEMOVE);
#endif
	if ( !ButtonUpCheckCtrl( rMEvt ) && ( aSelEng.GetSelectionMode() != NO_SELECTION ) )
		aSelEng.SelMouseButtonUp( rMEvt );
	EndScroll();
	if( nFlags & F_START_EDITTIMER )
	{
		nFlags &= (~F_START_EDITTIMER);
		aEditClickPos = rMEvt.GetPosPixel();
		aEditTimer.Start();
	}

	return;
}

void SvImpLBox::MouseMove( const MouseEvent& rMEvt)
{
#ifdef OS2
	if( nFlags & F_IGNORE_NEXT_MOUSEMOVE )
	{
		nFlags &= (~F_IGNORE_NEXT_MOUSEMOVE);
		return;
	}
#endif
	SvLBoxEntry* pEntry = GetClickedEntry( rMEvt.GetPosPixel() );
	if ( !MouseMoveCheckCtrl( rMEvt, pEntry ) && ( aSelEng.GetSelectionMode() != NO_SELECTION ) )
		aSelEng.SelMouseMove( rMEvt );
	return;
}

BOOL SvImpLBox::KeyInput( const KeyEvent& rKEvt)
{
	aEditTimer.Stop();
	const KeyCode&	rKeyCode = rKEvt.GetKeyCode();

	if( rKeyCode.IsMod2() )
		return FALSE; // Alt-Taste nicht auswerten

	nFlags &= (~F_FILLING);

	if( !pCursor )
		pCursor = pStartEntry;
	if( !pCursor )
		return FALSE;

	BOOL bKeyUsed = TRUE;

	USHORT  nDelta = (USHORT)aVerSBar.GetPageSize();
	USHORT  aCode = rKeyCode.GetCode();

	BOOL    bShift = rKeyCode.IsShift();
	BOOL    bMod1 = rKeyCode.IsMod1();

	SvLBoxEntry* pNewCursor;

	switch( aCode )
	{
		case KEY_UP:
			if( !IsEntryInView( pCursor ) )
				MakeVisible( pCursor );

			pNewCursor = pCursor;
			do
			{
				pNewCursor = (SvLBoxEntry*)(pView->PrevVisible( pNewCursor ));
			} while( pNewCursor && !IsSelectable(pNewCursor) );

			if ( pNewCursor )
				// new entry selected -> reset current tab position to first tab
				nCurTabPos = FIRST_ENTRY_TAB;
			// if there is no next entry, take the current one
			// this ensures that in case of _one_ entry in the list, this entry is selected when pressing
			// the cursor key
			// 06.09.20001 - 83416 - fs@openoffice.org
			if ( !pNewCursor && pCursor )
				pNewCursor = pCursor;

			if( pNewCursor )
			{
				aSelEng.CursorPosChanging( bShift, bMod1 );
				SetCursor( pNewCursor, bMod1 );		// no selection, when Ctrl is on
				if( !IsEntryInView( pNewCursor ) )
					KeyUp( FALSE );
			}
			break;

		case KEY_DOWN:
			if( !IsEntryInView( pCursor ) )
				MakeVisible( pCursor );

			pNewCursor = pCursor;
			do
			{
				pNewCursor = (SvLBoxEntry*)(pView->NextVisible( pNewCursor ));
			} while( pNewCursor && !IsSelectable(pNewCursor) );

			if ( pNewCursor )
				// new entry selected -> reset current tab position to first tab
				nCurTabPos = FIRST_ENTRY_TAB;

			// if there is no next entry, take the current one
			// this ensures that in case of _one_ entry in the list, this entry is selected when pressing
			// the cursor key
			// 06.09.20001 - 83416 - frank.schoenheit@sun.com
			if ( !pNewCursor && pCursor )
				pNewCursor = pCursor;

			if( pNewCursor )
			{
				aSelEng.CursorPosChanging( bShift, bMod1 );
				if( IsEntryInView( pNewCursor ) )
					SetCursor( pNewCursor, bMod1 );	// no selection, when Ctrl is on
				else
				{
					if( pCursor )
						pView->Select( pCursor, FALSE );
					KeyDown( FALSE );
					SetCursor( pNewCursor, bMod1 );	// no selection, when Ctrl is on
				}
			}
			else
				KeyDown( FALSE ); // weil ScrollBar-Range evtl. noch
								  // scrollen erlaubt
			break;

		case KEY_RIGHT:
		{
			if( bSubLstOpLR && IsNowExpandable() )
				pView->Expand( pCursor );
            else if ( bIsCellFocusEnabled && pCursor )
            {
                if ( nCurTabPos < ( pView->TabCount() - 1 /*!2*/ ) )
                {
                    ++nCurTabPos;
                    ShowCursor( TRUE );
                    CallEventListeners( VCLEVENT_LISTBOX_SELECT, pCursor );
                }
            }
			else if( pView->nWindowStyle & WB_HSCROLL )
			{
				long	nThumb = aHorSBar.GetThumbPos();
				nThumb += aHorSBar.GetLineSize();
				long	nOldThumb = aHorSBar.GetThumbPos();
				aHorSBar.SetThumbPos( nThumb );
				nThumb = nOldThumb;
				nThumb -= aHorSBar.GetThumbPos();
				nThumb *= -1;
				if( nThumb )
				{
					KeyLeftRight( nThumb );
					EndScroll();
				}
			}
			else
				bKeyUsed = FALSE;
			break;
		}

		case KEY_LEFT:
		{
            if ( bIsCellFocusEnabled )
            {
                if ( nCurTabPos > FIRST_ENTRY_TAB )
                {
                    --nCurTabPos;
                    ShowCursor( TRUE );
                    CallEventListeners( VCLEVENT_LISTBOX_SELECT, pCursor );
                }
            }
            else if ( pView->nWindowStyle & WB_HSCROLL )
			{
				long	nThumb = aHorSBar.GetThumbPos();
				nThumb -= aHorSBar.GetLineSize();
				long	nOldThumb = aHorSBar.GetThumbPos();
				aHorSBar.SetThumbPos( nThumb );
				nThumb = nOldThumb;
				nThumb -= aHorSBar.GetThumbPos();
				if( nThumb )
				{
					KeyLeftRight( -nThumb );
					EndScroll();
				}
				else if( bSubLstOpLR )
				{
					if( IsExpandable() && pView->IsExpanded( pCursor ) )
						pView->Collapse( pCursor );
					else
					{
						pNewCursor = pView->GetParent( pCursor );
						if( pNewCursor )
							SetCursor( pNewCursor );
					}
				}
			}
			else if( bSubLstOpLR && IsExpandable() )
				pView->Collapse( pCursor );
			else
				bKeyUsed = FALSE;
			break;
		}

		case KEY_PAGEUP:
			if( !bMod1 )
			{
				pNewCursor = (SvLBoxEntry*)(pView->PrevVisible( pCursor, nDelta ));

				while( nDelta && pNewCursor && !IsSelectable(pNewCursor) )
				{
					pNewCursor = (SvLBoxEntry*)(pView->NextVisible( pNewCursor ));
					nDelta--;
				}

				if( nDelta )
				{
					DBG_ASSERT(pNewCursor&&(ULONG)pNewCursor!=(ULONG)pCursor,"Cursor?");
					aSelEng.CursorPosChanging( bShift, bMod1 );
					if( IsEntryInView( pNewCursor ) )
						SetCursor( pNewCursor );
					else
					{
						SetCursor( pNewCursor );
						KeyUp( TRUE );
					}
				}
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_PAGEDOWN:
			if( !bMod1 )
			{
				pNewCursor= (SvLBoxEntry*)(pView->NextVisible( pCursor, nDelta ));

				while( nDelta && pNewCursor && !IsSelectable(pNewCursor) )
				{
					pNewCursor = (SvLBoxEntry*)(pView->PrevVisible( pNewCursor ));
					nDelta--;
				}

				if( nDelta )
				{
					DBG_ASSERT(pNewCursor&&(ULONG)pNewCursor!=(ULONG)pCursor,"Cursor?");
					aSelEng.CursorPosChanging( bShift, bMod1 );
					if( IsEntryInView( pNewCursor ) )
						SetCursor( pNewCursor );
					else
					{
						SetCursor( pNewCursor );
						KeyDown( TRUE );
					}
				}
				else
					KeyDown( FALSE ); // siehe KEY_DOWN
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_SPACE:
            if ( pView->GetSelectionMode() != NO_SELECTION )
            {
			    if ( bMod1 )
			    {
				    if ( pView->GetSelectionMode() == MULTIPLE_SELECTION && !bShift )
					    // toggle selection
					    pView->Select( pCursor, !pView->IsSelected( pCursor ) );
			    }
			    else if ( !bShift /*&& !bMod1*/ )
			    {
				    if ( aSelEng.IsAddMode() )
					    // toggle selection
					    pView->Select( pCursor, !pView->IsSelected( pCursor ) );
				    else
				    {
					    SelAllDestrAnch( FALSE );
					    pView->Select( pCursor, TRUE );
				    }
			    }
                else
				    bKeyUsed = FALSE;
            }
			else
				bKeyUsed = FALSE;
			break;

		case KEY_RETURN:
			if( bSubLstOpRet && IsExpandable() )
			{
				if( pView->IsExpanded( pCursor ) )
					pView->Collapse( pCursor );
				else
					pView->Expand( pCursor );
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_F2:
			if( !bShift && !bMod1 )
			{
				aEditClickPos = Point( -1, -1 );
				EditTimerCall( 0 );
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_F8:
			if( bShift && pView->GetSelectionMode()==MULTIPLE_SELECTION &&
				!(nWinBits & WB_SIMPLEMODE))
			{
				if( aSelEng.IsAlwaysAdding() )
					aSelEng.AddAlways( FALSE );
				else
					aSelEng.AddAlways( TRUE );
			}
			else
				bKeyUsed = FALSE;
			break;


#ifdef OV_DEBUG
		case KEY_F9:
			MakeVisible( pCursor );
			break;
		case KEY_F10:
			pView->RemoveSelection();
			break;
		case KEY_DELETE:
			pView->RemoveEntry( pCursor );
			break;
#endif

		case KEY_ADD:
			if( pCursor )
			{
				if( !pView->IsExpanded(pCursor))
					pView->Expand( pCursor );
				if( bMod1 )
				{
					USHORT nRefDepth = pTree->GetDepth( pCursor );
					SvLBoxEntry* pCur = pTree->Next( pCursor );
					while( pCur && pTree->GetDepth(pCur) > nRefDepth )
					{
						if( pCur->HasChilds() && !pView->IsExpanded(pCur))
							pView->Expand( pCur );
						pCur = pTree->Next( pCur );
					}
				}
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_A:
			if( bMod1 )
				SelAllDestrAnch( TRUE );
//			else
//				bKeyUsed = FALSE;	#105907# assume user wants to use quicksearch with key "a", so key is handled!
			break;

		case KEY_SUBTRACT:
			if( pCursor )
			{
				if( pView->IsExpanded(pCursor))
					pView->Collapse( pCursor );
				if( bMod1 )
				{
					// bis zur Root alle Parents einklappen
					SvLBoxEntry* pParentToCollapse = (SvLBoxEntry*)pTree->GetRootLevelParent(pCursor);
					if( pParentToCollapse )
					{
						USHORT nRefDepth;
						// Sonderbehandlung Explorer: Befindet sich auf der
						// Root nur ein Eintrag,dann den Root-Entry nicht
						// einklappen
						if( pTree->GetChildList(0)->Count() < 2 )
						{
							nRefDepth = 1;
							pParentToCollapse = pCursor;
							while( pTree->GetParent(pParentToCollapse) &&
								   pTree->GetDepth( pTree->GetParent(pParentToCollapse)) > 0)
							{
								pParentToCollapse = pTree->GetParent(pParentToCollapse);
							}
						}
						else
							nRefDepth = 0;

						if( pView->IsExpanded(pParentToCollapse) )
							pView->Collapse( pParentToCollapse );
						SvLBoxEntry* pCur = pTree->Next( pParentToCollapse );
						while( pCur && pTree->GetDepth(pCur) > nRefDepth )
						{
							if( pCur->HasChilds() && pView->IsExpanded(pCur) )
								pView->Collapse( pCur );
							pCur = pTree->Next( pCur );
						}
					}
				}
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_DIVIDE :
			if( bMod1 )
				SelAllDestrAnch( TRUE );
			else
				bKeyUsed = FALSE;
			break;

		case KEY_COMMA :
			if( bMod1 )
				SelAllDestrAnch( FALSE );
			else
				bKeyUsed = FALSE;
			break;

		case KEY_HOME :
			pNewCursor = pView->GetModel()->First();

			while( pNewCursor && !IsSelectable(pNewCursor) )
			{
				pNewCursor = (SvLBoxEntry*)(pView->NextVisible( pNewCursor ));
			}

			if( pNewCursor && pNewCursor != pCursor )
			{
//				SelAllDestrAnch( FALSE );
				aSelEng.CursorPosChanging( bShift, bMod1 );
				SetCursor( pNewCursor );
				if( !IsEntryInView( pNewCursor ) )
					MakeVisible( pNewCursor );
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_END :
			pNewCursor = pView->GetModel()->Last();

			while( pNewCursor && !IsSelectable(pNewCursor) )
			{
				pNewCursor = (SvLBoxEntry*)(pView->PrevVisible( pNewCursor ));
			}

			if( pNewCursor && pNewCursor != pCursor)
			{
//				SelAllDestrAnch( FALSE );
				aSelEng.CursorPosChanging( bShift, bMod1 );
				SetCursor( pNewCursor );
				if( !IsEntryInView( pNewCursor ) )
					MakeVisible( pNewCursor );
			}
			else
				bKeyUsed = FALSE;
			break;

		case KEY_ESCAPE:
		case KEY_TAB:
		case KEY_DELETE:
		case KEY_BACKSPACE:
			// #105907# must not be handled because this quits dialogs and does other magic things...
			// if there are other single keys which should not be handled, they can be added here
			bKeyUsed = FALSE;
			break;

		default:
			if( bMod1 || rKeyCode.GetGroup() == KEYGROUP_FKEYS )
				// #105907# CTRL or Function key is pressed, assume user don't want to use quicksearch...
				// if there are groups of keys which should not be handled, they can be added here
				bKeyUsed = FALSE;
	}
	return bKeyUsed;
}

void __EXPORT SvImpLBox::GetFocus()
{
	if( pCursor )
	{
		pView->SetEntryFocus( pCursor, TRUE );
		ShowCursor( TRUE );
// auskommentiert wg. deselectall
//		if( bSimpleTravel && !pView->IsSelected(pCursor) )
//			pView->Select( pCursor, TRUE );
	}
	if( nWinBits & WB_HIDESELECTION )
	{
		SvLBoxEntry* pEntry = pView->FirstSelected();
		while( pEntry )
		{
			InvalidateEntry( pEntry );
			pEntry = pView->NextSelected( pEntry );
		}
		/*
		SvLBoxEntry* pEntry = pView->GetModel()->First();
		while( pEntry )
		{
			SvViewData* pViewData = pView->GetViewData( pEntry );
			if( pViewData->IsCursored() )
			{
				pViewData->SetCursored( FALSE );
				InvalidateEntry( pEntry );
			}
			pEntry = pView->GetModel()->Next( pEntry );
		}
		*/


	}
}

void __EXPORT SvImpLBox::LoseFocus()
{
	aEditTimer.Stop();
	if( pCursor )
		pView->SetEntryFocus( pCursor,FALSE );
	ShowCursor( FALSE );

	if( nWinBits & WB_HIDESELECTION )
	{
		SvLBoxEntry* pEntry = pView->FirstSelected();
		while( pEntry )
		{
			//SvViewData* pViewData = pView->GetViewData( pEntry );
			//pViewData->SetCursored( TRUE );
			InvalidateEntry( pEntry );
			pEntry = pView->NextSelected( pEntry );
		}
	}
}


// ********************************************************************
// SelectionEngine
// ********************************************************************

inline void SvImpLBox::SelectEntry( SvLBoxEntry* pEntry, BOOL bSelect )
{
	pView->Select( pEntry, bSelect );
}

__EXPORT ImpLBSelEng::ImpLBSelEng( SvImpLBox* pImpl, SelectionEngine* pSEng,
	SvTreeListBox* pV )
{
	pImp = pImpl;
	pSelEng = pSEng;
	pView = pV;
}

__EXPORT ImpLBSelEng::~ImpLBSelEng()
{
}

void __EXPORT ImpLBSelEng::BeginDrag()
{
	pImp->BeginDrag();
}

/*
void __EXPORT ImpLBSelEng::EndDrag( const Point& )
{
}
*/

void __EXPORT ImpLBSelEng::CreateAnchor()
{
	pImp->pAnchor = pImp->pCursor;
}

void __EXPORT ImpLBSelEng::DestroyAnchor()
{
	pImp->pAnchor = 0;
}

/*
void __EXPORT ImpLBSelEng::CreateCursor()
{
	pImp->pAnchor = 0;
}
*/


BOOL __EXPORT ImpLBSelEng::SetCursorAtPoint(const Point& rPoint, BOOL bDontSelectAtCursor)
{
	SvLBoxEntry* pNewCursor = pImp->MakePointVisible( rPoint );
	if( pNewCursor != pImp->pCursor  )
		pImp->BeginScroll();

	if( pNewCursor )
	{
		// bei SimpleTravel wird in SetCursor selektiert und
		// der Select-Handler gerufen
		//if( !bDontSelectAtCursor && !pImp->bSimpleTravel )
		//	pImp->SelectEntry( pNewCursor, TRUE );
		pImp->SetCursor( pNewCursor, bDontSelectAtCursor );
		return TRUE;
	}
	return FALSE;
}

BOOL __EXPORT ImpLBSelEng::IsSelectionAtPoint( const Point& rPoint )
{
	SvLBoxEntry* pEntry = pImp->MakePointVisible( rPoint );
	if( pEntry )
		return pView->IsSelected(pEntry);
	return FALSE;
}

void __EXPORT ImpLBSelEng::DeselectAtPoint( const Point& rPoint )
{
	SvLBoxEntry* pEntry = pImp->MakePointVisible( rPoint );
	if( !pEntry )
		return;
	pImp->SelectEntry( pEntry, FALSE );
}

/*
void __EXPORT ImpLBSelEng::SelectAtPoint( const Point& rPoint )
{
	SvLBoxEntry* pEntry = pImp->MakePointVisible( rPoint );
	if( !pEntry )
		return;
	pImp->SelectEntry( pEntry, TRUE );
}
*/

void __EXPORT ImpLBSelEng::DeselectAll()
{
	pImp->SelAllDestrAnch( FALSE, FALSE ); // SelectionEngine nicht resetten!
	pImp->nFlags &= (~F_DESEL_ALL);
}

// ***********************************************************************
// Selektion
// ***********************************************************************

void SvImpLBox::SetAnchorSelection(SvLBoxEntry* pOldCursor,SvLBoxEntry* pNewCursor)
{
	SvLBoxEntry* pEntry;
	ULONG nAnchorVisPos = pView->GetVisiblePos( pAnchor );
	ULONG nOldVisPos = pView->GetVisiblePos( pOldCursor );
	ULONG nNewVisPos = pView->GetVisiblePos( pNewCursor );

	if( nOldVisPos > nAnchorVisPos ||
		( nAnchorVisPos==nOldVisPos && nNewVisPos > nAnchorVisPos) )
	{
		if( nNewVisPos > nOldVisPos )
		{
			pEntry = pOldCursor;
			while( pEntry && pEntry != pNewCursor )
			{
				pView->Select( pEntry, TRUE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, TRUE );
			return;
		}

		if( nNewVisPos < nAnchorVisPos )
		{
			pEntry = pAnchor;
			while( pEntry && pEntry != pOldCursor )
			{
				pView->Select( pEntry, FALSE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, FALSE );

			pEntry = pNewCursor;
			while( pEntry && pEntry != pAnchor )
			{
				pView->Select( pEntry, TRUE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, TRUE );
			return;
		}

		if( nNewVisPos < nOldVisPos )
		{
			pEntry = pNewCursor;
			pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			while( pEntry && pEntry != pOldCursor )
			{
				pView->Select( pEntry, FALSE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, FALSE );
			return;
		}
	}
	else
	{
		if( nNewVisPos < nOldVisPos )  // Vergroessern der Selektion
		{
			pEntry = pNewCursor;
			while( pEntry && pEntry != pOldCursor )
			{
				pView->Select( pEntry, TRUE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, TRUE );
			return;
		}

		if( nNewVisPos > nAnchorVisPos )
		{
			pEntry = pOldCursor;
			while( pEntry && pEntry != pAnchor )
			{
				pView->Select( pEntry, FALSE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, FALSE );
			pEntry = pAnchor;
			while( pEntry && pEntry != pNewCursor )
			{
				pView->Select( pEntry, TRUE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			if( pEntry )
				pView->Select( pEntry, TRUE );
			return;
		}

		if( nNewVisPos > nOldVisPos )
		{
			pEntry = pOldCursor;
			while( pEntry && pEntry != pNewCursor )
			{
				pView->Select( pEntry, FALSE );
				pEntry = (SvLBoxEntry*)(pView->NextVisible( pEntry ));
			}
			return;
		}
	}
}

void SvImpLBox::SelAllDestrAnch( BOOL bSelect, BOOL bDestroyAnchor,
	BOOL bSingleSelToo )
{
	SvLBoxEntry* pEntry;
	nFlags &= (~F_DESEL_ALL);
	if( bSelect && bSimpleTravel )
	{
		if( pCursor && !pView->IsSelected( pCursor ))
		{
			pView->Select( pCursor, TRUE );
		}
		return;
	}
	if( !bSelect && pView->GetSelectionCount() == 0 )
	{
		if( bSimpleTravel && ( !GetUpdateMode() || !pCursor) )
			nFlags |= F_DESEL_ALL;
		return;
	}
	if( bSelect && pView->GetSelectionCount() == pView->GetEntryCount())
		return;
	if( !bSingleSelToo && bSimpleTravel )
		return;

	if( !bSelect && pView->GetSelectionCount()==1 && pCursor &&
		pView->IsSelected( pCursor ))
	{
		pView->Select( pCursor, FALSE );
		if( bDestroyAnchor )
			DestroyAnchor(); // Anker loeschen & SelectionEngine zuruecksetzen
		else
			pAnchor = 0; // internen Anker immer loeschen
		return;
	}

	if( bSimpleTravel && !pCursor && !GetUpdateMode() )
		nFlags |= F_DESEL_ALL;

	ShowCursor( FALSE );
	BOOL bUpdate = GetUpdateMode();

	nFlags |= F_IGNORE_SELECT; // EntryInserted soll nix tun
	pEntry = pTree->First();
	while( pEntry )
	{
		if( pView->Select( pEntry, bSelect ) )
		{
			if( bUpdate && pView->IsEntryVisible(pEntry) )
			{
				long nY = GetEntryLine( pEntry );
				if( IsLineVisible( nY ) )
					pView->PaintEntry1( pEntry, nY, 0xffff ); // wg. ItemsetBrowser SV_LBOXTAB_SHOW_SELECTION );
			}
		}
		pEntry = pTree->Next( pEntry );
	}
	nFlags &= ~F_IGNORE_SELECT;

	if( bDestroyAnchor )
		DestroyAnchor(); // Anker loeschen & SelectionEngine zuruecksetzen
	else
		pAnchor = 0; // internen Anker immer loeschen
	ShowCursor( TRUE );
}

void SvImpLBox::SetSelectionMode( SelectionMode eSelMode  )
{
	aSelEng.SetSelectionMode( eSelMode);
	if( eSelMode == SINGLE_SELECTION )
		bSimpleTravel = TRUE;
	else
		bSimpleTravel = FALSE;
	if( (nWinBits & WB_SIMPLEMODE) && (eSelMode == MULTIPLE_SELECTION) )
		aSelEng.AddAlways( TRUE );
}

// ***********************************************************************
// Drag & Drop
// ***********************************************************************

void SvImpLBox::SetDragDropMode( DragDropMode eDDMode )
{
	if( eDDMode && eDDMode != SV_DRAGDROP_APP_DROP )
	{
		aSelEng.ExpandSelectionOnMouseMove( FALSE );
		aSelEng.EnableDrag( TRUE );
	}
	else
	{
		aSelEng.ExpandSelectionOnMouseMove( TRUE );
		aSelEng.EnableDrag( FALSE );
	}
}

void SvImpLBox::BeginDrag()
{
	nFlags &= (~F_FILLING);
	if( !bAsyncBeginDrag )
	{
		BeginScroll();
		pView->StartDrag( 0, aSelEng.GetMousePosPixel() );
		EndScroll();
	}
	else
	{
		aAsyncBeginDragPos = aSelEng.GetMousePosPixel();
		aAsyncBeginDragTimer.Start();
	}
}

IMPL_LINK( SvImpLBox, BeginDragHdl, void*, EMPTYARG )
{
	pView->StartDrag( 0, aAsyncBeginDragPos );
	return 0;
}

void SvImpLBox::PaintDDCursor( SvLBoxEntry* pInsertionPos )
{
	long nY;
	if( pInsertionPos )
	{
		nY = GetEntryLine( pInsertionPos );
		nY += pView->GetEntryHeight();
	}
	else
		nY = 1;
	RasterOp eOldOp = pView->GetRasterOp();
	pView->SetRasterOp( ROP_INVERT );
	Color aOldLineColor = pView->GetLineColor();
	pView->SetLineColor( Color( COL_BLACK ) );
	pView->DrawLine( Point( 0, nY ), Point( aOutputSize.Width(), nY ) );
	pView->SetLineColor( aOldLineColor );
	pView->SetRasterOp( eOldOp );
}
/* -----------------26.08.2003 12:52-----------------
    Delete all sub menues of a PopupMenu, recursively
 --------------------------------------------------*/
void lcl_DeleteSubPopups(PopupMenu* pPopup)
{
    for(USHORT i = 0; i < pPopup->GetItemCount(); i++)
    {
        PopupMenu* pSubPopup = pPopup->GetPopupMenu( pPopup->GetItemId( i ));
        if(pSubPopup)
		{
            lcl_DeleteSubPopups(pSubPopup);
			delete pSubPopup;
		}
    }
}

void SvImpLBox::Command( const CommandEvent& rCEvt )
{
	USHORT				nCommand = rCEvt.GetCommand();

    if( nCommand == COMMAND_CONTEXTMENU )
        aEditTimer.Stop();

	// Rollmaus-Event?
	if( ( ( nCommand == COMMAND_WHEEL ) || ( nCommand == COMMAND_STARTAUTOSCROLL ) || ( nCommand == COMMAND_AUTOSCROLL ) )
		&& pView->HandleScrollCommand( rCEvt, &aHorSBar, &aVerSBar ) )
			return;

	if( bContextMenuHandling && nCommand == COMMAND_CONTEXTMENU )
	{
		Point	aPopupPos;
		BOOL	bClickedIsFreePlace = FALSE;
		std::stack<SvLBoxEntry*> aSelRestore;

		if( rCEvt.IsMouseEvent() )
		{	// change selection, if mouse pos doesn't fit to selection

			aPopupPos = rCEvt.GetMousePosPixel();

			SvLBoxEntry*	pClickedEntry = GetEntry( aPopupPos );
			if( pClickedEntry )
			{	// mouse in non empty area
				BOOL				bClickedIsSelected = FALSE;

				// collect the currently selected entries
				SvLBoxEntry*		pSelected = pView->FirstSelected();
				while( pSelected )
				{
					bClickedIsSelected |= ( pClickedEntry == pSelected );
					pSelected = pView->NextSelected( pSelected );
				}

				// if the entry which the user clicked at is not selected
				if( !bClickedIsSelected )
				{	// deselect all other and select the clicked one
					pView->SelectAll( FALSE );
					pView->SetCursor( pClickedEntry );
				}
			}
			else if( aSelEng.GetSelectionMode() == SINGLE_SELECTION )
			{//modified by BerryJia for fixing Bug102739 2002-9-9 17:00(Beijing Time)
				bClickedIsFreePlace = TRUE;
				INT32				nSelectedEntries = pView->GetSelectionCount();
				SvLBoxEntry*		pSelected = pView->FirstSelected();
				for(USHORT nSel = 0; nSel < nSelectedEntries; nSel++ )
				{
					aSelRestore.push(pSelected);
					pSelected = pView->NextSelected( pSelected );
				}
				pView->SelectAll( FALSE );
			}
			else
			{	// deselect all
				pView->SelectAll( FALSE );
			}


		}
		else
		{	// key event (or at least no mouse event)
			INT32	nSelectionCount = pView->GetSelectionCount();

			if( nSelectionCount )
			{	// now allways take first visible as base for positioning the menu
				SvLBoxEntry*	pSelected = pView->FirstSelected();
				while( pSelected )
				{
					if( IsEntryInView( pSelected ) )
						break;

					pSelected = pView->NextSelected( pSelected );
				}

				if( !pSelected )
				{
					// no one was visible
					pSelected = pView->FirstSelected();
					pView->MakeVisible( pSelected );
				}

				aPopupPos = pView->GetFocusRect( pSelected, pView->GetEntryPosition( pSelected ).Y() ).Center();
			}
			else
				aPopupPos = Point( 0, 0 );
		}

		PopupMenu*	pPopup = pView->CreateContextMenu();

		if( pPopup )
		{
			// do action for selected entry in popup menu
            USHORT nMenuAction = pPopup->Execute( pView, aPopupPos );
            if ( nMenuAction )
			    pView->ExcecuteContextMenuAction( nMenuAction );
            lcl_DeleteSubPopups(pPopup);
            delete pPopup;
		}
		//added by BerryJia for fixing Bug102739 2002-9-9 17:00(Beijing Time)
		if( bClickedIsFreePlace )
		{
			while(!aSelRestore.empty())
			{
				SvLBoxEntry* pEntry = aSelRestore.top();
				//#i19717# the entry is maybe already deleted
				bool bFound = false;
				for(ULONG nEntry = 0; nEntry < pView->GetEntryCount(); nEntry++)
					if(pEntry == pView->GetEntry(nEntry))
					{
						bFound = true;
						break;
					}
				if(bFound)
					SetCurEntry( pEntry );
				aSelRestore.pop();
			}
		}
	}
#ifndef NOCOMMAND
	else
	{
		const Point& rPos = rCEvt.GetMousePosPixel();
		if( rPos.X() < aOutputSize.Width() && rPos.Y() < aOutputSize.Height() )
			aSelEng.Command( rCEvt );
	}
#endif
}

void SvImpLBox::BeginScroll()
{
	if( !(nFlags & F_IN_SCROLLING))
	{
		pView->NotifyBeginScroll();
		nFlags |= F_IN_SCROLLING;
	}
}

void SvImpLBox::EndScroll()
{
	if( nFlags & F_IN_SCROLLING)
	{
		pView->NotifyEndScroll();
		nFlags &= (~F_IN_SCROLLING);
	}
}


Rectangle SvImpLBox::GetVisibleArea() const
{
	Point aPos( pView->GetMapMode().GetOrigin() );
	aPos.X() *= -1;
	Rectangle aRect( aPos, aOutputSize );
	return aRect;
}

void SvImpLBox::Invalidate()
{
	pView->SetClipRegion();
}

void SvImpLBox::SetCurEntry( SvLBoxEntry* pEntry )
{
	if  (  ( aSelEng.GetSelectionMode() != SINGLE_SELECTION )
        && ( aSelEng.GetSelectionMode() != NO_SELECTION )
        )
		SelAllDestrAnch( FALSE, TRUE, FALSE );
    if ( pEntry )
	    MakeVisible( pEntry );
	SetCursor( pEntry );
    if ( pEntry && ( aSelEng.GetSelectionMode() != NO_SELECTION ) )
		pView->Select( pEntry, TRUE );
}

IMPL_LINK( SvImpLBox, EditTimerCall, Timer *, EMPTYARG )
{
	if( pView->IsInplaceEditingEnabled() )
	{
		sal_Bool bIsMouseTriggered = aEditClickPos.X() >= 0;
		if ( bIsMouseTriggered )
		{
			Point aCurrentMousePos = pView->GetPointerPosPixel();
			if	(	( abs( aCurrentMousePos.X() - aEditClickPos.X() ) > 5 )
				||	( abs( aCurrentMousePos.Y() - aEditClickPos.Y() ) > 5 )
				)
			{
				return 0L;
			}
		}

		SvLBoxEntry* pEntry = GetCurEntry();
		if( pEntry )
		{
			ShowCursor( FALSE );
			pView->ImplEditEntry( pEntry );
			ShowCursor( TRUE );
		}
	}
	return 0;
}

BOOL SvImpLBox::RequestHelp( const HelpEvent& rHEvt )
{
	if( rHEvt.GetMode() & HELPMODE_QUICK )
	{
		Point aPos( pView->ScreenToOutputPixel( rHEvt.GetMousePosPixel() ));
		if( !GetVisibleArea().IsInside( aPos ))
			return FALSE;

		SvLBoxEntry* pEntry = GetEntry( aPos );
		if( pEntry )
		{
			// Rechteck des Textes berechnen
			SvLBoxTab* pTab;
			SvLBoxString* pItem = (SvLBoxString*)(pView->GetItem( pEntry, aPos.X(), &pTab ));
			if( !pItem || pItem->IsA() != SV_ITEM_ID_LBOXSTRING )
				return FALSE;

			aPos = GetEntryPosition( pEntry );
			aPos.X() = pView->GetTabPos( pEntry, pTab ); //pTab->GetPos();
			Size aSize( pItem->GetSize( pView, pEntry ) );
			SvLBoxTab* pNextTab = NextTab( pTab );
			BOOL bItemClipped = FALSE;
			// wurde das Item von seinem rechten Nachbarn abgeschnitten?
			if( pNextTab && pView->GetTabPos(pEntry,pNextTab) < aPos.X()+aSize.Width() )
			{
				aSize.Width() = pNextTab->GetPos() - pTab->GetPos();
				bItemClipped = TRUE;
			}
			Rectangle aItemRect( aPos, aSize );

			Rectangle aViewRect( GetVisibleArea() );

			if( bItemClipped || !aViewRect.IsInside( aItemRect ) )
			{
				// rechten Item-Rand am View-Rand clippen
				//if( aItemRect.Right() > aViewRect.Right() )
				//	aItemRect.Right() = aViewRect.Right();

				Point aPt = pView->OutputToScreenPixel( aItemRect.TopLeft() );
				aItemRect.Left()   = aPt.X();
				aItemRect.Top()    = aPt.Y();
				aPt = pView->OutputToScreenPixel( aItemRect.BottomRight() );
				aItemRect.Right()  = aPt.X();
				aItemRect.Bottom() = aPt.Y();

				Help::ShowQuickHelp( pView, aItemRect,
									 pItem->GetText(), QUICKHELP_LEFT | QUICKHELP_VCENTER );
				return TRUE;
			}
		}
	}
	return FALSE;
}

SvLBoxTab* SvImpLBox::NextTab( SvLBoxTab* pTab )
{
	USHORT nTabCount = pView->TabCount();
	if( nTabCount <= 1 )
		return 0;
	for( USHORT nTab=0; nTab < (nTabCount-1); nTab++)
	{
		if( pView->aTabs[nTab]==pTab )
			return (SvLBoxTab*)(pView->aTabs[nTab+1]);
	}
	return 0;
}

void SvImpLBox::EndSelection()
{
	DestroyAnchor();
	nFlags &=  ~F_START_EDITTIMER;
}

void SvImpLBox::RepaintScrollBars()
{
}

void SvImpLBox::SetUpdateMode( BOOL bMode )
{
	if( bUpdateMode != bMode )
	{
		bUpdateMode = bMode;
		if( bUpdateMode )
			UpdateAll( FALSE );
	}
}

void SvImpLBox::SetUpdateModeFast( BOOL bMode )
{
	if( bUpdateMode != bMode )
	{
		bUpdateMode = bMode;
		if( bUpdateMode )
			UpdateAll( FALSE, FALSE );
	}
}


BOOL SvImpLBox::SetMostRight( SvLBoxEntry* pEntry )
{
	if( pView->nTreeFlags & TREEFLAG_RECALCTABS )
	{
		nFlags |= F_IGNORE_CHANGED_TABS;
		pView->SetTabs();
		nFlags &= ~F_IGNORE_CHANGED_TABS;
	}

	USHORT nLastTab = pView->aTabs.Count() - 1;
	USHORT nLastItem = pEntry->ItemCount() - 1;
	if( nLastTab != USHRT_MAX && nLastItem != USHRT_MAX )
	{
		if( nLastItem < nLastTab )
			nLastTab = nLastItem;

		SvLBoxTab* pTab = (SvLBoxTab*)pView->aTabs[ nLastTab ];
		SvLBoxItem* pItem = pEntry->GetItem( nLastTab );

		long nTabPos = pView->GetTabPos( pEntry, pTab );

		long nMaxRight = GetOutputSize().Width();
		Point aPos( pView->GetMapMode().GetOrigin() );
		aPos.X() *= -1; // Umrechnung Dokumentkoord.
		nMaxRight = nMaxRight + aPos.X() - 1;

		long nNextTab = nTabPos < nMaxRight ? nMaxRight : nMaxRight + 50;
		long nTabWidth = nNextTab - nTabPos + 1;
		long nItemSize = pItem->GetSize(pView,pEntry).Width();
		long nOffset = pTab->CalcOffset( nItemSize, nTabWidth );

		long nRight = nTabPos + nOffset + nItemSize;
		if( nRight > nMostRight )
		{
			nMostRight = nRight;
			pMostRightEntry = pEntry;
			return TRUE;
		}
	}
	return FALSE;
}

void SvImpLBox::FindMostRight( SvLBoxEntry* pEntryToIgnore )
{
	nMostRight = -1;
	pMostRightEntry = 0;
	if( !pView->GetModel() )
		return;

	SvLBoxEntry* pEntry = (SvLBoxEntry*)pView->FirstVisible();
	while( pEntry )
	{
		if( pEntry != pEntryToIgnore )
			SetMostRight( pEntry );
		pEntry = (SvLBoxEntry*)pView->NextVisible( pEntry );
	}
}

void SvImpLBox::FindMostRight( SvLBoxEntry* pParent, SvLBoxEntry* pEntryToIgnore )
{
	if( !pParent )
		FindMostRight( pEntryToIgnore );
	else
		FindMostRight_Impl( pParent, pEntryToIgnore  );
}

void SvImpLBox::FindMostRight_Impl( SvLBoxEntry* pParent, SvLBoxEntry* pEntryToIgnore )
{
	SvTreeEntryList* pList = pTree->GetChildList( pParent );

	if( !pList )
		return;

	ULONG nCount = pList->Count();
	for( ULONG nCur = 0; nCur < nCount; nCur++ )
	{
		SvLBoxEntry* pChild = (SvLBoxEntry*)pList->GetObject( nCur );
		if( pChild != pEntryToIgnore )
		{
			SetMostRight( pChild );
			if( pChild->HasChilds() && pView->IsExpanded( pChild ))
				FindMostRight_Impl( pChild, pEntryToIgnore );
		}
	}
}

void SvImpLBox::NotifyTabsChanged()
{
	if( GetUpdateMode() && !(nFlags & F_IGNORE_CHANGED_TABS ) &&
		nCurUserEvent == 0xffffffff )
	{
		nCurUserEvent = Application::PostUserEvent(LINK(this,SvImpLBox,MyUserEvent),(void*)0);
	}
}

IMPL_LINK(SvImpLBox,MyUserEvent,void*, pArg )
{
	nCurUserEvent = 0xffffffff;
	if( !pArg )
	{
		pView->Invalidate();
		pView->Update();
	}
	else
	{
		FindMostRight( 0 );
		ShowVerSBar();
		pView->Invalidate( GetVisibleArea() );
	}
	return 0;
}


void SvImpLBox::StopUserEvent()
{
	if( nCurUserEvent != 0xffffffff )
	{
		Application::RemoveUserEvent( nCurUserEvent );
		nCurUserEvent = 0xffffffff;
	}
}

void SvImpLBox::ShowFocusRect( const SvLBoxEntry* pEntry )
{
	if( pEntry )
	{
		long nY = GetEntryLine( (SvLBoxEntry*)pEntry );
		Rectangle aRect = pView->GetFocusRect( (SvLBoxEntry*)pEntry, nY );
		Region aOldClip( pView->GetClipRegion());
		Region aClipRegion( GetClipRegionRect() );
		pView->SetClipRegion( aClipRegion );
		pView->ShowFocus( aRect );
		pView->SetClipRegion( aOldClip );

	}
	else
	{
		pView->HideFocus();
	}
}

void SvImpLBox::SetTabBar( TabBar* _pTabBar )
{
	pTabBar = _pTabBar;
}

void SvImpLBox::CancelPendingEdit()
{
	if( aEditTimer.IsActive() )
		aEditTimer.Stop();
	nFlags &= ~F_START_EDITTIMER;
}

// -----------------------------------------------------------------------
void SvImpLBox::implInitDefaultNodeImages()
{
	if ( s_pDefCollapsed )
		// assume that all or nothing is initialized
		return;

	s_pDefCollapsed = new Image( SvtResId( RID_IMG_TREENODE_COLLAPSED ) );
	s_pDefCollapsedHC = new Image( SvtResId( RID_IMG_TREENODE_COLLAPSED_HC ) );
	s_pDefExpanded = new Image( SvtResId( RID_IMG_TREENODE_EXPANDED ) );
	s_pDefExpandedHC = new Image( SvtResId( RID_IMG_TREENODE_EXPANDED_HC ) );
}

// -----------------------------------------------------------------------
const Image& SvImpLBox::GetDefaultExpandedNodeImage( BmpColorMode _eMode )
{
	implInitDefaultNodeImages();
	return ( BMP_COLOR_NORMAL == _eMode ) ? *s_pDefExpanded : *s_pDefExpandedHC;
}

// -----------------------------------------------------------------------
const Image& SvImpLBox::GetDefaultCollapsedNodeImage( BmpColorMode _eMode )
{
	implInitDefaultNodeImages();
	return ( BMP_COLOR_NORMAL == _eMode ) ? *s_pDefCollapsed : *s_pDefCollapsedHC;
}

// -----------------------------------------------------------------------
void SvImpLBox::CallEventListeners( ULONG nEvent, void* pData )
{
	if ( pView )
		pView->CallImplEventListeners( nEvent, pData);
}

// -----------------------------------------------------------------------

bool SvImpLBox::SetCurrentTabPos( USHORT _nNewPos )
{
	bool bRet = false;

	if ( pView && _nNewPos < ( pView->TabCount() - 2 ) )
	{
		nCurTabPos = _nNewPos;
		ShowCursor( TRUE );
		bRet = true;
	}

	return bRet;
}

// -----------------------------------------------------------------------

bool SvImpLBox::IsSelectable( const SvLBoxEntry* pEntry )
{
	if( pEntry )
	{
		SvViewDataEntry* pViewDataNewCur = pView->GetViewDataEntry(const_cast<SvLBoxEntry*>(pEntry));
		return (pViewDataNewCur == 0) || pViewDataNewCur->IsSelectable();
	}
	else
	{
		return false;
	}
}

