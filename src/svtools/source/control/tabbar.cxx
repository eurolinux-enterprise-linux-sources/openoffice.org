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

#include "tabbar.hxx"
#include <tools/time.hxx>
#include <tools/debug.hxx>
#include <tools/poly.hxx>
#include <vcl/svapp.hxx>
#include <vcl/help.hxx>
#include <vcl/decoview.hxx>
#include <vcl/button.hxx>
#include <vcl/edit.hxx>
#include "svtaccessiblefactory.hxx"

// =======================================================================

#define TABBAR_OFFSET_X         7
#define TABBAR_OFFSET_X2        2
#define TABBAR_DRAG_SCROLLOFF   5
#define TABBAR_MINSIZE          5

const USHORT ADDNEWPAGE_AREAWIDTH = 10;

// =======================================================================

struct ImplTabBarItem
{
    USHORT          mnId;
    TabBarPageBits  mnBits;
    XubString       maText;
    XubString       maHelpText;
    Rectangle       maRect;
    long            mnWidth;
    ULONG           mnHelpId;
    BOOL            mbShort;
    BOOL            mbSelect;
    BOOL            mbEnable;

                    ImplTabBarItem( USHORT nItemId, const XubString& rText,
                                    TabBarPageBits nPageBits ) :
                        maText( rText )
                    {
                        mnId     = nItemId;
                        mnBits   = nPageBits;
                        mnWidth  = 0;
                        mnHelpId = 0;
                        mbShort  = FALSE;
                        mbSelect = FALSE;
                        mbEnable = TRUE;
                    }
};

DECLARE_LIST( ImplTabBarList, ImplTabBarItem* )

// =======================================================================

// -----------------
// - ImplTabButton -
// -----------------

class ImplTabButton : public PushButton
{
public:
                    ImplTabButton( TabBar* pParent, WinBits nWinStyle = 0 ) :
                        PushButton( pParent, nWinStyle | WB_RECTSTYLE | WB_SMALLSTYLE | WB_NOLIGHTBORDER | WB_NOPOINTERFOCUS  ) {}

    TabBar*         GetParent() const { return (TabBar*)Window::GetParent(); }

    virtual long    PreNotify( NotifyEvent& rNEvt );
};

// =======================================================================

long ImplTabButton::PreNotify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_MOUSEBUTTONDOWN )
    {
        if ( GetParent()->IsInEditMode() )
        {
            GetParent()->EndEditMode();
            return TRUE;
        }
    }

    return PushButton::PreNotify( rNEvt );
}

// =======================================================================

// ----------------
// - ImplTabSizer -
// ----------------

class ImplTabSizer : public Window
{
public:
                    ImplTabSizer( TabBar* pParent, WinBits nWinStyle = 0 );

    TabBar*         GetParent() const { return (TabBar*)Window::GetParent(); }

private:
    void            ImplTrack( const Point& rScreenPos );

    virtual void    MouseButtonDown( const MouseEvent& rMEvt );
    virtual void    Tracking( const TrackingEvent& rTEvt );
    virtual void    Paint( const Rectangle& rRect );

    Point           maStartPos;
    long            mnStartWidth;
};

// -----------------------------------------------------------------------

ImplTabSizer::ImplTabSizer( TabBar* pParent, WinBits nWinStyle ) :
    Window( pParent, nWinStyle & WB_3DLOOK )
{
    SetPointer( Pointer( POINTER_HSIZEBAR ) );
    SetSizePixel( Size( 7, 0 ) );
}

// -----------------------------------------------------------------------

void ImplTabSizer::ImplTrack( const Point& rScreenPos )
{
    TabBar* pParent = GetParent();
    long nDiff = rScreenPos.X() - maStartPos.X();
    pParent->mnSplitSize = mnStartWidth + (pParent->IsMirrored() ? -nDiff : nDiff);
    if ( pParent->mnSplitSize < TABBAR_MINSIZE )
        pParent->mnSplitSize = TABBAR_MINSIZE;
    pParent->Split();
    pParent->Update();
}

// -----------------------------------------------------------------------

void ImplTabSizer::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( GetParent()->IsInEditMode() )
    {
        GetParent()->EndEditMode();
        return;
    }

    if ( rMEvt.IsLeft() )
    {
        maStartPos = OutputToScreenPixel( rMEvt.GetPosPixel() );
        mnStartWidth = GetParent()->GetSizePixel().Width();
        StartTracking();
    }
}

// -----------------------------------------------------------------------

void ImplTabSizer::Tracking( const TrackingEvent& rTEvt )
{
    if ( rTEvt.IsTrackingEnded() )
    {
        if ( rTEvt.IsTrackingCanceled() )
            ImplTrack( maStartPos );
        GetParent()->mnSplitSize = 0;
    }
    else
        ImplTrack( OutputToScreenPixel( rTEvt.GetMouseEvent().GetPosPixel() ) );
}

// -----------------------------------------------------------------------

void ImplTabSizer::Paint( const Rectangle& )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    DecorationView  aDecoView( this );
    long            nOffX = 0;
    Size            aOutputSize = GetOutputSizePixel();

    if ( !(rStyleSettings.GetOptions() & STYLE_OPTION_MONO) )
    {
        SetLineColor( rStyleSettings.GetDarkShadowColor() );
        DrawLine( Point( 0, 0 ), Point( 0, aOutputSize.Height()-1 ) );
        nOffX++;
        aOutputSize.Width()--;
    }
    aDecoView.DrawButton( Rectangle( Point( nOffX, 0 ), aOutputSize ), BUTTON_DRAW_NOLIGHTBORDER );
}

// =======================================================================

// Heisst nicht Impl, da evtl. mal von aussen benutz- und ueberladbar

// --------------
// - TabBarEdit -
// --------------

class TabBarEdit : public Edit
{
private:
    Timer           maLoseFocusTimer;
    BOOL            mbPostEvt;

                    DECL_LINK( ImplEndEditHdl, void* );
                    DECL_LINK( ImplEndTimerHdl, void* );

public:
                    TabBarEdit( TabBar* pParent, WinBits nWinStyle = 0 );

    TabBar*         GetParent() const { return (TabBar*)Window::GetParent(); }

    void            SetPostEvent() { mbPostEvt = TRUE; }
    void            ResetPostEvent() { mbPostEvt = FALSE; }

    virtual long    PreNotify( NotifyEvent& rNEvt );
    virtual void    LoseFocus();
};

// -----------------------------------------------------------------------

TabBarEdit::TabBarEdit( TabBar* pParent, WinBits nWinStyle ) :
    Edit( pParent, nWinStyle )
{
    mbPostEvt = FALSE;
}

// -----------------------------------------------------------------------

long TabBarEdit::PreNotify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_KEYINPUT )
    {
        const KeyEvent* pKEvt = rNEvt.GetKeyEvent();
        if ( !pKEvt->GetKeyCode().GetModifier() )
        {
            if ( pKEvt->GetKeyCode().GetCode() == KEY_RETURN )
            {
                if ( !mbPostEvt )
                {
                    if ( PostUserEvent( LINK( this, TabBarEdit, ImplEndEditHdl ), (void*)FALSE ) )
                        mbPostEvt = TRUE;
                }
                return TRUE;
            }
            else if ( pKEvt->GetKeyCode().GetCode() == KEY_ESCAPE )
            {
                if ( !mbPostEvt )
                {
                    if ( PostUserEvent( LINK( this, TabBarEdit, ImplEndEditHdl ), (void*)TRUE ) )
                        mbPostEvt = TRUE;
                }
                return TRUE;
            }
        }
    }

    return Edit::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

void TabBarEdit::LoseFocus()
{
    if ( !mbPostEvt )
    {
        if ( PostUserEvent( LINK( this, TabBarEdit, ImplEndEditHdl ), (void*)FALSE ) )
            mbPostEvt = TRUE;
    }

    Edit::LoseFocus();
}

// -----------------------------------------------------------------------

