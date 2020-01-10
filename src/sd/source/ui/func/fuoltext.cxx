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
#include "precompiled_sd.hxx"


#include "fuoltext.hxx"

#include <sfx2/viewfrm.hxx>
#include <svx/outliner.hxx>
#include <svx/eeitem.hxx>
#include <svx/flditem.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/dispatch.hxx>

#include <svx/svxids.hrc>
#include "app.hrc"
#include "OutlineView.hxx"
#ifndef SD_WINDOW_SHELL_HXX
#include "Window.hxx"
#endif
#include "DrawDocShell.hxx"
#include "ViewShell.hxx"
#include "OutlineViewShell.hxx"

#include <stdio.h>          // Fuer SlotFilter-Listing

namespace sd {

static USHORT SidArray[] = {
				SID_STYLE_FAMILY2,
				SID_STYLE_FAMILY3,
				SID_STYLE_FAMILY5,
				SID_STYLE_UPDATE_BY_EXAMPLE,
				SID_CUT,
				SID_COPY,
				SID_PASTE,
				SID_SELECTALL,
				SID_ATTR_CHAR_FONT,
				SID_ATTR_CHAR_POSTURE,
				SID_ATTR_CHAR_WEIGHT,
				SID_ATTR_CHAR_UNDERLINE,
				SID_ATTR_CHAR_FONTHEIGHT,
				SID_ATTR_CHAR_COLOR,
				SID_OUTLINE_UP,
				SID_OUTLINE_DOWN,
				SID_OUTLINE_LEFT,
				SID_OUTLINE_RIGHT,
				//SID_OUTLINE_FORMAT,
				SID_OUTLINE_COLLAPSE_ALL,
				//SID_OUTLINE_BULLET,
				SID_OUTLINE_COLLAPSE,
				SID_OUTLINE_EXPAND_ALL,
				SID_OUTLINE_EXPAND,
				SID_SET_SUPER_SCRIPT,
				SID_SET_SUB_SCRIPT,
				SID_HYPERLINK_GETLINK,
				SID_PRESENTATION_TEMPLATES,
				SID_STATUS_PAGE,
				SID_STATUS_LAYOUT,
				SID_EXPAND_PAGE,
				SID_SUMMARY_PAGE,
				SID_PARASPACE_INCREASE,
				SID_PARASPACE_DECREASE,
				0 };

TYPEINIT1( FuOutlineText, FuOutline );

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuOutlineText::FuOutlineText(ViewShell* pViewShell, ::sd::Window* pWindow,
							 ::sd::View* pView, SdDrawDocument* pDoc,
							 SfxRequest& rReq)
	   : FuOutline(pViewShell, pWindow, pView, pDoc, rReq)
{
}

FunctionReference FuOutlineText::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
	FunctionReference xFunc( new FuOutlineText( pViewSh, pWin, pView, pDoc, rReq ) );
	xFunc->DoExecute( rReq );
	return xFunc;
}

/*************************************************************************
|*
|* MouseButtonDown-event
|*
\************************************************************************/

BOOL FuOutlineText::MouseButtonDown(const MouseEvent& rMEvt)
{
	BOOL bReturn = FALSE;

	mpWindow->GrabFocus();

	bReturn = pOutlineView->GetViewByWindow(mpWindow)->MouseButtonDown(rMEvt);

	if (bReturn)
	{
		// Attributierung der akt. Textstelle kann jetzt anders sein
		mpViewShell->GetViewFrame()->GetBindings().Invalidate( SidArray );
	}
	else
	{
		bReturn = FuOutline::MouseButtonDown(rMEvt);
	}

	return (bReturn);
}

/*************************************************************************
|*
|* MouseMove-event
|*
\************************************************************************/

BOOL FuOutlineText::MouseMove(const MouseEvent& rMEvt)
{
	BOOL bReturn = FALSE;

	bReturn = pOutlineView->GetViewByWindow(mpWindow)->MouseMove(rMEvt);

	if (!bReturn)
	{
		bReturn = FuOutline::MouseMove(rMEvt);
	}

    // MT 07/2002: Done in OutlinerView::MouseMove
    /*
	const SvxFieldItem* pFieldItem = pOutlineView->GetViewByWindow( mpWindow )->
										GetFieldUnderMousePointer();
	const SvxFieldData* pField = NULL;
	if( pFieldItem )
		pField = pFieldItem->GetField();

	if( pField && pField->ISA( SvxURLField ) )
	{
	   mpWindow->SetPointer( Pointer( POINTER_REFHAND ) );
	}
	else
	   mpWindow->SetPointer( Pointer( POINTER_TEXT ) );
    */

	return (bReturn);
}

/*************************************************************************
|*
|* MouseButtonUp-event
|*
\************************************************************************/

