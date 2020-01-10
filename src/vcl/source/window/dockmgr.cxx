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
#include "precompiled_vcl.hxx"

#ifndef _SV_SVSYS_HXX
#include <svsys.h>
#endif
#include <tools/time.hxx>
#ifndef _SV_RC_H
#include <tools/rc.h>
#endif
#include <vcl/event.hxx>
#include <vcl/brdwin.hxx>
#include <vcl/floatwin.hxx>
#include <vcl/dockwin.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/svapp.hxx>
#include <vcl/svdata.hxx>
#include <vcl/timer.hxx>
#include <vcl/lineinfo.hxx>
#include <vcl/window.h>
#include <vcl/unowrap.hxx>
#include <vcl/salframe.hxx>


// =======================================================================

#define DOCKWIN_FLOATSTYLES         (WB_SIZEABLE | WB_MOVEABLE | WB_CLOSEABLE | WB_STANDALONE | WB_PINABLE | WB_ROLLABLE )

// =======================================================================


// =======================================================================

class ImplDockFloatWin2 : public FloatingWindow
{
private:
    ImplDockingWindowWrapper*  mpDockWin;
    ULONG			mnLastTicks;
    Timer			maDockTimer;
    Timer			maEndDockTimer;
    Point			maDockPos;
    Rectangle		maDockRect;
    BOOL            mbInMove;
    ULONG			mnLastUserEvent;

    DECL_LINK( DockingHdl, ImplDockFloatWin2* );
    DECL_LINK( DockTimerHdl, ImplDockFloatWin2* );
    DECL_LINK( EndDockTimerHdl, ImplDockFloatWin2* );
public:
    ImplDockFloatWin2( Window* pParent, WinBits nWinBits,
                      ImplDockingWindowWrapper* pDockingWin );
    ~ImplDockFloatWin2();

    virtual void	Move();
    virtual void	Resize();
    virtual void	TitleButtonClick( USHORT nButton );
    virtual void	Pin();
    virtual void	Roll();
    virtual void	PopupModeEnd();
    virtual void	Resizing( Size& rSize );
    virtual BOOL	Close();
    using Window::SetPosSizePixel;
    virtual void    SetPosSizePixel( long nX, long nY,
                                     long nWidth, long nHeight,
                                     USHORT nFlags = WINDOW_POSSIZE_ALL );

    ULONG GetLastTicks() const { return mnLastTicks; }
};

// =======================================================================

ImplDockFloatWin2::ImplDockFloatWin2( Window* pParent, WinBits nWinBits,
                                    ImplDockingWindowWrapper* pDockingWin ) :
        FloatingWindow( pParent, nWinBits ),
        mpDockWin( pDockingWin ),
        mnLastTicks( Time::GetSystemTicks() ),
        mbInMove( FALSE ),
        mnLastUserEvent( 0 )
{
    // Daten vom DockingWindow uebernehmen
    if ( pDockingWin )
    {
        SetSettings( pDockingWin->GetWindow()->GetSettings() );
        Enable( pDockingWin->GetWindow()->IsEnabled(), FALSE );
        EnableInput( pDockingWin->GetWindow()->IsInputEnabled(), FALSE );
        AlwaysEnableInput( pDockingWin->GetWindow()->IsAlwaysEnableInput(), FALSE );
        EnableAlwaysOnTop( pDockingWin->GetWindow()->IsAlwaysOnTopEnabled() );
        SetActivateMode( pDockingWin->GetWindow()->GetActivateMode() );
    }

    SetBackground( GetSettings().GetStyleSettings().GetFaceColor() );

    maDockTimer.SetTimeoutHdl( LINK( this, ImplDockFloatWin2, DockTimerHdl ) );
    maDockTimer.SetTimeout( 50 );
    maEndDockTimer.SetTimeoutHdl( LINK( this, ImplDockFloatWin2, EndDockTimerHdl ) );
    maEndDockTimer.SetTimeout( 50 );
}

// -----------------------------------------------------------------------

ImplDockFloatWin2::~ImplDockFloatWin2()
{
    if( mnLastUserEvent )
        Application::RemoveUserEvent( mnLastUserEvent );
}

// -----------------------------------------------------------------------

IMPL_LINK( ImplDockFloatWin2, DockTimerHdl, ImplDockFloatWin2*, EMPTYARG )
{
    DBG_ASSERT( mpDockWin->IsFloatingMode(), "docktimer called but not floating" );

    maDockTimer.Stop();
    PointerState aState = GetPointerState();

    if( aState.mnState & KEY_MOD1 )
    {
        // i43499 CTRL disables docking now
        mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->HideTracking();
        if( aState.mnState & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT ) )
            maDockTimer.Start();
    }
    else if( ! ( aState.mnState & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT ) ) )
    {
        mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->HideTracking();
        mpDockWin->EndDocking( maDockRect, FALSE );
    }
    else
    {
        mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->ShowTracking( maDockRect, SHOWTRACK_BIG | SHOWTRACK_WINDOW );
        maDockTimer.Start();
    }

    return 0;
}

IMPL_LINK( ImplDockFloatWin2, EndDockTimerHdl, ImplDockFloatWin2*, EMPTYARG )
{
    DBG_ASSERT( mpDockWin->IsFloatingMode(), "enddocktimer called but not floating" );

    maEndDockTimer.Stop();
    PointerState aState = GetPointerState();
    if( ! ( aState.mnState & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT ) ) )
    {
        mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->HideTracking();
        mpDockWin->EndDocking( maDockRect, TRUE );
    }
    else
    {
        maEndDockTimer.Start();
    }

    return 0;
}