IMPL_LINK( TabBarEdit, ImplEndEditHdl, void*, pCancel )
{
    ResetPostEvent();
    maLoseFocusTimer.Stop();

    // We need this query, because the edit get a losefous event,
    // when it shows the context menu or the insert symbol dialog
    if ( !HasFocus() && HasChildPathFocus( TRUE ) )
    {
        maLoseFocusTimer.SetTimeout( 30 );
        maLoseFocusTimer.SetTimeoutHdl( LINK( this, TabBarEdit, ImplEndTimerHdl ) );
        maLoseFocusTimer.Start();
    }
    else
        GetParent()->EndEditMode( pCancel != 0 );

    return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( TabBarEdit, ImplEndTimerHdl, void*, EMPTYARG )
{
    if ( HasFocus() )
        return 0;

    // We need this query, because the edit get a losefous event,
    // when it shows the context menu or the insert symbol dialog
    if ( HasChildPathFocus( TRUE ) )
        maLoseFocusTimer.Start();
    else
        GetParent()->EndEditMode( TRUE );

    return 0;
}

// =======================================================================
struct TabBar_Impl
{
    ImplTabSizer*                   mpSizer;
    ::svt::AccessibleFactoryAccess  maAccessibleFactory;

    TabBar_Impl()
        :mpSizer( NULL )
    {
    }
    ~TabBar_Impl()
    {
        delete mpSizer;
    }
};

// =======================================================================

void TabBar::ImplInit( WinBits nWinStyle )
{
    mpItemList      = new ImplTabBarList;
    mpFirstBtn      = NULL;
    mpPrevBtn       = NULL;
    mpNextBtn       = NULL;
    mpLastBtn       = NULL;
    mpImpl          = new TabBar_Impl;
    mpEdit          = NULL;
    mnMaxPageWidth  = 0;
    mnCurMaxWidth   = 0;
    mnOffX          = 0;
    mnOffY          = 0;
    mnLastOffX      = 0;
    mnSplitSize     = 0;
    mnSwitchTime    = 0;
    mnWinStyle      = nWinStyle;
    mnCurPageId     = 0;
    mnFirstPos      = 0;
    mnDropPos       = 0;
    mnSwitchId      = 0;
    mnEditId        = 0;
    mbFormat        = TRUE;
    mbFirstFormat   = TRUE;
    mbSizeFormat    = TRUE;
    mbAutoMaxWidth  = TRUE;
    mbInSwitching   = FALSE;
    mbAutoEditMode  = FALSE;
    mbEditCanceled  = FALSE;
    mbDropPos       = FALSE;
    mbInSelect      = FALSE;
    mbSelColor      = FALSE;
    mbSelTextColor  = FALSE;
    mbMirrored      = FALSE;

    if ( nWinStyle & WB_3DTAB )
        mnOffY++;

    ImplInitControls();
    SetSizePixel( Size( 100, CalcWindowSizePixel().Height() ) );
    ImplInitSettings( TRUE, TRUE );
}

// -----------------------------------------------------------------------

TabBar::TabBar( Window* pParent, WinBits nWinStyle ) :
    Window( pParent, (nWinStyle & WB_3DLOOK) | WB_CLIPCHILDREN )
{
    ImplInit( nWinStyle );
}

// -----------------------------------------------------------------------

TabBar::~TabBar()
{
    EndEditMode( TRUE );

    // Controls loeschen
    if ( mpPrevBtn )
        delete mpPrevBtn;
    if ( mpNextBtn )
        delete mpNextBtn;
    if ( mpFirstBtn )
        delete mpFirstBtn;
    if ( mpLastBtn )
        delete mpLastBtn;
    delete mpImpl;

    // Alle Items loeschen
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        delete pItem;
        pItem = mpItemList->Next();
    }

    // Itemlist loeschen
    delete mpItemList;
}

// -----------------------------------------------------------------------

void TabBar::ImplInitSettings( BOOL bFont, BOOL bBackground )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();

    if ( bFont )
    {
        Font aToolFont;
        aToolFont = rStyleSettings.GetToolFont();
        if ( IsControlFont() )
            aToolFont.Merge( GetControlFont() );
        aToolFont.SetWeight( WEIGHT_BOLD );
        SetZoomedPointFont( aToolFont );

        // Font in der groesse Anpassen, wenn Fenster zu klein?
        while ( GetTextHeight() > (GetOutputSizePixel().Height()-1) )
        {
            Font aFont = GetFont();
            if ( aFont.GetHeight() <= 6 )
                break;
            aFont.SetHeight( aFont.GetHeight()-1 );
            SetFont( aFont );
        }
    }

    if ( bBackground )
    {
        Color aColor;
        if ( IsControlBackground() )
            aColor = GetControlBackground();
        else
            aColor = rStyleSettings.GetFaceColor();
        SetBackground( aColor );
    }
}

// -----------------------------------------------------------------------

void TabBar::ImplGetColors( Color& rFaceColor, Color& rFaceTextColor,
                            Color& rSelectColor, Color& rSelectTextColor )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();

    if ( IsControlBackground() )
        rFaceColor = GetControlBackground();
    else
        rFaceColor = rStyleSettings.GetInactiveTabColor();
    if ( IsControlForeground() )
        rFaceTextColor = GetControlForeground();
    else
        rFaceTextColor = rStyleSettings.GetButtonTextColor();
    if ( mbSelColor )
        rSelectColor = maSelColor;
    else
        rSelectColor = rStyleSettings.GetActiveTabColor();
    if ( mbSelTextColor )
        rSelectTextColor = maSelTextColor;
    else
        rSelectTextColor = rStyleSettings.GetWindowTextColor();

    // Bei 3D-Tabs wird Selektions- und Face-Farbe umgedreht, da die
    // selektierten Tabs in 3D erscheinen sollen
    if ( mnWinStyle & WB_3DTAB )
    {
        Color aTempColor = rFaceColor;
        rFaceColor = rSelectColor;
        rSelectColor = aTempColor;
        aTempColor = rFaceTextColor;
        rFaceTextColor = rSelectTextColor;
        rSelectTextColor = rFaceTextColor;
    }
}

// -----------------------------------------------------------------------

BOOL TabBar::ImplCalcWidth()
{
    // Groessen muessen nur ermittelt werden, wenn sich Text aendert oder
    // wenn der Font geaendert wurde
    if ( !mbSizeFormat )
        return FALSE;

    // Breiten der Tabs mit dem fetten Font ermitteln
    Font aFont = GetFont();
    if ( aFont.GetWeight() != WEIGHT_BOLD )
    {
        aFont.SetWeight( WEIGHT_BOLD );
        SetFont( aFont );
    }

    if ( mnMaxPageWidth )
        mnCurMaxWidth = mnMaxPageWidth;
    else if ( mbAutoMaxWidth )
    {
        mnCurMaxWidth = mnLastOffX-mnOffX-
                        TABBAR_OFFSET_X-TABBAR_OFFSET_X-
                        TABBAR_OFFSET_X2-TABBAR_OFFSET_X2-TABBAR_OFFSET_X2;
        if ( mnCurMaxWidth < 1 )
            mnCurMaxWidth = 1;
    }
    else
        mnCurMaxWidth = 0;

    BOOL            bChanged = FALSE;
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        long nNewWidth = GetTextWidth( pItem->maText );
        if ( mnCurMaxWidth && (nNewWidth > mnCurMaxWidth) )
        {
            pItem->mbShort = TRUE;
            nNewWidth = mnCurMaxWidth;
        }
        else
            pItem->mbShort = FALSE;
        nNewWidth += TABBAR_OFFSET_X+TABBAR_OFFSET_X2;
        if ( pItem->mnWidth != nNewWidth )
        {
            pItem->mnWidth = nNewWidth;
            if ( !pItem->maRect.IsEmpty() )
                bChanged = TRUE;
        }
        pItem = mpItemList->Next();
    }
    mbSizeFormat = FALSE;
    mbFormat = TRUE;
    return bChanged;
}

// -----------------------------------------------------------------------

void TabBar::ImplFormat()
{
    ImplCalcWidth();

    if ( !mbFormat )
        return;

    USHORT n = 0;
    long x = mnOffX;
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        // Bei allen nicht sichtbaren Tabs, wird ein leeres Rechteck
        // gesetzt
        if ( (n+1 < mnFirstPos) || (x > mnLastOffX) )
            pItem->maRect.SetEmpty();
        else
        {
            // Etwas von der Tab vor der ersten sichtbaren Page
            // muss auch zu sehen sein
            if ( n+1 == mnFirstPos )
                pItem->maRect.Left() = x-pItem->mnWidth;
            else
            {
                pItem->maRect.Left() = x;
                x += pItem->mnWidth;
            }
            pItem->maRect.Right() = x+TABBAR_OFFSET_X+TABBAR_OFFSET_X2;
            pItem->maRect.Bottom() = maWinSize.Height()-1;

            if( mbMirrored )
            {
                long nTmp = mnOffX + mnLastOffX - pItem->maRect.Right();
                pItem->maRect.Right() = mnOffX + mnLastOffX - pItem->maRect.Left();
                pItem->maRect.Left() = nTmp;
            }
        }

        n++;
        pItem = mpItemList->Next();
    }

    mbFormat = FALSE;

    // Button enablen/disablen
    ImplEnableControls();
}

// -----------------------------------------------------------------------

USHORT TabBar::ImplGetLastFirstPos()
{
    USHORT  nCount = (USHORT)(mpItemList->Count());
    if ( !nCount || mbSizeFormat || mbFormat )
        return 0;

    USHORT  nLastFirstPos = nCount-1;
    long    nWinWidth = mnLastOffX-mnOffX-TABBAR_OFFSET_X-ADDNEWPAGE_AREAWIDTH;
    long    nWidth = mpItemList->GetObject( nLastFirstPos )->mnWidth;
    while ( nLastFirstPos && (nWidth < nWinWidth) )
    {
        nLastFirstPos--;
        nWidth += mpItemList->GetObject( nLastFirstPos )->mnWidth;
    }
    if ( (nLastFirstPos != (USHORT)(mpItemList->Count()-1)) &&
         (nWidth > nWinWidth) )
        nLastFirstPos++;
    return nLastFirstPos;
}

// -----------------------------------------------------------------------

void TabBar::ImplInitControls()
{
    if ( mnWinStyle & WB_SIZEABLE )
    {
        if ( !mpImpl->mpSizer )
            mpImpl->mpSizer = new ImplTabSizer( this, mnWinStyle & (WB_DRAG | WB_3DLOOK) );
        mpImpl->mpSizer->Show();
    }
    else
    {
        DELETEZ( mpImpl->mpSizer );
    }

    Link aLink = LINK( this, TabBar, ImplClickHdl );

    if ( mnWinStyle & (WB_MINSCROLL | WB_SCROLL) )
    {
        if ( !mpPrevBtn )
        {
            mpPrevBtn = new ImplTabButton( this, WB_REPEAT );
            mpPrevBtn->SetClickHdl( aLink );
        }
        mpPrevBtn->SetSymbol( mbMirrored ? SYMBOL_NEXT : SYMBOL_PREV );
        mpPrevBtn->Show();

        if ( !mpNextBtn )
        {
            mpNextBtn = new ImplTabButton( this, WB_REPEAT );
            mpNextBtn->SetClickHdl( aLink );
        }
        mpNextBtn->SetSymbol( mbMirrored ? SYMBOL_PREV : SYMBOL_NEXT );
        mpNextBtn->Show();
    }
    else
    {
        DELETEZ( mpPrevBtn );
        DELETEZ( mpNextBtn );
    }

    if ( mnWinStyle & WB_SCROLL )
    {
        if ( !mpFirstBtn )
        {
            mpFirstBtn = new ImplTabButton( this );
            mpFirstBtn->SetClickHdl( aLink );
        }
        mpFirstBtn->SetSymbol( mbMirrored ? SYMBOL_LAST : SYMBOL_FIRST );
        mpFirstBtn->Show();

        if ( !mpLastBtn )
        {
            mpLastBtn = new ImplTabButton( this );
            mpLastBtn->SetClickHdl( aLink );
        }
        mpLastBtn->SetSymbol( mbMirrored ? SYMBOL_FIRST : SYMBOL_LAST );
        mpLastBtn->Show();
    }
    else
    {
        DELETEZ( mpFirstBtn );
        DELETEZ( mpLastBtn );
    }
}

