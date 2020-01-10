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
#include "precompiled_sc.hxx"

// INCLUDE ---------------------------------------------------------------

#include <com/sun/star/embed/EmbedStates.hpp>

#include <svx/eeitem.hxx>
#include <svx/flditem.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdotext.hxx>
#include <sfx2/dispatch.hxx>
#include <svtools/imapobj.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdomedia.hxx>
#include <svx/svdpagv.hxx>
#include <svx/outlobj.hxx>
#include <svx/svdocapt.hxx>
#include <sfx2/app.hxx>

#include "fusel.hxx"
#include "sc.hrc"
#include "fudraw.hxx"
#include "futext.hxx"
#include "drawview.hxx"
#include "tabvwsh.hxx"
#include "drawpage.hxx"
#include "globstr.hrc"
#include "drwlayer.hxx"
#include "userdat.hxx"
#include "scmod.hxx"

// -----------------------------------------------------------------------

//	Maximal erlaubte Mausbewegung um noch Drag&Drop zu starten
//!	fusel,fuconstr,futext - zusammenfassen!
#define SC_MAXDRAGMOVE	3

// -----------------------------------------------------------------------

#ifdef _MSC_VER
#pragma optimize ( "", off )
#endif

using namespace com::sun::star;

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuSelection::FuSelection(ScTabViewShell* pViewSh, Window* pWin, ScDrawView* pViewP,
			   SdrModel* pDoc, SfxRequest& rReq ) :
    FuDraw(pViewSh, pWin, pViewP, pDoc, rReq),
	bVCAction(FALSE)
{
}

/*************************************************************************
|*
|* Destruktor
|*
\************************************************************************/

FuSelection::~FuSelection()
{
}

BYTE FuSelection::Command(const CommandEvent& rCEvt)
{
	//	special code for non-VCL OS2/UNX removed

	return FuDraw::Command( rCEvt );
}

/*************************************************************************
|*
|* MouseButtonDown-event
|*
\************************************************************************/