BOOL FuOutlineText::MouseButtonUp(const MouseEvent& rMEvt)
{
	BOOL bReturn = FALSE;

	bReturn = pOutlineView->GetViewByWindow(mpWindow)->MouseButtonUp(rMEvt);

	if (bReturn)
	{
		// Attributierung der akt. Textstelle kann jetzt anders sein
		mpViewShell->GetViewFrame()->GetBindings().Invalidate( SidArray );
	}
	else
	{
		const SvxFieldItem* pFieldItem = pOutlineView->GetViewByWindow( mpWindow )->GetFieldUnderMousePointer();
		if( pFieldItem )
		{
			const SvxFieldData* pField = pFieldItem->GetField();

			if( pField && pField->ISA( SvxURLField ) )
			{
				bReturn = TRUE;
	            mpWindow->ReleaseMouse();
                SfxStringItem aStrItem( SID_FILE_NAME, ( (SvxURLField*) pField)->GetURL() );
                SfxStringItem aReferer( SID_REFERER, mpDocSh->GetMedium()->GetName() );
                SfxBoolItem aBrowseItem( SID_BROWSE, TRUE );
                SfxViewFrame* pFrame = mpViewShell->GetViewFrame();

				if ( rMEvt.IsMod1() )
				{
					// Im neuen Frame oeffnen
                	pFrame->GetDispatcher()->Execute(SID_OPENDOC, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD,
                    		    &aStrItem, &aBrowseItem, &aReferer, 0L);
				}
				else
				{
					// Im aktuellen Frame oeffnen
					SfxFrameItem aFrameItem( SID_DOCFRAME, pFrame );
                	pFrame->GetDispatcher()->Execute(SID_OPENDOC, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD,
                    		    &aStrItem, &aFrameItem, &aBrowseItem, &aReferer, 0L);
				}
			}
		}
	}

	if( !bReturn )
		bReturn = FuOutline::MouseButtonUp(rMEvt);

	return (bReturn);
}

/*************************************************************************
|*
|* Tastaturereignisse bearbeiten
|*
|* Wird ein KeyEvent bearbeitet, so ist der Return-Wert TRUE, andernfalls
|* FALSE.
|*
\************************************************************************/

BOOL FuOutlineText::KeyInput(const KeyEvent& rKEvt)
{
	BOOL bReturn = FALSE;

	USHORT nKeyGroup = rKEvt.GetKeyCode().GetGroup();
	if( !mpDocSh->IsReadOnly() || nKeyGroup == KEYGROUP_CURSOR )
	{
		mpWindow->GrabFocus();

		std::auto_ptr< OutlineViewModelChangeGuard > aGuard;

		if( (nKeyGroup != KEYGROUP_CURSOR) && (nKeyGroup != KEYGROUP_FKEYS) )
			aGuard.reset( new OutlineViewModelChangeGuard( *pOutlineView ) );

		bReturn = pOutlineView->GetViewByWindow(mpWindow)->PostKeyEvent(rKEvt);

		if (bReturn)
		{
            UpdateForKeyPress (rKEvt);
		}
		else
		{
			bReturn = FuOutline::KeyInput(rKEvt);
		}
	}

	return (bReturn);
}

void FuOutlineText::UpdateForKeyPress (const KeyEvent& rEvent)
{
    // Attributes at the current text position may have changed.
    mpViewShell->GetViewFrame()->GetBindings().Invalidate(SidArray);

    bool bUpdatePreview = true;
    switch (rEvent.GetKeyCode().GetCode())
    {
        // When just the cursor has been moved the preview only changes when
        // it moved to entries of another page.  To prevent unnecessary
        // updates we check this here.  This is an early rejection test, so
        // missing a key is not a problem.
        case KEY_UP:
        case KEY_DOWN:
        case KEY_LEFT:
        case KEY_RIGHT:
        case KEY_HOME:
        case KEY_END:
        case KEY_PAGEUP:
        case KEY_PAGEDOWN:
        {
            SdPage* pCurrentPage = pOutlineViewShell->GetActualPage();
            bUpdatePreview = (pCurrentPage != pOutlineViewShell->GetActualPage());
        }
        break;
    }
    if (bUpdatePreview)
        pOutlineViewShell->UpdatePreview (pOutlineViewShell->GetActualPage());
}




/*************************************************************************
|*
|* Function aktivieren
|*
\************************************************************************/

void FuOutlineText::Activate()
{
	FuOutline::Activate();
}

/*************************************************************************
|*
|* Function deaktivieren
|*
\************************************************************************/

void FuOutlineText::Deactivate()
{
	FuOutline::Deactivate();
}

/*************************************************************************
|*
|* Cut object to clipboard
|*
\************************************************************************/

void FuOutlineText::DoCut()
{
	pOutlineView->GetViewByWindow(mpWindow)->Cut();
}

/*************************************************************************
|*
|* Copy object to clipboard
|*
\************************************************************************/

void FuOutlineText::DoCopy()
{
	pOutlineView->GetViewByWindow(mpWindow)->Copy();
}

/*************************************************************************
|*
|* Paste object from clipboard
|*
\************************************************************************/

void FuOutlineText::DoPaste()
{
	pOutlineView->GetViewByWindow(mpWindow)->PasteSpecial();
}


} // end of namespace sd