// -----------------------------------------------------------------------

void TabBar::ImplEnableControls()
{
    if ( mbSizeFormat || mbFormat )
        return;

    // Buttons enablen/disblen
    BOOL bEnableBtn = mnFirstPos > 0;
    if ( mpFirstBtn )
        mpFirstBtn->Enable( bEnableBtn );
    if ( mpPrevBtn )
        mpPrevBtn->Enable( bEnableBtn );

    bEnableBtn = mnFirstPos < ImplGetLastFirstPos();
    if ( mpNextBtn )
        mpNextBtn->Enable( bEnableBtn );
    if ( mpLastBtn )
        mpLastBtn->Enable( bEnableBtn );
}

// -----------------------------------------------------------------------

void TabBar::ImplShowPage( USHORT nPos )
{
    // Breite berechnen
    long nWidth = GetOutputSizePixel().Width();
    if ( nWidth >= TABBAR_OFFSET_X )
        nWidth -= TABBAR_OFFSET_X;
    ImplTabBarItem* pItem = mpItemList->GetObject( nPos );
    if ( nPos < mnFirstPos )
        SetFirstPageId( pItem->mnId );
    else if ( pItem->maRect.Right() > nWidth )
    {
        while ( pItem->maRect.Right() > nWidth )
        {
            USHORT nNewPos = mnFirstPos+1;
            SetFirstPageId( GetPageId( nNewPos ) );
            ImplFormat();
            if ( nNewPos != mnFirstPos )
                break;
        }
    }
}

// -----------------------------------------------------------------------

IMPL_LINK( TabBar, ImplClickHdl, ImplTabButton*, pBtn )
{
    EndEditMode();

    USHORT nNewPos = mnFirstPos;

    if ( pBtn == mpFirstBtn )
        nNewPos = 0;
    else if ( pBtn == mpPrevBtn )
    {
        if ( mnFirstPos )
            nNewPos = mnFirstPos-1;
    }
    else if ( pBtn == mpNextBtn )
    {
        USHORT nCount = GetPageCount();
        if ( mnFirstPos <  nCount )
            nNewPos = mnFirstPos+1;
    }
    else
    {
        USHORT nCount = GetPageCount();
        if ( nCount )
            nNewPos = nCount-1;
    }

    if ( nNewPos != mnFirstPos )
        SetFirstPageId( GetPageId( nNewPos ) );
    return 0;
}

// -----------------------------------------------------------------------

void TabBar::MouseMove( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeaveWindow() )
        mbInSelect = FALSE;

    Window::MouseMove( rMEvt );
}

// -----------------------------------------------------------------------

void TabBar::MouseButtonDown( const MouseEvent& rMEvt )
{
    // Bei Klick in unser Fenster EditModus nur beenden und Klick nicht
    // ausfuehren
    if ( IsInEditMode() )
    {
        EndEditMode();
        return;
    }

	ImplTabBarItem* pItem;
	USHORT          nSelId = GetPageId( rMEvt.GetPosPixel() );

    if ( !rMEvt.IsLeft() )
    {
        Window::MouseButtonDown( rMEvt );
        if ( (nSelId > 0) && (nSelId != mnCurPageId) )
        {
            USHORT nPos = GetPagePos( nSelId );
            pItem = mpItemList->GetObject( nPos );

            if ( pItem->mbEnable )
            {
                if ( ImplDeactivatePage() )
                {
                    SetCurPageId( nSelId );
                    Update();
                    ImplActivatePage();
                    ImplSelect();
                }
                mbInSelect = TRUE;
            }
        }
        return;
    }

    if ( rMEvt.IsMod2() && mbAutoEditMode && nSelId )
    {
        if ( StartEditMode( nSelId ) )
            return;
    }

    if ( (rMEvt.GetMode() & (MOUSE_MULTISELECT | MOUSE_RANGESELECT)) && (rMEvt.GetClicks() == 1) )
    {
        if ( nSelId )
        {
            USHORT      nPos = GetPagePos( nSelId );
            BOOL        bSelectTab = FALSE;
            pItem = mpItemList->GetObject( nPos );

            if ( pItem->mbEnable )
            {
                if ( (rMEvt.GetMode() & MOUSE_MULTISELECT) && (mnWinStyle & WB_MULTISELECT) )
                {
                    if ( nSelId != mnCurPageId )
                    {
                        SelectPage( nSelId, !IsPageSelected( nSelId ) );
                        bSelectTab = TRUE;
                    }
                }
                else if ( mnWinStyle & (WB_MULTISELECT | WB_RANGESELECT) )
                {
                    bSelectTab = TRUE;
                    USHORT n;
                    BOOL   bSelect;
                    USHORT nCurPos = GetPagePos( mnCurPageId );
                    if ( nPos <= nCurPos )
                    {
                        // Alle Tabs bis zur angeklickten Tab deselektieren
                        // und alle Tabs von der angeklickten Tab bis
                        // zur aktuellen Position selektieren
                        n = 0;
                        while ( n < nCurPos )
                        {
                            pItem = mpItemList->GetObject( n );
                            if ( n < nPos )
                                bSelect = FALSE;
                            else
                                bSelect = TRUE;

                            if ( pItem->mbSelect != bSelect )
                            {
                                pItem->mbSelect = bSelect;
                                if ( !pItem->maRect.IsEmpty() )
                                    Invalidate( pItem->maRect );
                            }

                            n++;
                        }
                    }

                    if ( nPos >= nCurPos )
                    {
                        // Alle Tabs von der aktuellen bis zur angeklickten
                        // Tab selektieren und alle Tabs von der angeklickten
                        // Tab bis zur letzten Tab deselektieren
                        USHORT nCount = (USHORT)mpItemList->Count();
                        n = nCurPos;
                        while ( n < nCount )
                        {
                            pItem = mpItemList->GetObject( n );

                            if ( n <= nPos )
                                bSelect = TRUE;
                            else
                                bSelect = FALSE;

                            if ( pItem->mbSelect != bSelect )
                            {
                                pItem->mbSelect = bSelect;
                                if ( !pItem->maRect.IsEmpty() )
                                    Invalidate( pItem->maRect );
                            }

                            n++;
                        }
                    }
                }

                // Gegebenenfalls muss die selektierte Tab gescrollt werden
                if ( bSelectTab )
                {
                    ImplShowPage( nPos );
                    Update();
                    ImplSelect();
                }
            }
            else
                ImplShowPage( nPos );
            mbInSelect = TRUE;

            return;
        }
    }
    else if ( rMEvt.GetClicks() == 2 )
    {
        // Gegebenenfalls den Double-Click-Handler rufen
        if ( !rMEvt.GetModifier() && (!nSelId || (nSelId == mnCurPageId)) )
        {
            USHORT nOldCurId = mnCurPageId;
            mnCurPageId = nSelId;
            DoubleClick();
            // Abfrage, da im DoubleClick-Handler die aktuelle Seite
            // umgeschaltet werden konnte
            if ( mnCurPageId == nSelId )
                mnCurPageId = nOldCurId;
        }

        return;
    }
    else
    {
        if ( nSelId )
        {
            // Nur Select ausfuehren, wenn noch nicht aktuelle Page
            if ( nSelId != mnCurPageId )
            {
                USHORT nPos = GetPagePos( nSelId );
                pItem = mpItemList->GetObject( nPos );

                if ( pItem->mbEnable )
                {
                    if ( !pItem->mbSelect )
                    {
                        // Muss invalidiert werden
                        BOOL bUpdate = FALSE;
                        if ( IsReallyVisible() && IsUpdateMode() )
                            bUpdate = TRUE;

                        // Alle selektierten Items deselektieren
                        pItem = mpItemList->First();
                        while ( pItem )
                        {
                            if ( pItem->mbSelect || (pItem->mnId == mnCurPageId) )
                            {
                                pItem->mbSelect = FALSE;
                                if ( bUpdate )
                                    Invalidate( pItem->maRect );
                            }

                            pItem = mpItemList->Next();
                        }
                    }

                    if ( ImplDeactivatePage() )
                    {
                        SetCurPageId( nSelId );
                        Update();
                        ImplActivatePage();
                        ImplSelect();
                    }
                }
                else
                    ImplShowPage( nPos );
                mbInSelect = TRUE;
            }

            return;
        }
    }

    Window::MouseButtonDown( rMEvt );
}

// -----------------------------------------------------------------------

void TabBar::MouseButtonUp( const MouseEvent& rMEvt )
{
    mbInSelect = FALSE;
    Window::MouseButtonUp( rMEvt );
}

// -----------------------------------------------------------------------

