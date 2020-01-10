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


#include "fusearch.hxx"

#include <sfx2/viewfrm.hxx>

#include <svx/svxids.hrc>
#include <sfx2/srchitem.hxx>
#include <svx/srchdlg.hxx>
#include <sfx2/bindings.hxx>
#include "fupoor.hxx"
#include "fuspell.hxx"  // wegen SidArraySpell[]
#ifndef SD_WINDOW_SHELL_HXX
#include "Window.hxx"
#endif
#include "drawdoc.hxx"
#include "app.hrc"
#include "app.hxx"
#include "View.hxx"
#include "Outliner.hxx"
#include "DrawViewShell.hxx"
#include "OutlineViewShell.hxx"
#include "ViewShellBase.hxx"

class SfxRequest;

namespace sd {

TYPEINIT1( FuSearch, FuPoor );

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuSearch::FuSearch (
    ViewShell* pViewSh,
    ::sd::Window* pWin,
    ::sd::View* pView,
    SdDrawDocument* pDoc,
    SfxRequest& rReq )
    : FuPoor(pViewSh, pWin, pView, pDoc, rReq),
      pSdOutliner(NULL),
      bOwnOutliner(FALSE)
{
}

FunctionReference FuSearch::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
	FunctionReference xFunc( new FuSearch( pViewSh, pWin, pView, pDoc, rReq ) );
	xFunc->DoExecute(rReq);
	return xFunc;
}

void FuSearch::DoExecute( SfxRequest& )
{
	mpViewShell->GetViewFrame()->GetBindings().Invalidate( SidArraySpell );

	if ( mpViewShell->ISA(DrawViewShell) )
	{
		bOwnOutliner = TRUE;
		pSdOutliner = new ::sd::Outliner( mpDoc, OUTLINERMODE_TEXTOBJECT );
	}
	else if ( mpViewShell->ISA(OutlineViewShell) )
	{
		bOwnOutliner = FALSE;
		pSdOutliner = mpDoc->GetOutliner();
	}

	if (pSdOutliner)
	   pSdOutliner->PrepareSpelling();
}

/*************************************************************************
|*
|* Destruktor
|*
\************************************************************************/

FuSearch::~FuSearch()
{
    if ( ! mpDocSh->IsInDestruction() && mpDocSh->GetViewShell()!=NULL)
        mpDocSh->GetViewShell()->GetViewFrame()->GetBindings().Invalidate( SidArraySpell );

	if (pSdOutliner)
		pSdOutliner->EndSpelling();

	if (bOwnOutliner)
		delete pSdOutliner;
}


/*************************************************************************
|*
|* Suchen&Ersetzen
|*
\************************************************************************/

void FuSearch::SearchAndReplace( const SvxSearchItem* pSearchItem )
{
	ViewShellBase* pBase = PTR_CAST(ViewShellBase, SfxViewShell::Current());
    ViewShell* pViewShell = NULL;
    if (pBase != NULL)
        pViewShell = pBase->GetMainViewShell().get();

	if (pViewShell != NULL)
	{
		if ( pSdOutliner && pViewShell->ISA(DrawViewShell) && !bOwnOutliner )
		{
			pSdOutliner->EndSpelling();

			bOwnOutliner = TRUE;
			pSdOutliner = new ::sd::Outliner( mpDoc, OUTLINERMODE_TEXTOBJECT );
			pSdOutliner->PrepareSpelling();
		}
		else if ( pSdOutliner && pViewShell->ISA(OutlineViewShell) && bOwnOutliner )
		{
			pSdOutliner->EndSpelling();
			delete pSdOutliner;

			bOwnOutliner = FALSE;
			pSdOutliner = mpDoc->GetOutliner();
			pSdOutliner->PrepareSpelling();
		}

		if (pSdOutliner)
		{
			BOOL bEndSpelling = pSdOutliner->StartSearchAndReplace(pSearchItem);

			if (bEndSpelling)
			{
				pSdOutliner->EndSpelling();
				pSdOutliner->PrepareSpelling();
			}
		}
	}
}



} // end of namespace sd