BOOL __EXPORT FuSelection::MouseButtonDown(const MouseEvent& rMEvt)
{
	// #95491# remember button state for creation of own MouseEvents
	SetMouseButtonCode(rMEvt.GetButtons());
	const bool bSelectionOnly = rMEvt.IsRight();
	if ( pView->IsAction() )
	{
        if ( bSelectionOnly )
			pView->BckAction();
		return TRUE;
	}

	bVCAction = FALSE;
	bIsInDragMode = FALSE;		//	irgendwo muss es ja zurueckgesetzt werden (#50033#)

	BOOL bReturn = FuDraw::MouseButtonDown(rMEvt);

	aMDPos = pWindow->PixelToLogic( rMEvt.GetPosPixel() );

	if ( rMEvt.IsLeft() )
	{
		SdrHdl* pHdl = pView->PickHandle(aMDPos);
		SdrObject* pObj;
		SdrPageView* pPV;

		if ( pHdl!=NULL || pView->IsMarkedHit(aMDPos) )
		{
		    // Determine if this is the tail of a SdrCaptionObj i.e.
		    // we need to disable the drag option on the tail of a note
		    // object. Also, disable the ability to use the circular
		    // drag of a note object.
            bool bDrag = false;
		    const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
		    if( rMarkList.GetMarkCount() == 1 )
		    {
                SdrObject* pMarkedObj = rMarkList.GetMark( 0 )->GetMarkedSdrObj();
                if( ScDrawLayer::IsNoteCaption( pMarkedObj ) )
			    {
			        // move using the valid caption handles for note text box.
			        if(pHdl && (pHdl->GetKind() != HDL_POLY && pHdl->GetKind() != HDL_CIRC))
                        bDrag = true;
			        // move the complete note box.
			        else if(!pHdl)
                        bDrag = true;
			    }
			    else
                    bDrag = true;   // different object
		    }
            else
                bDrag = true;       // several objects

            if ( bDrag )
            {
                aDragTimer.Start();
                pView->BegDragObj(aMDPos, (OutputDevice*) NULL, pHdl);
                bReturn = TRUE;
            }
		}
		else
		{
			BOOL bAlt = rMEvt.IsMod2();
			if ( !bAlt && pView->PickObj(aMDPos, pView->getHitTolLog(), pObj, pPV, SDRSEARCH_PICKMACRO) )
			{
				pView->BegMacroObj(aMDPos, pObj, pPV, pWindow);
				bReturn = TRUE;
			}
			else
			{
                String sURL, sTarget;
                if ( !bAlt && pView->PickObj(aMDPos, pView->getHitTolLog(), pObj, pPV, SDRSEARCH_ALSOONMASTER))
                {
                   // Support for imported Excel docs
                   // Excel is of course not consistent and allows
                   // a hyperlink to be assigned for an object group
                   // and even though the hyperlink is exported in the Escher layer
                   // its never used, when dealing with a group object the link
                   // associated with the clicked object is used only

                   // additionally you can also select a macro in Excel for a grouped
                   // objects and this results in the macro being set for the elements
                   // in the group and no macro is exported for the group

                   // if a macro and hlink are defined favour the hlink

                   // If a group object has no hyperlink use the hyperlink of the
                   // object clicked

                   if ( pObj->IsGroupObject() )
                   {
                       SdrObject* pHit = NULL;
                       if ( pView->PickObj(aMDPos, pView->getHitTolLog(), pHit, pPV, SDRSEARCH_DEEP ) )
                           pObj = pHit;
                   }

                   ScMacroInfo* pInfo = ScDrawLayer::GetMacroInfo( pObj, TRUE );
#ifdef ISSUE66550_HLINK_FOR_SHAPES
                   // For interoperability favour links over macros if both are defined
                   if ( pInfo->GetHlink().getLength() > 0 )
                   {
                       OSL_TRACE("** Got URL");
                       sURL = pInfo->GetHlink();
                   }
                   else if ( pInfo->GetMacro().getLength() > 0 )
#else
                   if ( pInfo->GetMacro().getLength() > 0 )
#endif
                   {
                       SfxObjectShell* pObjSh = SfxObjectShell::Current();
                       if ( pObjSh && SfxApplication::IsXScriptURL( pInfo->GetMacro() ) )
                       {
                           uno::Any aRet;
                           uno::Sequence< sal_Int16 > aOutArgsIndex;
                           uno::Sequence< uno::Any > aOutArgs;
                           uno::Sequence< uno::Any >* pInArgs =
                               new uno::Sequence< uno::Any >(0);
                           pObjSh->CallXScript( pInfo->GetMacro(),
                               *pInArgs, aRet, aOutArgsIndex, aOutArgs);
                           pViewShell->FakeButtonUp( pViewShell->GetViewData()->GetActivePart() );
                           return TRUE;        // kein CaptureMouse etc.
                       }
                   }
                }

				//	URL / ImageMap

				SdrViewEvent aVEvt;
				if ( !bAlt &&
					pView->PickAnything( rMEvt, SDRMOUSEBUTTONDOWN, aVEvt ) != SDRHIT_NONE &&
					aVEvt.pObj != NULL )
				{
					if ( ScDrawLayer::GetIMapInfo( aVEvt.pObj ) )		// ImageMap
					{
						const IMapObject* pIMapObj =
								ScDrawLayer::GetHitIMapObject( aVEvt.pObj, aMDPos, *pWindow );
						if ( pIMapObj && pIMapObj->GetURL().Len() )
						{
                            sURL = pIMapObj->GetURL();
                            sTarget = pIMapObj->GetTarget();
						}
					}
					if ( aVEvt.eEvent == SDREVENT_EXECUTEURL && aVEvt.pURLField )	// URL
                    {
                        sURL = aVEvt.pURLField->GetURL();
                        sTarget = aVEvt.pURLField->GetTargetFrame();
                    }
				}

                // open hyperlink, if found at object or in object's text
                if ( sURL.Len() > 0 )
                {
                    ScGlobal::OpenURL( sURL, sTarget );
                    pViewShell->FakeButtonUp( pViewShell->GetViewData()->GetActivePart() );
                    return TRUE;        // kein CaptureMouse etc.
                }

				//	Is another object being edited in this view?
				//	(Editing is ended in MarkListHasChanged - test before UnmarkAll)
				SfxInPlaceClient* pClient = pViewShell->GetIPClient();
                BOOL bWasOleActive = ( pClient && pClient->IsObjectInPlaceActive() );

				//	Markieren

                // do not allow multiselection with note caption
				bool bCaptionClicked = IsNoteCaptionClicked( aMDPos );
				if ( !rMEvt.IsShift() || bCaptionClicked || IsNoteCaptionMarked() )
					pView->UnmarkAll();

                /*  Unlock internal layer, if a note caption is clicked. The
                    layer will be relocked in ScDrawView::MarkListHasChanged(). */
				if( bCaptionClicked )
                    pView->UnlockInternalLayer();

                // try to select the clicked object
				if ( pView->MarkObj( aMDPos, -2, FALSE, rMEvt.IsMod1() ) )
				{
					//*********************************************************
					//Objekt verschieben
					//********************************************************
					if (pView->IsMarkedHit(aMDPos))
					{
						//	#95834# Don't start drag timer if inplace editing of an OLE object
						//	was just ended with this mouse click - the view will be moved
						//	(different tool bars) and the object that was clicked on would
						//	be moved unintentionally.
						if ( !bWasOleActive )
							aDragTimer.Start();

						pHdl=pView->PickHandle(aMDPos);
						pView->BegDragObj(aMDPos, (OutputDevice*) NULL, pHdl);
						bReturn = TRUE;
					}
					else									// Objekt am Rand getroffen
						if (pViewShell->IsDrawSelMode())
							bReturn = TRUE;
				}
				else
				{
					//		nichts getroffen

					if (pViewShell->IsDrawSelMode())
					{
						//*********************************************************
						//Objekt selektieren
						//********************************************************
						pView->BegMarkObj(aMDPos);
						bReturn = TRUE;
					}
				}
			}
		}

	}

	if (!bIsInDragMode)
	{
		if (!bVCAction)					// VC rufen selber CaptureMouse
			pWindow->CaptureMouse();
		ForcePointer(&rMEvt);
	}

	return bReturn;
}