IMPL_LINK( ImplDockFloatWin2, DockingHdl, ImplDockFloatWin2*, EMPTYARG )
{
    // called during move of a floating window
    mnLastUserEvent = 0;

    Window *pDockingArea = mpDockWin->GetWindow()->GetParent();
    PointerState aState = pDockingArea->GetPointerState();

    BOOL bRealMove = TRUE;
    if( GetStyle() & WB_OWNERDRAWDECORATION )
    {
        // for windows with ownerdraw decoration
        // we allow docking only when the window was moved
        // by dragging its caption
        // and ignore move request due to resizing
        Window *pBorder = GetWindow( WINDOW_BORDER );
        if( pBorder != this )
        {
            Point aPt;
            Rectangle aBorderRect( aPt, pBorder->GetSizePixel() );
            sal_Int32 nLeft, nTop, nRight, nBottom;
            GetBorder( nLeft, nTop, nRight, nBottom );
            // limit borderrect to the caption part only and without the resizing borders
            aBorderRect.nBottom = aBorderRect.nTop + nTop;
            aBorderRect.nLeft += nLeft;
            aBorderRect.nRight -= nRight;

            PointerState aBorderState = pBorder->GetPointerState();
            if( aBorderRect.IsInside( aBorderState.maPos ) )
                bRealMove = TRUE;
            else
                bRealMove = FALSE;
        }
    }

    if( mpDockWin->IsDockable()	&& 
        mpDockWin->GetWindow()->IsVisible() && 
        (Time::GetSystemTicks() - mnLastTicks > 500) &&
        ( aState.mnState & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT ) ) &&
        !(aState.mnState & KEY_MOD1) && // i43499 CTRL disables docking now
        bRealMove )
    {
        maDockPos = Point( pDockingArea->OutputToScreenPixel( pDockingArea->AbsoluteScreenToOutputPixel( OutputToAbsoluteScreenPixel( Point() ) ) ) );
        maDockRect = Rectangle( maDockPos, mpDockWin->GetSizePixel() );

        // mouse pos in screen pixels
        Point aMousePos = pDockingArea->OutputToScreenPixel( aState.maPos );

        if( ! mpDockWin->IsDocking() )
            mpDockWin->StartDocking( aMousePos, maDockRect );

        BOOL bFloatMode = mpDockWin->Docking( aMousePos, maDockRect );

        if( ! bFloatMode )
        {
            // indicates that the window could be docked at maDockRect
            maDockRect.SetPos( mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->ScreenToOutputPixel(
                 maDockRect.TopLeft() ) );
            mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->ShowTracking( maDockRect, SHOWTRACK_BIG | SHOWTRACK_WINDOW );
            maEndDockTimer.Stop();
            DockTimerHdl( this );
        }
        else
        {
            mpDockWin->GetWindow()->GetParent()->ImplGetFrameWindow()->HideTracking();
            maDockTimer.Stop();
            EndDockTimerHdl( this );
        }
    }
    mbInMove = FALSE;
    return 0;
}
// -----------------------------------------------------------------------

void ImplDockFloatWin2::Move()
{
    if( mbInMove )
        return;

    mbInMove = TRUE;
    FloatingWindow::Move();
    mpDockWin->GetWindow()->Move();

    /*
     *  note: the window should only dock if KEY_MOD1 is pressed
     *  and the user releases all mouse buttons. The real problem here
     *  is that we don't get mouse events (at least not on X)
     *  if the mouse is on the decoration. So we have to start an
     *  awkward timer based process that polls the modifier/buttons
     *  to see whether they are in the right condition shortly after the
     *  last Move message.
     */
    if( ! mnLastUserEvent )
        mnLastUserEvent = Application::PostUserEvent( LINK( this, ImplDockFloatWin2, DockingHdl ) );
}

// -----------------------------------------------------------------------

void ImplDockFloatWin2::Resize()
{
    // forwarding of resize only required if we have no borderwindow ( GetWindow() then returns 'this' )
    if( GetWindow( WINDOW_BORDER ) == this )
    {
        FloatingWindow::Resize();
        Size aSize( GetSizePixel() );
        mpDockWin->GetWindow()->ImplPosSizeWindow( 0, 0, aSize.Width(), aSize.Height(), WINDOW_POSSIZE_POSSIZE ); // is this needed ???
    }
}

void ImplDockFloatWin2::SetPosSizePixel( long nX, long nY,
                                     long nWidth, long nHeight,
                                     USHORT nFlags )
{
    FloatingWindow::SetPosSizePixel( nX, nY, nWidth, nHeight, nFlags );
}

// -----------------------------------------------------------------------


void ImplDockFloatWin2::TitleButtonClick( USHORT nButton )
{
    FloatingWindow::TitleButtonClick( nButton );
    mpDockWin->TitleButtonClick( nButton );
}

// -----------------------------------------------------------------------

void ImplDockFloatWin2::Pin()
{
    FloatingWindow::Pin();
    mpDockWin->Pin();
}

// -----------------------------------------------------------------------

void ImplDockFloatWin2::Roll()
{
    FloatingWindow::Roll();
    mpDockWin->Roll();
}

// -----------------------------------------------------------------------

void ImplDockFloatWin2::PopupModeEnd()
{
    FloatingWindow::PopupModeEnd();
    mpDockWin->PopupModeEnd();
}

// -----------------------------------------------------------------------

void ImplDockFloatWin2::Resizing( Size& rSize )
{
    FloatingWindow::Resizing( rSize );
    mpDockWin->Resizing( rSize );
}

// -----------------------------------------------------------------------

BOOL ImplDockFloatWin2::Close()
{
    return mpDockWin->Close();
}

// =======================================================================

DockingManager::DockingManager()
{
}

DockingManager::~DockingManager()
{
    ::std::vector< ImplDockingWindowWrapper* >::iterator p;
    p = mDockingWindows.begin();
    for(; p != mDockingWindows.end(); ++p )
    {
        delete (*p);
    }
    mDockingWindows.clear();
}

ImplDockingWindowWrapper* DockingManager::GetDockingWindowWrapper( const Window *pWindow )
{
	::std::vector< ImplDockingWindowWrapper* >::iterator p;
    p = mDockingWindows.begin();
    while( p != mDockingWindows.end() )
    {
	    if( (*p)->mpDockingWindow == pWindow )
            return (*p);
        else
            p++;
    }
    return NULL;
}

BOOL DockingManager::IsDockable( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );

    /*
    if( pWindow->HasDockingHandler() )
        return TRUE;
    */
    return (pWrapper != NULL);
}

BOOL DockingManager::IsFloating( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        return pWrapper->IsFloatingMode();
    else
        return FALSE;
}

BOOL DockingManager::IsLocked( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper && pWrapper->IsLocked() )
        return TRUE;
    else
        return FALSE;
}

void DockingManager::Lock( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        pWrapper->Lock();
}

void DockingManager::Unlock( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        pWrapper->Unlock();
}

void DockingManager::SetFloatingMode( const Window *pWindow, BOOL bFloating )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        pWrapper->SetFloatingMode( bFloating );
}

void DockingManager::StartPopupMode( ToolBox *pParentToolBox, const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        pWrapper->StartPopupMode( pParentToolBox );
}

BOOL DockingManager::IsInPopupMode( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper && pWrapper->IsInPopupMode() )
        return TRUE;
    else
        return FALSE;
}

void DockingManager::AddWindow( const Window *pWindow )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        return;
    else
        pWrapper = new ImplDockingWindowWrapper( pWindow );

    mDockingWindows.push_back( pWrapper );
}

void DockingManager::RemoveWindow( const Window *pWindow )
{
	::std::vector< ImplDockingWindowWrapper* >::iterator p;
    p = mDockingWindows.begin();
    while( p != mDockingWindows.end() )
    {
	    if( (*p)->mpDockingWindow == pWindow )
        {
            delete (*p);
	        mDockingWindows.erase( p );
            break;
        }
        else
            p++;
    }
}

