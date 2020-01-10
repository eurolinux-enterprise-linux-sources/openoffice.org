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

#ifndef _SVXLINK_HXX
#include <svx/linkmgr.hxx>
#endif

#include "pglink.hxx"
#include "sdpage.hxx"
#include "drawdoc.hxx"


/*************************************************************************
|*
|*		Ctor
|*
\************************************************************************/

SdPageLink::SdPageLink(SdPage* pPg, const String& rFileName,
					   const String& rBookmarkName) :
    ::sfx2::SvBaseLink( ::sfx2::LINKUPDATE_ONCALL, FORMAT_FILE),
	pPage(pPg)
{
	pPage->SetFileName(rFileName);
	pPage->SetBookmarkName(rBookmarkName);
}


/*************************************************************************
|*
|* Dtor
|*
\************************************************************************/


SdPageLink::~SdPageLink()
{
}

/*************************************************************************
|*
|* Daten haben sich geaendert
|*
\************************************************************************/

void SdPageLink::DataChanged( const String& ,
									   const ::com::sun::star::uno::Any& )
{
	SdDrawDocument* pDoc = (SdDrawDocument*) pPage->GetModel();
	SvxLinkManager* pLinkManager = pDoc!=NULL ? pDoc->GetLinkManager() : NULL;

	if (pLinkManager)
	{
		/**********************************************************************
		* Nur Standardseiten duerfen gelinkt sein
		* Die entsprechenden Notizseiten werden automatisch aktualisiert
		**********************************************************************/
		String aFileName;
		String aBookmarkName;
		String aFilterName;
		pLinkManager->GetDisplayNames( this,0, &aFileName, &aBookmarkName,
									  &aFilterName);
		pPage->SetFileName(aFileName);
		pPage->SetBookmarkName(aBookmarkName);

		SdDrawDocument* pBookmarkDoc = pDoc->OpenBookmarkDoc(aFileName);

		if (pBookmarkDoc)
		{
			/******************************************************************
			* Die gelinkte Seite wird im Model replaced
			******************************************************************/
			if (aBookmarkName.Len() == 0)
			{
				// Kein Seitenname angegeben: es wird die erste Seite genommen
				aBookmarkName = pBookmarkDoc->GetSdPage(0, PK_STANDARD)->GetName();
				pPage->SetBookmarkName(aBookmarkName);
			}

			List aBookmarkList;
			aBookmarkList.Insert(&aBookmarkName);
			USHORT nInsertPos = pPage->GetPageNum();
			BOOL bLink = TRUE;
			BOOL bReplace = TRUE;
			BOOL bNoDialogs = FALSE;
			BOOL bCopy = FALSE;

			if( pDoc->pDocLockedInsertingLinks )
			{
				// resolving links while loading pDoc
				bNoDialogs = TRUE;
				bCopy = TRUE;
			}

			pDoc->InsertBookmarkAsPage(&aBookmarkList, NULL, bLink, bReplace,
									   nInsertPos, bNoDialogs, NULL, bCopy, TRUE, TRUE);

			if( !pDoc->pDocLockedInsertingLinks )
				pDoc->CloseBookmarkDoc();
		}
	}
}

/*************************************************************************
|*
|* Link an oder abmelden
|*
\************************************************************************/

void SdPageLink::Closed()
{
	// Die Verbindung wird aufgehoben
	pPage->SetFileName(String());
	pPage->SetBookmarkName(String());

	SvBaseLink::Closed();
}