void TabBar::Paint( const Rectangle& )
{
    // Items berechnen und ausgeben
    USHORT          nItemCount = (USHORT)mpItemList->Count();
    ImplTabBarItem* pItem;

    // kein Item, dann auch nichts zu tun
    if ( nItemCount )
    {
        // TabBar muss formatiert sein
        ImplFormat();

        // Beim ersten Format auch dafuer sorgen, das aktuelle TabPage
        // sichtbar wird
        if ( mbFirstFormat )
        {
            mbFirstFormat = FALSE;

            if ( mnCurPageId && (mnFirstPos == 0) && !mbDropPos )
            {
                pItem = mpItemList->GetObject( GetPagePos( mnCurPageId ) );
                if ( pItem->maRect.IsEmpty() )
                {
                    // mbDropPos setzen (bzw. misbrauchen) um Invalidate()
                    // zu unterbinden
                    mbDropPos = TRUE;
                    SetFirstPageId( mnCurPageId );
                    mbDropPos = FALSE;
                    if ( mnFirstPos != 0 )
                        ImplFormat();
                }
            }
        }
    }

    // Farben ermitteln
    const StyleSettings&    rStyleSettings = GetSettings().GetStyleSettings();
    Color                   aFaceColor;
    Color                   aSelectColor;
    Color                   aFaceTextColor;
    Color                   aSelectTextColor;
    ImplGetColors( aFaceColor, aFaceTextColor, aSelectColor, aSelectTextColor );

    // Font selektieren
    Font aFont = GetFont();
    Font aLightFont = aFont;
    aLightFont.SetWeight( WEIGHT_LIGHT );

    // #i36013# exclude push buttons from painting area
    Rectangle aClipRect( Point( mnOffX, 0 ), Point( mnLastOffX, GetOutputHeightPixel() - 1 ) );
    SetClipRegion( Region( aClipRect ) );

    // Bei Border oben und unten einen Strich extra malen
    if ( (mnWinStyle & WB_BORDER) || (mnWinStyle & WB_TOPBORDER) )
    {
        Size aOutputSize = GetOutputSizePixel();

        // Bei 3D-Tabs wird auch der Border in 3D gemalt
        if ( mnWinStyle & WB_3DTAB )
        {
            SetLineColor( rStyleSettings.GetShadowColor() );
            DrawLine( Point( mnOffX, 0 ), Point( aOutputSize.Width(), 0 ) );
        }

        // Border malen (Strich oben und Strich unten)
        SetLineColor( rStyleSettings.GetDarkShadowColor() );
        DrawLine( Point( mnOffX, mnOffY ), Point( aOutputSize.Width()-1, mnOffY ) );
    }
    else
        SetLineColor( rStyleSettings.GetDarkShadowColor() );

    // Items ausgeben
    if ( nItemCount )
    {
        // letzten sichtbaren Eintrag suchen
        USHORT n = mnFirstPos+1;
        if ( n >= nItemCount )
            n = nItemCount-1;
        pItem = mpItemList->Seek( n );
        while ( pItem )
        {
            if ( !pItem->maRect.IsEmpty() )
            {
                n++;
                pItem = mpItemList->Next();
            }
            else
                break;
        }

        // Alle Tabs ausgeben (von hinten nach vorn und aktuellen zuletzt)
        if ( pItem )
            n--;
        else if ( n >= nItemCount )
            n = nItemCount-1;
        pItem = mpItemList->Seek( n );
        ImplTabBarItem* pCurItem = NULL;
        while ( pItem )
        {
            // CurrentItem als letztes ausgeben, da es alle anderen ueberdeckt
            if ( !pCurItem && (pItem->mnId == mnCurPageId) )
            {
                pCurItem = pItem;
                pItem = mpItemList->Prev();
                if ( !pItem )
                    pItem = pCurItem;
                continue;
            }

            if ( !pItem->maRect.IsEmpty() )
            {
                Rectangle aRect = pItem->maRect;

                // Aktuelle Page wird mit einem fetten Font ausgegeben
                if ( pItem->mnId == mnCurPageId )
                    SetFont( aFont );
                else
                    SetFont( aLightFont );

                // Je nach Status die richtige FillInBrush setzen
                if ( pItem->mbSelect || (pItem->mnId == mnCurPageId) )
                {
                    SetFillColor( aSelectColor );
                    SetTextColor( aSelectTextColor );
                }
                else
                {
                    SetFillColor( aFaceColor );
                    SetTextColor( aFaceTextColor );
                }

                // Muss Font Kursiv geschaltet werden
                if ( pItem->mnBits & TPB_SPECIAL )
                {
                    SetTextColor( Color( COL_LIGHTBLUE ) );
                }

                // Position der Page berechnen
                Point   aPos0 = Point( aRect.Left(), mnOffY );
                Point   aPos1 = Point( aRect.Left()+TABBAR_OFFSET_X, aRect.Bottom() );
                Point   aPos2 = Point( aRect.Right()-TABBAR_OFFSET_X, aRect.Bottom() );
                Point   aPos3 = Point( aRect.Right(), mnOffY );

                // Zuerst geben wir das Polygon gefuellt aus
                Polygon aPoly( 4 );
                aPoly[0] = aPos0;
                aPoly[1] = aPos1;
                aPoly[2] = aPos2;
                aPoly[3] = aPos3;
                DrawPolygon( aPoly );

                // Danach den Text zentiert ausgeben
                XubString aText = pItem->maText;
                if ( pItem->mbShort )
                    aText = GetEllipsisString( aText, mnCurMaxWidth, TEXT_DRAW_ENDELLIPSIS );
                Size    aRectSize = aRect.GetSize();
                long    nTextWidth = GetTextWidth( aText );
                long    nTextHeight = GetTextHeight();
                Point   aTxtPos( aRect.Left()+(aRectSize.Width()-nTextWidth)/2,
                                 (aRectSize.Height()-nTextHeight)/2 );
                if ( !pItem->mbEnable )
                    DrawCtrlText( aTxtPos, aText, 0, STRING_LEN, (TEXT_DRAW_DISABLE | TEXT_DRAW_MNEMONIC) );
                else
                    DrawText( aTxtPos, aText );

                // Jetzt im Inhalt den 3D-Effekt ausgeben
                aPos0.X()++;
                aPos1.X()++;
                aPos2.X()--;
                aPos3.X()--;
                SetLineColor( rStyleSettings.GetLightColor() );
                DrawLine( aPos0, aPos1 );

                if ( !pItem->mbSelect && (pItem->mnId != mnCurPageId) )
                {
                    DrawLine( Point( aPos0.X(), aPos0.Y()+1 ),
                                Point( aPos3.X(), aPos3.Y()+1 ) );
                }

                SetLineColor( rStyleSettings.GetShadowColor() );
                DrawLine( aPos2, aPos3 );
                aPos1.X()--;
                aPos1.Y()--;
                aPos2.Y()--;
                DrawLine( aPos1, aPos2 );

                // Da etwas uebermalt werden konnte, muessen wir die Polygon-
                // umrandung nocheinmal ausgeben
                SetLineColor( rStyleSettings.GetDarkShadowColor() );
                SetFillColor();
                DrawPolygon( aPoly );

                // Beim dem aktuellen Tab die restlichten Ausgaben vornehmen und
                // die Schleife abbrechen, da die aktuelle Tab als letztes
                // ausgegeben wird
                if ( pItem == pCurItem )
                {
                    // Beim aktuellen Item muss der oberstes Strich geloescht
                    // werden
                    SetLineColor();
                    SetFillColor( aSelectColor );
                    Rectangle aDelRect( aPos0, aPos3 );
                    DrawRect( aDelRect );
                    if ( mnWinStyle & WB_3DTAB )
                    {
                        aDelRect.Top()--;
                        DrawRect( aDelRect );
                    }

                    break;
                }

                pItem = mpItemList->Prev();
            }
            else
            {
                if ( pItem == pCurItem )
                    break;

                pItem = NULL;
            }

            if ( !pItem )
                pItem = pCurItem;
        }
    }

    // Font wieder herstellen
    SetFont( aFont );
    // remove clip region
    SetClipRegion();
}

// -----------------------------------------------------------------------

void TabBar::Resize()
{
    Size aNewSize = GetOutputSizePixel();

    long nSizerWidth = 0;
    long nButtonWidth = 0;

    // Sizer anordnen
    if ( mpImpl->mpSizer )
    {
        Size    aSizerSize = mpImpl->mpSizer->GetSizePixel();
        Point   aNewSizerPos( mbMirrored ? 0 : (aNewSize.Width()-aSizerSize.Width()), 0 );
        Size    aNewSizerSize( aSizerSize.Width(), aNewSize.Height() );
        mpImpl->mpSizer->SetPosSizePixel( aNewSizerPos, aNewSizerSize );
        nSizerWidth = aSizerSize.Width();
    }

    // Scroll-Buttons anordnen
    long nHeight = aNewSize.Height();
    // Font in der groesse Anpassen?
    ImplInitSettings( TRUE, FALSE );

    long nX = mbMirrored ? (aNewSize.Width()-nHeight) : 0;
    long nXDiff = mbMirrored ? -nHeight : nHeight;

    Size aBtnSize( nHeight, nHeight );
    if ( mpFirstBtn )
    {
        mpFirstBtn->SetPosSizePixel( Point( nX, 0 ), aBtnSize );
        nX += nXDiff;
        nButtonWidth += nHeight;
    }
    if ( mpPrevBtn )
    {
        mpPrevBtn->SetPosSizePixel( Point( nX, 0 ), aBtnSize );
        nX += nXDiff;
        nButtonWidth += nHeight;
    }
    if ( mpNextBtn )
    {
        mpNextBtn->SetPosSizePixel( Point( nX, 0 ), aBtnSize );
        nX += nXDiff;
        nButtonWidth += nHeight;
    }
    if ( mpLastBtn )
    {
        mpLastBtn->SetPosSizePixel( Point( nX, 0 ), aBtnSize );
        nX += nXDiff;
        nButtonWidth += nHeight;
    }

    // Groesse merken
    maWinSize = aNewSize;

    if( mbMirrored )
    {
        mnOffX = nSizerWidth;
        mnLastOffX = maWinSize.Width() - nButtonWidth - 1;
    }
    else
    {
        mnOffX = nButtonWidth;
        mnLastOffX = maWinSize.Width() - nSizerWidth - 1;
    }

    // Neu formatieren
    mbSizeFormat = TRUE;
    if ( IsReallyVisible() )
    {
        if ( ImplCalcWidth() )
            Invalidate();
        ImplFormat();
    }

    // Button enablen/disablen
    ImplEnableControls();
}