void DockingManager::SetPosSizePixel( Window *pWindow, long nX, long nY,
	                                long nWidth, long nHeight,
							        USHORT nFlags )
{
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        pWrapper->SetPosSizePixel( nX, nY, nWidth, nHeight, nFlags );
}

Rectangle DockingManager::GetPosSizePixel( const Window *pWindow )
{
    Rectangle aRect;
    ImplDockingWindowWrapper* pWrapper = GetDockingWindowWrapper( pWindow );
    if( pWrapper )
        aRect = Rectangle( pWrapper->GetPosPixel(), pWrapper->GetSizePixel() );

    return aRect;
}

// =======================================================================
// special floating window for popup mode
// main purpose: provides tear-off area for undocking
// =======================================================================

// if TEAROFF_DASHED defined a single dashed line is used
// otherwise multiple smaller lines will be painted
//#define TEAROFF_DASHED  

// size of the drag area
#ifdef TEAROFF_DASHED
#define POPUP_DRAGBORDER    2 
#define POPUP_DRAGGRIP      5 
#else
#define POPUP_DRAGBORDER    3  
#define POPUP_DRAGGRIP      5
#endif
#define POPUP_DRAGHEIGHT    (POPUP_DRAGGRIP+POPUP_DRAGBORDER+POPUP_DRAGBORDER)
#define POPUP_DRAGWIDTH     20

class ImplPopupFloatWin : public FloatingWindow
{
private:
    ImplDockingWindowWrapper*   mpDockingWin;
    BOOL                        mbHighlight;
    BOOL                        mbMoving;
    Point                       maDelta;
    Point                       maTearOffPosition;
    void                        ImplSetBorder();

public:
    ImplPopupFloatWin( Window* pParent, ImplDockingWindowWrapper* pDockingWin );
    ~ImplPopupFloatWin();

    virtual ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > CreateAccessible();
    virtual void        Paint( const Rectangle& rRect );
    virtual void        MouseMove( const MouseEvent& rMEvt );
    virtual void        MouseButtonDown( const MouseEvent& rMEvt );
    virtual void        MouseButtonUp( const MouseEvent& rMEvt );
    virtual void        Tracking( const TrackingEvent& rTEvt );
    virtual void        Resize();
    virtual Window*     GetPreferredKeyInputWindow();

    Rectangle           GetDragRect() const;
    Point               GetToolboxPosition() const;
    Point               GetTearOffPosition() const;
    void                DrawGrip();
    void                DrawBorder();
};

ImplPopupFloatWin::ImplPopupFloatWin( Window* pParent, ImplDockingWindowWrapper* pDockingWin ) : 
    FloatingWindow( pParent, WB_NOBORDER | WB_SYSTEMWINDOW | WB_NOSHADOW)
{
    mpWindowImpl->mbToolbarFloatingWindow = TRUE;   // indicate window type, required for accessibility 
                                                    // which should not see this window as a toplevel window
    mpDockingWin = pDockingWin;
    mbHighlight = FALSE;
    mbMoving = FALSE;

    ImplSetBorder();
}

ImplPopupFloatWin::~ImplPopupFloatWin()
{
    mpDockingWin = NULL;
}

::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > ImplPopupFloatWin::CreateAccessible()
{
    // switch off direct accessibilty support for this window

    // this is to avoid appearance of this window as standalone window in the accessibility hierarchy
    // as this window is only used as a helper for subtoolbars that are not teared-off, the parent toolbar
    // has to provide accessibility support (as implemented in the toolkit) 
    // so the contained toolbar should appear as child of the correponsing toolbar item of the parent toolbar
    return ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible >();
}

Window* ImplPopupFloatWin::GetPreferredKeyInputWindow()
{
    if( mpWindowImpl->mpClientWindow )
        return mpWindowImpl->mpClientWindow;
    else
        return FloatingWindow::GetPreferredKeyInputWindow();
}


void ImplPopupFloatWin::ImplSetBorder()
{
    // although we have no border in the sense of a borderwindow
    //  we're using a special border for the grip
    // by setting those members the method SetOutputSizePixel() can
    //  be used to set the proper window size
    mpWindowImpl->mnTopBorder     = 1 + POPUP_DRAGHEIGHT+2;
    mpWindowImpl->mnBottomBorder  = 1;
    mpWindowImpl->mnLeftBorder    = 1;
    mpWindowImpl->mnRightBorder   = 1;
}

void ImplPopupFloatWin::Resize()
{
    // the borderview overwrites the border during resize so restore it
    ImplSetBorder();    
}

Rectangle ImplPopupFloatWin::GetDragRect() const
{
    return Rectangle( 1, 1, GetOutputSizePixel().Width()-1, 2+POPUP_DRAGHEIGHT );
}

Point ImplPopupFloatWin::GetToolboxPosition() const
{
    // return inner position where a toolbox could be placed
    Point aPt( 1, 1+GetDragRect().getHeight() );    // grip + border
    return aPt;
}

Point ImplPopupFloatWin::GetTearOffPosition() const
{
    Point aPt( maTearOffPosition );
    //aPt += GetToolboxPosition();    // remove 'decoration'
    return aPt;
}

void ImplPopupFloatWin::DrawBorder()
{
    SetFillColor();
    SetLineColor( GetSettings().GetStyleSettings().GetShadowColor() );
    Point aPt;
    Rectangle aRect( aPt, GetOutputSizePixel() );

    Region oldClipRgn( GetClipRegion( ) );
    Region aClipRgn( aRect );
    Rectangle aItemClipRect( ImplGetItemEdgeClipRect() );
    if( !aItemClipRect.IsEmpty() )
    {
        aItemClipRect.SetPos( AbsoluteScreenToOutputPixel( aItemClipRect.TopLeft() ) );
        aClipRgn.Exclude( aItemClipRect );
        SetClipRegion( aClipRgn );
    }
    DrawRect( aRect );
    SetClipRegion( oldClipRgn );
}

