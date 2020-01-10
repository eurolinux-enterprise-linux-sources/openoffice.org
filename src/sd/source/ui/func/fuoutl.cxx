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


#include "fuoutl.hxx"

#include <svx/outliner.hxx>
#include "OutlineView.hxx"
#include "OutlineViewShell.hxx"
#ifndef SD_WINDOW_SHELL_HXX
#include "Window.hxx"
#endif

namespace sd {

TYPEINIT1( FuOutline, FuPoor );

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuOutline::FuOutline (
    ViewShell* pViewShell, 
    ::sd::Window* pWindow, 
    ::sd::View* pView,
    SdDrawDocument* pDoc, 
    SfxRequest& rReq)
    : FuPoor(pViewShell, pWindow, pView, pDoc, rReq),
      pOutlineViewShell (static_cast<OutlineViewShell*>(pViewShell)),
      pOutlineView (static_cast<OutlineView*>(pView))
{
}

FunctionReference FuOutline::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
	FunctionReference xFunc( new FuOutline( pViewSh, pWin, pView, pDoc, rReq ) );
	return xFunc;
}

/*************************************************************************
|*
|* Command, weiterleiten an OutlinerView
|*
\************************************************************************/

BOOL FuOutline::Command(const CommandEvent& rCEvt)
{
	BOOL bResult = FALSE;

	OutlinerView* pOlView = 
        static_cast<OutlineView*>(mpView)->GetViewByWindow(mpWindow);
	DBG_ASSERT (pOlView, "keine OutlinerView gefunden");

	if (pOlView)
	{
		pOlView->Command(rCEvt);		// liefert leider keinen Returnwert
		bResult = TRUE;
	}
	return bResult;
}

void FuOutline::ScrollStart()
{
}

void FuOutline::ScrollEnd()
{
}


} // end of namespace sd