// -----------------------------------------------------------------------

void TabBar::RequestHelp( const HelpEvent& rHEvt )
{
    USHORT nItemId = GetPageId( ScreenToOutputPixel( rHEvt.GetMousePosPixel() ) );
    if ( nItemId )
    {
        if ( rHEvt.GetMode() & HELPMODE_BALLOON )
        {
            XubString aStr = GetHelpText( nItemId );
            if ( aStr.Len() )
            {
                Rectangle aItemRect = GetPageRect( nItemId );
                Point aPt = OutputToScreenPixel( aItemRect.TopLeft() );
                aItemRect.Left()   = aPt.X();
                aItemRect.Top()    = aPt.Y();
                aPt = OutputToScreenPixel( aItemRect.BottomRight() );
                aItemRect.Right()  = aPt.X();
                aItemRect.Bottom() = aPt.Y();
                Help::ShowBalloon( this, aItemRect.Center(), aItemRect, aStr );
                return;
            }
        }
        else if ( rHEvt.GetMode() & HELPMODE_EXTENDED )
        {
            ULONG nHelpId = GetHelpId( nItemId );
            if ( nHelpId )
            {
                // Wenn eine Hilfe existiert, dann ausloesen
                Help* pHelp = Application::GetHelp();
                if ( pHelp )
                    pHelp->Start( nHelpId, this );
                return;
            }
        }

        // Bei Quick- oder Ballloon-Help zeigen wir den Text an,
        // wenn dieser abgeschnitten oder nicht voll sichtbar ist
        if ( rHEvt.GetMode() & (HELPMODE_QUICK | HELPMODE_BALLOON) )
        {
            USHORT nPos = GetPagePos( nItemId );
            ImplTabBarItem* pItem = mpItemList->GetObject( nPos );
            if ( pItem->mbShort ||
                (pItem->maRect.Right()-TABBAR_OFFSET_X-5 > mnLastOffX) )
            {
                Rectangle aItemRect = GetPageRect( nItemId );
                Point aPt = OutputToScreenPixel( aItemRect.TopLeft() );
                aItemRect.Left()   = aPt.X();
                aItemRect.Top()    = aPt.Y();
                aPt = OutputToScreenPixel( aItemRect.BottomRight() );
                aItemRect.Right()  = aPt.X();
                aItemRect.Bottom() = aPt.Y();
                XubString aStr = mpItemList->GetObject( nPos )->maText;
                if ( aStr.Len() )
                {
                    if ( rHEvt.GetMode() & HELPMODE_BALLOON )
                        Help::ShowBalloon( this, aItemRect.Center(), aItemRect, aStr );
                    else
                        Help::ShowQuickHelp( this, aItemRect, aStr );
                    return;
                }
            }
        }
    }

    Window::RequestHelp( rHEvt );
}

// -----------------------------------------------------------------------

void TabBar::StateChanged( StateChangedType nType )
{
    Window::StateChanged( nType );

    if ( nType == STATE_CHANGE_INITSHOW )
    {
        if ( (mbSizeFormat || mbFormat) && mpItemList->Count() )
            ImplFormat();
    }
    else if ( (nType == STATE_CHANGE_ZOOM) ||
              (nType == STATE_CHANGE_CONTROLFONT) )
    {
        ImplInitSettings( TRUE, FALSE );
        Invalidate();
    }
    else if ( nType == STATE_CHANGE_CONTROLFOREGROUND )
        Invalidate();
    else if ( nType == STATE_CHANGE_CONTROLBACKGROUND )
    {
        ImplInitSettings( FALSE, TRUE );
        Invalidate();
    }
    else if ( nType == STATE_CHANGE_MIRRORING )
    {
        // reacts on calls of EnableRTL, have to mirror all child controls
        if( mpFirstBtn ) mpFirstBtn->EnableRTL( IsRTLEnabled() );
        if( mpPrevBtn ) mpPrevBtn->EnableRTL( IsRTLEnabled() );
        if( mpNextBtn ) mpNextBtn->EnableRTL( IsRTLEnabled() );
        if( mpLastBtn ) mpLastBtn->EnableRTL( IsRTLEnabled() );
        if( mpImpl->mpSizer ) mpImpl->mpSizer->EnableRTL( IsRTLEnabled() );
        if( mpEdit ) mpEdit->EnableRTL( IsRTLEnabled() );
    }
}

// -----------------------------------------------------------------------

void TabBar::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_FONTS) ||
         (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
         ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
          (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
    {
        ImplInitSettings( TRUE, TRUE );
        Invalidate();
    }
}

// -----------------------------------------------------------------------

void TabBar::ImplSelect()
{
	Select();

	CallEventListeners( VCLEVENT_TABBAR_PAGESELECTED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(mnCurPageId)) );
}

// -----------------------------------------------------------------------

void TabBar::Select()
{
    maSelectHdl.Call( this );
}

// -----------------------------------------------------------------------

void TabBar::DoubleClick()
{
    maDoubleClickHdl.Call( this );
}

// -----------------------------------------------------------------------

void TabBar::Split()
{
    maSplitHdl.Call( this );
}

// -----------------------------------------------------------------------

void TabBar::ImplActivatePage()
{
	ActivatePage();

	CallEventListeners( VCLEVENT_TABBAR_PAGEACTIVATED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(mnCurPageId)) );
}

// -----------------------------------------------------------------------

void TabBar::ActivatePage()
{
    maActivatePageHdl.Call( this );
}

// -----------------------------------------------------------------------

long TabBar::ImplDeactivatePage()
{
	long nRet = DeactivatePage();

	CallEventListeners( VCLEVENT_TABBAR_PAGEDEACTIVATED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(mnCurPageId)) );

	return nRet;
}

// -----------------------------------------------------------------------

long TabBar::DeactivatePage()
{
    if ( maDeactivatePageHdl.IsSet() )
        return maDeactivatePageHdl.Call( this );
    else
        return TRUE;
}

// -----------------------------------------------------------------------

long TabBar::StartRenaming()
{
    if ( maStartRenamingHdl.IsSet() )
        return maStartRenamingHdl.Call( this );
    else
        return TRUE;
}

// -----------------------------------------------------------------------

long TabBar::AllowRenaming()
{
    if ( maAllowRenamingHdl.IsSet() )
        return maAllowRenamingHdl.Call( this );
    else
        return TRUE;
}

// -----------------------------------------------------------------------

void TabBar::EndRenaming()
{
    maEndRenamingHdl.Call( this );
}

// -----------------------------------------------------------------------

void TabBar::Mirror()
{

}

// -----------------------------------------------------------------------

void TabBar::InsertPage( USHORT nPageId, const XubString& rText,
                         TabBarPageBits nBits, USHORT nPos )
{
    DBG_ASSERT( nPageId, "TabBar::InsertPage(): PageId == 0" );
    DBG_ASSERT( GetPagePos( nPageId ) == TABBAR_PAGE_NOTFOUND,
                "TabBar::InsertPage(): PageId already exists" );
    DBG_ASSERT( nBits <= TPB_SPECIAL, "TabBar::InsertPage(): nBits is wrong" );

    // PageItem anlegen und in die Item-Liste eintragen
    ImplTabBarItem* pItem = new ImplTabBarItem( nPageId, rText, nBits );
    mpItemList->Insert( pItem, nPos );
    mbSizeFormat = TRUE;

    // CurPageId gegebenenfalls setzen
    if ( !mnCurPageId )
        mnCurPageId = nPageId;

    // Leiste neu ausgeben
    if ( IsReallyVisible() && IsUpdateMode() )
        Invalidate();

	CallEventListeners( VCLEVENT_TABBAR_PAGEINSERTED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(nPageId)) );
}

// -----------------------------------------------------------------------

void TabBar::RemovePage( USHORT nPageId )
{
    USHORT nPos = GetPagePos( nPageId );

    // Existiert Item
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        if ( mnCurPageId == nPageId )
            mnCurPageId = 0;

        // Testen, ob erste sichtbare Seite verschoben werden muss
        if ( mnFirstPos > nPos )
            mnFirstPos--;

        // Item-Daten loeschen
        delete mpItemList->Remove( nPos );
        mbFormat = TRUE;

        // Leiste neu ausgeben
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate();

		CallEventListeners( VCLEVENT_TABBAR_PAGEREMOVED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(nPageId)) );
    }
}

// -----------------------------------------------------------------------