void ImplPopupFloatWin::DrawGrip()
{
    BOOL bLinecolor     = IsLineColor();
    Color aLinecolor    = GetLineColor();
    BOOL bFillcolor     = IsFillColor();
    Color aFillcolor    = GetFillColor();

    // draw background
    Rectangle aRect( GetDragRect() );
    aRect.nTop      += POPUP_DRAGBORDER;
    aRect.nBottom   -= POPUP_DRAGBORDER;
    aRect.nLeft+=3;
    aRect.nRight-=3;

    if( mbHighlight )
    {
        Erase( aRect );
        DrawSelectionBackground( aRect, 2, FALSE, TRUE, FALSE );
    }
    else
    {
        SetFillColor( GetSettings().GetStyleSettings().GetFaceColor() );
        SetLineColor();
        DrawRect( aRect );
    }

    if( !ToolBox::AlwaysLocked() )  // no grip if toolboxes are locked
    {
#ifdef TEAROFF_DASHED
        // draw single dashed line
        LineInfo aLineInfo( LINE_DASH );
        aLineInfo.SetDistance( 4 );
        aLineInfo.SetDashLen( 12 );
        aLineInfo.SetDashCount( 1 );

        aRect.nLeft+=2; aRect.nRight-=2;

        aRect.nTop+=2;
        aRect.nBottom = aRect.nTop;
        SetLineColor( GetSettings().GetStyleSettings().GetDarkShadowColor() );
        DrawLine( aRect.TopLeft(), aRect.TopRight(), aLineInfo );

        if( !mbHighlight )
        {
            aRect.nTop++; aRect.nBottom++;
            SetLineColor( GetSettings().GetStyleSettings().GetLightColor() );
            DrawLine( aRect.TopLeft(), aRect.TopRight(), aLineInfo );
        }

#else
        // draw several grip lines
        SetFillColor( GetSettings().GetStyleSettings().GetShadowColor() );
        aRect.nTop++;
        aRect.nBottom = aRect.nTop;

        int width = POPUP_DRAGWIDTH;
        while( width >= aRect.getWidth() )
            width -= 4;
        if( width <= 0 )
            width = aRect.getWidth();
        //aRect.nLeft = aRect.nLeft + (aRect.getWidth() - width) / 2;
        aRect.nLeft = (aRect.nLeft + aRect.nRight - width) / 2;
        aRect.nRight = aRect.nLeft + width;
        
        int i=0; 
        while( i< POPUP_DRAGGRIP )
        {
            DrawRect( aRect );
            aRect.nTop+=2;
            aRect.nBottom+=2;
            i+=2;
        }
#endif
    }

    if( bLinecolor )
        SetLineColor( aLinecolor );
    else
        SetLineColor();
    if( bFillcolor )
        SetFillColor( aFillcolor );
    else
        SetFillColor();
}

void ImplPopupFloatWin::Paint( const Rectangle& )
{
    Point aPt;
    Rectangle aRect( aPt, GetOutputSizePixel() );
    DrawWallpaper( aRect, Wallpaper( GetSettings().GetStyleSettings().GetFaceGradientColor() ) );
    DrawBorder();
    DrawGrip();
}

void ImplPopupFloatWin::MouseMove( const MouseEvent& rMEvt )
{
    Point aMousePos = rMEvt.GetPosPixel();

    if( !ToolBox::AlwaysLocked() )  // no tear off if locking is enabled
    {
        if( rMEvt.IsLeft() && GetDragRect().IsInside( aMousePos ) )
        {
            // start window move
            mbMoving = TRUE;
            StartTracking( STARTTRACK_NOKEYCANCEL );
            return;
        }
        if( !mbHighlight && GetDragRect().IsInside( aMousePos ) )
        {
            mbHighlight = TRUE;
            DrawGrip();
        }
        if( mbHighlight && ( rMEvt.IsLeaveWindow() || !GetDragRect().IsInside( aMousePos ) ) )
        {
            mbHighlight = FALSE;
            DrawGrip();
        }
    }
}

void ImplPopupFloatWin::MouseButtonUp( const MouseEvent& rMEvt )
{
    FloatingWindow::MouseButtonUp( rMEvt );
}

void ImplPopupFloatWin::MouseButtonDown( const MouseEvent& rMEvt )
{
    Point aMousePos = rMEvt.GetPosPixel();
    if( GetDragRect().IsInside( aMousePos ) )
    {
        // get mouse pos at a static window to have a fixed reference point
        PointerState aState = GetParent()->GetPointerState();
        if (ImplHasMirroredGraphics() && IsRTLEnabled())
            ImplMirrorFramePos(aState.maPos);
        maTearOffPosition = GetWindow( WINDOW_BORDER )->GetPosPixel();
        maDelta = aState.maPos - maTearOffPosition;
    }
}

void ImplPopupFloatWin::Tracking( const TrackingEvent& rTEvt )
{
    if( mbMoving )
    {
        if ( rTEvt.IsTrackingEnded() )
        {
            mbMoving = FALSE;
            EndPopupMode( FLOATWIN_POPUPMODEEND_TEAROFF );
        }
        else if ( !rTEvt.GetMouseEvent().IsSynthetic() )
        {
            // move the window according to mouse pos
            PointerState aState = GetParent()->GetPointerState();
            if (ImplHasMirroredGraphics() && IsRTLEnabled()) 
                ImplMirrorFramePos(aState.maPos);
            maTearOffPosition = aState.maPos - maDelta;
            GetWindow( WINDOW_BORDER )->SetPosPixel( maTearOffPosition );
        }
    }
}


// =======================================================================

ImplDockingWindowWrapper::ImplDockingWindowWrapper( const Window *pWindow )
{
    ImplInitData();

    mpDockingWindow = (Window*) pWindow;
    mpParent        = pWindow->GetParent();
    mbDockable      = TRUE; 
    mbLocked        = FALSE; 
    mnFloatBits     = WB_BORDER | WB_CLOSEABLE | WB_SIZEABLE | (pWindow->GetStyle() & DOCKWIN_FLOATSTYLES);
    DockingWindow *pDockWin = dynamic_cast< DockingWindow* > ( mpDockingWindow );
    if( pDockWin )
        mnFloatBits = pDockWin->GetFloatStyle();

    // must be enabled in Window::Notify to prevent permanent docking during mouse move
    mbStartDockingEnabled = FALSE;
}

ImplDockingWindowWrapper::~ImplDockingWindowWrapper()
{
    if ( IsFloatingMode() )
    {
        GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );
        SetFloatingMode( FALSE );
    }
}

// -----------------------------------------------------------------------

BOOL ImplDockingWindowWrapper::ImplStartDocking( const Point& rPos )
{
    if ( !mbDockable )
        return FALSE;

    if( !mbStartDockingEnabled )
        return FALSE;

    maMouseOff      = rPos;
    maMouseStart    = maMouseOff;
    mbDocking       = TRUE;
    mbLastFloatMode = IsFloatingMode();
    mbStartFloat    = mbLastFloatMode;

    // FloatingBorder berechnen
    FloatingWindow* pWin;
    if ( mpFloatWin )
        pWin = mpFloatWin;
    else
        pWin = new ImplDockFloatWin2( mpParent, mnFloatBits, NULL );
    pWin->GetBorder( mnDockLeft, mnDockTop, mnDockRight, mnDockBottom );
    if ( !mpFloatWin )
        delete pWin;

    Point   aPos    = GetWindow()->ImplOutputToFrame( Point() );
    Size    aSize   = GetWindow()->GetOutputSizePixel();
    mnTrackX        = aPos.X();
    mnTrackY        = aPos.Y();
    mnTrackWidth    = aSize.Width();
    mnTrackHeight   = aSize.Height();

    if ( mbLastFloatMode )
    {
        maMouseOff.X()  += mnDockLeft;
        maMouseOff.Y()  += mnDockTop;
        mnTrackX        -= mnDockLeft;
        mnTrackY        -= mnDockTop;
        mnTrackWidth    += mnDockLeft+mnDockRight;
        mnTrackHeight   += mnDockTop+mnDockBottom;
    }

    Window *pDockingArea = GetWindow()->GetParent();
    Window::PointerState aState = pDockingArea->GetPointerState();

    // mouse pos in screen pixels
    Point aMousePos = pDockingArea->OutputToScreenPixel( aState.maPos );
    Point aDockPos = Point( pDockingArea->AbsoluteScreenToOutputPixel( GetWindow()->OutputToAbsoluteScreenPixel( GetWindow()->GetPosPixel() ) ) );
    Rectangle aDockRect( aDockPos, GetWindow()->GetSizePixel() );
    StartDocking( aMousePos, aDockRect );

    GetWindow()->ImplUpdateAll();
    GetWindow()->ImplGetFrameWindow()->ImplUpdateAll();

    GetWindow()->StartTracking( STARTTRACK_KEYMOD );
    return TRUE;
}

