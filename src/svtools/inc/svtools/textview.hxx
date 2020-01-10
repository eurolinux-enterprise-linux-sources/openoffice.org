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

#ifndef _TEXTVIEW_HXX
#define _TEXTVIEW_HXX

#include "svtools/svtdllapi.h"
#include <svtools/textdata.hxx>
#include <tools/gen.hxx>
#include <vcl/dndhelp.hxx>

class TextEngine;
class OutputDevice;
class Window;
class Cursor;
class KeyEvent;
class MouseEvent;
class CommandEvent;
class TextSelFunctionSet;
class SelectionEngine;
class VirtualDevice;
struct TextDDInfo;

namespace com {
namespace sun {
namespace star {
namespace datatransfer {
namespace clipboard {
	class XClipboard;
}}}}}

struct ImpTextView;

class SVT_DLLPUBLIC TextView : public vcl::unohelper::DragAndDropClient
{
	friend class 		TextEngine;
	friend class 		TextUndo;
	friend class 		TextUndoManager;
	friend class		TextSelFunctionSet;
	friend class		ExtTextView;

private:
    ImpTextView*        mpImpl;

						TextView( const TextView& ) : vcl::unohelper::DragAndDropClient() 		{}
	TextView&			operator=( const TextView& ) 		{ return *this; }

protected:
	void				ShowSelection();
	void				HideSelection();
	void				ShowSelection( const TextSelection& rSel );
    void                ImpShowHideSelection( BOOL bShow, const TextSelection* pRange = NULL );

	TextSelection		ImpMoveCursor( const KeyEvent& rKeyEvent );
	TextPaM				ImpDelete( BOOL bForward, BYTE nMode );
	void				ImpSetSelection( const TextSelection& rNewSel, BOOL bUI );
	BOOL 				IsInSelection( const TextPaM& rPaM );

	void				ImpPaint( OutputDevice* pOut, const Point& rStartPos, Rectangle const* pPaintArea, TextSelection const* pPaintRange = 0, TextSelection const* pSelection = 0 );
	void				ImpPaint( const Rectangle& rRect, BOOL bUseVirtDev );
	void				ImpShowCursor( BOOL bGotoCursor, BOOL bForceVisCursor, BOOL bEndKey );
	void				ImpHighlight( const TextSelection& rSel );
    void                ImpSetSelection( const TextSelection& rSelection );
    Point               ImpGetOutputStartPos( const Point& rStartDocPos ) const;

	void				ImpHideDDCursor();
	void				ImpShowDDCursor();

    bool                ImplTruncateNewText( rtl::OUString& rNewText ) const;
	BOOL				ImplCheckTextLen( const String& rNewText );

	VirtualDevice* 		GetVirtualDevice();

    // DragAndDropClient
    virtual void        dragGestureRecognized( const ::com::sun::star::datatransfer::dnd::DragGestureEvent& dge ) throw (::com::sun::star::uno::RuntimeException);
    virtual void        dragDropEnd( const ::com::sun::star::datatransfer::dnd::DragSourceDropEvent& dsde ) throw (::com::sun::star::uno::RuntimeException);
    virtual void        drop( const ::com::sun::star::datatransfer::dnd::DropTargetDropEvent& dtde ) throw (::com::sun::star::uno::RuntimeException);
    virtual void        dragEnter( const ::com::sun::star::datatransfer::dnd::DropTargetDragEnterEvent& dtdee ) throw (::com::sun::star::uno::RuntimeException);
    virtual void        dragExit( const ::com::sun::star::datatransfer::dnd::DropTargetEvent& dte ) throw (::com::sun::star::uno::RuntimeException);
    virtual void        dragOver( const ::com::sun::star::datatransfer::dnd::DropTargetDragEvent& dtde ) throw (::com::sun::star::uno::RuntimeException);

            using       DragAndDropClient::dragEnter;
            using       DragAndDropClient::dragExit;
            using       DragAndDropClient::dragOver;

public:
						TextView( TextEngine* pEng, Window* pWindow );
	virtual			   ~TextView();

    TextEngine*         GetTextEngine() const;
    Window*             GetWindow() const;

	void				Invalidate();
	void				Scroll( long nHorzScroll, long nVertScroll );

	void				ShowCursor( BOOL bGotoCursor = TRUE, BOOL bForceVisCursor = TRUE );
	void				HideCursor();

