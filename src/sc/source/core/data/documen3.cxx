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

#include "scitems.hxx"
#include <svx/langitem.hxx>
#include <svx/srchitem.hxx>
#include <svx/linkmgr.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/objsh.hxx>
#include <svtools/zforlist.hxx>
#include <svtools/PasswordHelper.hxx>
#include <vcl/svapp.hxx>
#include "document.hxx"
#include "attrib.hxx"
#include "cell.hxx"
#include "table.hxx"
#include "rangenam.hxx"
#include "dbcolect.hxx"
#include "pivot.hxx"
#include "docpool.hxx"
#include "poolhelp.hxx"
#include "autoform.hxx"
#include "rangelst.hxx"
#include "chartarr.hxx"
#include "chartlock.hxx"
#include "refupdat.hxx"
#include "docoptio.hxx"
#include "viewopti.hxx"
#include "scextopt.hxx"
#include "brdcst.hxx"
#include "bcaslot.hxx"
#include "tablink.hxx"
#include "externalrefmgr.hxx"
#include "markdata.hxx"
#include "validat.hxx"
#include "dociter.hxx"
#include "detdata.hxx"
#include "detfunc.hxx"
#include "scmod.hxx"   		// SC_MOD
#include "inputopt.hxx" 	// GetExpandRefs
#include "chartlis.hxx"
#include "sc.hrc"			// SID_LINK
#include "hints.hxx"
#include "dpobject.hxx"
#include "unoguard.hxx"
#include "drwlayer.hxx"
#include "unoreflist.hxx"
#include "listenercalls.hxx"
#include "tabprotection.hxx"
#include "formulaparserpool.hxx"
#include "clipparam.hxx"

#include <memory>

using namespace com::sun::star;
using ::std::auto_ptr;

//------------------------------------------------------------------------

ScRangeName* ScDocument::GetRangeName()
{
	return pRangeName;
}

void ScDocument::SetRangeName( ScRangeName* pNewRangeName )
{
	if (pRangeName)
		delete pRangeName;
	pRangeName = pNewRangeName;
}

//UNUSED2008-05  ScRangeData* ScDocument::GetRangeAtCursor(SCCOL nCol, SCROW nRow, SCTAB nTab,
//UNUSED2008-05                                              BOOL bStartOnly) const
//UNUSED2008-05  {
//UNUSED2008-05      if ( pRangeName )
//UNUSED2008-05          return pRangeName->GetRangeAtCursor( ScAddress( nCol, nRow, nTab ), bStartOnly );
//UNUSED2008-05      else
//UNUSED2008-05          return NULL;
//UNUSED2008-05  }

ScRangeData* ScDocument::GetRangeAtBlock( const ScRange& rBlock, String* pName ) const
{
	ScRangeData* pData = NULL;
	if ( pRangeName )
	{
		pData = pRangeName->GetRangeAtBlock( rBlock );
		if (pData && pName)
			*pName = pData->GetName();
	}
	return pData;
}

ScDBCollection* ScDocument::GetDBCollection() const
{
	return pDBCollection;
}

void ScDocument::SetDBCollection( ScDBCollection* pNewDBCollection, BOOL bRemoveAutoFilter )
{
	if ( bRemoveAutoFilter )
	{
		//	remove auto filter attribute if new db data don't contain auto filter flag
		//	start position is also compared, so bRemoveAutoFilter must not be set from ref-undo!

		if ( pDBCollection )
		{
			USHORT nOldCount = pDBCollection->GetCount();
			for (USHORT nOld=0; nOld<nOldCount; nOld++)
			{
				ScDBData* pOldData = (*pDBCollection)[nOld];
				if ( pOldData->HasAutoFilter() )
				{
					ScRange aOldRange;
					pOldData->GetArea( aOldRange );

					BOOL bFound = FALSE;
					USHORT nNewIndex = 0;
					if ( pNewDBCollection &&
						pNewDBCollection->SearchName( pOldData->GetName(), nNewIndex ) )
					{
						ScDBData* pNewData = (*pNewDBCollection)[nNewIndex];
						if ( pNewData->HasAutoFilter() )
						{
							ScRange aNewRange;
							pNewData->GetArea( aNewRange );
							if ( aOldRange.aStart == aNewRange.aStart )
								bFound = TRUE;
						}
					}

					if ( !bFound )
					{
						aOldRange.aEnd.SetRow( aOldRange.aStart.Row() );
						RemoveFlagsTab( aOldRange.aStart.Col(), aOldRange.aStart.Row(),
										aOldRange.aEnd.Col(),   aOldRange.aEnd.Row(),
										aOldRange.aStart.Tab(), SC_MF_AUTO );
						if (pShell)
							pShell->Broadcast( ScPaintHint( aOldRange, PAINT_GRID ) );
					}
				}
			}
		}
	}

	if (pDBCollection)
		delete pDBCollection;
	pDBCollection = pNewDBCollection;
}

ScDBData* ScDocument::GetDBAtCursor(SCCOL nCol, SCROW nRow, SCTAB nTab, BOOL bStartOnly) const
{
	if (pDBCollection)
		return pDBCollection->GetDBAtCursor(nCol, nRow, nTab, bStartOnly);
	else
		return NULL;
}

ScDBData* ScDocument::GetDBAtArea(SCTAB nTab, SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2) const
{
	if (pDBCollection)
		return pDBCollection->GetDBAtArea(nTab, nCol1, nRow1, nCol2, nRow2);
	else
		return NULL;
}

ScDPCollection* ScDocument::GetDPCollection()
{
	if (!pDPCollection)
		pDPCollection = new ScDPCollection(this);
	return pDPCollection;
}

ScDPObject* ScDocument::GetDPAtCursor(SCCOL nCol, SCROW nRow, SCTAB nTab) const
{
	if (!pDPCollection)
		return NULL;

	USHORT nCount = pDPCollection->GetCount();
	ScAddress aPos( nCol, nRow, nTab );
	for (USHORT i=0; i<nCount; i++)
		if ( (*pDPCollection)[i]->GetOutRange().In( aPos ) )
			return (*pDPCollection)[i];

	return NULL;
}

ScDPObject* ScDocument::GetDPAtBlock( const ScRange & rBlock ) const
{
    if (!pDPCollection)
        return NULL;

    /* Walk the collection in reverse order to get something of an
     * approximation of MS Excels 'most recent' effect. */
    USHORT i = pDPCollection->GetCount();
    while ( i-- > 0 )
        if ( (*pDPCollection)[i]->GetOutRange().In( rBlock ) )
            return (*pDPCollection)[i];

    return NULL;
}

ScChartCollection* ScDocument::GetChartCollection() const
{
	return pChartCollection;
}

void ScDocument::StopTemporaryChartLock()
{
    if( apTemporaryChartLock.get() )
        apTemporaryChartLock->StopLocking();
}

void ScDocument::SetChartListenerCollection(
			ScChartListenerCollection* pNewChartListenerCollection,
			BOOL bSetChartRangeLists )
{
	ScChartListenerCollection* pOld = pChartListenerCollection;
	pChartListenerCollection = pNewChartListenerCollection;
	if ( pChartListenerCollection )
	{
		if ( pOld )
			pChartListenerCollection->SetDiffDirty( *pOld, bSetChartRangeLists );
		pChartListenerCollection->StartAllListeners();
	}
	delete pOld;
}

void ScDocument::SetScenario( SCTAB nTab, BOOL bFlag )
{
	if (ValidTab(nTab) && pTab[nTab])
		pTab[nTab]->SetScenario(bFlag);
}

BOOL ScDocument::IsScenario( SCTAB nTab ) const
{
    return ValidTab(nTab) && pTab[nTab] &&pTab[nTab]->IsScenario();
	//if (ValidTab(nTab) && pTab[nTab])
	//	return pTab[nTab]->IsScenario();

	//return FALSE;
}

void ScDocument::SetScenarioData( SCTAB nTab, const String& rComment,
										const Color& rColor, USHORT nFlags )
{
	if (ValidTab(nTab) && pTab[nTab] && pTab[nTab]->IsScenario())
	{
		pTab[nTab]->SetScenarioComment( rComment );
		pTab[nTab]->SetScenarioColor( rColor );
		pTab[nTab]->SetScenarioFlags( nFlags );
	}
}