// =======================================================================

void ImplDockingWindowWrapper::ImplInitData()
{
    mpDockingWindow     = NULL;

    //GetWindow()->mpWindowImpl->mbDockWin  = TRUE;     // TODO: must be eliminated
    mpFloatWin          = NULL;
    mbDockCanceled      = FALSE;
    mbFloatPrevented    = FALSE;
    mbDocking           = FALSE;
    mbPined             = FALSE;
    mbRollUp            = FALSE;
    mbDockBtn           = FALSE;
    mbHideBtn           = FALSE;
    maMaxOutSize        = Size( SHRT_MAX, SHRT_MAX );
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::Tracking( const TrackingEvent& rTEvt )
{
    // used during docking of a currently docked window
    if ( mbDocking )
    {
        if ( rTEvt.IsTrackingEnded() )
        {
            mbDocking = FALSE;
            GetWindow()->HideTracking();
            if ( rTEvt.IsTrackingCanceled() )
            {
                mbDockCanceled = TRUE;
                EndDocking( Rectangle( Point( mnTrackX, mnTrackY ), Size( mnTrackWidth, mnTrackHeight ) ), mbLastFloatMode );
                mbDockCanceled = FALSE;
            }
            else
                EndDocking( Rectangle( Point( mnTrackX, mnTrackY ), Size( mnTrackWidth, mnTrackHeight ) ), mbLastFloatMode );
        }
        // Docking only upon non-synthetic MouseEvents
        else if ( !rTEvt.GetMouseEvent().IsSynthetic() || rTEvt.GetMouseEvent().IsModifierChanged() )
        {
            Point   aMousePos = rTEvt.GetMouseEvent().GetPosPixel();
            Point   aFrameMousePos = GetWindow()->ImplOutputToFrame( aMousePos );
            Size    aFrameSize = GetWindow()->ImplGetFrameWindow()->GetOutputSizePixel();
            if ( aFrameMousePos.X() < 0 )
                aFrameMousePos.X() = 0;
            if ( aFrameMousePos.Y() < 0 )
                aFrameMousePos.Y() = 0;
            if ( aFrameMousePos.X() > aFrameSize.Width()-1 )
                aFrameMousePos.X() = aFrameSize.Width()-1;
            if ( aFrameMousePos.Y() > aFrameSize.Height()-1 )
                aFrameMousePos.Y() = aFrameSize.Height()-1;
            aMousePos = GetWindow()->ImplFrameToOutput( aFrameMousePos );
            aMousePos.X() -= maMouseOff.X();
            aMousePos.Y() -= maMouseOff.Y();
            Point aPos = GetWindow()->ImplOutputToFrame( aMousePos );
            Rectangle aTrackRect( aPos, Size( mnTrackWidth, mnTrackHeight ) );
            Rectangle aCompRect = aTrackRect;
            aPos.X()    += maMouseOff.X();
            aPos.Y()    += maMouseOff.Y();

            BOOL bFloatMode = Docking( aPos, aTrackRect );

            mbFloatPrevented = FALSE;
            if ( mbLastFloatMode != bFloatMode )
            {
                if ( bFloatMode )
                {
                    aTrackRect.Left()   -= mnDockLeft;
                    aTrackRect.Top()    -= mnDockTop;
                    aTrackRect.Right()  += mnDockRight;
                    aTrackRect.Bottom() += mnDockBottom;
                }
                else
                {
                    if ( aCompRect == aTrackRect )
                    {
                        aTrackRect.Left()   += mnDockLeft;
                        aTrackRect.Top()    += mnDockTop;
                        aTrackRect.Right()  -= mnDockRight;
                        aTrackRect.Bottom() -= mnDockBottom;
                    }
                }
                mbLastFloatMode = bFloatMode;
            }

            USHORT nTrackStyle;
            if ( bFloatMode )
                nTrackStyle = SHOWTRACK_OBJECT;
            else
                nTrackStyle = SHOWTRACK_BIG;
            Rectangle aShowTrackRect = aTrackRect;
            aShowTrackRect.SetPos( GetWindow()->ImplFrameToOutput( aShowTrackRect.TopLeft() ) );
            //if( bFloatMode )
                GetWindow()->ShowTracking( aShowTrackRect, nTrackStyle );
            /*else
            {
                GetWindow()->HideTracking();
                Point aPt( GetWindow()->GetParent()->ScreenToOutputPixel( aTrackRect.TopLeft() ) );
                GetWindow()->SetPosPixel( aPt );
            }*/

            // Maus-Offset neu berechnen, da Rechteck veraendert werden
            // konnte
            maMouseOff.X()  = aPos.X() - aTrackRect.Left();
            maMouseOff.Y()  = aPos.Y() - aTrackRect.Top();

            mnTrackX        = aTrackRect.Left();
            mnTrackY        = aTrackRect.Top();
            mnTrackWidth    = aTrackRect.GetWidth();
            mnTrackHeight   = aTrackRect.GetHeight();
        }
    }
}


// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::StartDocking( const Point& rPoint, Rectangle& rRect )
{
    DockingData data( rPoint, rRect, IsFloatingMode() );

    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_STARTDOCKING, &data );
    mbDocking = TRUE;
}

// -----------------------------------------------------------------------

