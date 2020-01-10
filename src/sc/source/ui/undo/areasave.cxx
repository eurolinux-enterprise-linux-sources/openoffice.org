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

// -----------------------------------------------------------------------



// INCLUDE ---------------------------------------------------------------

#include <svx/linkmgr.hxx>
#include <tools/debug.hxx>

#include "areasave.hxx"
#include "arealink.hxx"
#include "document.hxx"

// -----------------------------------------------------------------------

ScAreaLinkSaver::ScAreaLinkSaver( const ScAreaLink& rSource ) :
	aFileName	( rSource.GetFile() ),
	aFilterName	( rSource.GetFilter() ),
	aOptions	( rSource.GetOptions() ),
	aSourceArea	( rSource.GetSource() ),
	aDestArea	( rSource.GetDestArea() ),
    nRefresh    ( rSource.GetRefreshDelay() )       // seconds
{
}

ScAreaLinkSaver::ScAreaLinkSaver( const ScAreaLinkSaver& rCopy ) :
    ScDataObject(),
	aFileName	( rCopy.aFileName ),
	aFilterName	( rCopy.aFilterName ),
	aOptions	( rCopy.aOptions ),
	aSourceArea	( rCopy.aSourceArea ),
	aDestArea	( rCopy.aDestArea ),
    nRefresh    ( rCopy.nRefresh )
{
}

ScAreaLinkSaver::~ScAreaLinkSaver()
{
}

ScDataObject*	ScAreaLinkSaver::Clone() const
{
	return new ScAreaLinkSaver( *this );
}

BOOL ScAreaLinkSaver::IsEqualSource( const ScAreaLink& rCompare ) const
{
	return ( aFileName	 == rCompare.GetFile() &&
			 aFilterName == rCompare.GetFilter() &&
			 aOptions	 == rCompare.GetOptions() &&
			 aSourceArea == rCompare.GetSource() &&
             nRefresh    == rCompare.GetRefreshDelay() );
}

BOOL ScAreaLinkSaver::IsEqual( const ScAreaLink& rCompare ) const
{
	return ( IsEqualSource( rCompare ) &&
			 aDestArea == rCompare.GetDestArea() );
}

void ScAreaLinkSaver::WriteToLink( ScAreaLink& rLink ) const
{
	rLink.SetDestArea( aDestArea );
}

void ScAreaLinkSaver::InsertNewLink( ScDocument* pDoc ) const
{
    // (see ScUndoRemoveAreaLink::Undo)

    SvxLinkManager* pLinkManager = pDoc->GetLinkManager();
    SfxObjectShell* pObjSh = pDoc->GetDocumentShell();

    if ( pLinkManager && pObjSh )
    {
        ScAreaLink* pLink = new ScAreaLink( pObjSh, aFileName, aFilterName, aOptions,
                                            aSourceArea, aDestArea.aStart, nRefresh );
        pLink->SetInCreate( TRUE );
        pLink->SetDestArea( aDestArea );
        pLinkManager->InsertFileLink( *pLink, OBJECT_CLIENT_FILE, aFileName, &aFilterName, &aSourceArea );
        pLink->Update();
        pLink->SetInCreate( FALSE );
    }
}

// -----------------------------------------------------------------------

ScAreaLinkSaveCollection::ScAreaLinkSaveCollection()
{
}

ScAreaLinkSaveCollection::ScAreaLinkSaveCollection( const ScAreaLinkSaveCollection& rCopy ) :
	ScCollection( rCopy )
{
}

ScAreaLinkSaveCollection::~ScAreaLinkSaveCollection()
{
}

ScDataObject*	ScAreaLinkSaveCollection::Clone() const
{
	return new ScAreaLinkSaveCollection( *this );
}

BOOL ScAreaLinkSaveCollection::IsEqual( const ScDocument* pDoc ) const
{
    // IsEqual can be checked in sequence.
    // Neither ref-update nor removing links will change the order.

	SvxLinkManager* pLinkManager = const_cast<ScDocument*>(pDoc)->GetLinkManager();
	if (pLinkManager)
	{
		USHORT nPos = 0;
        const ::sfx2::SvBaseLinks& rLinks = pLinkManager->GetLinks();
		USHORT nLinkCount = rLinks.Count();
		for (USHORT i=0; i<nLinkCount; i++)
		{
            ::sfx2::SvBaseLink* pBase = *rLinks[i];
			if (pBase->ISA(ScAreaLink))
			{
				if ( nPos >= GetCount() || !(*this)[nPos]->IsEqual( *(ScAreaLink*)pBase ) )
					return FALSE;

				++nPos;
			}
		}
        if ( nPos < GetCount() )
            return FALSE;           // fewer links in the document than in the save collection
	}

	return TRUE;
}

ScAreaLink* lcl_FindLink( const ::sfx2::SvBaseLinks& rLinks, const ScAreaLinkSaver& rSaver )
{
    USHORT nLinkCount = rLinks.Count();
    for (USHORT i=0; i<nLinkCount; i++)
    {
        ::sfx2::SvBaseLink* pBase = *rLinks[i];
    	if ( pBase->ISA(ScAreaLink) &&
    	     rSaver.IsEqualSource( *static_cast<ScAreaLink*>(pBase) ) )
    	{
    	    return static_cast<ScAreaLink*>(pBase);     // found
    	}
    }
    return NULL;    // not found
}

void ScAreaLinkSaveCollection::Restore( ScDocument* pDoc ) const
{
    // The save collection may contain additional entries that are not in the document.
    // They must be inserted again.
    // Entries from the save collection must be searched via source data, as the order
    // of links changes if deleted entries are re-added to the link manager (always at the end).

    SvxLinkManager* pLinkManager = pDoc->GetLinkManager();
    if (pLinkManager)
    {
        const ::sfx2::SvBaseLinks& rLinks = pLinkManager->GetLinks();
        USHORT nSaveCount = GetCount();
        for (USHORT nPos=0; nPos<nSaveCount; nPos++)
        {
            ScAreaLinkSaver* pSaver = (*this)[nPos];
            ScAreaLink* pLink = lcl_FindLink( rLinks, *pSaver );
            if ( pLink )
                pSaver->WriteToLink( *pLink );          // restore output position
            else
                pSaver->InsertNewLink( pDoc );          // re-insert deleted link
        }
    }
}

// static
ScAreaLinkSaveCollection* ScAreaLinkSaveCollection::CreateFromDoc( const ScDocument* pDoc )
{
	ScAreaLinkSaveCollection* pColl = NULL;

	SvxLinkManager* pLinkManager = const_cast<ScDocument*>(pDoc)->GetLinkManager();
	if (pLinkManager)
	{
        const ::sfx2::SvBaseLinks& rLinks = pLinkManager->GetLinks();
		USHORT nLinkCount = rLinks.Count();
		for (USHORT i=0; i<nLinkCount; i++)
		{
            ::sfx2::SvBaseLink* pBase = *rLinks[i];
			if (pBase->ISA(ScAreaLink))
			{
				if (!pColl)
					pColl = new ScAreaLinkSaveCollection;

				ScAreaLinkSaver* pSaver = new ScAreaLinkSaver( *(ScAreaLink*)pBase );
				if (!pColl->Insert(pSaver))
					delete pSaver;
			}
		}
	}

	return pColl;
}