void ScDocument::GetScenarioData( SCTAB nTab, String& rComment,
										Color& rColor, USHORT& rFlags ) const
{
	if (ValidTab(nTab) && pTab[nTab] && pTab[nTab]->IsScenario())
	{
		pTab[nTab]->GetScenarioComment( rComment );
		rColor = pTab[nTab]->GetScenarioColor();
		rFlags = pTab[nTab]->GetScenarioFlags();
	}
}

void ScDocument::GetScenarioFlags( SCTAB nTab, USHORT& rFlags ) const
{
    if (VALIDTAB(nTab) && pTab[nTab] && pTab[nTab]->IsScenario())
        rFlags = pTab[nTab]->GetScenarioFlags();
}

BOOL ScDocument::IsLinked( SCTAB nTab ) const
{
    return ValidTab(nTab) && pTab[nTab] && pTab[nTab]->IsLinked();
    // euqivalent to
	//if (ValidTab(nTab) && pTab[nTab])
	//	return pTab[nTab]->IsLinked();
	//return FALSE;
}

formula::FormulaGrammar::AddressConvention ScDocument::GetAddressConvention() const
{
    return formula::FormulaGrammar::extractRefConvention(eGrammar);
}

formula::FormulaGrammar::Grammar ScDocument::GetGrammar() const
{
    return eGrammar;
}

void ScDocument::SetGrammar( formula::FormulaGrammar::Grammar eGram )
{
    eGrammar = eGram;
}

BOOL ScDocument::GetLinkMode( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetLinkMode();
	return SC_LINK_NONE;
}

const String& ScDocument::GetLinkDoc( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetLinkDoc();
	return EMPTY_STRING;
}

const String& ScDocument::GetLinkFlt( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetLinkFlt();
	return EMPTY_STRING;
}

const String& ScDocument::GetLinkOpt( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetLinkOpt();
	return EMPTY_STRING;
}

const String& ScDocument::GetLinkTab( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetLinkTab();
	return EMPTY_STRING;
}

ULONG ScDocument::GetLinkRefreshDelay( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetLinkRefreshDelay();
	return 0;
}

void ScDocument::SetLink( SCTAB nTab, BYTE nMode, const String& rDoc,
							const String& rFilter, const String& rOptions,
							const String& rTabName, ULONG nRefreshDelay )
{
	if (ValidTab(nTab) && pTab[nTab])
		pTab[nTab]->SetLink( nMode, rDoc, rFilter, rOptions, rTabName, nRefreshDelay );
}

BOOL ScDocument::HasLink( const String& rDoc,
							const String& rFilter, const String& rOptions ) const
{
	SCTAB nCount = GetTableCount();
	for (SCTAB i=0; i<nCount; i++)
		if (pTab[i]->IsLinked()
				&& pTab[i]->GetLinkDoc() == rDoc
				&& pTab[i]->GetLinkFlt() == rFilter
				&& pTab[i]->GetLinkOpt() == rOptions)
			return TRUE;

	return FALSE;
}

BOOL ScDocument::LinkExternalTab( SCTAB& rTab, const String& aDocTab,
		const String& aFileName, const String& aTabName )
{
	if ( IsClipboard() )
	{
		DBG_ERRORFILE( "LinkExternalTab in Clipboard" );
		return FALSE;
	}
	rTab = 0;
	String	aFilterName;		// wird vom Loader gefuellt
	String	aOptions;		// Filter-Optionen
    sal_uInt32 nLinkCnt = pExtDocOptions ? pExtDocOptions->GetDocSettings().mnLinkCnt : 0;
    ScDocumentLoader aLoader( aFileName, aFilterName, aOptions, nLinkCnt + 1 );
	if ( aLoader.IsError() )
		return FALSE;
	ScDocument* pSrcDoc = aLoader.GetDocument();

	//	Tabelle kopieren
	SCTAB nSrcTab;
	if ( pSrcDoc->GetTable( aTabName, nSrcTab ) )
	{
		if ( !InsertTab( SC_TAB_APPEND, aDocTab, TRUE ) )
		{
			DBG_ERRORFILE("can't insert external document table");
			return FALSE;
		}
		rTab = GetTableCount() - 1;
		// nicht neu einfuegen, nur Ergebnisse
		TransferTab( pSrcDoc, nSrcTab, rTab, FALSE, TRUE );
	}
	else
		return FALSE;

	ULONG nRefreshDelay = 0;

	BOOL bWasThere = HasLink( aFileName, aFilterName, aOptions );
	SetLink( rTab, SC_LINK_VALUE, aFileName, aFilterName, aOptions, aTabName, nRefreshDelay );
	if ( !bWasThere )		// Link pro Quelldokument nur einmal eintragen
	{
		ScTableLink* pLink = new ScTableLink( pShell, aFileName, aFilterName, aOptions, nRefreshDelay );
		pLink->SetInCreate( TRUE );
		GetLinkManager()->InsertFileLink( *pLink, OBJECT_CLIENT_FILE, aFileName,
										&aFilterName );
		pLink->Update();
		pLink->SetInCreate( FALSE );
		SfxBindings* pBindings = GetViewBindings();
		if (pBindings)
			pBindings->Invalidate( SID_LINKS );
	}
	return TRUE;
}

ScExternalRefManager* ScDocument::GetExternalRefManager() const
{
    ScDocument* pThis = const_cast<ScDocument*>(this);
    if (!pExternalRefMgr.get())
        pThis->pExternalRefMgr.reset( new ScExternalRefManager( pThis));

    return pExternalRefMgr.get();
}

bool ScDocument::IsInExternalReferenceMarking() const
{
    return pExternalRefMgr.get() && pExternalRefMgr->isInReferenceMarking();
}

void ScDocument::MarkUsedExternalReferences()
{
    if (!pExternalRefMgr.get())
        return;
    if (!pExternalRefMgr->hasExternalData())
        return;
    // Charts.
    bool bAllMarked = pExternalRefMgr->markUsedByLinkListeners();
    // Formula cells.
    for (SCTAB nTab = 0; !bAllMarked && nTab < nMaxTableNumber; ++nTab)
    {
        if (pTab[nTab])
            bAllMarked = pTab[nTab]->MarkUsedExternalReferences();
    }
    /* NOTE: Conditional formats and validation objects are marked when
     * collecting them during export. */
}

ScFormulaParserPool& ScDocument::GetFormulaParserPool() const
{
    if( !mxFormulaParserPool.get() )
        mxFormulaParserPool.reset( new ScFormulaParserPool( *this ) );
    return *mxFormulaParserPool;
}

ScOutlineTable* ScDocument::GetOutlineTable( SCTAB nTab, BOOL bCreate )
{
	ScOutlineTable* pVal = NULL;

	if (VALIDTAB(nTab))
		if (pTab[nTab])
		{
			pVal = pTab[nTab]->GetOutlineTable();
			if (!pVal)
				if (bCreate)
				{
					pTab[nTab]->StartOutlineTable();
					pVal = pTab[nTab]->GetOutlineTable();
				}
		}

	return pVal;
}

BOOL ScDocument::SetOutlineTable( SCTAB nTab, const ScOutlineTable* pNewOutline )
{
    return VALIDTAB(nTab) && pTab[nTab] && pTab[nTab]->SetOutlineTable(pNewOutline);
	//if (VALIDTAB(nTab))
	//	if (pTab[nTab])
	//		return pTab[nTab]->SetOutlineTable(pNewOutline);

	//return FALSE;
}

void ScDocument::DoAutoOutline( SCCOL nStartCol, SCROW nStartRow,
								SCCOL nEndCol, SCROW nEndRow, SCTAB nTab )
{
	if (VALIDTAB(nTab) && pTab[nTab])
	    pTab[nTab]->DoAutoOutline( nStartCol, nStartRow, nEndCol, nEndRow );
}

BOOL ScDocument::TestRemoveSubTotals( SCTAB nTab, const ScSubTotalParam& rParam )
{
    return VALIDTAB(nTab) && pTab[nTab] && pTab[nTab]->TestRemoveSubTotals( rParam );
	//if (VALIDTAB(nTab) && pTab[nTab] )
	//	return pTab[nTab]->TestRemoveSubTotals( rParam );

	//return FALSE;
}