BOOL ImplDockingWindowWrapper::Docking( const Point& rPoint, Rectangle& rRect )
{
    DockingData data( rPoint, rRect, IsFloatingMode() );

    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_DOCKING, &data );
	rRect = data.maTrackRect;
    return data.mbFloating;
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::EndDocking( const Rectangle& rRect, BOOL bFloatMode )
{
	Rectangle aRect( rRect );

    if ( !IsDockingCanceled() )
    {
        BOOL bShow = FALSE;
        if ( bFloatMode != IsFloatingMode() )
        {
            GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );
            SetFloatingMode( bFloatMode );
            bShow = TRUE;
            if ( bFloatMode )
            {
                // #i44800# always use outputsize - as in all other places
                mpFloatWin->SetOutputSizePixel( aRect.GetSize() );
                mpFloatWin->SetPosPixel( aRect.TopLeft() );
            }
        }
        if ( !bFloatMode )
        {
            Point aPos = aRect.TopLeft();
            aPos = GetWindow()->GetParent()->ScreenToOutputPixel( aPos );
            GetWindow()->SetPosSizePixel( aPos, aRect.GetSize() );
        }

        if ( bShow )
            GetWindow()->Show( TRUE, SHOW_NOFOCUSCHANGE | SHOW_NOACTIVATE );
    }

    EndDockingData data( aRect, IsFloatingMode(), IsDockingCanceled() );
    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_ENDDOCKING, &data );

    mbDocking = FALSE;

    // must be enabled in Window::Notify to prevent permanent docking during mouse move
    mbStartDockingEnabled = FALSE;
}

// -----------------------------------------------------------------------

BOOL ImplDockingWindowWrapper::PrepareToggleFloatingMode()
{
    BOOL bFloating = TRUE;
    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_PREPARETOGGLEFLOATING, &bFloating );
    return bFloating;
}

// -----------------------------------------------------------------------