/*************************************************************************
|*
|* MouseMove-event
|*
\************************************************************************/

BOOL __EXPORT FuSelection::MouseMove(const MouseEvent& rMEvt)
{
	BOOL bReturn = FuDraw::MouseMove(rMEvt);

	if (aDragTimer.IsActive() )
	{
		Point aOldPixel = pWindow->LogicToPixel( aMDPos );
		Point aNewPixel = rMEvt.GetPosPixel();
		if ( Abs( aOldPixel.X() - aNewPixel.X() ) > SC_MAXDRAGMOVE ||
			 Abs( aOldPixel.Y() - aNewPixel.Y() ) > SC_MAXDRAGMOVE )
			aDragTimer.Stop();
	}

	if ( pView->IsAction() )
	{
		Point aPix(rMEvt.GetPosPixel());
		Point aPnt(pWindow->PixelToLogic(aPix));

		ForceScroll(aPix);
		pView->MovAction(aPnt);
		bReturn = TRUE;
	}

	// Ein VCControl ist aktiv
	// Event an den Manager weiterleiten
	if( bVCAction )
	{
		//	GetSbxForm gibts nicht mehr - Basic-Controls sind tot
		//SdrPageView* pPgView = pView->GetPageViewByIndex(0);
		//ScDrawPage*  pPage	 = (ScDrawPage*)pPgView->GetPage();
		//VCSbxForm* pForm = (VCSbxForm*)(SbxObject*)(pPage->GetSbxForm());
		//((VCManager*)(pForm->GetVCContainer()))->
		//    MouseMove( pWindow, rMEvt );
		bReturn = TRUE;
	}

	ForcePointer(&rMEvt);

	return (bReturn);
}

/*************************************************************************
|*
|* MouseButtonUp-event
|*
\************************************************************************/