    void                EnableCursor( BOOL bEnable );
    BOOL                IsCursorEnabled() const;  

	const TextSelection&	GetSelection() const;
    TextSelection&      GetSelection();
    void                SetSelection( const TextSelection& rNewSel );
	void				SetSelection( const TextSelection& rNewSel, BOOL bGotoCursor );
    BOOL                HasSelection() const;

	String				GetSelected();
	String				GetSelected( LineEnd aSeparator );
	void				DeleteSelected();

	void				InsertNewText( const rtl::OUString& rNew, BOOL bSelect = FALSE );
    // deprecated: use InsertNewText instead
	void				InsertText( const String& rNew, BOOL bSelect = FALSE );

	BOOL				KeyInput( const KeyEvent& rKeyEvent );
	void				Paint( const Rectangle& rRect );
	void				MouseButtonUp( const MouseEvent& rMouseEvent );
	void				MouseButtonDown( const MouseEvent& rMouseEvent );
	void				MouseMove( const MouseEvent& rMouseEvent );
	void				Command( const CommandEvent& rCEvt );

	void				Cut();
	void				Copy();
	void				Paste();

    void                Copy( ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboard >& rxClipboard );
    void                Paste( ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboard >& rxClipboard );

	void				Undo();
	void				Redo();

	BOOL				Read( SvStream& rInput );
	BOOL				Write( SvStream& rOutput );

    void                SetStartDocPos( const Point& rPos );
    const Point&        GetStartDocPos() const;

	Point		        GetDocPos( const Point& rWindowPos ) const;
	Point		        GetWindowPos( const Point& rDocPos ) const;

	void				SetInsertMode( BOOL bInsert );
    BOOL                IsInsertMode() const;

    void                SetAutoIndentMode( BOOL bAutoIndent );
    BOOL                IsAutoIndentMode() const;

	void				SetReadOnly( BOOL bReadOnly );
    BOOL                IsReadOnly() const;

    void                SetAutoScroll( BOOL bAutoScroll );
    BOOL                IsAutoScroll() const;

	BOOL 				SetCursorAtPoint( const Point& rPointPixel );
	BOOL 				IsSelectionAtPoint( const Point& rPointPixel );

	void				SetPaintSelection( BOOL bPaint);
    BOOL                IsPaintSelection() const;

	void				SetHighlightSelection( BOOL bSelectByHighlight );
    BOOL                IsHighlightSelection() const;

	void 				EraseVirtualDevice();

	// aus dem protected Teil hierher verschoben
	// F�r 'SvtXECTextCursor' (TL). Mu� ggf nochmal anders gel�st werden.
	TextPaM				PageUp( const TextPaM& rPaM );
	TextPaM				PageDown( const TextPaM& rPaM );
	TextPaM				CursorUp( const TextPaM& rPaM );
	TextPaM				CursorDown( const TextPaM& rPaM );
	TextPaM				CursorLeft( const TextPaM& rPaM, USHORT nCharacterIteratorMode );
	TextPaM				CursorRight( const TextPaM& rPaM, USHORT nCharacterIteratorMode );
	TextPaM				CursorWordLeft( const TextPaM& rPaM );
	TextPaM				CursorWordRight( const TextPaM& rPaM );
	TextPaM				CursorStartOfLine( const TextPaM& rPaM );
	TextPaM				CursorEndOfLine( const TextPaM& rPaM );
	TextPaM				CursorStartOfParagraph( const TextPaM& rPaM );
	TextPaM				CursorEndOfParagraph( const TextPaM& rPaM );
	TextPaM				CursorStartOfDoc();
	TextPaM				CursorEndOfDoc();

    // Old, remove!
	TextPaM				CursorLeft( const TextPaM& rPaM, BOOL bWordMode = FALSE );
	TextPaM				CursorRight( const TextPaM& rPaM, BOOL bWordMode = FALSE );

    /**
        Drag and Drop, deleting and selection regards all text that has an attribute 
        TEXTATTR_PROTECTED set as one entitity. Drag and dropped text is automatically 
        attibuted as protected.
     */
    void                SupportProtectAttribute(sal_Bool bSupport);

    /**
        Returns the number in paragraph of the line in which the cursor is blinking
        if enabled, -1 otherwise.
     */
    sal_Int32           GetLineNumberOfCursorInSelection() const;
};

#endif // _TEXTVIEW_HXX