void ScDocument::RemoveSubTotals( SCTAB nTab, ScSubTotalParam& rParam )
{
	if ( VALIDTAB(nTab) && pTab[nTab] )
		pTab[nTab]->RemoveSubTotals( rParam );
}

BOOL ScDocument::DoSubTotals( SCTAB nTab, ScSubTotalParam& rParam )
{
    return VALIDTAB(nTab) && pTab[nTab] && pTab[nTab]->DoSubTotals( rParam );
	//if (VALIDTAB(nTab))
	//	if (pTab[nTab])
	//		return pTab[nTab]->DoSubTotals( rParam );

	//return FALSE;
}

BOOL ScDocument::HasSubTotalCells( const ScRange& rRange )
{
	ScCellIterator aIter( this, rRange );
	ScBaseCell* pCell = aIter.GetFirst();
	while (pCell)
	{
		if ( pCell->GetCellType() == CELLTYPE_FORMULA && ((ScFormulaCell*)pCell)->IsSubTotal() )
			return TRUE;

		pCell = aIter.GetNext();
	}
	return FALSE;	// none found
}

//	kopiert aus diesem Dokument die Zellen von Positionen, an denen in pPosDoc
//	auch Zellen stehen, nach pDestDoc

void ScDocument::CopyUpdated( ScDocument* pPosDoc, ScDocument* pDestDoc )
{
	SCTAB nCount = GetTableCount();
	for (SCTAB nTab=0; nTab<nCount; nTab++)
		if (pTab[nTab] && pPosDoc->pTab[nTab] && pDestDoc->pTab[nTab])
			pTab[nTab]->CopyUpdated( pPosDoc->pTab[nTab], pDestDoc->pTab[nTab] );
}

void ScDocument::CopyScenario( SCTAB nSrcTab, SCTAB nDestTab, BOOL bNewScenario )
{
	if (ValidTab(nSrcTab) && ValidTab(nDestTab) && pTab[nSrcTab] && pTab[nDestTab])
	{
		//	Flags fuer aktive Szenarios richtig setzen
		//	und aktuelle Werte in bisher aktive Szenarios zurueckschreiben

		ScRangeList aRanges = *pTab[nSrcTab]->GetScenarioRanges();
		const ULONG nRangeCount = aRanges.Count();

		//	nDestTab ist die Zieltabelle
		for ( SCTAB nTab = nDestTab+1;
				nTab<=MAXTAB && pTab[nTab] && pTab[nTab]->IsScenario();
				nTab++ )
		{
			if ( pTab[nTab]->IsActiveScenario() )		// auch wenn's dasselbe Szenario ist
			{
				BOOL bTouched = FALSE;
				for ( ULONG nR=0; nR<nRangeCount && !bTouched; nR++)
				{
					const ScRange* pRange = aRanges.GetObject(nR);
					if ( pTab[nTab]->HasScenarioRange( *pRange ) )
						bTouched = TRUE;
				}
				if (bTouched)
				{
					pTab[nTab]->SetActiveScenario(FALSE);
					if ( pTab[nTab]->GetScenarioFlags() & SC_SCENARIO_TWOWAY )
						pTab[nTab]->CopyScenarioFrom( pTab[nDestTab] );
				}
			}
		}

		pTab[nSrcTab]->SetActiveScenario(TRUE);		// da kommt's her...
		if (!bNewScenario)							// Daten aus dem ausgewaehlten Szenario kopieren
		{
			BOOL bOldAutoCalc = GetAutoCalc();
			SetAutoCalc( FALSE );	// Mehrfachberechnungen vermeiden
			pTab[nSrcTab]->CopyScenarioTo( pTab[nDestTab] );
			SetDirty();
			SetAutoCalc( bOldAutoCalc );
		}
	}
}

void ScDocument::MarkScenario( SCTAB nSrcTab, SCTAB nDestTab, ScMarkData& rDestMark,
								BOOL bResetMark, USHORT nNeededBits ) const
{
	if (bResetMark)
		rDestMark.ResetMark();

	if (ValidTab(nSrcTab) && pTab[nSrcTab])
		pTab[nSrcTab]->MarkScenarioIn( rDestMark, nNeededBits );

	rDestMark.SetAreaTab( nDestTab );
}

BOOL ScDocument::HasScenarioRange( SCTAB nTab, const ScRange& rRange ) const
{
    return ValidTab(nTab) && pTab[nTab] && pTab[nTab]->HasScenarioRange( rRange );
	//if (ValidTab(nTab) && pTab[nTab])
	//	return pTab[nTab]->HasScenarioRange( rRange );

	//return FALSE;
}

const ScRangeList* ScDocument::GetScenarioRanges( SCTAB nTab ) const
{
	if (ValidTab(nTab) && pTab[nTab])
		return pTab[nTab]->GetScenarioRanges();

	return NULL;
}

BOOL ScDocument::IsActiveScenario( SCTAB nTab ) const
{
    return ValidTab(nTab) && pTab[nTab] && pTab[nTab]->IsActiveScenario(  );
	//if (ValidTab(nTab) && pTab[nTab])
	//	return pTab[nTab]->IsActiveScenario();

	//return FALSE;
}

void ScDocument::SetActiveScenario( SCTAB nTab, BOOL bActive )
{
	if (ValidTab(nTab) && pTab[nTab])
		pTab[nTab]->SetActiveScenario( bActive );
}

BOOL ScDocument::TestCopyScenario( SCTAB nSrcTab, SCTAB nDestTab ) const
{
	if (ValidTab(nSrcTab) && ValidTab(nDestTab))
		return pTab[nSrcTab]->TestCopyScenarioTo( pTab[nDestTab] );

	DBG_ERROR("falsche Tabelle bei TestCopyScenario");
	return FALSE;
}

void ScDocument::AddUnoObject( SfxListener& rObject )
{
	if (!pUnoBroadcaster)
		pUnoBroadcaster = new SfxBroadcaster;

	rObject.StartListening( *pUnoBroadcaster );
}

void ScDocument::RemoveUnoObject( SfxListener& rObject )
{
	if (pUnoBroadcaster)
	{
		rObject.EndListening( *pUnoBroadcaster );

		if ( bInUnoBroadcast )
		{
			//	#107294# Broadcasts from ScDocument::BroadcastUno are the only way that
			//	uno object methods are called without holding a reference.
			//
			//	If RemoveUnoObject is called from an object dtor in the finalizer thread
			//	while the main thread is calling BroadcastUno, the dtor thread must wait
			//	(or the object's Notify might try to access a deleted object).
			//	The SolarMutex can't be locked here because if a component is called from
			//	a VCL event, the main thread has the SolarMutex locked all the time.
			//
			//	This check is done after calling EndListening, so a later BroadcastUno call
			//	won't touch this object.

			vos::IMutex& rSolarMutex = Application::GetSolarMutex();
			if ( rSolarMutex.tryToAcquire() )
			{
				//	BroadcastUno is always called with the SolarMutex locked, so if it
				//	can be acquired, this is within the same thread (should not happen)
				DBG_ERRORFILE( "RemoveUnoObject called from BroadcastUno" );
				rSolarMutex.release();
			}
			else
			{
				//	let the thread that called BroadcastUno continue
				while ( bInUnoBroadcast )
				{
					vos::OThread::yield();
				}
			}
		}
	}
	else
	{
		DBG_ERROR("No Uno broadcaster");
	}
}

void ScDocument::BroadcastUno( const SfxHint &rHint )
{
	if (pUnoBroadcaster)
	{
		bInUnoBroadcast = TRUE;
		pUnoBroadcaster->Broadcast( rHint );
		bInUnoBroadcast = FALSE;

		// During Broadcast notification, Uno objects can add to pUnoListenerCalls.
		// The listener calls must be processed after completing the broadcast,
		// because they can add or remove objects from pUnoBroadcaster.

		if ( pUnoListenerCalls && rHint.ISA( SfxSimpleHint ) &&
				((const SfxSimpleHint&)rHint).GetId() == SFX_HINT_DATACHANGED &&
				!bInUnoListenerCall )
		{
			// Listener calls may lead to BroadcastUno calls again. The listener calls
			// are not nested, instead the calls are collected in the list, and the
			// outermost call executes them all.

            ScChartLockGuard aChartLockGuard(this);
			bInUnoListenerCall = TRUE;
			pUnoListenerCalls->ExecuteAndClear();
			bInUnoListenerCall = FALSE;
		}
	}
}