BOOL ImplDockingWindowWrapper::Close()
{
    // TODO: send event
/*
    ImplDelData aDelData;
    ImplAddDel( &aDelData );
    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_CLOSE );
    if ( aDelData.IsDelete() )
        return FALSE;
    ImplRemoveDel( &aDelData );

    if ( mpWindowImpl->mxWindowPeer.is() && IsCreatedWithToolkit() )
        return FALSE;

    GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );
    */
    return TRUE;
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::ToggleFloatingMode()
{
    // notify dockingwindow/toolbox
    // note: this must be done *before* notifying the 
    //       listeners to have the toolbox in the proper state
    if( GetWindow()->ImplIsDockingWindow() )
        ((DockingWindow*) GetWindow())->ToggleFloatingMode();

    // now notify listeners
    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_TOGGLEFLOATING );

    // must be enabled in Window::Notify to prevent permanent docking during mouse move
    mbStartDockingEnabled = FALSE;
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::TitleButtonClick( USHORT nType )
{
    if( nType == TITLE_BUTTON_MENU )
    {
        ToolBox *pToolBox = dynamic_cast< ToolBox* >( GetWindow() );
        if( pToolBox )
        {
            pToolBox->ExecuteCustomMenu();
        }
    }
    if( nType == TITLE_BUTTON_DOCKING )
    {
        SetFloatingMode( !IsFloatingMode() );
    }
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::Pin()
{
    // TODO: send event
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::Roll()
{
    // TODO: send event
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::PopupModeEnd()
{
    // TODO: send event
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::Resizing( Size& rSize )
{
    // TODO: add virtual Resizing() to class Window, so we can get rid of class DockingWindow
    DockingWindow *pDockingWindow = dynamic_cast< DockingWindow* >( GetWindow() );
    if( pDockingWindow )
        pDockingWindow->Resizing( rSize );
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::ShowTitleButton( USHORT nButton, BOOL bVisible )
{
    if ( mpFloatWin )
        mpFloatWin->ShowTitleButton( nButton, bVisible );
    else
    {
        if ( nButton == TITLE_BUTTON_DOCKING )
            mbDockBtn = bVisible;
        else // if ( nButton == TITLE_BUTTON_HIDE )
            mbHideBtn = bVisible;
    }
}

// -----------------------------------------------------------------------

BOOL ImplDockingWindowWrapper::IsTitleButtonVisible( USHORT nButton ) const
{
    if ( mpFloatWin )
        return mpFloatWin->IsTitleButtonVisible( nButton );
    else
    {
        if ( nButton == TITLE_BUTTON_DOCKING )
            return mbDockBtn;
        else // if ( nButton == TITLE_BUTTON_HIDE ) 
            return mbHideBtn;
    }
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::StartPopupMode( ToolBox *pParentToolBox )
{
    // do nothing if window is floating
    if( IsFloatingMode() )
        return;

    GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );

    // prepare reparenting
    Window* pRealParent = GetWindow()->GetWindow( WINDOW_PARENT );
    mpOldBorderWin = GetWindow()->GetWindow( WINDOW_BORDER );
    if( mpOldBorderWin == GetWindow() )
        mpOldBorderWin = NULL;  // no border window found

    // the new parent for popup mode
    ImplPopupFloatWin* pWin = new ImplPopupFloatWin( mpParent, this );

    pWin->SetPopupModeEndHdl( LINK( this, ImplDockingWindowWrapper, PopupModeEnd ) );
    pWin->SetText( GetWindow()->GetText() );

    pWin->SetOutputSizePixel( GetWindow()->GetSizePixel() );

    GetWindow()->mpWindowImpl->mpBorderWindow  = NULL;
    GetWindow()->mpWindowImpl->mnLeftBorder    = 0;
    GetWindow()->mpWindowImpl->mnTopBorder     = 0;
    GetWindow()->mpWindowImpl->mnRightBorder   = 0;
    GetWindow()->mpWindowImpl->mnBottomBorder  = 0;

    // position toolbox below dragrect
    GetWindow()->SetPosPixel( pWin->GetToolboxPosition() );

    // reparent borderwindow and window
    if ( mpOldBorderWin )
        mpOldBorderWin->SetParent( pWin );
    GetWindow()->SetParent( pWin );

    // correct border window pointers
    GetWindow()->mpWindowImpl->mpBorderWindow = pWin;
    pWin->mpWindowImpl->mpClientWindow = GetWindow();
    GetWindow()->mpWindowImpl->mpRealParent = pRealParent;

    // set mpFloatWin not until all window positioning is done !!!
    // (SetPosPixel etc. check for valid mpFloatWin pointer)
    mpFloatWin = pWin;

    ULONG nFlags =  FLOATWIN_POPUPMODE_ALLOWTEAROFF         |
                    FLOATWIN_POPUPMODE_NOFOCUSCLOSE 	    |
                    FLOATWIN_POPUPMODE_ALLMOUSEBUTTONCLOSE  |
                    FLOATWIN_POPUPMODE_NOMOUSEUPCLOSE;
//                    |FLOATWIN_POPUPMODE_NOAPPFOCUSCLOSE;

    // if the subtoolbar was opened via keyboard make sure that key events
    // will go into subtoolbar
    if( pParentToolBox->IsKeyEvent() )
        nFlags |= FLOATWIN_POPUPMODE_GRABFOCUS;

    mpFloatWin->StartPopupMode( pParentToolBox, nFlags );
    GetWindow()->Show();

    if( pParentToolBox->IsKeyEvent() )
    {
        // send HOME key to subtoolbar in order to select first item
        KeyEvent aEvent( 0, KeyCode( KEY_HOME ) );
        mpFloatWin->GetPreferredKeyInputWindow()->KeyInput( aEvent );
    }
}

IMPL_LINK( ImplDockingWindowWrapper, PopupModeEnd, void*, EMPTYARG )
{
    GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );

    // set parameter for handler before destroying floating window
    ImplPopupFloatWin *pPopupFloatWin = (ImplPopupFloatWin*) mpFloatWin;
    EndPopupModeData aData( pPopupFloatWin->GetTearOffPosition(), mpFloatWin->IsPopupModeTearOff() );

    // before deleting change parent back, so we can delete the floating window alone
    Window* pRealParent = GetWindow()->GetWindow( WINDOW_PARENT );
    GetWindow()->mpWindowImpl->mpBorderWindow = NULL;
    if ( mpOldBorderWin )
    {
        GetWindow()->SetParent( mpOldBorderWin );
        ((ImplBorderWindow*)mpOldBorderWin)->GetBorder( 
            GetWindow()->mpWindowImpl->mnLeftBorder, GetWindow()->mpWindowImpl->mnTopBorder,
            GetWindow()->mpWindowImpl->mnRightBorder, GetWindow()->mpWindowImpl->mnBottomBorder );
        mpOldBorderWin->Resize();
    }
    GetWindow()->mpWindowImpl->mpBorderWindow = mpOldBorderWin;
    GetWindow()->SetParent( pRealParent );
    GetWindow()->mpWindowImpl->mpRealParent = pRealParent;

    delete mpFloatWin;
    mpFloatWin = NULL;

    // call handler - which will destroy the window and thus the wrapper as well !
    GetWindow()->ImplCallEventListeners( VCLEVENT_WINDOW_ENDPOPUPMODE, &aData );

    return 0;
}


BOOL ImplDockingWindowWrapper::IsInPopupMode() const
{
    if( GetFloatingWindow() )
        return GetFloatingWindow()->IsInPopupMode();
    else
        return FALSE;
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::SetFloatingMode( BOOL bFloatMode )
{
    // do nothing if window is docked and locked
    if( !IsFloatingMode() && IsLocked() )
        return;

    if ( IsFloatingMode() != bFloatMode )
    {
        if ( PrepareToggleFloatingMode() )
        {
            BOOL bVisible = GetWindow()->IsVisible();

            if ( bFloatMode )
            {
                GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );

                maDockPos = GetWindow()->GetPosPixel();

                Window* pRealParent = GetWindow()->GetWindow( WINDOW_PARENT );
                mpOldBorderWin = GetWindow()->GetWindow( WINDOW_BORDER );
                if( mpOldBorderWin == mpDockingWindow )
                    mpOldBorderWin = NULL;  // no border window found

                ImplDockFloatWin2* pWin =
                    new ImplDockFloatWin2(
                                         mpParent,
                                         mnFloatBits & ( WB_MOVEABLE | WB_SIZEABLE | WB_CLOSEABLE ) ?
                                          mnFloatBits | WB_SYSTEMWINDOW
//#ifdef __USE_OWNERDRAWDECORATION__
                                          | WB_OWNERDRAWDECORATION 
//#endif
                                          : mnFloatBits,
                                         this );

                // reduce the border width for seamless NWF painting
                // (especially for the toolbar gradient on Windows XP)
                /*AllSettings aSettings( pWin->GetSettings() );
                StyleSettings aStyleSettings( aSettings.GetStyleSettings() );
                aStyleSettings.SetBorderSize( 0 );
                aSettings.SetStyleSettings( aStyleSettings );
                pWin->SetSettings( aSettings );*/

                mpFloatWin      = pWin;


                GetWindow()->mpWindowImpl->mpBorderWindow  = NULL;
                GetWindow()->mpWindowImpl->mnLeftBorder    = 0;
                GetWindow()->mpWindowImpl->mnTopBorder     = 0;
                GetWindow()->mpWindowImpl->mnRightBorder   = 0;
                GetWindow()->mpWindowImpl->mnBottomBorder  = 0;

                // Falls Parent zerstoert wird, muessen wir auch vom
                // BorderWindow den Parent umsetzen
                if ( mpOldBorderWin )
                    mpOldBorderWin->SetParent( pWin );
                GetWindow()->SetParent( pWin );
                pWin->SetPosPixel( Point() );

                GetWindow()->mpWindowImpl->mpBorderWindow = pWin;
                pWin->mpWindowImpl->mpClientWindow = mpDockingWindow;
                GetWindow()->mpWindowImpl->mpRealParent = pRealParent;

                pWin->SetText( GetWindow()->GetText() );
                pWin->SetOutputSizePixel( GetWindow()->GetSizePixel() );
                pWin->SetPosPixel( maFloatPos );
                // DockingDaten ans FloatingWindow weiterreichen
                pWin->ShowTitleButton( TITLE_BUTTON_DOCKING, mbDockBtn );
                pWin->ShowTitleButton( TITLE_BUTTON_HIDE, mbHideBtn );
                pWin->SetPin( mbPined );
                if ( mbRollUp )
                    pWin->RollUp();
                else
                    pWin->RollDown();
                pWin->SetRollUpOutputSizePixel( maRollUpOutSize );
                pWin->SetMinOutputSizePixel( maMinOutSize );
                pWin->SetMaxOutputSizePixel( maMaxOutSize );

                if ( bVisible )
                    GetWindow()->Show( TRUE, SHOW_NOFOCUSCHANGE | SHOW_NOACTIVATE );

                ToggleFloatingMode();
            }
            else
            {
                GetWindow()->Show( FALSE, SHOW_NOFOCUSCHANGE );

                // FloatingDaten wird im FloatingWindow speichern
                maFloatPos      = mpFloatWin->GetPosPixel();
                mbDockBtn       = mpFloatWin->IsTitleButtonVisible( TITLE_BUTTON_DOCKING );
                mbHideBtn       = mpFloatWin->IsTitleButtonVisible( TITLE_BUTTON_HIDE );
                mbPined         = mpFloatWin->IsPined();
                mbRollUp        = mpFloatWin->IsRollUp();
                maRollUpOutSize = mpFloatWin->GetRollUpOutputSizePixel();
                maMinOutSize    = mpFloatWin->GetMinOutputSizePixel();
                maMaxOutSize    = mpFloatWin->GetMaxOutputSizePixel();

                Window* pRealParent = GetWindow()->GetWindow( WINDOW_PARENT ); //mpWindowImpl->mpRealParent;
                GetWindow()->mpWindowImpl->mpBorderWindow = NULL;
                if ( mpOldBorderWin )
                {
                    GetWindow()->SetParent( mpOldBorderWin );
                    ((ImplBorderWindow*)mpOldBorderWin)->GetBorder( 
                        GetWindow()->mpWindowImpl->mnLeftBorder, GetWindow()->mpWindowImpl->mnTopBorder,
                        GetWindow()->mpWindowImpl->mnRightBorder, GetWindow()->mpWindowImpl->mnBottomBorder );
                    mpOldBorderWin->Resize();
                }
                GetWindow()->mpWindowImpl->mpBorderWindow = mpOldBorderWin;
                GetWindow()->SetParent( pRealParent );
                GetWindow()->mpWindowImpl->mpRealParent = pRealParent;

                delete static_cast<ImplDockFloatWin2*>(mpFloatWin);
                mpFloatWin = NULL;
                GetWindow()->SetPosPixel( maDockPos );

                if ( bVisible )
                    GetWindow()->Show();

                ToggleFloatingMode();

            }
        }
    }
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::SetFloatStyle( WinBits nStyle )
{
    mnFloatBits = nStyle;
}

// -----------------------------------------------------------------------

WinBits ImplDockingWindowWrapper::GetFloatStyle() const
{
    return mnFloatBits;
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::SetTabStop()
{
    GetWindow()->SetStyle( GetWindow()->GetStyle() | (WB_GROUP | WB_TABSTOP) );
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::SetPosSizePixel( long nX, long nY,
                                     long nWidth, long nHeight,
                                     USHORT nFlags )
{
    if ( mpFloatWin )
        mpFloatWin->SetPosSizePixel( nX, nY, nWidth, nHeight, nFlags );
    else
        GetWindow()->SetPosSizePixel( nX, nY, nWidth, nHeight, nFlags );
}

// -----------------------------------------------------------------------

Point ImplDockingWindowWrapper::GetPosPixel() const
{
    if ( mpFloatWin )
        return mpFloatWin->GetPosPixel();
    else
        return mpDockingWindow->GetPosPixel();
}

// -----------------------------------------------------------------------

Size ImplDockingWindowWrapper::GetSizePixel() const
{
    if ( mpFloatWin )
        return mpFloatWin->GetSizePixel();
    else
        return mpDockingWindow->GetSizePixel();
}

// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::SetOutputSizePixel( const Size& rNewSize )
{
    if ( mpFloatWin )
        mpFloatWin->SetOutputSizePixel( rNewSize );
    else
        GetWindow()->SetOutputSizePixel( rNewSize );
}

// -----------------------------------------------------------------------

Size ImplDockingWindowWrapper::GetOutputSizePixel() const
{
    if ( mpFloatWin )
        return mpFloatWin->GetOutputSizePixel();
    else
        return mpDockingWindow->GetOutputSizePixel();
}

Point ImplDockingWindowWrapper::GetFloatingPos() const
{
	if ( mpFloatWin )
    {
        //Rectangle aRect = mpFloatWin->GetWindow( WINDOW_CLIENT)->GetWindowExtentsRelative( mpFloatWin->GetParent() );
        WindowStateData aData;
        aData.SetMask( WINDOWSTATE_MASK_POS );
        mpFloatWin->GetWindowStateData( aData );
        Point aPos( aData.GetX(), aData.GetY() );
        aPos = mpFloatWin->GetParent()->ImplGetFrameWindow()->AbsoluteScreenToOutputPixel( aPos );
        return aPos;
    }
	else
		return maFloatPos;
}

// -----------------------------------------------------------------------
// old inlines from DockingWindow
// -----------------------------------------------------------------------

void ImplDockingWindowWrapper::SetPin( BOOL bPin )
{
	if ( mpFloatWin )
		mpFloatWin->SetPin( bPin );
	mbPined = bPin;
}

BOOL ImplDockingWindowWrapper::IsPined() const
{
	if ( mpFloatWin )
		return mpFloatWin->IsPined();
	return mbPined;
}

void ImplDockingWindowWrapper::RollUp()
{
	if ( mpFloatWin )
		mpFloatWin->RollUp();
	mbRollUp = TRUE;
}

void ImplDockingWindowWrapper::RollDown()
{
	if ( mpFloatWin )
		mpFloatWin->RollDown();
	mbRollUp = FALSE;
}

BOOL ImplDockingWindowWrapper::IsRollUp() const
{
	if ( mpFloatWin )
		return mpFloatWin->IsRollUp();
	return mbRollUp;
}

void ImplDockingWindowWrapper::SetRollUpOutputSizePixel( const Size& rSize )
{
	if ( mpFloatWin )
		mpFloatWin->SetRollUpOutputSizePixel( rSize );
	maRollUpOutSize = rSize;
}

Size ImplDockingWindowWrapper::GetRollUpOutputSizePixel() const
{
	if ( mpFloatWin )
		return mpFloatWin->GetRollUpOutputSizePixel();
	return maRollUpOutSize;
}

void ImplDockingWindowWrapper::SetMinOutputSizePixel( const Size& rSize )
{
	if ( mpFloatWin )
		mpFloatWin->SetMinOutputSizePixel( rSize );
	maMinOutSize = rSize;
}

void ImplDockingWindowWrapper::SetMaxOutputSizePixel( const Size& rSize )
{
	if ( mpFloatWin )
		mpFloatWin->SetMaxOutputSizePixel( rSize );
	maMaxOutSize = rSize;
}

const Size& ImplDockingWindowWrapper::GetMinOutputSizePixel() const
{
	if ( mpFloatWin )
		return mpFloatWin->GetMinOutputSizePixel();
	return maMinOutSize;
}

const Size& ImplDockingWindowWrapper::GetMaxOutputSizePixel() const
{
	if ( mpFloatWin )
		return mpFloatWin->GetMaxOutputSizePixel();
	return maMaxOutSize;
}

void ImplDockingWindowWrapper::SetFloatingPos( const Point& rNewPos )
{
	if ( mpFloatWin )
		mpFloatWin->SetPosPixel( rNewPos );
	else
		maFloatPos = rNewPos;
}

BOOL ImplDockingWindowWrapper::IsFloatingMode() const
{
	return (mpFloatWin != NULL);
}


void    ImplDockingWindowWrapper::SetDragArea( const Rectangle& rRect )
{
    maDragArea = rRect;
}

Rectangle  ImplDockingWindowWrapper::GetDragArea() const
{
    return maDragArea;
}

void ImplDockingWindowWrapper::Lock()
{
    mbLocked = TRUE;
    // only toolbars support locking
    ToolBox *pToolBox = dynamic_cast< ToolBox * >( GetWindow() );
    if( pToolBox )
        pToolBox->Lock( mbLocked );
}

void ImplDockingWindowWrapper::Unlock()
{
    mbLocked = FALSE;
    // only toolbars support locking
    ToolBox *pToolBox = dynamic_cast< ToolBox * >( GetWindow() );
    if( pToolBox )
        pToolBox->Lock( mbLocked );
}

BOOL ImplDockingWindowWrapper::IsLocked() const
{
    return mbLocked;
}