BOOL __EXPORT FuSelection::MouseButtonUp(const MouseEvent& rMEvt)
{
	// #95491# remember button state for creation of own MouseEvents
	SetMouseButtonCode(rMEvt.GetButtons());

	BOOL bReturn = FuDraw::MouseButtonUp(rMEvt);
//	BOOL bOle	 = pViewShell->GetViewData()->IsOle();
    BOOL bOle = pViewShell->GetViewFrame()->GetFrame()->IsInPlace();

	if (aDragTimer.IsActive() )
	{
		aDragTimer.Stop();
	}

	Point aPnt( pWindow->PixelToLogic( rMEvt.GetPosPixel() ) );

	if ( rMEvt.IsLeft() )
	{
		if ( pView->IsDragObj() )
		{
			/******************************************************************
			* Objekt wurde verschoben
			******************************************************************/
			pView->EndDragObj( rMEvt.IsMod1() );
			pView->ForceMarkedToAnotherPage();

			const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
			if (rMarkList.GetMarkCount() == 1)
			{
		  	    SdrMark* pMark = rMarkList.GetMark(0);
		  	    SdrObject* pObj = pMark->GetMarkedSdrObj();
		  	    FuPoor* pPoor = pViewShell->GetViewData()->GetView()->GetDrawFuncPtr();
		  	    FuText* pText = static_cast<FuText*>(pPoor);
			    pText->StopDragMode(pObj );
			}
			bReturn = TRUE;
		}
		else if (pView->IsAction() )
		{
            // unlock internal layer to include note captions
            pView->UnlockInternalLayer();
			pView->EndAction();
			if ( pView->AreObjectsMarked() )
            {
				bReturn = TRUE;

                /*  if multi-selection contains a note caption object, remove
                    all other objects from selection. */
                const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
                ULONG nCount = rMarkList.GetMarkCount();
                if( nCount > 1 )
                {
                    bool bFound = false;
                    for( ULONG nIdx = 0; !bFound && (nIdx < nCount); ++nIdx )
                    {
                        SdrObject* pObj = rMarkList.GetMark( nIdx )->GetMarkedSdrObj();
                        bFound = ScDrawLayer::IsNoteCaption( pObj );
                        if( bFound )
                        {
                            pView->UnMarkAll();
                            pView->MarkObj( pObj, pView->GetSdrPageView() );
                        }
                    }
                }
            }
		}
	}

/*
	if ( pView->IsObjEdit() )
	{
		BOOL bShowCursor = TRUE;
//! 	pOutlinerView = pView->GetOutlinerView(pWindow, bShowCursor);
		bReturn = TRUE;
	}
*/
	/**************************************************************************
	* Ggf. OLE-Objekt beruecksichtigen
	**************************************************************************/
	SfxInPlaceClient* pIPClient = pViewShell->GetIPClient();

	if (pIPClient)
	{
        ScModule* pScMod = SC_MOD();
        bool bUnoRefDialog = pScMod->IsRefDialogOpen() && pScMod->GetCurRefDlgId() == WID_SIMPLE_REF;

        if ( pIPClient->IsObjectInPlaceActive() && !bUnoRefDialog )
            pIPClient->DeactivateObject();
	}

	USHORT nClicks = rMEvt.GetClicks();
	if ( nClicks == 2 && rMEvt.IsLeft() )
	{
		if ( pView->AreObjectsMarked() )
		{
			const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
			if (rMarkList.GetMarkCount() == 1)
			{
				SdrMark* pMark = rMarkList.GetMark(0);
				SdrObject* pObj = pMark->GetMarkedSdrObj();

				//	#43984# aktivieren nur, wenn die Maus auch (noch) ueber dem
				//	selektierten Objekt steht

				SdrViewEvent aVEvt;
				SdrHitKind eHit = pView->PickAnything( rMEvt, SDRMOUSEBUTTONDOWN, aVEvt );
				if ( eHit != SDRHIT_NONE && aVEvt.pObj == pObj )
				{
					UINT16 nSdrObjKind = pObj->GetObjIdentifier();

					//
					//	OLE: aktivieren
					//

					if (nSdrObjKind == OBJ_OLE2)
					{
						if (!bOle)
						{
                            if (((SdrOle2Obj*) pObj)->GetObjRef().is())
							{
								//HMHpView->HideMarkHdl();
								pViewShell->ActivateObject( (SdrOle2Obj*) pObj, 0 );
							}
						}
					}

					//
                    //  Edit text
                    //  #49458# not in UNO controls
                    //  #i32352# not in media objects
					//
                    else if ( pObj->ISA(SdrTextObj) && !pObj->ISA(SdrUnoObj) && !pObj->ISA(SdrMediaObj) )
					{
						OutlinerParaObject* pOPO = pObj->GetOutlinerParaObject();
						BOOL bVertical = ( pOPO && pOPO->IsVertical() );
						USHORT nTextSlotId = bVertical ? SID_DRAW_TEXT_VERTICAL : SID_DRAW_TEXT;

						pViewShell->GetViewData()->GetDispatcher().
							Execute(nTextSlotId, SFX_CALLMODE_SYNCHRON | SFX_CALLMODE_RECORD);

						// jetzt den erzeugten FuText holen und in den EditModus setzen
						FuPoor* pPoor = pViewShell->GetViewData()->GetView()->GetDrawFuncPtr();
						if ( pPoor && pPoor->GetSlotID() == nTextSlotId )	 //	hat keine RTTI
						{
							FuText* pText = (FuText*)pPoor;
							Point aMousePixel = rMEvt.GetPosPixel();
							pText->SetInEditMode( pObj, &aMousePixel );
						}
						bReturn = TRUE;
					}
				}
			}
		}
		else if ( TestDetective( pView->GetSdrPageView(), aPnt ) )
			bReturn = TRUE;
	}

	// Ein VCControl ist aktiv
	// Event an den Manager weiterleiten
	if( bVCAction )
	{
		//	GetSbxForm gibts nicht mehr - Basic-Controls sind tot
		//SdrPageView* pPgView = pView->GetPageViewByIndex(0);
		//ScDrawPage*  pPage	 = (ScDrawPage*)pPgView->GetPage();
		//VCSbxForm* pForm = (VCSbxForm*)(SbxObject*)(pPage->GetSbxForm());
		//((VCManager*)(pForm->GetVCContainer()))->
		//    MouseButtonUp( pWindow, rMEvt );
		//HMHpView->ShowMarkHdl();
		bVCAction = FALSE;
		bReturn = TRUE;
	}

	ForcePointer(&rMEvt);

	pWindow->ReleaseMouse();

	//	Command-Handler fuer Kontext-Menue kommt erst nach MouseButtonUp,
	//	darum hier die harte IsLeft-Abfrage
	if ( !bReturn && rMEvt.IsLeft() )
		if (pViewShell->IsDrawSelMode())
			pViewShell->GetViewData()->GetDispatcher().
				Execute(SID_OBJECT_SELECT, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD);

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

BOOL FuSelection::KeyInput(const KeyEvent& rKEvt)
{
	BOOL bReturn = FALSE;

	if (!bReturn)
	{
		bReturn = FuDraw::KeyInput(rKEvt);
	}

	return(bReturn);
}


/*************************************************************************
|*
|* Function aktivieren
|*
\************************************************************************/

void FuSelection::Activate()
{
/*
	SdrDragMode eMode;
	switch (aSfxRequest.GetSlot() )
	{
		case SID_OBJECT_SELECT:
			eMode = SDRDRAG_MOVE;
			break;
		case SID_OBJECT_ROTATE:
			eMode = SDRDRAG_ROTATE;
			break;
		case SID_OBJECT_MIRROR:
			eMode = SDRDRAG_MIRROR;
			break;
	}
	pView->SetDragMode(eMode);
*/
	FuDraw::Activate();
}



/*************************************************************************
|*
|* Function deaktivieren
|*
\************************************************************************/

void FuSelection::Deactivate()
{
	/**************************************************************************
	* Hide Cursor
	**************************************************************************/
//    BOOL bShowCursor = FALSE;
//! pOutlinerView = pView->GetOutlinerView(pWindow, bShowCursor);

//	pView->SetDragMode(SDRDRAG_MOVE);
	FuDraw::Deactivate();
}


#ifdef _MSC_VER
#pragma optimize ( "", on )
#endif





