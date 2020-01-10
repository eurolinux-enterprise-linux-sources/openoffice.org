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


#include "drawdoc.hxx"
#include "unoaprms.hxx"
#include "anminfo.hxx"


TYPEINIT1(SdAnimationPrmsUndoAction, SdUndoAction);


/*************************************************************************
|*
|* 2. Ctor, der den ersten (inline) nach der Version 4.0 einmal ersetzen
|* soll (mit 3. Parameter dann)
|* Hier werden die Member mit den Animations-Informationen vorbelegt,
|* um nicht immer alle inline-Methoden aufrufen zu muessen, auch im
|* Hinblick auf zukuenftige Erweiterungen (neue Member etc.)
|*
\************************************************************************/

SdAnimationPrmsUndoAction::SdAnimationPrmsUndoAction(
								SdDrawDocument* pTheDoc,
								SdrObject* pObj ) :
	SdUndoAction	( pTheDoc ),
	pObject 		( pObj ),
	bInfoCreated    ( FALSE ) // Fuer Animationsreihenfolge existiert Info
{
	SdAnimationInfo* pInfo = pTheDoc->GetAnimationInfo( pObject );
	if( pInfo )
	{
		bNewActive		= bOldActive	 = pInfo->mbActive;
		eNewEffect		= eOldEffect	 = pInfo->meEffect;
		eNewTextEffect	= eOldTextEffect = pInfo->meTextEffect;
		eNewSpeed		= eOldSpeed		 = pInfo->meSpeed;
		bNewDimPrevious = bOldDimPrevious= pInfo->mbDimPrevious;
		aNewDimColor	= aOldDimColor	 = pInfo->maDimColor;
		bNewDimHide		= bOldDimHide	 = pInfo->mbDimHide;
		bNewSoundOn		= bOldSoundOn	 = pInfo->mbSoundOn;
		aNewSoundFile	= aOldSoundFile	 = pInfo->maSoundFile;
		bNewPlayFull	= bOldPlayFull 	 = pInfo->mbPlayFull;

		pNewPathObj 	= pOldPathObj	 = pInfo->mpPathObj;

		eNewClickAction		= eOldClickAction	 = pInfo->meClickAction;
		aNewBookmark		= aOldBookmark 		 = pInfo->GetBookmark();
//		bNewInvisibleInPres	= bOldInvisibleInPres= pInfo->mbInvisibleInPresentation;
		nNewVerb			= nOldVerb			 = pInfo->mnVerb;
		nNewPresOrder		= nOldPresOrder		 = pInfo->mnPresOrder;

		eNewSecondEffect	= eOldSecondEffect	 = pInfo->meSecondEffect;
		eNewSecondSpeed		= eOldSecondSpeed	 = pInfo->meSecondSpeed;
		bNewSecondSoundOn	= bOldSecondSoundOn	 = pInfo->mbSecondSoundOn;
		bNewSecondPlayFull	= bOldSecondPlayFull = pInfo->mbSecondPlayFull;
	}
}

/*************************************************************************
|*
|* Undo()
|*
\************************************************************************/

void SdAnimationPrmsUndoAction::Undo()
{
	// keine neu Info erzeugt: Daten restaurieren
	if (!bInfoCreated)
	{
		SdDrawDocument*	pDoc   = (SdDrawDocument*)pObject->GetModel();
		if( pDoc )
		{
			SdAnimationInfo* pInfo = pDoc->GetAnimationInfo( pObject );
			// So nicht...
			//SdAnimationInfo* pInfo = (SdAnimationInfo*)pObject->GetUserData(0);
			pInfo->mbActive		= bOldActive;
			pInfo->meEffect      = eOldEffect;
			pInfo->meTextEffect  = eOldTextEffect;
			pInfo->meSpeed		= eOldSpeed;
			pInfo->mbDimPrevious = bOldDimPrevious;
			pInfo->maDimColor    = aOldDimColor;
			pInfo->mbDimHide     = bOldDimHide;
			pInfo->mbSoundOn     = bOldSoundOn;
			pInfo->maSoundFile   = aOldSoundFile;
			pInfo->mbPlayFull    = bOldPlayFull;
//			pInfo->mSetPath(pOldPathObj);
			pInfo->meClickAction = eOldClickAction;
			pInfo->SetBookmark( aOldBookmark );
//			pInfo->mbInvisibleInPresentation = bOldInvisibleInPres;
			pInfo->mnVerb        = nOldVerb;
			pInfo->mnPresOrder   = nOldPresOrder;

			pInfo->meSecondEffect    = eOldSecondEffect;
			pInfo->meSecondSpeed     = eOldSecondSpeed;
			pInfo->mbSecondSoundOn   = bOldSecondSoundOn;
			pInfo->mbSecondPlayFull  = bOldSecondPlayFull;
		}
	}
	// Info wurde durch Aktion erzeugt: Info loeschen
	else
	{
		pObject->DeleteUserData(0);
	}
	// Damit ein ModelHasChanged() ausgeloest wird, um das Effekte-Window
	// auf Stand zu bringen (Animations-Reihenfolge)
	pObject->SetChanged();
	pObject->BroadcastObjectChange();
}

/*************************************************************************
|*
|* Redo()
|*
\************************************************************************/

void SdAnimationPrmsUndoAction::Redo()
{
	SdAnimationInfo* pInfo = NULL;

	pInfo = SdDrawDocument::GetShapeUserData(*pObject,true);

	pInfo->mbActive      = bNewActive;
	pInfo->meEffect      = eNewEffect;
	pInfo->meTextEffect  = eNewTextEffect;
	pInfo->meSpeed       = eNewSpeed;
	pInfo->mbDimPrevious = bNewDimPrevious;
	pInfo->maDimColor    = aNewDimColor;
	pInfo->mbDimHide     = bNewDimHide;
	pInfo->mbSoundOn     = bNewSoundOn;
	pInfo->maSoundFile   = aNewSoundFile;
	pInfo->mbPlayFull    = bNewPlayFull;
//	pInfo->mSetPath(pNewPathObj);
	pInfo->meClickAction = eNewClickAction;
	pInfo->SetBookmark( aNewBookmark );
//	pInfo->mbInvisibleInPresentation = bNewInvisibleInPres;
	pInfo->mnVerb        = nNewVerb;
	pInfo->mnPresOrder   = nNewPresOrder;

	pInfo->meSecondEffect    = eNewSecondEffect;
	pInfo->meSecondSpeed     = eNewSecondSpeed;
	pInfo->mbSecondSoundOn   = bNewSecondSoundOn;
	pInfo->mbSecondPlayFull  = bNewSecondPlayFull;

	// Damit ein ModelHasChanged() ausgeloest wird, um das Effekte-Window
	// auf Stand zu bringen (Animations-Reihenfolge)
	pObject->SetChanged();
	pObject->BroadcastObjectChange();
}

/*************************************************************************
|*
|* Destruktor
|*
\************************************************************************/

SdAnimationPrmsUndoAction::~SdAnimationPrmsUndoAction()
{
}