void TabBar::MovePage( USHORT nPageId, USHORT nNewPos )
{
    USHORT nPos = GetPagePos( nPageId );
	Pair aPair( nPos, nNewPos );

    if ( nPos < nNewPos )
        nNewPos--;

    if ( nPos == nNewPos )
        return;

    // Existiert Item
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        // TabBar-Item in der Liste verschieben
        ImplTabBarItem* pItem = mpItemList->Remove( nPos );
        mpItemList->Insert( pItem, nNewPos );
        mbFormat = TRUE;

        // Leiste neu ausgeben
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate();

		CallEventListeners( VCLEVENT_TABBAR_PAGEMOVED, (void*) &aPair );
    }
}

// -----------------------------------------------------------------------

void TabBar::Clear()
{
    // Alle Items loeschen
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        // Item-Daten loeschen
        delete pItem;
        pItem = mpItemList->Next();
    }

    // Items aus der Liste loeschen
    mpItemList->Clear();
    mbSizeFormat = TRUE;
    mnCurPageId = 0;
    mnFirstPos = 0;

    // Leiste neu ausgeben
    if ( IsReallyVisible() && IsUpdateMode() )
        Invalidate();

    CallEventListeners( VCLEVENT_TABBAR_PAGEREMOVED, (void*) TABBAR_PAGE_NOTFOUND );
}

// -----------------------------------------------------------------------

void TabBar::EnablePage( USHORT nPageId, BOOL bEnable )
{
    USHORT nPos = GetPagePos( nPageId );

    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        ImplTabBarItem* pItem = mpItemList->GetObject( nPos );

        if ( pItem->mbEnable != bEnable )
        {
            pItem->mbEnable = bEnable;

            // Leiste neu ausgeben
            if ( IsReallyVisible() && IsUpdateMode() )
                Invalidate( pItem->maRect );

			CallEventListeners( bEnable ? VCLEVENT_TABBAR_PAGEENABLED : VCLEVENT_TABBAR_PAGEDISABLED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(nPageId)) );
        }
    }
}

// -----------------------------------------------------------------------

BOOL TabBar::IsPageEnabled( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );

    if ( nPos != TABBAR_PAGE_NOTFOUND )
        return mpItemList->GetObject( nPos )->mbEnable;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

void TabBar::SetPageBits( USHORT nPageId, TabBarPageBits nBits )
{
    USHORT nPos = GetPagePos( nPageId );

    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        ImplTabBarItem* pItem = mpItemList->GetObject( nPos );

        if ( pItem->mnBits != nBits )
        {
            pItem->mnBits = nBits;

            // Leiste neu ausgeben
            if ( IsReallyVisible() && IsUpdateMode() )
                Invalidate( pItem->maRect );
        }
    }
}

// -----------------------------------------------------------------------

TabBarPageBits TabBar::GetPageBits( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );

    if ( nPos != TABBAR_PAGE_NOTFOUND )
        return mpItemList->GetObject( nPos )->mnBits;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

USHORT TabBar::GetPageCount() const
{
    return (USHORT)mpItemList->Count();
}

// -----------------------------------------------------------------------

USHORT TabBar::GetPageId( USHORT nPos ) const
{
    ImplTabBarItem* pItem = mpItemList->GetObject( nPos );
    if ( pItem )
        return pItem->mnId;
    else
        return 0;
}

// -----------------------------------------------------------------------

USHORT TabBar::GetPagePos( USHORT nPageId ) const
{
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        if ( pItem->mnId == nPageId )
            return (USHORT)mpItemList->GetCurPos();

        pItem = mpItemList->Next();
    }

    return TABBAR_PAGE_NOTFOUND;
}

// -----------------------------------------------------------------------

USHORT TabBar::GetPageId( const Point& rPos ) const
{
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        if ( pItem->maRect.IsInside( rPos ) )
            return pItem->mnId;

        pItem = mpItemList->Next();
    }

    return 0;
}

// -----------------------------------------------------------------------

Rectangle TabBar::GetPageRect( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );

    if ( nPos != TABBAR_PAGE_NOTFOUND )
        return mpItemList->GetObject( nPos )->maRect;
    else
        return Rectangle();
}

// -----------------------------------------------------------------------

void TabBar::SetCurPageId( USHORT nPageId )
{
    USHORT nPos = GetPagePos( nPageId );

    // Wenn Item nicht existiert, dann nichts machen
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        // Wenn sich aktuelle Page nicht geaendert hat, dann muessen wir
        // jetzt nichts mehr machen
        if ( nPageId == mnCurPageId )
            return;

        // Muss invalidiert werden
        BOOL bUpdate = FALSE;
        if ( IsReallyVisible() && IsUpdateMode() )
            bUpdate = TRUE;

        ImplTabBarItem* pItem = mpItemList->GetObject( nPos );
        ImplTabBarItem* pOldItem;

        if ( mnCurPageId )
            pOldItem = mpItemList->GetObject( GetPagePos( mnCurPageId ) );
        else
            pOldItem = NULL;

        // Wenn Page nicht selektiert, dann vorher selektierte Seite
        // deselktieren, wenn dies die einzige selektierte Seite ist
        if ( !pItem->mbSelect && pOldItem )
        {
            USHORT nSelPageCount = GetSelectPageCount();
            if ( nSelPageCount == 1 )
                pOldItem->mbSelect = FALSE;
            pItem->mbSelect = TRUE;
        }

        mnCurPageId = nPageId;
        mbFormat = TRUE;

        // Dafuer sorgen, das aktuelle Page sichtbar wird
        if ( IsReallyVisible() )
        {
            if ( nPos < mnFirstPos )
                SetFirstPageId( nPageId );
            else
            {
                // sichtbare Breite berechnen
                long nWidth = mnLastOffX;
                if ( nWidth > TABBAR_OFFSET_X )
                    nWidth -= TABBAR_OFFSET_X;
                if ( nWidth > ADDNEWPAGE_AREAWIDTH )
					nWidth -= ADDNEWPAGE_AREAWIDTH;

                if ( pItem->maRect.IsEmpty() )
                    ImplFormat();

                while ( (mbMirrored ? (pItem->maRect.Left() < mnOffX) : (pItem->maRect.Right() > nWidth)) ||
                        pItem->maRect.IsEmpty() )
                {
                    USHORT nNewPos = mnFirstPos+1;
                    // Dafuer sorgen, das min. die aktuelle TabPages als
                    // erste TabPage sichtbar ist
                    if ( nNewPos >= nPos )
                    {
                        SetFirstPageId( nPageId );
                        break;
                    }
                    else
                        SetFirstPageId( GetPageId( nNewPos ) );
                    ImplFormat();
                    // Falls erste Seite nicht weitergeschaltet wird, dann
                    // koennen wir abbrechen
                    if ( nNewPos != mnFirstPos )
                        break;
                }
            }
        }

        // Leiste neu ausgeben
        if ( bUpdate )
        {
            Invalidate( pItem->maRect );
            if ( pOldItem )
                Invalidate( pOldItem->maRect );
        }
    }
}

// -----------------------------------------------------------------------

void TabBar::MakeVisible( USHORT nPageId )
{
    if ( !IsReallyVisible() )
        return;

    USHORT nPos = GetPagePos( nPageId );

    // Wenn Item nicht existiert, dann nichts machen
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        if ( nPos < mnFirstPos )
            SetFirstPageId( nPageId );
        else
        {
            ImplTabBarItem* pItem = mpItemList->GetObject( nPos );

            // sichtbare Breite berechnen
            long nWidth = mnLastOffX;
            if ( nWidth > TABBAR_OFFSET_X )
                nWidth -= TABBAR_OFFSET_X;

            if ( mbFormat || pItem->maRect.IsEmpty() )
            {
                mbFormat = TRUE;
                ImplFormat();
            }

            while ( (pItem->maRect.Right() > nWidth) ||
                    pItem->maRect.IsEmpty() )
            {
                USHORT nNewPos = mnFirstPos+1;
                // Dafuer sorgen, das min. die aktuelle TabPages als
                // erste TabPage sichtbar ist
                if ( nNewPos >= nPos )
                {
                    SetFirstPageId( nPageId );
                    break;
                }
                else
                    SetFirstPageId( GetPageId( nNewPos ) );
                ImplFormat();
                // Falls erste Seite nicht weitergeschaltet wird, dann
                // koennen wir abbrechen
                if ( nNewPos != mnFirstPos )
                    break;
            }
        }
    }
}

// -----------------------------------------------------------------------

void TabBar::SetFirstPageId( USHORT nPageId )
{
    USHORT nPos = GetPagePos( nPageId );

    // Wenn Item nicht existiert, dann FALSE zurueckgeben
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        if ( nPos != mnFirstPos )
        {
            // Dafuer sorgen, das nach Moeglichkteit soviele Pages wie
            // moeglich sichtbar sind
            ImplFormat();
            USHORT nLastFirstPos = ImplGetLastFirstPos();
            USHORT nNewPos;
            if ( nPos > nLastFirstPos )
                nNewPos = nLastFirstPos;
            else
                nNewPos = nPos;

            if ( nNewPos != mnFirstPos )
            {
                mnFirstPos = nNewPos;
                mbFormat = TRUE;

                // Leiste neu ausgeben (Achtung: mbDropPos beachten, da wenn
                // dieses Flag gesetzt ist, wird direkt gepaintet)
                if ( IsReallyVisible() && IsUpdateMode() && !mbDropPos )
                    Invalidate();
            }
        }
    }
}

// -----------------------------------------------------------------------

void TabBar::SelectPage( USHORT nPageId, BOOL bSelect )
{
    USHORT nPos = GetPagePos( nPageId );

    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        ImplTabBarItem* pItem = mpItemList->GetObject( nPos );

        if ( pItem->mbSelect != bSelect )
        {
            pItem->mbSelect = bSelect;

            // Leiste neu ausgeben
            if ( IsReallyVisible() && IsUpdateMode() )
                Invalidate( pItem->maRect );
        }
    }
}

