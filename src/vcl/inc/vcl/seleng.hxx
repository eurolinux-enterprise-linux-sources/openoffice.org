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

#ifndef _SV_SELENG_HXX
#define _SV_SELENG_HXX

#include <vcl/dllapi.h>
#include <vcl/timer.hxx>
#include <vcl/event.hxx>

class Window;
class CommandEvent;

// Timerticks
#define SELENG_DRAGDROP_TIMEOUT     400
#define SELENG_AUTOREPEAT_INTERVAL  50

enum SelectionMode { NO_SELECTION, SINGLE_SELECTION, RANGE_SELECTION, MULTIPLE_SELECTION };

// ---------------
// - FunctionSet -
// ---------------

class VCL_DLLPUBLIC FunctionSet
{
public:
    virtual void    BeginDrag() = 0;

    virtual void    CreateAnchor() = 0;  // Anker-Pos := Cursor-Pos
	virtual void	DestroyAnchor() = 0;

    // Cursor neu setzen, dabei die beim Anker beginnende
    // Selektion der neuen Cursor-Position anpassen. TRUE == Ok
    virtual BOOL    SetCursorAtPoint( const Point& rPointPixel,
                                      BOOL bDontSelectAtCursor = FALSE ) = 0;

    virtual BOOL    IsSelectionAtPoint( const Point& rPointPixel ) = 0;
	virtual void    DeselectAtPoint( const Point& rPointPixel ) = 0;
	// Anker loeschen & alles deselektieren
    virtual void    DeselectAll() = 0;
};

// -------------------
// - SelectionEngine -
// -------------------

#define SELENG_DRG_ENAB     0x0001
#define SELENG_IN_SEL       0x0002
#define SELENG_IN_ADD       0x0004
#define SELENG_ADD_ALW      0x0008
#define SELENG_IN_DRG       0x0010
#define SELENG_HAS_ANCH     0x0020
#define SELENG_CMDEVT       0x0040
#define SELENG_WAIT_UPEVT   0x0080
#define SELENG_EXPANDONMOVE	0x0100

class VCL_DLLPUBLIC SelectionEngine
{
private:
    FunctionSet*        pFunctionSet;
    Window*             pWin;
    Rectangle           aArea;
    Timer               aWTimer; // erzeugt kuenstliche Mouse-Moves
    MouseEvent          aLastMove;
    SelectionMode       eSelMode;
    // Stufigkeit fuer Mausbewegungen waehrend einer Selektion
    USHORT              nMouseSensitivity;
    USHORT              nLockedMods;
    USHORT              nFlags;
//#if 0 // _SOLAR__PRIVATE
    DECL_DLLPRIVATE_LINK( ImpWatchDog, Timer * );
//#endif

	inline BOOL			ShouldDeselect( BOOL bModifierKey1 ) const;
								// determines to deselect or not when Ctrl-key is pressed on CursorPosChanging
public:

                        SelectionEngine( Window* pWindow,
                                         FunctionSet* pFunctions = NULL );
                        ~SelectionEngine();

    // TRUE: Event wurde von Selection-Engine verarbeitet.
    BOOL                SelMouseButtonDown( const MouseEvent& rMEvt );
    BOOL                SelMouseButtonUp( const MouseEvent& rMEvt );
    BOOL                SelMouseMove( const MouseEvent& rMEvt );

    // Tastatur
    void                CursorPosChanging( BOOL bShift, BOOL bMod1 );

    // wird benoetigt, um bei ausserhalb des Bereichs stehender
    // Maus ueber einen Timer Move-Events zu erzeugen
    void                SetVisibleArea( const Rectangle rNewArea )
                            { aArea = rNewArea; }
    const Rectangle&    GetVisibleArea() const { return aArea; }

    void                SetAddMode( BOOL);
    BOOL                IsAddMode() const;

    void                AddAlways( BOOL bOn );
    BOOL                IsAlwaysAdding() const;

    void                EnableDrag( BOOL bOn );
    BOOL                IsDragEnabled() const;
    void                ActivateDragMode();
    BOOL                IsInDragMode() const;

    void                SetSelectionMode( SelectionMode eMode );
    SelectionMode       GetSelectionMode() const { return eSelMode; }

    void                SetFunctionSet( FunctionSet* pFuncs )
                            { pFunctionSet = pFuncs; }
    const FunctionSet*  GetFunctionSet() const { return pFunctionSet; }

    void                SetMouseSensitivity( USHORT nSensitivity )
                            { nMouseSensitivity = nSensitivity; }
    USHORT              GetMouseSensitivity() const
                            { return nMouseSensitivity; }

    const Point&        GetMousePosPixel() const
                            { return aLastMove.GetPosPixel(); }
    const MouseEvent&   GetMouseEvent() const { return aLastMove; }

    void                SetWindow( Window*);
    Window*             GetWindow() const { return pWin; }

    void                LockModifiers( USHORT nModifiers )
                            { nLockedMods = nModifiers; }
    USHORT              GetLockedModifiers() const { return nLockedMods; }

    BOOL                IsInSelection() const;
    void                Reset();

    void                Command( const CommandEvent& rCEvt );

    BOOL                HasAnchor() const;
    void                SetAnchor( BOOL bAnchor );

	// wird im Ctor eingeschaltet
	void				ExpandSelectionOnMouseMove( BOOL bExpand = TRUE )
						{
							if( bExpand )
								nFlags |= SELENG_EXPANDONMOVE;
							else
								nFlags &= ~SELENG_EXPANDONMOVE;
						}
};

inline BOOL SelectionEngine::IsDragEnabled() const
{
    if ( nFlags & SELENG_DRG_ENAB )
        return TRUE;
    else
        return FALSE;
}

inline BOOL SelectionEngine::IsAddMode()  const
{
    if ( nFlags & (SELENG_IN_ADD | SELENG_ADD_ALW) )
        return TRUE;
    else
        return FALSE;
}

inline void SelectionEngine::SetAddMode( BOOL bNewMode )
{
    if ( bNewMode )
        nFlags |= SELENG_IN_ADD;
    else
        nFlags &= (~SELENG_IN_ADD);
}

inline void SelectionEngine::EnableDrag( BOOL bOn )
{
    if ( bOn )
        nFlags |= SELENG_DRG_ENAB;
    else
        nFlags &= (~SELENG_DRG_ENAB);
}

inline void SelectionEngine::AddAlways( BOOL bOn )
{
    if( bOn )
        nFlags |= SELENG_ADD_ALW;
    else
        nFlags &= (~SELENG_ADD_ALW);
}

inline BOOL SelectionEngine::IsAlwaysAdding() const
{
    if ( nFlags & SELENG_ADD_ALW )
        return TRUE;
    else
        return FALSE;
}

inline BOOL SelectionEngine::IsInDragMode() const
{
    if ( nFlags & SELENG_IN_DRG )
        return TRUE;
    else
        return FALSE;
}

inline BOOL SelectionEngine::IsInSelection() const
{
    if ( nFlags & SELENG_IN_SEL )
        return TRUE;
    else
        return FALSE;
}

inline BOOL SelectionEngine::HasAnchor() const
{
    if ( nFlags & SELENG_HAS_ANCH )
        return TRUE;
    else
        return FALSE;
}

inline void SelectionEngine::SetAnchor( BOOL bAnchor )
{
    if ( bAnchor )
        nFlags |= SELENG_HAS_ANCH;
    else
        nFlags &= (~SELENG_HAS_ANCH);
}

#endif	// _SV_SELENG_HXX