void ScDocument::AddUnoListenerCall( const uno::Reference<util::XModifyListener>& rListener,
										const lang::EventObject& rEvent )
{
	DBG_ASSERT( bInUnoBroadcast, "AddUnoListenerCall is supposed to be called from BroadcastUno only" );

	if ( !pUnoListenerCalls )
		pUnoListenerCalls = new ScUnoListenerCalls;
	pUnoListenerCalls->Add( rListener, rEvent );
}

void ScDocument::BeginUnoRefUndo()
{
    DBG_ASSERT( !pUnoRefUndoList, "BeginUnoRefUndo twice" );
    delete pUnoRefUndoList;

    pUnoRefUndoList = new ScUnoRefList;
}

ScUnoRefList* ScDocument::EndUnoRefUndo()
{
    ScUnoRefList* pRet = pUnoRefUndoList;
    pUnoRefUndoList = NULL;
    return pRet;                // must be deleted by caller!
}

void ScDocument::AddUnoRefChange( sal_Int64 nId, const ScRangeList& rOldRanges )
{
    if ( pUnoRefUndoList )
        pUnoRefUndoList->Add( nId, rOldRanges );
}

sal_Int64 ScDocument::GetNewUnoId()
{
    return ++nUnoObjectId;
}

void ScDocument::UpdateReference( UpdateRefMode eUpdateRefMode,
									SCCOL nCol1, SCROW nRow1, SCTAB nTab1,
									SCCOL nCol2, SCROW nRow2, SCTAB nTab2,
									SCsCOL nDx, SCsROW nDy, SCsTAB nDz,
									ScDocument* pUndoDoc, BOOL bIncludeDraw,
                                    bool bUpdateNoteCaptionPos )
{
	PutInOrder( nCol1, nCol2 );
	PutInOrder( nRow1, nRow2 );
	PutInOrder( nTab1, nTab2 );
	if (VALIDTAB(nTab1) && VALIDTAB(nTab2))
	{
		BOOL bExpandRefsOld = IsExpandRefs();
		if ( eUpdateRefMode == URM_INSDEL && (nDx > 0 || nDy > 0 || nDz > 0) )
			SetExpandRefs( SC_MOD()->GetInputOptions().GetExpandRefs() );
		SCTAB i;
		SCTAB iMax;
		if ( eUpdateRefMode == URM_COPY )
		{
			i = nTab1;
			iMax = nTab2;
		}
		else
		{
			ScRange aRange( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2 );
			xColNameRanges->UpdateReference( eUpdateRefMode, this, aRange, nDx, nDy, nDz );
			xRowNameRanges->UpdateReference( eUpdateRefMode, this, aRange, nDx, nDy, nDz );
			pDBCollection->UpdateReference( eUpdateRefMode, nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, nDx, nDy, nDz );
			pRangeName->UpdateReference( eUpdateRefMode, aRange, nDx, nDy, nDz );
			if ( pDPCollection )
				pDPCollection->UpdateReference( eUpdateRefMode, aRange, nDx, nDy, nDz );
			UpdateChartRef( eUpdateRefMode, nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, nDx, nDy, nDz );
			UpdateRefAreaLinks( eUpdateRefMode, aRange, nDx, nDy, nDz );
			if ( pCondFormList )
				pCondFormList->UpdateReference( eUpdateRefMode, aRange, nDx, nDy, nDz );
			if ( pValidationList )
				pValidationList->UpdateReference( eUpdateRefMode, aRange, nDx, nDy, nDz );
			if ( pDetOpList )
				pDetOpList->UpdateReference( this, eUpdateRefMode, aRange, nDx, nDy, nDz );
			if ( pUnoBroadcaster )
				pUnoBroadcaster->Broadcast( ScUpdateRefHint(
									eUpdateRefMode, aRange, nDx, nDy, nDz ) );
			i = 0;
			iMax = MAXTAB;
		}
		for ( ; i<=iMax; i++)
			if (pTab[i])
				pTab[i]->UpdateReference(
					eUpdateRefMode, nCol1, nRow1, nTab1, nCol2, nRow2, nTab2,
					nDx, nDy, nDz, pUndoDoc, bIncludeDraw, bUpdateNoteCaptionPos );

		if ( bIsEmbedded )
		{
            SCCOL theCol1;
            SCROW theRow1;
            SCTAB theTab1;
            SCCOL theCol2;
            SCROW theRow2;
            SCTAB theTab2;
			theCol1 = aEmbedRange.aStart.Col();
			theRow1 = aEmbedRange.aStart.Row();
			theTab1 = aEmbedRange.aStart.Tab();
			theCol2 = aEmbedRange.aEnd.Col();
			theRow2 = aEmbedRange.aEnd.Row();
			theTab2 = aEmbedRange.aEnd.Tab();
			if ( ScRefUpdate::Update( this, eUpdateRefMode, nCol1,nRow1,nTab1, nCol2,nRow2,nTab2,
										nDx,nDy,nDz, theCol1,theRow1,theTab1, theCol2,theRow2,theTab2 ) )
			{
				aEmbedRange = ScRange( theCol1,theRow1,theTab1, theCol2,theRow2,theTab2 );
			}
		}
		SetExpandRefs( bExpandRefsOld );

		// #30428# after moving, no clipboard move ref-updates are possible
		if ( eUpdateRefMode != URM_COPY && IsClipboardSource() )
		{
			ScDocument* pClipDoc = SC_MOD()->GetClipDoc();
			if (pClipDoc)
				pClipDoc->GetClipParam().mbCutMode = false;
		}
	}
}

void ScDocument::UpdateTranspose( const ScAddress& rDestPos, ScDocument* pClipDoc,
										const ScMarkData& rMark, ScDocument* pUndoDoc )
{
	DBG_ASSERT(pClipDoc->bIsClip, "UpdateTranspose: kein Clip");

    ScRange aSource;
    ScClipParam& rClipParam = GetClipParam();
    if (rClipParam.maRanges.Count())
        aSource = *rClipParam.maRanges.First();
	ScAddress aDest = rDestPos;

	SCTAB nClipTab = 0;
	for (SCTAB nDestTab=0; nDestTab<=MAXTAB && pTab[nDestTab]; nDestTab++)
		if (rMark.GetTableSelect(nDestTab))
		{
			while (!pClipDoc->pTab[nClipTab]) nClipTab = (nClipTab+1) % (MAXTAB+1);
			aSource.aStart.SetTab( nClipTab );
			aSource.aEnd.SetTab( nClipTab );
			aDest.SetTab( nDestTab );

			//	wie UpdateReference

			pRangeName->UpdateTranspose( aSource, aDest );		// vor den Zellen!
			for (SCTAB i=0; i<=MAXTAB; i++)
				if (pTab[i])
					pTab[i]->UpdateTranspose( aSource, aDest, pUndoDoc );

			nClipTab = (nClipTab+1) % (MAXTAB+1);
		}
}

void ScDocument::UpdateGrow( const ScRange& rArea, SCCOL nGrowX, SCROW nGrowY )
{
	//!	pDBCollection
	//!	pPivotCollection
	//!	UpdateChartRef

	pRangeName->UpdateGrow( rArea, nGrowX, nGrowY );

	for (SCTAB i=0; i<=MAXTAB && pTab[i]; i++)
		pTab[i]->UpdateGrow( rArea, nGrowX, nGrowY );
}

void ScDocument::Fill(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, const ScMarkData& rMark,
						ULONG nFillCount, FillDir eFillDir, FillCmd eFillCmd, FillDateCmd eFillDateCmd,
						double nStepValue, double nMaxValue)
{
	PutInOrder( nCol1, nCol2 );
	PutInOrder( nRow1, nRow2 );
	for (SCTAB i=0; i <= MAXTAB; i++)
		if (pTab[i])
			if (rMark.GetTableSelect(i))
				pTab[i]->Fill(nCol1, nRow1, nCol2, nRow2,
								nFillCount, eFillDir, eFillCmd, eFillDateCmd,
								nStepValue, nMaxValue);
}

void ScDocument::AutoFormat( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									USHORT nFormatNo, const ScMarkData& rMark )
{
	PutInOrder( nStartCol, nEndCol );
	PutInOrder( nStartRow, nEndRow );
	for (SCTAB i=0; i <= MAXTAB; i++)
		if (pTab[i])
			if (rMark.GetTableSelect(i))
				pTab[i]->AutoFormat( nStartCol, nStartRow, nEndCol, nEndRow, nFormatNo );
}