// -----------------------------------------------------------------------

void TabBar::SelectPageRange( BOOL bSelect, USHORT nStartPos, USHORT nEndPos )
{
    Rectangle       aPaintRect;
    USHORT          nPos = nStartPos;
    ImplTabBarItem* pItem = mpItemList->Seek( nPos );
    while ( pItem && (nPos <= nEndPos) )
    {
        if ( (pItem->mbSelect != bSelect) && (pItem->mnId != mnCurPageId) )
        {
            pItem->mbSelect = bSelect;
            aPaintRect.Union( pItem->maRect );
        }

        nPos++;
        pItem = mpItemList->Next();
    }

    // Leiste neu ausgeben
    if ( IsReallyVisible() && IsUpdateMode() && !aPaintRect.IsEmpty() )
        Invalidate( aPaintRect );
}

// -----------------------------------------------------------------------

USHORT TabBar::GetSelectPage( USHORT nSelIndex ) const
{
    USHORT          nSelected = 0;
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        if ( pItem->mbSelect )
            nSelected++;

        if ( nSelected == nSelIndex )
            return pItem->mnId;

        pItem = mpItemList->Next();
    }

    return 0;
}

// -----------------------------------------------------------------------

USHORT TabBar::GetSelectPageCount() const
{
    USHORT          nSelected = 0;
    ImplTabBarItem* pItem = mpItemList->First();
    while ( pItem )
    {
        if ( pItem->mbSelect )
            nSelected++;

        pItem = mpItemList->Next();
    }

    return nSelected;
}

// -----------------------------------------------------------------------

BOOL TabBar::IsPageSelected( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
        return mpItemList->GetObject( nPos )->mbSelect;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

BOOL TabBar::StartEditMode( USHORT nPageId )
{
    USHORT nPos = GetPagePos( nPageId );
    if ( mpEdit || (nPos == TABBAR_PAGE_NOTFOUND) || (mnLastOffX < 8) )
        return FALSE;

    mnEditId = nPageId;
    if ( StartRenaming() )
    {
        ImplShowPage( nPos );
        ImplFormat();
        Update();

        mpEdit = new TabBarEdit( this, WB_CENTER );
        Rectangle aRect = GetPageRect( mnEditId );
        long nX = aRect.Left()+TABBAR_OFFSET_X+(TABBAR_OFFSET_X2/2);
        long nWidth = aRect.GetWidth()-(TABBAR_OFFSET_X*2)-TABBAR_OFFSET_X2;
        if ( mnEditId != GetCurPageId() )
            nX += 1;
        if ( nX+nWidth > mnLastOffX )
            nWidth = mnLastOffX-nX;
        if ( nWidth < 3 )
        {
            nX = aRect.Left();
            nWidth = aRect.GetWidth();
        }
        mpEdit->SetText( GetPageText( mnEditId ) );
        mpEdit->SetPosSizePixel( nX, aRect.Top()+mnOffY+1, nWidth, aRect.GetHeight()-3 );
        Font    aFont = GetPointFont();
        Color   aForegroundColor;
        Color   aBackgroundColor;
        Color   aFaceColor;
        Color   aSelectColor;
        Color   aFaceTextColor;
        Color   aSelectTextColor;
        ImplGetColors( aFaceColor, aFaceTextColor, aSelectColor, aSelectTextColor );
        if ( mnEditId != GetCurPageId() )
            aFont.SetWeight( WEIGHT_LIGHT );
        if ( IsPageSelected( mnEditId ) || (mnEditId == GetCurPageId()) )
        {
            aForegroundColor = aSelectTextColor;
            aBackgroundColor = aSelectColor;
        }
        else
        {
            aForegroundColor = aFaceTextColor;
            aBackgroundColor = aFaceColor;
        }
        if ( GetPageBits( mnEditId ) & TPB_SPECIAL )
            aForegroundColor = Color( COL_LIGHTBLUE );
        mpEdit->SetControlFont( aFont );
        mpEdit->SetControlForeground( aForegroundColor );
        mpEdit->SetControlBackground( aBackgroundColor );
        mpEdit->GrabFocus();
        mpEdit->SetSelection( Selection( 0, mpEdit->GetText().Len() ) );
        mpEdit->Show();
        return TRUE;
    }
    else
    {
        mnEditId = 0;
        return FALSE;
    }
}

// -----------------------------------------------------------------------

void TabBar::EndEditMode( BOOL bCancel )
{
    if ( mpEdit )
    {
        // call hdl
        BOOL bEnd = TRUE;
        mbEditCanceled = bCancel;
        maEditText = mpEdit->GetText();
        mpEdit->SetPostEvent();
        if ( !bCancel )
        {
            long nAllowRenaming = AllowRenaming();
            if ( nAllowRenaming == TABBAR_RENAMING_YES )
                SetPageText( mnEditId, maEditText );
            else if ( nAllowRenaming == TABBAR_RENAMING_NO )
                bEnd = FALSE;
            else // nAllowRenaming == TABBAR_RENAMING_CANCEL
                mbEditCanceled = TRUE;
        }

        // renaming not allowed, than reset edit data
        if ( !bEnd )
        {
            mpEdit->ResetPostEvent();
            mpEdit->GrabFocus();
        }
        else
        {
            // close edit and call end hdl
            delete mpEdit;
            mpEdit = NULL;
            EndRenaming();
            mnEditId = 0;
        }

        // reset
        maEditText.Erase();
        mbEditCanceled = FALSE;
    }
}

// -----------------------------------------------------------------------

void TabBar::SetMirrored( BOOL bMirrored )
{
    if( mbMirrored != bMirrored )
    {
        mbMirrored = bMirrored;
        mbSizeFormat = TRUE;
        ImplInitControls();     // for button images
        Resize();               // recalculates control positions
        Mirror();
    }
}

void TabBar::SetEffectiveRTL( BOOL bRTL )
{
    SetMirrored( bRTL != Application::GetSettings().GetLayoutRTL() );
}

BOOL TabBar::IsEffectiveRTL() const
{
    return IsMirrored() != Application::GetSettings().GetLayoutRTL();
}

// -----------------------------------------------------------------------

void TabBar::SetMaxPageWidth( long nMaxWidth )
{
    if ( mnMaxPageWidth != nMaxWidth )
    {
        mnMaxPageWidth = nMaxWidth;
        mbSizeFormat = TRUE;

        // Leiste neu ausgeben
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate();
    }
}

// -----------------------------------------------------------------------

void TabBar::SetSelectColor()
{
    if ( mbSelColor )
    {
        maSelColor = Color( COL_TRANSPARENT );
        mbSelColor = FALSE;
        Invalidate();
    }
}

// -----------------------------------------------------------------------

void TabBar::SetSelectColor( const Color& rColor )
{
    if ( rColor.GetTransparency() )
    {
        if ( mbSelColor )
        {
            maSelColor = Color( COL_TRANSPARENT );
            mbSelColor = FALSE;
            Invalidate();
        }
    }
    else
    {
        if ( maSelColor != rColor )
        {
            maSelColor = rColor;
            mbSelColor = TRUE;
            Invalidate();
        }
    }
}

// -----------------------------------------------------------------------

void TabBar::SetSelectTextColor()
{
    if ( mbSelTextColor )
    {
        maSelTextColor = Color( COL_TRANSPARENT );
        mbSelTextColor = FALSE;
        Invalidate();
    }
}

// -----------------------------------------------------------------------

void TabBar::SetSelectTextColor( const Color& rColor )
{
    if ( rColor.GetTransparency() )
    {
        if ( mbSelTextColor )
        {
            maSelTextColor = Color( COL_TRANSPARENT );
            mbSelTextColor = FALSE;
            Invalidate();
        }
    }
    else
    {
        if ( maSelTextColor != rColor )
        {
            maSelTextColor = rColor;
            mbSelTextColor = TRUE;
            Invalidate();
        }
    }
}

// -----------------------------------------------------------------------

void TabBar::SetPageText( USHORT nPageId, const XubString& rText )
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        mpItemList->GetObject( nPos )->maText = rText;
        mbSizeFormat = TRUE;

        // Leiste neu ausgeben
        if ( IsReallyVisible() && IsUpdateMode() )
            Invalidate();

		CallEventListeners( VCLEVENT_TABBAR_PAGETEXTCHANGED, reinterpret_cast<void*>(sal::static_int_cast<sal_IntPtr>(nPageId)) );
    }
}

// -----------------------------------------------------------------------

XubString TabBar::GetPageText( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
        return mpItemList->GetObject( nPos )->maText;
    else
        return XubString();
}

// -----------------------------------------------------------------------

void TabBar::SetHelpText( USHORT nPageId, const XubString& rText )
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
        mpItemList->GetObject( nPos )->maHelpText = rText;
}

// -----------------------------------------------------------------------

XubString TabBar::GetHelpText( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
    {
        ImplTabBarItem* pItem = mpItemList->GetObject( nPos );
        if ( !pItem->maHelpText.Len() && pItem->mnHelpId )
        {
            Help* pHelp = Application::GetHelp();
            if ( pHelp )
                pItem->maHelpText = pHelp->GetHelpText( pItem->mnHelpId, this );
        }

        return pItem->maHelpText;
    }
    else
        return XubString();
}

// -----------------------------------------------------------------------

void TabBar::SetHelpId( USHORT nPageId, ULONG nHelpId )
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
        mpItemList->GetObject( nPos )->mnHelpId = nHelpId;
}

// -----------------------------------------------------------------------

ULONG TabBar::GetHelpId( USHORT nPageId ) const
{
    USHORT nPos = GetPagePos( nPageId );
    if ( nPos != TABBAR_PAGE_NOTFOUND )
        return mpItemList->GetObject( nPos )->mnHelpId;
    else
        return 0;
}

