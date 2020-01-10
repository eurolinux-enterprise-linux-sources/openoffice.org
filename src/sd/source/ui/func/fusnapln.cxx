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

#include "fusnapln.hxx"
#include <svtools/aeitem.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/request.hxx>


#include "strings.hrc"

#include "sdattr.hxx"
#include "View.hxx"
#include "ViewShell.hxx"
#include "DrawViewShell.hxx"
#ifndef SD_WINDOW_SHELL_HXX
#include "Window.hxx"
#endif
#include "sdenumdef.hxx"
#include "sdresid.hxx"
#include "sdabstdlg.hxx"
#include "app.hrc"
#include <svx/svdpagv.hxx>

namespace sd {

TYPEINIT1( FuSnapLine, FuPoor );

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuSnapLine::FuSnapLine(ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView,
					   SdDrawDocument* pDoc, SfxRequest& rReq) :
	FuPoor(pViewSh, pWin, pView, pDoc, rReq)
{
}

FunctionReference FuSnapLine::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
	FunctionReference xFunc( new FuSnapLine( pViewSh, pWin, pView, pDoc, rReq ) );
	xFunc->DoExecute(rReq);
	return xFunc;
}

void FuSnapLine::DoExecute( SfxRequest& rReq )
{
	const SfxItemSet* pArgs = rReq.GetArgs();
	SdrPageView* pPV = 0;
	USHORT	nHelpLine = 0;
	BOOL	bCreateNew = TRUE;

    // Get index of snap line or snap point from the request.
    SFX_REQUEST_ARG (rReq, pHelpLineIndex, SfxUInt32Item, ID_VAL_INDEX, FALSE);
    if (pHelpLineIndex != NULL)
    {
        nHelpLine = static_cast<USHORT>(pHelpLineIndex->GetValue());
        // Reset the argument pointer to trigger the display of the dialog.
        pArgs = NULL;
    }
    
	if ( !pArgs )
	{
		SfxItemSet aNewAttr(mpViewShell->GetPool(), ATTR_SNAPLINE_START, ATTR_SNAPLINE_END);
        bool bLineExist (false);
        pPV = mpView->GetSdrPageView();
		Point aLinePos;
        
        if (pHelpLineIndex == NULL)
        {
            // The index of the snap line is not provided as argument to the
            // request.  Determine it from the mouse position.
            
            aLinePos = static_cast<DrawViewShell*>(mpViewShell)->GetMousePos();
            static_cast<DrawViewShell*>(mpViewShell)->SetMousePosFreezed( FALSE );


            if ( aLinePos.X() >= 0 )
            {
                aLinePos = mpWindow->PixelToLogic(aLinePos);
                USHORT nHitLog = (USHORT) mpWindow->PixelToLogic(Size(HITPIX,0)).Width();
                bLineExist = mpView->PickHelpLine(aLinePos, nHitLog, *mpWindow, nHelpLine, pPV);
                if ( bLineExist )
                    aLinePos = (pPV->GetHelpLines())[nHelpLine].GetPos();
                else
                    pPV = mpView->GetSdrPageView();

                pPV->LogicToPagePos(aLinePos);
            }
            else
                aLinePos = Point(0,0);
        }
        else
        {
            OSL_ASSERT(pPV!=NULL);
            aLinePos = (pPV->GetHelpLines())[nHelpLine].GetPos();
            bLineExist = true;
        }
		aNewAttr.Put(SfxUInt32Item(ATTR_SNAPLINE_X, aLinePos.X()));
		aNewAttr.Put(SfxUInt32Item(ATTR_SNAPLINE_Y, aLinePos.Y()));

		SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
		AbstractSdSnapLineDlg* pDlg = pFact ? pFact->CreateSdSnapLineDlg( NULL, aNewAttr, mpView ) : 0;
		if( pDlg )
		{
			if ( bLineExist )
			{
				pDlg->HideRadioGroup();

				const SdrHelpLine& rHelpLine = (pPV->GetHelpLines())[nHelpLine];

				if ( rHelpLine.GetKind() == SDRHELPLINE_POINT )
				{
					pDlg->SetText(String(SdResId(STR_SNAPDLG_SETPOINT)));
					pDlg->SetInputFields(TRUE, TRUE);
				}
				else
				{
					pDlg->SetText(String(SdResId(STR_SNAPDLG_SETLINE)));

					if ( rHelpLine.GetKind() == SDRHELPLINE_VERTICAL )
						pDlg->SetInputFields(TRUE, FALSE);
					else
						pDlg->SetInputFields(FALSE, TRUE);
				}
				bCreateNew = FALSE;
			}
			else
				pDlg->HideDeleteBtn();

			USHORT nResult = pDlg->Execute();

			pDlg->GetAttr(aNewAttr);
			delete pDlg;

			switch( nResult )
			{
				case RET_OK:
					rReq.Done(aNewAttr);
					pArgs = rReq.GetArgs();
					break;

				case RET_SNAP_DELETE:
					// Fangobjekt loeschen
					if ( !bCreateNew )
						pPV->DeleteHelpLine(nHelpLine);
					// und weiter wie bei default
				default:
					return;
			}
		}
	}
	Point aHlpPos;

	aHlpPos.X() = ((const SfxUInt32Item&) pArgs->Get(ATTR_SNAPLINE_X)).GetValue();
	aHlpPos.Y() = ((const SfxUInt32Item&) pArgs->Get(ATTR_SNAPLINE_Y)).GetValue();
	pPV->PagePosToLogic(aHlpPos);

	if ( bCreateNew )
	{
		SdrHelpLineKind eKind;

		pPV = mpView->GetSdrPageView();

		switch ( (SnapKind) ((const SfxAllEnumItem&)
				 pArgs->Get(ATTR_SNAPLINE_KIND)).GetValue() )
		{
			case SK_HORIZONTAL	: eKind = SDRHELPLINE_HORIZONTAL;	break;
			case SK_VERTICAL	: eKind = SDRHELPLINE_VERTICAL; 	break;
			default 			: eKind = SDRHELPLINE_POINT;		break;
		}
		pPV->InsertHelpLine(SdrHelpLine(eKind, aHlpPos));
	}
	else
	{
		const SdrHelpLine& rHelpLine = (pPV->GetHelpLines())[nHelpLine];
		pPV->SetHelpLine(nHelpLine, SdrHelpLine(rHelpLine.GetKind(), aHlpPos));
	}
}

void FuSnapLine::Activate()
{
}

void FuSnapLine::Deactivate()
{
}

} // end of namespace sd