void ScDocument::GetAutoFormatData(SCTAB nTab, SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									ScAutoFormatData& rData)
{
	if (VALIDTAB(nTab))
	{
		if (pTab[nTab])
		{
			PutInOrder(nStartCol, nEndCol);
			PutInOrder(nStartRow, nEndRow);
			pTab[nTab]->GetAutoFormatData(nStartCol, nStartRow, nEndCol, nEndRow, rData);
		}
	}
}

// static
void ScDocument::GetSearchAndReplaceStart( const SvxSearchItem& rSearchItem,
		SCCOL& rCol, SCROW& rRow )
{
	USHORT nCommand = rSearchItem.GetCommand();
	BOOL bReplace = ( nCommand == SVX_SEARCHCMD_REPLACE ||
		nCommand == SVX_SEARCHCMD_REPLACE_ALL );
	if ( rSearchItem.GetBackward() )
	{
		if ( rSearchItem.GetRowDirection() )
		{
			if ( rSearchItem.GetPattern() )
			{
				rCol = MAXCOL;
				rRow = MAXROW+1;
			}
			else if ( bReplace )
			{
				rCol = MAXCOL;
				rRow = MAXROW;
			}
			else
			{
				rCol = MAXCOL+1;
				rRow = MAXROW;
			}
		}
		else
		{
			if ( rSearchItem.GetPattern() )
			{
				rCol = MAXCOL+1;
				rRow = MAXROW;
			}
			else if ( bReplace )
			{
				rCol = MAXCOL;
				rRow = MAXROW;
			}
			else
			{
				rCol = MAXCOL;
				rRow = MAXROW+1;
			}
		}
	}
	else
	{
		if ( rSearchItem.GetRowDirection() )
		{
			if ( rSearchItem.GetPattern() )
			{
				rCol = 0;
				rRow = (SCROW) -1;
			}
			else if ( bReplace )
			{
				rCol = 0;
				rRow = 0;
			}
			else
			{
				rCol = (SCCOL) -1;
				rRow = 0;
			}
		}
		else
		{
			if ( rSearchItem.GetPattern() )
			{
				rCol = (SCCOL) -1;
				rRow = 0;
			}
			else if ( bReplace )
			{
				rCol = 0;
				rRow = 0;
			}
			else
			{
				rCol = 0;
				rRow = (SCROW) -1;
			}
		}
	}
}

BOOL ScDocument::SearchAndReplace(const SvxSearchItem& rSearchItem,
								SCCOL& rCol, SCROW& rRow, SCTAB& rTab,
								ScMarkData& rMark,
								String& rUndoStr, ScDocument* pUndoDoc)
{
	//!		getrennte Markierungen pro Tabelle verwalten !!!!!!!!!!!!!

	rMark.MarkToMulti();

	BOOL bFound = FALSE;
	if (VALIDTAB(rTab))
	{
		SCCOL nCol;
		SCROW nRow;
		SCTAB nTab;
		USHORT nCommand = rSearchItem.GetCommand();
		if ( nCommand == SVX_SEARCHCMD_FIND_ALL ||
			 nCommand == SVX_SEARCHCMD_REPLACE_ALL )
		{
			for (nTab = 0; nTab <= MAXTAB; nTab++)
				if (pTab[nTab])
				{
					if (rMark.GetTableSelect(nTab))
					{
						nCol = 0;
						nRow = 0;
						bFound |= pTab[nTab]->SearchAndReplace(
									rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc );
					}
				}

			//	Markierung wird innen schon komplett gesetzt
		}
		else
		{
			nCol = rCol;
			nRow = rRow;
			if (rSearchItem.GetBackward())
			{
				for (nTab = rTab; ((SCsTAB)nTab >= 0) && !bFound; nTab--)
					if (pTab[nTab])
					{
						if (rMark.GetTableSelect(nTab))
						{
							bFound = pTab[nTab]->SearchAndReplace(
										rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc );
							if (bFound)
							{
								rCol = nCol;
								rRow = nRow;
								rTab = nTab;
							}
							else
								ScDocument::GetSearchAndReplaceStart(
									rSearchItem, nCol, nRow );
						}
					}
			}
			else
			{
				for (nTab = rTab; (nTab <= MAXTAB) && !bFound; nTab++)
					if (pTab[nTab])
					{
						if (rMark.GetTableSelect(nTab))
						{
							bFound = pTab[nTab]->SearchAndReplace(
										rSearchItem, nCol, nRow, rMark, rUndoStr, pUndoDoc );
							if (bFound)
							{
								rCol = nCol;
								rRow = nRow;
								rTab = nTab;
							}
							else
								ScDocument::GetSearchAndReplaceStart(
									rSearchItem, nCol, nRow );
						}
					}
			}
		}
	}
	return bFound;
}

BOOL ScDocument::IsFiltered( SCROW nRow, SCTAB nTab ) const
{
	if (VALIDTAB(nTab))
		if (pTab[nTab])
			return pTab[nTab]->IsFiltered( nRow );
	DBG_ERROR("Falsche Tabellennummer");
	return 0;
}

//	Outline anpassen

BOOL ScDocument::UpdateOutlineCol( SCCOL nStartCol, SCCOL nEndCol, SCTAB nTab, BOOL bShow )
{
	if ( ValidTab(nTab) && pTab[nTab] )
		return pTab[nTab]->UpdateOutlineCol( nStartCol, nEndCol, bShow );

	DBG_ERROR("missing tab");
	return FALSE;
}

BOOL ScDocument::UpdateOutlineRow( SCROW nStartRow, SCROW nEndRow, SCTAB nTab, BOOL bShow )
{
	if ( ValidTab(nTab) && pTab[nTab] )
		return pTab[nTab]->UpdateOutlineRow( nStartRow, nEndRow, bShow );

	DBG_ERROR("missing tab");
	return FALSE;
}

void ScDocument::Sort(SCTAB nTab, const ScSortParam& rSortParam, BOOL bKeepQuery)
{
	if ( ValidTab(nTab) && pTab[nTab] )
	{
		BOOL bOldDisableIdle = IsIdleDisabled();
		DisableIdle( TRUE );
		pTab[nTab]->Sort(rSortParam, bKeepQuery);
		DisableIdle( bOldDisableIdle );
	}
}

SCSIZE ScDocument::Query(SCTAB nTab, const ScQueryParam& rQueryParam, BOOL bKeepSub)
{
	if ( ValidTab(nTab) && pTab[nTab] )
		return pTab[nTab]->Query((ScQueryParam&)rQueryParam, bKeepSub);

	DBG_ERROR("missing tab");
	return 0;
}


BOOL ScDocument::ValidQuery( SCROW nRow, SCTAB nTab, const ScQueryParam& rQueryParam, BOOL* pSpecial )
{
	if ( ValidTab(nTab) && pTab[nTab] )
		return pTab[nTab]->ValidQuery( nRow, rQueryParam, pSpecial );

	DBG_ERROR("missing tab");
	return FALSE;
}


void ScDocument::GetUpperCellString(SCCOL nCol, SCROW nRow, SCTAB nTab, String& rStr)
{
	if ( ValidTab(nTab) && pTab[nTab] )
		pTab[nTab]->GetUpperCellString( nCol, nRow, rStr );
	else
		rStr.Erase();
}

BOOL ScDocument::CreateQueryParam(SCCOL nCol1, SCROW nRow1, SCCOL nCol2, SCROW nRow2, SCTAB nTab, ScQueryParam& rQueryParam)
{
	if ( ValidTab(nTab) && pTab[nTab] )
		return pTab[nTab]->CreateQueryParam(nCol1, nRow1, nCol2, nRow2, rQueryParam);

	DBG_ERROR("missing tab");
	return FALSE;
}