// -----------------------------------------------------------------------

long TabBar::GetMinSize() const
{
    long nMinSize = TABBAR_MINSIZE + TABBAR_OFFSET_X;
    if ( mnWinStyle & WB_MINSCROLL )
        nMinSize += mpPrevBtn->GetSizePixel().Width()*2;
    else if ( mnWinStyle & WB_SCROLL )
        nMinSize += mpFirstBtn->GetSizePixel().Width()*4;
    return nMinSize;
}

// -----------------------------------------------------------------------

BOOL TabBar::StartDrag( const CommandEvent& rCEvt, Region& rRegion )
{
    if ( !(mnWinStyle & WB_DRAG) || (rCEvt.GetCommand() != COMMAND_STARTDRAG) )
        return FALSE;

    // Testen, ob angeklickte Seite selektiert ist. Falls dies nicht
    // der Fall ist, setzen wir ihn als aktuellen Eintrag. Falls Drag and
    // Drop auch mal ueber Tastatur ausgeloest werden kann, testen wir
    // dies nur bei einer Mausaktion.
    // Ausserdem machen wir das nur, wenn kein Select() ausgeloest wurde,
    // da der Select schon den Bereich gescrollt haben kann
    if ( rCEvt.IsMouseEvent() && !mbInSelect )
    {
        USHORT nSelId = GetPageId( rCEvt.GetMousePosPixel() );

        // Falls kein Eintrag angeklickt wurde, starten wir kein Dragging
        if ( !nSelId )
            return FALSE;

        // Testen, ob Seite selektiertiert ist. Falls nicht, als aktuelle
        // Seite setzen und Select rufen.
        if ( !IsPageSelected( nSelId ) )
        {
            if ( ImplDeactivatePage() )
            {
                SetCurPageId( nSelId );
                Update();
                ImplActivatePage();
                ImplSelect();
            }
            else
                return FALSE;
        }
    }
    mbInSelect = FALSE;

    Region aRegion;

    // Region zuweisen
    rRegion = aRegion;

    return TRUE;
}

// -----------------------------------------------------------------------

USHORT TabBar::ShowDropPos( const Point& rPos )
{
    ImplTabBarItem* pItem;
    USHORT      nDropId;
    USHORT      nNewDropPos;
    USHORT      nItemCount = (USHORT)mpItemList->Count();
    short       nScroll = 0;

    if ( rPos.X() > mnLastOffX-TABBAR_DRAG_SCROLLOFF )
    {
        pItem = mpItemList->GetObject( mpItemList->Count()-1 );
        if ( !pItem->maRect.IsEmpty() && (rPos.X() > pItem->maRect.Right()) )
            nNewDropPos = (USHORT)mpItemList->Count();
        else
        {
            nNewDropPos = mnFirstPos+1;
            nScroll = 1;
        }
    }
    else if ( (rPos.X() <= mnOffX) ||
              (!mnOffX && (rPos.X() <= TABBAR_DRAG_SCROLLOFF)) )
    {
        if ( mnFirstPos )
        {
            nNewDropPos = mnFirstPos;
            nScroll = -1;
        }
        else
            nNewDropPos = 0;
    }
    else
    {
        nDropId = GetPageId( rPos );
        if ( nDropId )
        {
            nNewDropPos = GetPagePos( nDropId );
            if ( mnFirstPos && (nNewDropPos == mnFirstPos-1) )
                nScroll = -1;
        }
        else
            nNewDropPos = nItemCount;
    }

    if ( mbDropPos && (nNewDropPos == mnDropPos) && !nScroll )
        return mnDropPos;

    if ( mbDropPos )
        HideDropPos();
    mbDropPos = TRUE;
    mnDropPos = nNewDropPos;

    if ( nScroll )
    {
        USHORT nOldFirstPos = mnFirstPos;
        SetFirstPageId( GetPageId( mnFirstPos+nScroll ) );

        // Direkt ausgeben, da kein Paint bei Drag and Drop moeglich
        if ( nOldFirstPos != mnFirstPos )
        {
            Rectangle aRect( mnOffX, 0, mnLastOffX, maWinSize.Height() );
            SetFillColor( GetBackground().GetColor() );
            DrawRect( aRect );
            Paint( aRect );
        }
    }

    // Drop-Position-Pfeile ausgeben
    Color       aBlackColor( COL_BLACK );
    long        nX;
    long        nY = (maWinSize.Height()/2)-1;
    USHORT      nCurPos = GetPagePos( mnCurPageId );

    SetLineColor( aBlackColor );
    if ( mnDropPos < nItemCount )
    {
        pItem = mpItemList->GetObject( mnDropPos );
        nX = pItem->maRect.Left()+TABBAR_OFFSET_X;
        if ( mnDropPos == nCurPos )
            nX--;
        else
            nX++;
        DrawLine( Point( nX, nY ), Point( nX, nY ) );
        DrawLine( Point( nX+1, nY-1 ), Point( nX+1, nY+1 ) );
        DrawLine( Point( nX+2, nY-2 ), Point( nX+2, nY+2 ) );
    }
    if ( (mnDropPos > 0) && (mnDropPos < nItemCount+1) )
    {
        pItem = mpItemList->GetObject( mnDropPos-1 );
        nX = pItem->maRect.Right()-TABBAR_OFFSET_X;
        if ( mnDropPos == nCurPos )
            nX++;
        DrawLine( Point( nX, nY ), Point( nX, nY ) );
        DrawLine( Point( nX-1, nY-1 ), Point( nX-1, nY+1 ) );
        DrawLine( Point( nX-2, nY-2 ), Point( nX-2, nY+2 ) );
    }

    return mnDropPos;
}

// -----------------------------------------------------------------------

void TabBar::HideDropPos()
{
    if ( mbDropPos )
    {
        ImplTabBarItem* pItem;
        long        nX;
        long        nY1 = (maWinSize.Height()/2)-3;
        long        nY2 = nY1 + 5;
        USHORT      nItemCount = (USHORT)mpItemList->Count();

        if ( mnDropPos < nItemCount )
        {
            pItem = mpItemList->GetObject( mnDropPos );
            nX = pItem->maRect.Left()+TABBAR_OFFSET_X;
            // Paint direkt aufrufen, da bei Drag and Drop kein Paint
            // moeglich
            Rectangle aRect( nX-1, nY1, nX+3, nY2 );
            Region aRegion( aRect );
            SetClipRegion( aRegion );
            Paint( aRect );
            SetClipRegion();
        }
        if ( (mnDropPos > 0) && (mnDropPos < nItemCount+1) )
        {
            pItem = mpItemList->GetObject( mnDropPos-1 );
            nX = pItem->maRect.Right()-TABBAR_OFFSET_X;
            // Paint direkt aufrufen, da bei Drag and Drop kein Paint
            // moeglich
            Rectangle aRect( nX-2, nY1, nX+1, nY2 );
            Region aRegion( aRect );
            SetClipRegion( aRegion );
            Paint( aRect );
            SetClipRegion();
        }

        mbDropPos = FALSE;
        mnDropPos = 0;
    }
}

// -----------------------------------------------------------------------

BOOL TabBar::SwitchPage( const Point& rPos )
{
    BOOL    bSwitch = FALSE;
    USHORT  nSwitchId = GetPageId( rPos );
    if ( !nSwitchId )
        EndSwitchPage();
    else
    {
        if ( nSwitchId != mnSwitchId )
        {
            mnSwitchId = nSwitchId;
            mnSwitchTime = Time::GetSystemTicks();
        }
        else
        {
            // Erst nach 500 ms umschalten
            if ( mnSwitchId != GetCurPageId() )
            {
                if ( Time::GetSystemTicks() > mnSwitchTime+500 )
                {
                    mbInSwitching = TRUE;
                    if ( ImplDeactivatePage() )
                    {
                        SetCurPageId( mnSwitchId );
                        Update();
                        ImplActivatePage();
                        ImplSelect();
                        bSwitch = TRUE;
                    }
                    mbInSwitching = FALSE;
                }
            }
        }
    }

    return bSwitch;
}

// -----------------------------------------------------------------------

void TabBar::EndSwitchPage()
{
    mnSwitchTime    = 0;
    mnSwitchId      = 0;
}

// -----------------------------------------------------------------------

void TabBar::SetStyle( WinBits nStyle )
{
    mnWinStyle = nStyle;
    ImplInitControls();
    // Evt. Controls neu anordnen
    if ( IsReallyVisible() && IsUpdateMode() )
        Resize();
}

// -----------------------------------------------------------------------

Size TabBar::CalcWindowSizePixel() const
{
    long nWidth = 0;

    if ( mpItemList->Count() )
    {
        ((TabBar*)this)->ImplCalcWidth();
        ImplTabBarItem* pItem = mpItemList->First();
        while ( pItem )
        {
            nWidth += pItem->mnWidth;
            pItem = mpItemList->Next();
        }
        nWidth += TABBAR_OFFSET_X+TABBAR_OFFSET_X2;
    }

    return Size( nWidth, GetSettings().GetStyleSettings().GetScrollBarSize() );
}
// -----------------------------------------------------------------------

Rectangle TabBar::GetPageArea() const
{
    return Rectangle( Point( mnOffX, mnOffY ), Size( mnLastOffX-mnOffX+1, GetSizePixel().Height()-mnOffY ) );
}

// -----------------------------------------------------------------------

::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > TabBar::CreateAccessible()
{
    return mpImpl->maAccessibleFactory.getFactory().createAccessibleTabBar( *this );
}

// -----------------------------------------------------------------------