BOOL ScDocument::HasAutoFilter( SCCOL nCurCol, SCROW nCurRow, SCTAB nCurTab )
{
	ScDBData*		pDBData			= GetDBAtCursor( nCurCol, nCurRow, nCurTab );
	BOOL			bHasAutoFilter	= ( pDBData != NULL );

	if ( pDBData )
	{
		if ( pDBData->HasHeader() )
		{
			SCCOL nCol;
			SCROW nRow;
			INT16  nFlag;

			ScQueryParam aParam;
			pDBData->GetQueryParam( aParam );
			nRow = aParam.nRow1;

			for ( nCol=aParam.nCol1; nCol<=aParam.nCol2 && bHasAutoFilter; nCol++ )
			{
				nFlag = ((ScMergeFlagAttr*)
							GetAttr( nCol, nRow, nCurTab, ATTR_MERGE_FLAG ))->
								GetValue();

				if ( (nFlag & SC_MF_AUTO) == 0 )
					bHasAutoFilter = FALSE;
			}
		}
		else
			bHasAutoFilter = FALSE;
	}

	return bHasAutoFilter;
}

BOOL ScDocument::HasColHeader( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									SCTAB nTab )
{
    return VALIDTAB(nTab) && pTab[nTab] && pTab[nTab]->HasColHeader( nStartCol, nStartRow, nEndCol, nEndRow );
	//if (VALIDTAB(nTab))
	//	if (pTab[nTab])
	//		return pTab[nTab]->HasColHeader( nStartCol, nStartRow, nEndCol, nEndRow );

	//return FALSE;
}

BOOL ScDocument::HasRowHeader( SCCOL nStartCol, SCROW nStartRow, SCCOL nEndCol, SCROW nEndRow,
									SCTAB nTab )
{
    return VALIDTAB(nTab) && pTab[nTab] && pTab[nTab]->HasRowHeader( nStartCol, nStartRow, nEndCol, nEndRow );
	//if (VALIDTAB(nTab))
	//	if (pTab[nTab])
	//		return pTab[nTab]->HasRowHeader( nStartCol, nStartRow, nEndCol, nEndRow );

	//return FALSE;
}

//
//	GetFilterEntries - Eintraege fuer AutoFilter-Listbox
//

BOOL ScDocument::GetFilterEntries( SCCOL nCol, SCROW nRow, SCTAB nTab, TypedScStrCollection& rStrings, bool bFilter )
{
	if ( ValidTab(nTab) && pTab[nTab] && pDBCollection )
	{
		ScDBData* pDBData = pDBCollection->GetDBAtCursor(nCol, nRow, nTab, FALSE);	//!??
		if (pDBData)
		{
			SCTAB nAreaTab;
			SCCOL nStartCol;
			SCROW nStartRow;
			SCCOL nEndCol;
			SCROW nEndRow;
			pDBData->GetArea( nAreaTab, nStartCol, nStartRow, nEndCol, nEndRow );
			if (pDBData->HasHeader())
				++nStartRow;

			ScQueryParam aParam;
			pDBData->GetQueryParam( aParam );
			rStrings.SetCaseSensitive( aParam.bCaseSens );

            // return all filter entries, if a filter condition is connected with a boolean OR
            if ( bFilter )
            {
                SCSIZE nEntryCount = aParam.GetEntryCount();
                for ( SCSIZE i = 0; i < nEntryCount && aParam.GetEntry(i).bDoQuery; ++i )
                {
                    ScQueryEntry& rEntry = aParam.GetEntry(i);
                    if ( rEntry.eConnect != SC_AND )
                    {
                        bFilter = false;
                        break;
                    }
                }
            }

            if ( bFilter )
            {
                pTab[nTab]->GetFilteredFilterEntries( nCol, nStartRow, nEndRow, aParam, rStrings );
            }
            else
            {
			    pTab[nTab]->GetFilterEntries( nCol, nStartRow, nEndRow, rStrings );
            }

			return TRUE;
		}
	}

	return FALSE;
}

//
//	GetFilterEntriesArea - Eintraege fuer Filter-Dialog
//

BOOL ScDocument::GetFilterEntriesArea( SCCOL nCol, SCROW nStartRow, SCROW nEndRow,
										SCTAB nTab, TypedScStrCollection& rStrings )
{
	if ( ValidTab(nTab) && pTab[nTab] )
	{
		pTab[nTab]->GetFilterEntries( nCol, nStartRow, nEndRow, rStrings );
		return TRUE;
	}

	return FALSE;
}

//
//	GetDataEntries - Eintraege fuer Auswahlliste-Listbox (keine Zahlen / Formeln)
//

BOOL ScDocument::GetDataEntries( SCCOL nCol, SCROW nRow, SCTAB nTab,
									TypedScStrCollection& rStrings, BOOL bLimit )
{
    if( !bLimit )
    {
        /*  Try to generate the list from list validation. This part is skipped,
            if bLimit==TRUE, because in that case this function is called to get
            cell values for auto completion on input. */
        sal_uInt32 nValidation = static_cast< const SfxUInt32Item* >( GetAttr( nCol, nRow, nTab, ATTR_VALIDDATA ) )->GetValue();
        if( nValidation )
        {
            const ScValidationData* pData = GetValidationEntry( nValidation );
            if( pData && pData->FillSelectionList( rStrings, ScAddress( nCol, nRow, nTab ) ) )
                return TRUE;
        }
    }

    return ValidTab(nTab) && pTab[nTab] && pTab[nTab]->GetDataEntries( nCol, nRow, rStrings, bLimit );
	//if (ValidTab(nTab) && pTab[nTab])
	//	return pTab[nTab]->GetDataEntries( nCol, nRow, rStrings, bLimit );

	//return FALSE;
}

//
//	GetFormulaEntries - Eintraege fuer Formel-AutoEingabe
//

//	Funktionen werden als 1 schon vom InputHandler eingefuegt
#define SC_STRTYPE_NAMES		2
#define SC_STRTYPE_DBNAMES		3
#define SC_STRTYPE_HEADERS		4

BOOL ScDocument::GetFormulaEntries( TypedScStrCollection& rStrings )
{
	USHORT i;

	//
	//	Bereichsnamen
	//

	if ( pRangeName )
	{
		USHORT nRangeCount = pRangeName->GetCount();
		for ( i=0; i<nRangeCount; i++ )
		{
			ScRangeData* pData = (*pRangeName)[i];
			if (pData)
			{
				TypedStrData* pNew = new TypedStrData( pData->GetName(), 0.0, SC_STRTYPE_NAMES );
				if ( !rStrings.Insert(pNew) )
					delete pNew;
			}
		}
	}

	//
	//	Datenbank-Bereiche
	//

	if ( pDBCollection )
	{
		USHORT nDBCount = pDBCollection->GetCount();
		for ( i=0; i<nDBCount; i++ )
		{
			ScDBData* pData = (*pDBCollection)[i];
			if (pData)
			{
				TypedStrData* pNew = new TypedStrData( pData->GetName(), 0.0, SC_STRTYPE_DBNAMES );
				if ( !rStrings.Insert(pNew) )
					delete pNew;
			}
		}
	}

	//
	//	Inhalte von Beschriftungsbereichen
	//

	ScRangePairList* pLists[2];
	pLists[0] = GetColNameRanges();
	pLists[1] = GetRowNameRanges();
	for (USHORT nListNo=0; nListNo<2; nListNo++)
	{
		ScRangePairList* pList = pLists[nListNo];
		if (pList)
			for ( ScRangePair* pPair = pList->First(); pPair; pPair = pList->Next() )
			{
				ScRange aRange = pPair->GetRange(0);
				ScCellIterator aIter( this, aRange );
				for ( ScBaseCell* pCell = aIter.GetFirst(); pCell; pCell = aIter.GetNext() )
					if ( pCell->HasStringData() )
					{
						String aStr = pCell->GetStringData();
						TypedStrData* pNew = new TypedStrData( aStr, 0.0, SC_STRTYPE_HEADERS );
						if ( !rStrings.Insert(pNew) )
							delete pNew;
					}
			}
	}

	return TRUE;
}


BOOL ScDocument::IsEmbedded() const
{
	return bIsEmbedded;
}

void ScDocument::GetEmbedded( ScRange& rRange ) const
{
    rRange = aEmbedRange;
}

Rectangle ScDocument::GetEmbeddedRect() const						// 1/100 mm
{
	Rectangle aRect;
	ScTable* pTable = pTab[aEmbedRange.aStart.Tab()];
	if (!pTable)
	{
		DBG_ERROR("GetEmbeddedRect ohne Tabelle");
	}
	else
	{
		SCCOL i;

		for (i=0; i<aEmbedRange.aStart.Col(); i++)
			aRect.Left() += pTable->GetColWidth(i);
        aRect.Top() += pTable->GetRowHeight( 0, aEmbedRange.aStart.Row() - 1);
		aRect.Right() = aRect.Left();
		for (i=aEmbedRange.aStart.Col(); i<=aEmbedRange.aEnd.Col(); i++)
			aRect.Right() += pTable->GetColWidth(i);
		aRect.Bottom() = aRect.Top();
        aRect.Bottom() += pTable->GetRowHeight( aEmbedRange.aStart.Row(), aEmbedRange.aEnd.Row());

		aRect.Left()   = (long) ( aRect.Left()   * HMM_PER_TWIPS );
		aRect.Right()  = (long) ( aRect.Right()  * HMM_PER_TWIPS );
		aRect.Top()    = (long) ( aRect.Top()    * HMM_PER_TWIPS );
		aRect.Bottom() = (long) ( aRect.Bottom() * HMM_PER_TWIPS );
	}
	return aRect;
}

void ScDocument::SetEmbedded( const ScRange& rRange )
{
	bIsEmbedded = TRUE;
	aEmbedRange = rRange;
}

void ScDocument::ResetEmbedded()
{
	bIsEmbedded = FALSE;
	aEmbedRange = ScRange();
}

ScRange ScDocument::GetRange( SCTAB nTab, const Rectangle& rMMRect )
{
	ScTable* pTable = pTab[nTab];
	if (!pTable)
	{
		DBG_ERROR("GetRange ohne Tabelle");
		return ScRange();
	}

	Rectangle aPosRect = rMMRect;
	if ( IsNegativePage( nTab ) )
		ScDrawLayer::MirrorRectRTL( aPosRect );			// always with positive (LTR) values

	long nSize;
	long nTwips;
	long nAdd;
	BOOL bEnd;

	nSize = 0;
	nTwips = (long) (aPosRect.Left() / HMM_PER_TWIPS);

	SCCOL nX1 = 0;
	bEnd = FALSE;
	while (!bEnd)
	{
		nAdd = (long) pTable->GetColWidth(nX1);
		if (nSize+nAdd <= nTwips+1 && nX1<MAXCOL)
		{
			nSize += nAdd;
			++nX1;
		}
		else
			bEnd = TRUE;
	}

	nTwips = (long) (aPosRect.Right() / HMM_PER_TWIPS);

	SCCOL nX2 = nX1;
	bEnd = FALSE;
	while (!bEnd)
	{
		nAdd = (long) pTable->GetColWidth(nX2);
		if (nSize+nAdd < nTwips && nX2<MAXCOL)
		{
			nSize += nAdd;
			++nX2;
		}
		else
			bEnd = TRUE;
	}


	nSize = 0;
	nTwips = (long) (aPosRect.Top() / HMM_PER_TWIPS);

	SCROW nY1 = 0;
    ScCoupledCompressedArrayIterator< SCROW, BYTE, USHORT> aIter(
            *(pTable->GetRowFlagsArray()), nY1, MAXROW, CR_HIDDEN, 0,
            *(pTable->GetRowHeightArray()));
	bEnd = FALSE;
	while (!bEnd && aIter)
	{
        nY1 = aIter.GetPos();
		nAdd = (long) *aIter;
		if (nSize+nAdd <= nTwips+1 && nY1<MAXROW)
        {
			nSize += nAdd;
			++nY1;
            ++aIter;
        }
		else
			bEnd = TRUE;
	}
    if (!aIter)
        nY1 = aIter.GetIterEnd();   // all hidden down to the bottom

	nTwips = (long) (aPosRect.Bottom() / HMM_PER_TWIPS);

	SCROW nY2 = nY1;
    aIter.NewLimits( nY2, MAXROW);
	bEnd = FALSE;
	while (!bEnd && aIter)
	{
        nY2 = aIter.GetPos();
		nAdd = (long) *aIter;
		if (nSize+nAdd < nTwips && nY2<MAXROW)
		{
			nSize += nAdd;
			++nY2;
            ++aIter;
		}
		else
			bEnd = TRUE;
	}
    if (!aIter)
        nY2 = aIter.GetIterEnd();   // all hidden down to the bottom

	return ScRange( nX1,nY1,nTab, nX2,nY2,nTab );
}

void ScDocument::SetEmbedded( const Rectangle& rRect )			// aus VisArea (1/100 mm)
{
	bIsEmbedded = TRUE;
	aEmbedRange = GetRange( nVisibleTab, rRect );
}

//	VisArea auf Zellgrenzen anpassen

void lcl_SnapHor( ScTable* pTable, long& rVal, SCCOL& rStartCol )
{
	SCCOL nCol = 0;
	long nTwips = (long) (rVal / HMM_PER_TWIPS);
	long nSnap = 0;
	while ( nCol<MAXCOL )
	{
		long nAdd = pTable->GetColWidth(nCol);
		if ( nSnap + nAdd/2 < nTwips || nCol < rStartCol )
		{
			nSnap += nAdd;
			++nCol;
		}
		else
			break;
	}
	rVal = (long) ( nSnap * HMM_PER_TWIPS );
	rStartCol = nCol;
}

void lcl_SnapVer( ScTable* pTable, long& rVal, SCROW& rStartRow )
{
	SCROW nRow = 0;
	long nTwips = (long) (rVal / HMM_PER_TWIPS);
	long nSnap = 0;
    ScCoupledCompressedArrayIterator< SCROW, BYTE, USHORT> aIter(
            *(pTable->GetRowFlagsArray()), nRow, MAXROW, CR_HIDDEN, 0,
            *(pTable->GetRowHeightArray()));
	while ( aIter )
	{
        nRow = aIter.GetPos();
		long nAdd = *aIter;
		if ( nSnap + nAdd/2 < nTwips || nRow < rStartRow )
		{
			nSnap += nAdd;
			++nRow;
            ++aIter;
		}
		else
			break;
	}
    if (!aIter)
        nRow = MAXROW;  // all hidden down to the bottom
	rVal = (long) ( nSnap * HMM_PER_TWIPS );
	rStartRow = nRow;
}

void ScDocument::SnapVisArea( Rectangle& rRect ) const
{
	ScTable* pTable = pTab[nVisibleTab];
	if (!pTable)
	{
		DBG_ERROR("SetEmbedded ohne Tabelle");
		return;
	}

    BOOL bNegativePage = IsNegativePage( nVisibleTab );
    if ( bNegativePage )
        ScDrawLayer::MirrorRectRTL( rRect );        // calculate with positive (LTR) values

	SCCOL nCol = 0;
	lcl_SnapHor( pTable, rRect.Left(), nCol );
	++nCol;											// mindestens eine Spalte
	lcl_SnapHor( pTable, rRect.Right(), nCol );

	SCROW nRow = 0;
	lcl_SnapVer( pTable, rRect.Top(), nRow );
	++nRow;											// mindestens eine Zeile
	lcl_SnapVer( pTable, rRect.Bottom(), nRow );

    if ( bNegativePage )
        ScDrawLayer::MirrorRectRTL( rRect );        // back to real rectangle
}

ScDocProtection* ScDocument::GetDocProtection() const
{
    return pDocProtection.get();
}

void ScDocument::SetDocProtection(const ScDocProtection* pProtect)
{
    if (pProtect)
        pDocProtection.reset(new ScDocProtection(*pProtect));
    else
        pDocProtection.reset(NULL);
}

BOOL ScDocument::IsDocProtected() const
{
    return pDocProtection.get() && pDocProtection->isProtected();
}

BOOL ScDocument::IsDocEditable() const
{
    // import into read-only document is possible
    return !IsDocProtected() && ( bImportingXML || mbChangeReadOnlyEnabled || !pShell || !pShell->IsReadOnly() );
}

BOOL ScDocument::IsTabProtected( SCTAB nTab ) const
{
	if (VALIDTAB(nTab) && pTab[nTab])
		return pTab[nTab]->IsProtected();

	DBG_ERROR("Falsche Tabellennummer");
	return FALSE;
}

ScTableProtection* ScDocument::GetTabProtection( SCTAB nTab ) const
{
    if (VALIDTAB(nTab) && pTab[nTab])
        return pTab[nTab]->GetProtection();

    return NULL;
}

void ScDocument::SetTabProtection(SCTAB nTab, const ScTableProtection* pProtect)
{
    if (!ValidTab(nTab))
        return;

    pTab[nTab]->SetProtection(pProtect);
}

void ScDocument::CopyTabProtection(SCTAB nTabSrc, SCTAB nTabDest)
{
    if (!ValidTab(nTabSrc) || !ValidTab(nTabDest))
        return;

    pTab[nTabDest]->SetProtection( pTab[nTabSrc]->GetProtection() );
}

const ScDocOptions& ScDocument::GetDocOptions() const
{
	DBG_ASSERT( pDocOptions, "No DocOptions! :-(" );
	return *pDocOptions;
}

void ScDocument::SetDocOptions( const ScDocOptions& rOpt )
{
	DBG_ASSERT( pDocOptions, "No DocOptions! :-(" );
	*pDocOptions = rOpt;

	xPoolHelper->SetFormTableOpt(rOpt);
}

const ScViewOptions& ScDocument::GetViewOptions() const
{
	DBG_ASSERT( pViewOptions, "No ViewOptions! :-(" );
	return *pViewOptions;
}

void ScDocument::SetViewOptions( const ScViewOptions& rOpt )
{
	DBG_ASSERT( pViewOptions, "No ViewOptions! :-(" );
	*pViewOptions = rOpt;
}

void ScDocument::GetLanguage( LanguageType& rLatin, LanguageType& rCjk, LanguageType& rCtl ) const
{
	rLatin = eLanguage;
	rCjk = eCjkLanguage;
	rCtl = eCtlLanguage;
}

void ScDocument::SetLanguage( LanguageType eLatin, LanguageType eCjk, LanguageType eCtl )
{
	eLanguage = eLatin;
	eCjkLanguage = eCjk;
	eCtlLanguage = eCtl;
	if ( xPoolHelper.isValid() )
	{
		ScDocumentPool* pPool = xPoolHelper->GetDocPool();
		pPool->SetPoolDefaultItem( SvxLanguageItem( eLanguage, ATTR_FONT_LANGUAGE ) );
		pPool->SetPoolDefaultItem( SvxLanguageItem( eCjkLanguage, ATTR_CJK_FONT_LANGUAGE ) );
		pPool->SetPoolDefaultItem( SvxLanguageItem( eCtlLanguage, ATTR_CTL_FONT_LANGUAGE ) );
	}

	UpdateDrawLanguages();		// set edit engine defaults in drawing layer pool
}

void ScDocument::SetDrawDefaults()
{
    bSetDrawDefaults = TRUE;
    UpdateDrawDefaults();
}

Rectangle ScDocument::GetMMRect( SCCOL nStartCol, SCROW nStartRow,
								SCCOL nEndCol, SCROW nEndRow, SCTAB nTab )
{
	if (!ValidTab(nTab) || !pTab[nTab])
	{
		DBG_ERROR("GetMMRect: falsche Tabelle");
		return Rectangle(0,0,0,0);
	}

	SCCOL i;
	Rectangle aRect;

	for (i=0; i<nStartCol; i++)
		aRect.Left() += GetColWidth(i,nTab);
    aRect.Top() += FastGetRowHeight( 0, nStartRow-1, nTab);

	aRect.Right()  = aRect.Left();
	aRect.Bottom() = aRect.Top();

	for (i=nStartCol; i<=nEndCol; i++)
		aRect.Right() += GetColWidth(i,nTab);
    aRect.Bottom() += FastGetRowHeight( nStartRow, nEndRow, nTab);

	aRect.Left()	= (long)(aRect.Left()	* HMM_PER_TWIPS);
	aRect.Right()	= (long)(aRect.Right()	* HMM_PER_TWIPS);
	aRect.Top()		= (long)(aRect.Top()	* HMM_PER_TWIPS);
	aRect.Bottom()	= (long)(aRect.Bottom()	* HMM_PER_TWIPS);

	if ( IsNegativePage( nTab ) )
		ScDrawLayer::MirrorRectRTL( aRect );

	return aRect;
}

void ScDocument::SetExtDocOptions( ScExtDocOptions* pNewOptions )
{
	delete pExtDocOptions;
	pExtDocOptions = pNewOptions;
}

void ScDocument::DoMergeContents( SCTAB nTab, SCCOL nStartCol, SCROW nStartRow,
									SCCOL nEndCol, SCROW nEndRow )
{
	String aEmpty;
	String aTotal;
	String aCellStr;
	SCCOL nCol;
	SCROW nRow;
	for (nRow=nStartRow; nRow<=nEndRow; nRow++)
		for (nCol=nStartCol; nCol<=nEndCol; nCol++)
		{
			GetString(nCol,nRow,nTab,aCellStr);
			if (aCellStr.Len())
			{
				if (aTotal.Len())
					aTotal += ' ';
				aTotal += aCellStr;
			}
			if (nCol != nStartCol || nRow != nStartRow)
				SetString(nCol,nRow,nTab,aEmpty);
		}

	SetString(nStartCol,nStartRow,nTab,aTotal);
}

void ScDocument::DoMerge( SCTAB nTab, SCCOL nStartCol, SCROW nStartRow,
                                    SCCOL nEndCol, SCROW nEndRow, bool bDeleteCaptions )
{
	ScMergeAttr aAttr( nEndCol-nStartCol+1, nEndRow-nStartRow+1 );
	ApplyAttr( nStartCol, nStartRow, nTab, aAttr );

	if ( nEndCol > nStartCol )
		ApplyFlagsTab( nStartCol+1, nStartRow, nEndCol, nStartRow, nTab, SC_MF_HOR );
	if ( nEndRow > nStartRow )
		ApplyFlagsTab( nStartCol, nStartRow+1, nStartCol, nEndRow, nTab, SC_MF_VER );
	if ( nEndCol > nStartCol && nEndRow > nStartRow )
		ApplyFlagsTab( nStartCol+1, nStartRow+1, nEndCol, nEndRow, nTab, SC_MF_HOR | SC_MF_VER );

    // remove all covered notes (removed captions are collected by drawing undo if active)
    USHORT nDelFlag = IDF_NOTE | (bDeleteCaptions ? 0 : IDF_NOCAPTIONS);
    if( nStartCol < nEndCol )
        DeleteAreaTab( nStartCol + 1, nStartRow, nEndCol, nStartRow, nTab, nDelFlag );
    if( nStartRow < nEndRow )
        DeleteAreaTab( nStartCol, nStartRow + 1, nEndCol, nEndRow, nTab, nDelFlag );
}

void ScDocument::RemoveMerge( SCCOL nCol, SCROW nRow, SCTAB nTab )
{
	const ScMergeAttr* pAttr = (const ScMergeAttr*)
									GetAttr( nCol, nRow, nTab, ATTR_MERGE );

	if ( pAttr->GetColMerge() <= 1 && pAttr->GetRowMerge() <= 1 )
		return;

	SCCOL nEndCol = nCol + pAttr->GetColMerge() - 1;
	SCROW nEndRow = nRow + pAttr->GetRowMerge() - 1;

	RemoveFlagsTab( nCol, nRow, nEndCol, nEndRow, nTab, SC_MF_HOR | SC_MF_VER );

	const ScMergeAttr* pDefAttr = (const ScMergeAttr*)
										&xPoolHelper->GetDocPool()->GetDefaultItem( ATTR_MERGE );
	ApplyAttr( nCol, nRow, nTab, *pDefAttr );
}

void ScDocument::ExtendPrintArea( OutputDevice* pDev, SCTAB nTab,
					SCCOL nStartCol, SCROW nStartRow, SCCOL& rEndCol, SCROW nEndRow )
{
	if ( ValidTab(nTab)  && pTab[nTab] )
		pTab[nTab]->ExtendPrintArea( pDev, nStartCol, nStartRow, rEndCol, nEndRow );
}

void ScDocument::IncSizeRecalcLevel( SCTAB nTab )
{
	if ( ValidTab(nTab)  && pTab[nTab] )
		pTab[nTab]->IncRecalcLevel();
}

void ScDocument::DecSizeRecalcLevel( SCTAB nTab, bool bUpdateNoteCaptionPos )
{
	if ( ValidTab(nTab)  && pTab[nTab] )
		pTab[nTab]->DecRecalcLevel( bUpdateNoteCaptionPos );
}




